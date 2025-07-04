/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 *
 * =======================================================================
 *
 * The Quake II file system, implements generic file system operations
 * as well as the .pak file format and support for .pk3 files.
 *
 * =======================================================================
 */

#ifndef _MSC_VER
#include <libgen.h>
#endif

#include "header/common.h"
#include "header/glob.h"
#include "unzip/unzip.h"

#include "../client/sound/header/vorbis.h"

#define MAX_HANDLES 512
#define MAX_FILENAME 128
#define MAX_MODS 32
#define MAX_PAKS 100

#ifdef SYSTEMWIDE
 #ifndef SYSTEMDIR
  #define SYSTEMDIR "/usr/share/games/quake2"
 #endif
#endif

typedef enum
{
	PAK_MODE_Q2,
	PAK_MODE_DK,
	PAK_MODE_DAT,
} fsPackCompress_t;

typedef struct
{
	char name[MAX_FILENAME];
	fsMode_t mode;
	FILE *file;           /* Only one will be used. */
	unzFile *zip;        /* (file or zip) */
	int compressed_size; /* Should be zero for original PAK files */
	fsPackCompress_t format;
} fsHandle_t;

typedef struct fsLink_s
{
	char *from;
	char *to;
	int length;
	struct fsLink_s *next;
} fsLink_t;

typedef struct
{
	char name[MAX_FILENAME];
	int size;
	int offset;     /* Ignored in PK3 files. */
	int compressed_size; /* Should be zero for original PAK files */
	fsPackCompress_t format;
} fsPackFile_t;

typedef struct
{
	char name[MAX_OSPATH];
	int numFiles;
	FILE *pak;
	unzFile *pk3;
	qboolean isProtectedPak;
	fsPackFile_t *files;
} fsPack_t;

typedef struct fsSearchPath_s
{
	char path[MAX_OSPATH]; /* Only one used. */
	fsPack_t *pack; /* (path or pack) */
	struct fsSearchPath_s *next;
} fsSearchPath_t;

typedef enum
{
	DAT,
	SIN,
	PAK,
	PK3
} fsPackFormat_t;

typedef struct
{
	char *suffix;
	fsPackFormat_t format;
} fsPackTypes_t;

fsHandle_t fs_handles[MAX_HANDLES];
fsLink_t *fs_links = NULL;
fsSearchPath_t *fs_searchPaths = NULL;
fsSearchPath_t *fs_baseSearchPaths = NULL;

/* Pack formats / suffixes. */
fsPackTypes_t fs_packtypes[] = {
	{"dat", DAT},
	{"sin", SIN},
	{"pak", PAK},
	{"pk2", PK3},
	{"pk3", PK3},
	{"pkz", PK3},
	{"zip", PK3}
};

char datadir[MAX_OSPATH];
char fs_gamedir[MAX_OSPATH];
qboolean file_from_protected_pak;

cvar_t *fs_basedir;
cvar_t *fs_cddir;
cvar_t *fs_gamedirvar;
cvar_t *fs_debug;

fsHandle_t *FS_GetFileByHandle(fileHandle_t f);

// --------

// Raw search path, the actual search
// bath is build from this one.
typedef struct fsRawPath_s {
	char path[MAX_OSPATH];
	qboolean create;
	struct fsRawPath_s *next;
} fsRawPath_t;

fsRawPath_t *fs_rawPath;

// --------

#if _WIN32
/*
 * We need some trickery to make minizip Unicode compatible...
 */

#include <windows.h>
#include "unzip/ioapi.h"

zlib_filefunc_def zlib_file_api;

static voidpf ZCALLBACK fopen_file_func_utf(voidpf opaque, const char *filename, int mode)
{
	FILE* file = NULL;
	WCHAR *mode_fopen = NULL;
	WCHAR wfilename[MAX_OSPATH];

	if ((mode & ZLIB_FILEFUNC_MODE_READWRITEFILTER) == ZLIB_FILEFUNC_MODE_READ)
	{
		mode_fopen = L"rb";
	}
	else if (mode & ZLIB_FILEFUNC_MODE_EXISTING)
	{
		mode_fopen = L"r+b";
	}
	else if (mode & ZLIB_FILEFUNC_MODE_CREATE)
	{
		mode_fopen = L"wb";
	}

	if (!((filename == NULL) || (mode_fopen == NULL)))
	{
		MultiByteToWideChar(CP_UTF8, 0, filename, -1, wfilename, sizeof(wfilename));
		file = _wfopen((const wchar_t *) wfilename, mode_fopen);
	}

	return file;
}
#endif

// --------

/*
 * All of Quake's data access is through a hierchal file system, but the
 * contents of the file system can be transparently merged from several
 * sources.
 *
 * The "base directory" is the path to the directory holding the quake.exe and
 * all game directories.  The sys_* files pass this to host_init in
 * quakeparms_t->basedir.  This can be overridden with the "-basedir" command
 * line parm to allow code debugging in a different directory.  The base
 * directory is only used during filesystem initialization.
 *
 * The "game directory" is the first tree on the search path and directory that
 * all generated files (savegames, screenshots, demos, config files) will be
 * saved to.  This can be overridden with the "-game" command line parameter.
 * The game directory can never be changed while quake is executing.  This is
 * a precacution against having a malicious server instruct clients to write
 * files over areas they shouldn't.
 *
 */

static int
FS_FileLength(FILE *f)
{
	int pos; /* Current position. */
	int end; /* End of file. */

	pos = ftell(f);
	if (pos < 0)
	{
		Com_Printf("%s: refusing to ftell\n", __func__);
		return -1;
	}

	if (fseek(f, 0, SEEK_END))
	{
		Com_Printf("%s: seek failed", __func__);
		return -1;
	}

	end = ftell(f);

	if (fseek(f, pos, SEEK_SET))
	{
		Com_Printf("%s: seek failed", __func__);
		return -1;
	}

	return end;
}

/*
 * Creates any directories needed to store the given filename.
 */
void
FS_CreatePath(const char *path)
{
	char *cur; /* Current '/'. */
	char *old; /* Old '/'. */
	char dir_path[MAX_OSPATH];

	FS_DPrintf("%s(%s)\n", __func__, path);

	if (strstr(path, "..") != NULL)
	{
		Com_Printf("WARNING: refusing to create relative path '%s'.\n", path);
		return;
	}

	Q_strlcpy(dir_path, path, sizeof(dir_path));

	cur = old = dir_path;

	while (cur != NULL)
	{
		if ((cur - old) > 1)
		{
			*cur = '\0';
			Sys_Mkdir(dir_path);
			*cur = '/';
		}

		old = cur;
		cur = strchr(old + 1, '/');
	}
}

void
FS_DPrintf(const char *format, ...)
{
	char msg[1024];
	va_list argPtr;

	if (fs_debug->value != 1)
	{
		return;
	}

	va_start(argPtr, format);
	vsnprintf(msg, sizeof(msg), format, argPtr);
	va_end(argPtr);

	Com_Printf("%s", msg);
}

const char *
FS_Gamedir(void)
{
	return fs_gamedir;
}

/*
 * Finds a free fileHandle_t.
 */
static fsHandle_t *
FS_HandleForFile(const char *path, fileHandle_t *f)
{
	int i;
	fsHandle_t *handle;

	handle = fs_handles;

	for (i = 0; i < MAX_HANDLES; i++, handle++)
	{
		if ((handle->file == NULL) && (handle->zip == NULL))
		{
			Q_strlcpy(handle->name, path, sizeof(handle->name));
			*f = i + 1;
			return handle;
		}
	}

	/* Failed. */
	Com_Error(ERR_DROP, "%s: none free", __func__);

	return NULL;
}

/*
 * Returns a fsHandle_t * for the given fileHandle_t.
 */
fsHandle_t *
FS_GetFileByHandle(fileHandle_t f)
{
	if ((f < 0) || (f > MAX_HANDLES))
	{
		Com_Error(ERR_DROP, "%s: out of range", __func__);
	}

	if (f == 0)
	{
		f++;
	}

	return &fs_handles[f - 1];
}

/*
 * Other dll's can't just call fclose() on files returned by FS_FOpenFile.
 */
void
FS_FCloseFile(fileHandle_t f)
{
	fsHandle_t *handle;

	handle = FS_GetFileByHandle(f);

	if (handle->file)
	{
		fclose(handle->file);
	}
	else if (handle->zip)
	{
		unzCloseCurrentFile(handle->zip);
		unzClose(handle->zip);
	}

	memset(handle, 0, sizeof(*handle));
}

static int
FS_SortPackCompare(const void *p1, const void *p2)
{
	fsPackFile_t *file1, *file2;

	file1 = (fsPackFile_t*)p1;
	file2 = (fsPackFile_t*)p2;
	return Q_stricmp(file1->name, file2->name);
}

static void
FS_SortPack(fsPack_t *pak)
{
	qsort(pak->files, pak->numFiles, sizeof(fsPackFile_t), FS_SortPackCompare);
}

static int
FS_PackQuickSearch(const fsPack_t *pak, const char *name)
{
	int start, end;

	start = 0;
	end = pak->numFiles - 1;

	while (start <= end)
	{
		int i, res;

		i = start + (end - start) / 2;

		res = Q_stricmp(pak->files[i].name, name);
		if (res == 0)
		{
			return i;
		}
		else if (res < 0)
		{
			start = i + 1;
		}
		else
		{
			end = i - 1;
		}
	}

	return -1;
}

/*
 * Finds the file in the search path. Returns filesize and an open FILE *. Used
 * for streaming data out of either a pak file or a seperate file.
 */
int
FS_FOpenFile(const char *rawname, fileHandle_t *f, qboolean gamedir_only)
{
	char path[MAX_OSPATH], lwrName[MAX_OSPATH];
	fsHandle_t *handle;
	fsPack_t *pack;
	fsSearchPath_t *search;
	int input, output;

	// Remove self references and empty dirs from the requested path.
	// ZIPs and PAKs don't support them, but they may be hardcoded in
	// some custom maps or models.
	char name[MAX_QPATH] = {0};
	size_t namelen = strlen(rawname);
	for (input = 0, output = 0; input < namelen; input++)
	{
		// Remove self reference.
		if (rawname[input] == '.')
		{
			if (output > 0)
			{
				// Inside the path.
				if (name[output - 1] == '/' && rawname[input + 1] == '/')
				{
					input++;
					continue;
				}
			}
			else
			{
				// At the beginning. Note: This is save because the Quake II
				// VFS doesn't have a current working dir. Paths are always
				// absolute.
				if (rawname[input + 1] == '/')
				{
					continue;
				}
			}
		}

		// Empty dir.
		if (rawname[input] == '/')
		{
			if (rawname[input + 1] == '/')
			{
				continue;
			}
		}

		// Paths starting with a /. I'm not sure if this is
		// a problem. It shouldn't hurt to remove the leading
		// slash, though.
		if (rawname[input] == '/' && output == 0)
		{
			continue;
		}

		name[output] = rawname[input];
		output++;
	}

	file_from_protected_pak = false;
	handle = FS_HandleForFile(name, f);
	Q_strlcpy(handle->name, name, sizeof(handle->name));
	handle->mode = FS_READ;

	/* Search through the path, one element at a time. */
	for (search = fs_searchPaths; search; search = search->next)
	{
		if (gamedir_only)
		{
			if (strstr(search->path, FS_Gamedir()) == NULL)
			{
				continue;
			}
		}

		// Evil hack for maps.lst and players/
		// TODO: A flag to ignore paks would be better
		if ((strcmp(fs_gamedirvar->string, "") == 0) && search->pack) {
			if ((strcmp(name, "maps.lst") == 0) || (strncmp(name, "players/", 8) == 0)) {
				if (FS_FileInGamedir(name))
				{
					continue;
				}
			}
		}

		/* Search inside a pack file. */
		if (search->pack)
		{
			int i;

			pack = search->pack;
			i = FS_PackQuickSearch(pack, handle->name);

			if (i >= 0)
			{
				/* Found it! */
				if (fs_debug->value)
				{
					Com_Printf("%s: '%s' (found in '%s').\n",
						__func__, handle->name, pack->name);
				}

				// save the name with *correct case* in the handle
				// (relevant for savegames, when starting map with wrong case but it's still found
				//  because it's from pak, but save/bla/MAPname.sav/sv2 will have wrong case and can't be found then)
				Q_strlcpy(handle->name, pack->files[i].name, sizeof(handle->name));
				handle->compressed_size = 0;
				handle->format = PAK_MODE_Q2;

				if (pack->pak)
				{
					/* PAK and DAT */
					if (pack->isProtectedPak)
					{
						file_from_protected_pak = true;
					}

					handle->file = Q_fopen(pack->name, "rb");

					if (handle->file)
					{
						handle->compressed_size = pack->files[i].compressed_size;
						handle->format = pack->files[i].format;
						if (fseek(handle->file, pack->files[i].offset, SEEK_SET))
						{
							Com_Printf("%s: '%s' seek failed", __func__, handle->name);
							return 0;
						}

						return pack->files[i].size;
					}
				}
				else if (pack->pk3)
				{
					/* PK3 */
					if (pack->isProtectedPak)
					{
						file_from_protected_pak = true;
					}

#ifdef _WIN32
					handle->zip = unzOpen2(pack->name, &zlib_file_api);
#else
					handle->zip = unzOpen(pack->name);
#endif

					if (handle->zip)
					{
						if (unzLocateFile(handle->zip, handle->name, 2) == UNZ_OK)
						{
							if (unzOpenCurrentFile(handle->zip) == UNZ_OK)
							{
								return pack->files[i].size;
							}
						}

						unzClose(handle->zip);
					}
				}

				Com_Error(ERR_FATAL, "Couldn't reopen '%s'", pack->name);
			}
		}
		else
		{
			/* Search in a directory tree. */
			Com_sprintf(path, sizeof(path), "%s/%s", search->path, handle->name);

			handle->file = Q_fopen(path, "rb");

			if (!handle->file)
			{
				Com_sprintf(lwrName, sizeof(lwrName), "%s", handle->name);
				Q_strlwr(lwrName);
				Com_sprintf(path, sizeof(path), "%s/%s", search->path, lwrName);
				handle->file = Q_fopen(path, "rb");
			}

			if (handle->file)
			{
				if (fs_debug->value)
				{
					Com_Printf("%s: '%s' (found in '%s').\n",
						__func__, handle->name, search->path);
				}

				return FS_FileLength(handle->file);
			}
		}
	}
	if (fs_debug->value)
	{
		Com_Printf("%s: couldn't find '%s'.\n", __func__, handle->name);
	}

	/* Couldn't open, so free the handle. */
	memset(handle, 0, sizeof(*handle));
	*f = 0;
	return -1;
}

static int
FS_DecompressFile(void *buffer, int size, const fsHandle_t *handle)
{
	if (handle->compressed_size)
	{
		unsigned long uncomressed_size;
		byte *comressed_buffer;

		comressed_buffer = malloc(handle->compressed_size);
		uncomressed_size = size;

		memcpy(comressed_buffer, buffer, handle->compressed_size);

		if (handle->format == PAK_MODE_DAT)
		{
			int status;

			status = uncompress(buffer, &uncomressed_size, comressed_buffer,
				handle->compressed_size);

			free(comressed_buffer);

			if (status != MZ_OK)
			{
				Com_Error(ERR_FATAL, "%s: can't decompress file '%s'",
					__func__, handle->name);
				return 0;
			}
		}
		else if (handle->format == PAK_MODE_DK)
		{
			int read = 0;
			int written = 0;

			while ((read < handle->compressed_size) || (written < size))
			{
				byte x;

				x = comressed_buffer[read];
				++read;

				/* x + 1 bytes of uncompressed data */
				if (x < 64)
				{
					if ((written + x + 1) > size)
					{
						Com_Error(ERR_FATAL, "%s: can't decompress file '%s'",
							__func__, handle->name);
						return 0;
					}

					memmove((byte *)buffer + written, comressed_buffer + read, x + 1);

					read += x + 1;
					written += x + 1;
				}
				/* x - 62 zeros */
				else if (x < 128)
				{
					if ((written + x - 62) > size)
					{
						Com_Error(ERR_FATAL, "%s: can't decompress file '%s'",
							__func__, handle->name);
						return 0;
					}

					memset((byte *)buffer + written, 0, x - 62);

					written += x - 62;
				}
				/* x - 126 times the next byte */
				else if (x < 192)
				{
					if ((written + x - 126) > size)
					{
						Com_Error(ERR_FATAL, "%s: can't decompress file '%s'",
							__func__, handle->name);
						return 0;
					}

					memset((byte *)buffer + written, comressed_buffer[read], x - 126);

					++read;
					written += x - 126;
				}
				/* Reference previously uncompressed data */
				else if (x < 254)
				{
					if ((written + x - 190) > size)
					{
						Com_Error(ERR_FATAL, "%s: can't decompress file '%s'",
							__func__, handle->name);
						return 0;
					}

					memmove((byte *)buffer + written, ((byte *)buffer + written) - (
						(int)comressed_buffer[read] + 2), x - 190);

					++read;
					written += x - 190;
				}
				/* Terminate */
				else if (x == 255)
				{
					break;
				}
			}

			free(comressed_buffer);
		}
	}

	return size;
}

/*
 * Properly handles partial reads.
 */
int
FS_Read(void *buffer, int size, fileHandle_t f)
{
	qboolean tried = false;    /* Tried to read from a CD. */
	byte *buf, *compressed_buf;        /* Buffer. */
	int r;         /* Number of bytes read. */
	int remaining;        /* Remaining bytes. */
	fsHandle_t *handle;  /* File handle. */

	handle = FS_GetFileByHandle(f);

	buf = (byte *)buffer;
	compressed_buf = (byte *)buffer;

	/* Read. */
	if (handle->compressed_size)
	{
		remaining = handle->compressed_size;
		if (size < handle->compressed_size)
		{
			/* compressed chunk bigger than provided buffer */
			compressed_buf = malloc(handle->compressed_size);
			buf = compressed_buf;
		}
	}
	else
	{
		remaining = size;
	}

	while (remaining)
	{
		if (handle->file)
		{
			r = fread(buf, 1, remaining, handle->file);
		}
		else if (handle->zip)
		{
			r = unzReadCurrentFile(handle->zip, buf, remaining);
		}
		else
		{
			return 0;
		}

		if (r == 0)
		{
			if (!tried)
			{
				/* Might tried to read from a CD. */
				tried = true;
			}
			else
			{
				/* Already tried once. */
				Com_Error(ERR_FATAL, "%s: 0 bytes read from '%s'",
					__func__, handle->name);
				return size - remaining;
			}
		}
		else if (r == -1)
		{
			Com_Error(ERR_FATAL, "%s: -1 bytes read from '%s'",
				__func__, handle->name);
		}

		remaining -= r;
		buf += r;
	}

	remaining = FS_DecompressFile(compressed_buf, size, handle);
	if (buffer != compressed_buf)
	{
		memcpy(buffer, compressed_buf, size);
		free(compressed_buf);
	}
	return remaining;
}

/*
 * Properly handles partial reads of size up to count times. No error if it
 * can't read.
 */
int
FS_FRead(void *buffer, int size, int count, fileHandle_t f)
{
	qboolean tried = false;    /* Tried to read from a CD. */
	byte *buf, *compressed_buf;        /* Buffer. */
	int loops;         /* Loop indicator. */
	int r;         /* Number of bytes read. */
	int remaining;         /* Remaining bytes. */
	fsHandle_t *handle;  /* File handle. */

	handle = FS_GetFileByHandle(f);

	/* Read. */
	loops = count;
	buf = (byte *)buffer;
	compressed_buf = (byte *)buffer;

	if (handle->compressed_size && size < handle->compressed_size)
	{
		/* compressed chunk bigger than provided buffer */
		compressed_buf = malloc(handle->compressed_size);
		buf = compressed_buf;
	}

	while (loops)
	{
		/* Read in chunks. */
		if (handle->compressed_size)
		{
			remaining = handle->compressed_size;
		}
		else
		{
			remaining = size;
		}

		while (remaining)
		{
			if (handle->file)
			{
				r = fread(buf, 1, remaining, handle->file);
			}
			else if (handle->zip)
			{
				r = unzReadCurrentFile(handle->zip, buf, remaining);
			}
			else
			{
				return 0;
			}

			if (r == 0)
			{
				if (!tried)
				{
					/* Might tried to read from a CD. */
					tried = true;
				}
				else
				{
					/* Already tried once. */
					return size - remaining;
				}
			}
			else if (r == -1)
			{
				Com_Error(ERR_FATAL,
						"%s: -1 bytes read from '%s'",
						__func__, handle->name);
			}

			remaining -= r;
			buf += r;
		}

		loops--;
	}

	remaining = FS_DecompressFile(compressed_buf, size, handle);
	if (buffer != compressed_buf)
	{
		memcpy(buffer, compressed_buf, size);
		free(compressed_buf);
	}
	return remaining;
}

/*
 * Filename are reletive to the quake search path. A null buffer will just
 * return the file length without loading.
 */
int
FS_LoadFile(const char *path, void **buffer)
{
	byte *buf; /* Buffer. */
	int size; /* File size. */
	fileHandle_t f; /* File handle. */

	buf = NULL;
	size = FS_FOpenFile(path, &f, false);

	if (size <= 0)
	{
		if (size == 0)
		{
			/* empty file, close before exit*/
			FS_FCloseFile(f);
		}

		if (buffer)
		{
			*buffer = NULL;
		}

		return size;
	}

	if (buffer == NULL)
	{
		FS_FCloseFile(f);
		return size;
	}

	buf = Z_Malloc(size);
	*buffer = buf;

	FS_Read(buf, size, f);
	FS_FCloseFile(f);

	return size;
}

void
FS_FreeFile(void *buffer)
{
	if (buffer == NULL)
	{
		FS_DPrintf("FS_FreeFile: NULL buffer.\n");
		return;
	}

	Z_Free(buffer);
}

static fsRawPath_t *
FS_FreeRawPaths(fsRawPath_t *start, fsRawPath_t *end)
{
	fsRawPath_t *cur = start;
	fsRawPath_t *next;

	while (cur != end)
	{
		next = cur->next;
		Z_Free(cur);
		cur = next;
	}

	return cur;
}

static fsSearchPath_t *
FS_FreeSearchPaths(fsSearchPath_t *start, fsSearchPath_t *end)
{
	fsSearchPath_t *cur = start;
	fsSearchPath_t *next;

	while (cur != end)
	{
		if (cur->pack)
		{
			if (cur->pack->pak)
			{
				fclose(cur->pack->pak);
			}

			if (cur->pack->pk3)
			{
				unzClose(cur->pack->pk3);
			}

			Z_Free(cur->pack->files);
			Z_Free(cur->pack);
		}

		next = cur->next;
		Z_Free(cur);
		cur = next;
	}

	return cur;
}

static fsPack_t *
FS_LoadDAT(const char *packPath)
{
	int i; /* Loop counter. */
	int numFiles; /* Number of files in DAT. */
	FILE *handle; /* File handle. */
	fsPackFile_t *files; /* List of files in DAT. */
	fsPack_t *pack; /* DAT file. */
	ddatheader_t header; /* DAT file header. */
	ddatfile_t *info = NULL; /* DAT info. */
	const char *prefixpos, *pos;
	char prefix[MAX_QPATH];
	size_t prefix_size;

	handle = Q_fopen(packPath, "rb");

	if (handle == NULL)
	{
		return NULL;
	}

	if (fread(&header, sizeof(ddatheader_t), 1, handle) != 1)
	{
		fclose(handle);
		Com_Error(ERR_FATAL, "%s: '%s' too short file",
			__func__, packPath);
	}

	if (LittleLong(header.ident) != DATHEADER ||
		LittleLong(header.version) != DATVERSION)
	{
		fclose(handle);
		Com_Error(ERR_FATAL, "%s: '%s' is not a dat file", __func__, packPath);
	}

	header.dirofs = LittleLong(header.dirofs);
	header.dirlen = LittleLong(header.dirlen);

	numFiles = header.dirlen / sizeof(*info);

	if ((numFiles == 0) || (header.dirlen < 0) || (header.dirofs < 0))
	{
		fclose(handle);
		Com_Error(ERR_FATAL, "%s: '%s' is too short.",
				__func__, packPath);
	}

	if (numFiles > MAX_FILES_IN_PACK)
	{
		Com_Printf("%s: '%s' has %i > %i files\n",
				__func__, packPath, numFiles, MAX_FILES_IN_PACK);
	}

	info = malloc(header.dirlen);
	if (!info)
	{
		Com_Error(ERR_FATAL, "%s: '%s' is to big for read %d",
				__func__, packPath, header.dirlen);
	}

	files = Z_Malloc(numFiles * sizeof(fsPackFile_t));

	if (fseek(handle, header.dirofs, SEEK_SET))
	{
		free(info);
		Z_Free(files);
		Com_Error(ERR_FATAL, "%s: '%s' seek failed", __func__, packPath);
	}

	if (fread(info, header.dirlen, 1, handle) != 1)
	{
		free(info);
		Z_Free(files);
		Com_Error(ERR_FATAL, "%s: '%s' is too short", __func__, packPath);
	}

	prefixpos = pos = packPath;
	while (*pos)
	{
		if (*pos == '\\' || *pos == '/')
		{
			prefixpos = pos;
		}
		pos++;
	}

	/* '/' + '.dat' */
	memset(prefix, 0, sizeof(prefix));
	Q_strlcpy(prefix, prefixpos + 1,
		Q_min(strlen(prefixpos) - 4, sizeof(prefix)));

	Q_strlwr(prefix);
	prefix_size = strlen(prefix);

	/* Parse the directory. */
	for (i = 0; i < numFiles; i++)
	{
		size_t name_len;

		/* name */
		memcpy(files[i].name, prefix, prefix_size);
		files[i].name[prefix_size] = '/';
		name_len = strlen(info[i].name);
		name_len = Q_min(name_len, sizeof(files[i].name) - prefix_size - 2);
		memcpy(files[i].name + prefix_size + 1, info[i].name, name_len);
		files[i].name[prefix_size + name_len + 1] = 0;

		/* fix naming */
		Q_replacebackslash(files[i].name);

		/* copy length */
		files[i].offset = LittleLong(info[i].filepos);
		files[i].size = LittleLong(info[i].filelen);
		files[i].compressed_size = LittleLong(info[i].compressed_size);
		files[i].format = files[i].compressed_size ? PAK_MODE_DAT : PAK_MODE_Q2;
	}
	free(info);

	pack = Z_Malloc(sizeof(fsPack_t));
	Q_strlcpy(pack->name, packPath, sizeof(pack->name));
	pack->pak = handle;
	pack->pk3 = NULL;
	pack->numFiles = numFiles;
	pack->files = files;

	Com_Printf("Added datfile '%s' (%i files).\n", pack->name, numFiles);

	return pack;
}

static fsPack_t *
FS_LoadSIN(const char *packPath)
{
	int i; /* Loop counter. */
	int numFiles; /* Number of files in SIN. */
	FILE *handle; /* File handle. */
	fsPackFile_t *files; /* List of files in PAK. */
	fsPack_t *pack; /* SIN file. */
	dpackheader_t header; /* SIN file header. */
	dsinfile_t *info = NULL; /* SIN info. */

	handle = Q_fopen(packPath, "rb");

	if (handle == NULL)
	{
		return NULL;
	}

	if (fread(&header, sizeof(dpackheader_t), 1, handle) != 1)
	{
		fclose(handle);
		Com_Error(ERR_FATAL, "%s: '%s' too short file",
			__func__, packPath);
	}

	if (LittleLong(header.ident) != SINHEADER)
	{
		fclose(handle);
		Com_Error(ERR_FATAL, "%s: '%s' is not a pack file", __func__, packPath);
	}

	header.dirofs = LittleLong(header.dirofs);
	header.dirlen = LittleLong(header.dirlen);

	if ((header.dirlen <= 0) ||
		(header.dirofs < 0) ||
		((header.dirlen % sizeof(*info)) != 0))
	{
		fclose(handle);
		Com_Error(ERR_FATAL, "%s: '%s' is too short.",
				__func__, packPath);
	}

	numFiles = header.dirlen / sizeof(*info);

	if (numFiles > MAX_FILES_IN_PACK)
	{
		Com_Printf("%s: '%s' has %i > %i files\n",
			__func__, packPath, numFiles, MAX_FILES_IN_PACK);
	}

	info = malloc(header.dirlen);
	if (!info)
	{
		Com_Error(ERR_FATAL, "%s: '%s' is to big for read %d",
				__func__, packPath, header.dirlen);
	}

	files = Z_Malloc(numFiles * sizeof(fsPackFile_t));

	if (fseek(handle, header.dirofs, SEEK_SET))
	{
		free(info);
		Z_Free(files);
		Com_Error(ERR_FATAL, "%s: '%s' seek failed", __func__, packPath);
	}

	if (fread(info, header.dirlen, 1, handle) != 1)
	{
		free(info);
		Z_Free(files);
		Com_Error(ERR_FATAL, "%s: '%s' is too short", __func__, packPath);
	}

	/* Parse the directory. */
	for (i = 0; i < numFiles; i++)
	{
		Q_strlcpy(files[i].name, info[i].name,
			Q_min(sizeof(files[i].name), sizeof(files[i].name)));
		files[i].offset = LittleLong(info[i].filepos);
		files[i].size = LittleLong(info[i].filelen);
		files[i].compressed_size = 0;
		files[i].format = PAK_MODE_Q2;
	}
	free(info);

	pack = Z_Malloc(sizeof(fsPack_t));
	Q_strlcpy(pack->name, packPath, sizeof(pack->name));
	pack->pak = handle;
	pack->pk3 = NULL;
	pack->numFiles = numFiles;
	pack->files = files;

	Com_Printf("Added sinfile '%s' (%i files).\n", pack->name, numFiles);

	return pack;
}

/*
 * Takes an explicit (not game tree related) path to a pak file.
 *
 * Loads the header and directory, adding the files at the beginning of the
 * list so they override previous pack files.
 */
static fsPack_t *
FS_LoadPAKQ2(dpackheader_t *header, FILE *handle, const char *packPath)
{
	int i; /* Loop counter. */
	int numFiles; /* Number of files in PAK. */
	fsPackFile_t *files; /* List of files in PAK. */
	fsPack_t *pack; /* PAK file. */
	dpackfile_t *info = NULL; /* PAK info. */

	numFiles = header->dirlen / sizeof(*info);

	if (numFiles == 0)
	{
		fclose(handle);
		Com_Error(ERR_FATAL, "%s: '%s' is too short.",
				__func__, packPath);
	}

	if (numFiles > MAX_FILES_IN_PACK)
	{
		Com_Printf("%s: '%s' has %i > %i files\n",
				__func__, packPath, numFiles, MAX_FILES_IN_PACK);
	}

	info = malloc(header->dirlen);
	if (!info)
	{
		Com_Error(ERR_FATAL, "%s: '%s' is to big for read %d",
				__func__, packPath, header->dirlen);
	}

	files = Z_Malloc(numFiles * sizeof(fsPackFile_t));

	if (fseek(handle, header->dirofs, SEEK_SET))
	{
		free(info);
		Z_Free(files);
		Com_Error(ERR_FATAL, "%s: '%s' seek failed", __func__, packPath);
	}

	if (fread(info, header->dirlen, 1, handle) != 1)
	{
		free(info);
		Z_Free(files);
		Com_Error(ERR_FATAL, "%s: '%s' is too short", __func__, packPath);
	}

	/* Parse the directory. */
	for (i = 0; i < numFiles; i++)
	{
		Q_strlcpy(files[i].name, info[i].name,
			Q_min(sizeof(files[i].name), sizeof(files[i].name)));
		files[i].offset = LittleLong(info[i].filepos);
		files[i].size = LittleLong(info[i].filelen);
		files[i].compressed_size = 0;
		files[i].format = PAK_MODE_Q2;
	}
	free(info);

	pack = Z_Malloc(sizeof(fsPack_t));
	Q_strlcpy(pack->name, packPath, sizeof(pack->name));
	pack->pak = handle;
	pack->pk3 = NULL;
	pack->numFiles = numFiles;
	pack->files = files;

	Com_Printf("Added packfile '%s' (%i files).\n", pack->name, numFiles);

	return pack;
}

static fsPack_t *
FS_LoadPAKDK(dpackheader_t *header, FILE *handle, const char *packPath)
{
	int i; /* Loop counter. */
	int numFiles; /* Number of files in PAK. */
	fsPackFile_t *files; /* List of files in PAK. */
	fsPack_t *pack; /* PAK file. */
	dpackdkfile_t *info = NULL; /* PAK info. */

	numFiles = header->dirlen / sizeof(*info);

	if (numFiles == 0)
	{
		fclose(handle);
		Com_Error(ERR_FATAL, "%s: '%s' is too short.",
				__func__, packPath);
	}

	if (numFiles > MAX_FILES_IN_PACK)
	{
		Com_Printf("%s: '%s' has %i > %i files\n",
				__func__, packPath, numFiles, MAX_FILES_IN_PACK);
	}

	info = malloc(header->dirlen);
	if (!info)
	{
		Com_Error(ERR_FATAL, "%s: '%s' is to big for read %d",
				__func__, packPath, header->dirlen);
	}

	files = Z_Malloc(numFiles * sizeof(fsPackFile_t));

	if (fseek(handle, header->dirofs, SEEK_SET))
	{
		free(info);
		Z_Free(files);
		Com_Error(ERR_FATAL, "%s: '%s' seek failed", __func__, packPath);
	}

	if (fread(info, header->dirlen, 1, handle) != 1)
	{
		free(info);
		Z_Free(files);
		Com_Error(ERR_FATAL, "%s: '%s' is too short", __func__, packPath);
	}

	/* Parse the directory. */
	for (i = 0; i < numFiles; i++)
	{
		Q_strlcpy(files[i].name, info[i].name,
			Q_min(sizeof(files[i].name), sizeof(files[i].name)));
		files[i].offset = LittleLong(info[i].filepos);
		files[i].size = LittleLong(info[i].filelen);
		if (info[i].is_compressed)
		{
			files[i].compressed_size = LittleLong(info[i].compressed_size);
			files[i].format = PAK_MODE_DK;
		}
		else
		{
			files[i].compressed_size = 0;
			files[i].format = PAK_MODE_Q2;
		}
	}
	free(info);

	pack = Z_Malloc(sizeof(fsPack_t));
	Q_strlcpy(pack->name, packPath, sizeof(pack->name));
	pack->pak = handle;
	pack->pk3 = NULL;
	pack->numFiles = numFiles;
	pack->files = files;

	Com_Printf("Added packfile '%s' (%i files).\n", pack->name, numFiles);

	return pack;
}

/*
 * Takes an explicit (not game tree related) path to a pak file.
 *
 * Loads the header and directory, adding the files at the beginning of the
 * list so they override previous pack files.
 */
static fsPack_t *
FS_LoadPAK(const char *packPath)
{
	FILE *handle; /* File handle. */
	dpackheader_t header; /* PAK file header. */

	handle = Q_fopen(packPath, "rb");

	if (handle == NULL)
	{
		return NULL;
	}

	if (fread(&header, sizeof(dpackheader_t), 1, handle) != 1)
	{
		fclose(handle);
		Com_Error(ERR_FATAL, "%s: '%s' too short file",
			__func__, packPath);
	}

	if (LittleLong(header.ident) != IDPAKHEADER)
	{
		fclose(handle);
		Com_Error(ERR_FATAL, "%s: '%s' is not a pack file",
			__func__, packPath);
	}

	header.dirofs = LittleLong(header.dirofs);
	header.dirlen = LittleLong(header.dirlen);

	if((header.dirlen <= 0) || (header.dirofs < 0))
	{
		fclose(handle);
		Com_Error(ERR_FATAL, "%s: '%s' is too short.",
				__func__, packPath);
	}

	if ((header.dirlen % sizeof(dpackfile_t)) == 0)
	{
		return FS_LoadPAKQ2(&header, handle, packPath);
	}

	if ((header.dirlen % sizeof(dpackdkfile_t)) == 0)
	{
		return FS_LoadPAKDK(&header, handle, packPath);
	}

	fclose(handle);

	Com_Printf("WARNING: skipped unsupported '%s' PAK format.\n",
			packPath);

	return NULL;
}

/*
 * Takes an explicit (not game tree related) path to a pack file.
 *
 * Loads the header and directory, adding the files at the beginning of the list
 * so they override previous pack files.
 */
static fsPack_t *
FS_LoadPK3(const char *packPath)
{
	int i = 0; /* Loop counter. */
	int numFiles; /* Number of files in PK3. */
	int status; /* Error indicator. */
	fsPackFile_t *files; /* List of files in PK3. */
	fsPack_t *pack; /* PK3 file. */
	unzFile *handle; /* Zip file handle. */
	unz_file_info info; /* Zip file info. */
	unz_global_info global; /* Zip file global info. */

#ifdef _WIN32
	handle = unzOpen2(packPath, &zlib_file_api);
#else
	handle = unzOpen(packPath);
#endif

	if (handle == NULL)
	{
		return NULL;
	}

	if (unzGetGlobalInfo(handle, &global) != UNZ_OK)
	{
		unzClose(handle);
		Com_Error(ERR_FATAL, "%s: '%s' is not a pack file", __func__, packPath);
	}

	numFiles = global.number_entry;

	if (numFiles <= 0)
	{
		unzClose(handle);
		Com_Error(ERR_FATAL, "%s: '%s' has %i files",
				__func__, packPath, numFiles);
	}

	files = Z_Malloc(numFiles * sizeof(fsPackFile_t));

	/* Parse the directory. */
	status = unzGoToFirstFile(handle);

	while (status == UNZ_OK)
	{
		char fileName[MAX_FILENAME] = {0}; /* File name. */

		unzGetCurrentFileInfo(handle, &info, fileName, sizeof(fileName),
				NULL, 0, NULL, 0);
		Q_strlcpy(files[i].name, fileName, sizeof(files[i].name));
		files[i].offset = -1; /* Not used in ZIP files */
		files[i].size = info.uncompressed_size;
		i++;
		status = unzGoToNextFile(handle);
	}

	pack = Z_Malloc(sizeof(fsPack_t));
	Q_strlcpy(pack->name, packPath, sizeof(pack->name));
	pack->pak = NULL;
	pack->pk3 = handle;
	pack->numFiles = numFiles;
	pack->files = files;

	Com_Printf("Added packfile '%s' (%i files).\n", pack->name, numFiles);

	return pack;
}

/*
 * Allows enumerating all of the directories in the search path.
 */
const char *
FS_NextPath(const char *prevPath)
{
	char *prev;
	fsSearchPath_t *search;

	if (prevPath == NULL)
	{
		return fs_gamedir;
	}

	prev = fs_gamedir;

	for (search = fs_searchPaths; search; search = search->next)
	{
		if (search->pack != NULL)
		{
			continue;
		}

		if (prevPath == prev)
		{
			return search->path;
		}

		prev = search->path;
	}

	return NULL;
}

static void
FS_Path_f(void)
{
	int i;
	int totalFiles = 0;
	fsSearchPath_t *search;
	fsHandle_t *handle;
	fsLink_t *link;

	Com_Printf("Current search path:\n");

	for (search = fs_searchPaths; search; search = search->next)
	{
		if (search->pack != NULL)
		{
			Com_Printf("%s (%i files)\n", search->pack->name,
					search->pack->numFiles);
			totalFiles += search->pack->numFiles;
		}
		else
		{
			Com_Printf("%s\n", search->path);
		}
	}

	Com_Printf("\n");

	Com_Printf("Raw search paths:\n");
	for (fsRawPath_t* raw = fs_rawPath; raw != NULL; raw = raw->next)
	{
		Com_Printf("%s\n", raw->path);
	}

	Com_Printf("\n");

	for (i = 0, handle = fs_handles; i < MAX_HANDLES; i++, handle++)
	{
		if ((handle->file != NULL) || (handle->zip != NULL))
		{
			Com_Printf("Handle %i: '%s'.\n", i + 1, handle->name);
		}
	}

	for (i = 0, link = fs_links; link; i++, link = link->next)
	{
		Com_Printf("Link %i: '%s' -> '%s'.\n", i, link->from, link->to);
	}

	Com_Printf("----------------------\n");

	Com_Printf("%i files in PAK/PK2/PK3/ZIP files.\n", totalFiles);
}

/*
 * Creates a filelink_t.
 */
static void
FS_Link_f(void)
{
	fsLink_t *l, **prev;

	if (Cmd_Argc() != 3)
	{
		Com_Printf("USAGE: link <from> <to>\n");
		return;
	}

	/* See if the link already exists. */
	prev = &fs_links;

	for (l = fs_links; l != NULL; l = l->next)
	{
		if (strcmp(l->from, Cmd_Argv(1)) == 0)
		{
			Z_Free(l->to);

			if (strlen(Cmd_Argv(2)) == 0)
			{
				/* Delete it. */
				*prev = l->next;
				Z_Free(l->from);
				Z_Free(l);
				return;
			}

			l->to = CopyString(Cmd_Argv(2));
			return;
		}

		prev = &l->next;
	}

	/* Create a new link. */
	l = Z_Malloc(sizeof(*l));
	l->next = fs_links;
	fs_links = l;
	l->from = CopyString(Cmd_Argv(1));
	l->length = strlen(l->from);
	l->to = CopyString(Cmd_Argv(2));
}

/*
 * Create a list of files that match a criteria.
 */
char **
FS_ListFiles(const char *findname, int *numfiles,
		unsigned musthave, unsigned canthave)
{
	char **list; /* List of files. */
	char *s; /* Next file in list. */
	int nfiles; /* Number of files in list. */

	/* Initialize variables. */
	list = NULL;
	nfiles = 0;

	/* Count the number of matches. */
	s = Sys_FindFirst(findname, musthave, canthave);

	while (s != NULL)
	{
		if (s[strlen(s) - 1] != '.')
		{
			nfiles++;
		}

		s = Sys_FindNext(musthave, canthave);
	}

	Sys_FindClose();

	/* Check if there are matches. */
	if (nfiles == 0)
	{
		return NULL;
	}

	nfiles++; /* Add space for a guard. */
	*numfiles = nfiles;

	/* Allocate the list. */
	list = calloc(nfiles, sizeof(char *));
	YQ2_COM_CHECK_OOM(list, "calloc()", (size_t)nfiles*sizeof(char*))

	/* Fill the list. */
	s = Sys_FindFirst(findname, musthave, canthave);
	nfiles = 0;

	while (s)
	{
		if (s[strlen(s) - 1] != '.')
		{
			list[nfiles] = strdup(s);
			nfiles++;
		}

		s = Sys_FindNext(musthave, canthave);
	}

	Sys_FindClose();

	return list;
}

/*
 * Compare file attributes (musthave and canthave) in packed files. If
 * "output" is not NULL, "size" is greater than zero and the file matches the
 * attributes then a copy of the matching string will be placed there (with
 * SFF_SUBDIR it changes).
 */
static qboolean
ComparePackFiles(const char *findname, const char *name, unsigned musthave,
		unsigned canthave, char *output, int size)
{
	qboolean retval;
	char *ptr;
	char buffer[MAX_OSPATH];

	Q_strlcpy(buffer, name, sizeof(buffer));

	if ((canthave & SFF_SUBDIR) && (name[strlen(name) - 1] == '/'))
	{
		return false;
	}

	if (musthave & SFF_SUBDIR)
	{
		if ((ptr = strrchr(buffer, '/')) != NULL)
		{
			*ptr = '\0';
		}
		else
		{
			return false;
		}
	}

	if ((musthave & SFF_HIDDEN) || (canthave & SFF_HIDDEN))
	{
		if ((ptr = strrchr(buffer, '/')) == NULL)
		{
			ptr = buffer;
		}

		if (((musthave & SFF_HIDDEN) && (ptr[1] != '.')) ||
			((canthave & SFF_HIDDEN) && (ptr[1] == '.')))
		{
			return false;
		}
	}

	if (canthave & SFF_RDONLY)
	{
		return false;
	}

	retval = glob_match((char *)findname, buffer);

	if (retval && (output != NULL))
	{
		Q_strlcpy(output, buffer, size);
	}

	return retval;
}

/*
 * Create a list of files that match a criteria.
 * Searchs are relative to the game directory and use all the search paths
 * including .pak and .pk3 files.
 */
char **
FS_ListFiles2(const char *findname, int *numfiles,
		unsigned musthave, unsigned canthave)
{
	fsSearchPath_t *search; /* Search path. */
	int i, j; /* Loop counters. */
	int nfiles; /* Number of files found. */
	int tmpnfiles; /* Temp number of files. */
	char **tmplist; /* Temporary list of files. */
	char **list; /* List of files found. */
	char path[MAX_OSPATH]; /* Temporary path. */

	nfiles = 0;
	list = malloc(sizeof(char *));
	YQ2_COM_CHECK_OOM(list, "malloc()", sizeof(char*))

	for (search = fs_searchPaths; search != NULL; search = search->next)
	{
		if (search->pack != NULL)
		{
			if (canthave & SFF_INPACK)
			{
				continue;
			}

			for (i = 0, j = 0; i < search->pack->numFiles; i++)
			{
				if (ComparePackFiles(findname, search->pack->files[i].name,
							musthave, canthave, NULL, 0))
				{
					j++;
				}
			}

			if (j == 0)
			{
				continue;
			}

			nfiles += j;
			list = realloc(list, nfiles * sizeof(char *));
			YQ2_COM_CHECK_OOM(list, "realloc()", (size_t)nfiles*sizeof(char*))

			for (i = 0, j = nfiles - j; i < search->pack->numFiles; i++)
			{
				if (ComparePackFiles(findname, search->pack->files[i].name,
							musthave, canthave, path, sizeof(path)))
				{
					list[j++] = strdup(path);
				}
			}
		}

		if (musthave & SFF_INPACK)
		{
			continue;
		}

		Com_sprintf(path, sizeof(path), "%s/%s", search->path, findname);
		tmplist = FS_ListFiles(path, &tmpnfiles, musthave, canthave);

		if (tmplist != NULL)
		{
			tmpnfiles--;
			nfiles += tmpnfiles;
			list = realloc(list, nfiles * sizeof(char *));
			YQ2_COM_CHECK_OOM(list, "2nd realloc()", (size_t)nfiles*sizeof(char*))

			for (i = 0, j = nfiles - tmpnfiles; i < tmpnfiles; i++, j++)
			{
				list[j] = strdup(tmplist[i] + strlen(search->path) + 1);
			}

			FS_FreeList(tmplist, tmpnfiles + 1);
		}
	}

	/* Delete duplicates. */
	tmpnfiles = 0;

	for (i = 0; i < nfiles; i++)
	{
		if (list[i] == NULL)
		{
			continue;
		}

		for (j = i + 1; j < nfiles; j++)
		{
			if ((list[j] != NULL) &&
				(strcmp(list[i], list[j]) == 0))
			{
				free(list[j]);
				list[j] = NULL;
				tmpnfiles++;
			}
		}
	}

	if (tmpnfiles > 0)
	{
		nfiles -= tmpnfiles;
		tmplist = malloc(nfiles * sizeof(char *));
		YQ2_COM_CHECK_OOM(tmplist, "malloc()", (size_t)nfiles*sizeof(char*))

		for (i = 0, j = 0; i < nfiles + tmpnfiles; i++)
		{
			if (list[i] != NULL)
			{
				tmplist[j++] = list[i];
			}
		}

		free(list);
		list = tmplist;
	}

	/* Add a guard. */
	if (nfiles > 0)
	{
		nfiles++;
		list = realloc(list, nfiles * sizeof(char *));
		YQ2_COM_CHECK_OOM(list, "3rd realloc()", (size_t)nfiles*sizeof(char*))
		list[nfiles - 1] = NULL;
	}

	else
	{
		free(list);
		list = NULL;
	}

	*numfiles = nfiles;

	return list;
}

/*
 * Free list of files created by FS_ListFiles().
 */
void
FS_FreeList(char **list, int nfiles)
{
	int i;

	for (i = 0; i < nfiles - 1; i++)
	{
		free(list[i]);
		list[i] = 0;
	}

	free(list);
	list = 0;
}

/*
 * Comparator for mod sorting
 */
static int
Q_sort_modcmp(const void *p1, const void *p2)
{
	static const char *first_mods[] = {BASEDIRNAME, "xatrix", "rogue", "ctf"};
	static const unsigned short int first_mods_qty = 4;

	const char * s1 = * (char * const *)p1;
	const char * s2 = * (char * const *)p2;

	for (unsigned short int i = 0; i < first_mods_qty; i++)
	{
		if (!Q_stricmp(first_mods[i], s1))
		{
			return -1;
		}
		if (!Q_stricmp(first_mods[i], s2))
		{
			return 1;
		}
	}
	return Q_stricmp(s1, s2);
}

/*
 * Combs all Raw search paths to find game dirs containing PAK/PK2/PK3 files.
 * Returns an alphabetized array of unique relative dir names.
 */
char**
FS_ListMods(int *nummods)
{
	int nmods = 0, numdirchildren, numpacksinchilddir;
	size_t searchpathlength;
	char findnamepattern[MAX_OSPATH], modname[MAX_QPATH], searchpath[MAX_OSPATH];
	char **dirchildren, **packsinchilddir, **modnames;

	modnames = malloc((MAX_QPATH + 1) * (MAX_MODS + 1));
	memset(modnames, 0, (MAX_QPATH + 1) * (MAX_MODS + 1));

	// iterate over all Raw paths
	for (fsRawPath_t *search = fs_rawPath; search; search = search->next)
	{
		searchpathlength = strlen(search->path);
		if(!searchpathlength)
		{
			continue;
		}

		// make sure this Raw path ends with a '/' otherwise FS_ListFiles will open its parent dir
		if(search->path[searchpathlength - 1] != '/')
		{
			Com_sprintf(searchpath, sizeof(searchpath), "%s/*", search->path);
		}
		else
		{
			Com_sprintf(searchpath, sizeof(searchpath), "%s*", search->path);
		}

		dirchildren = FS_ListFiles(searchpath, &numdirchildren, 0, 0);

		if (dirchildren == NULL)
		{
			continue;
		}

		// iterate over the children of this Raw path (unless we've already got enough mods)
		for (int i = 0; i < numdirchildren && nmods < MAX_MODS; i++)
		{
			if(dirchildren[i] == NULL)
			{
				continue;
			}

			numpacksinchilddir = 0;

			// iterate over supported pack types, but ignore ZIP files (they cause false positives)
			for (int j = 0; j < sizeof(fs_packtypes) / sizeof(fs_packtypes[0]); j++)
			{
				if (strcmp("zip", fs_packtypes[j].suffix) != 0)
				{
					Com_sprintf(findnamepattern, sizeof(findnamepattern), "%s/*.%s", dirchildren[i], fs_packtypes[j].suffix);

					packsinchilddir = FS_ListFiles(findnamepattern, &numpacksinchilddir, 0, 0);
					FS_FreeList(packsinchilddir, numpacksinchilddir);

					// if this dir has some pack files, add it if not already in the list
					if (numpacksinchilddir > 0)
					{
						qboolean matchfound = false;

						Com_sprintf(modname, sizeof(modname), "%s", strrchr(dirchildren[i], '/') + 1);

						for (int k = 0; k < nmods; k++)
						{
							if (strcmp(modname, modnames[k]) == 0)
							{
								matchfound = true;
								break;
							}
						}

						if (!matchfound)
						{
							modnames[nmods] = malloc(strlen(modname) + 1);
							strcpy(modnames[nmods], modname);

							nmods++;
						}

						break;
					}
				}
			}
		}

		FS_FreeList(dirchildren, numdirchildren);
	}

	modnames[nmods] = 0;

	qsort(modnames, nmods, sizeof(modnames[0]), Q_sort_modcmp);

	*nummods = nmods;
	return modnames;
}

/*
 * Directory listing.
 */
static void
FS_Dir_f(void)
{
	char **dirnames; /* File list. */
	char findname[1024]; /* File search path and pattern. */
	const char *path = NULL; /* Search path. */
	char *lastsep;
	char wildcard[1024] = "*.*"; /* File pattern. */
	int i; /* Loop counter. */
	int ndirs; /* Number of files in list. */

	/* Check for pattern in arguments. */
	if (Cmd_Argc() != 1)
	{
		Q_strlcpy(wildcard, Cmd_Argv(1), sizeof(wildcard));
	}

	/* Scan search paths and list files. */
	while ((path = FS_NextPath(path)) != NULL)
	{
		Com_sprintf(findname, sizeof(findname), "%s/%s", path, wildcard);
		Com_Printf("Directory of '%s'.\n", findname);
		Com_Printf("----\n");

		if ((dirnames = FS_ListFiles(findname, &ndirs, 0, 0)) != 0)
		{
			for (i = 0; i < ndirs - 1; i++)
			{
				lastsep = strrchr(dirnames[i], '/');
				if (lastsep)
				{
					Com_Printf("%s\n", lastsep + 1);
				}
				else
				{
					Com_Printf("%s\n", dirnames[i]);
				}
			}

			FS_FreeList(dirnames, ndirs);
		}

		Com_Printf("\n");
	}
}

// --------

/*
 * This function returns true if a real file (e.g. not something
 * in a pak, something in the file system itself) exists in the
 * current gamedir.
 */
qboolean
FS_FileInGamedir(const char *file)
{
	char path[MAX_OSPATH];
	FILE *fd;

	Com_sprintf(path, sizeof(path), "%s/%s", fs_gamedir, file);

	if ((fd = Q_fopen(path, "rb")) != NULL)
	{
		fclose(fd);
		return true;
	}
	else
	{
		return false;
	}
}

/*
 * This function loads the given .pak / .pk3 File from the
 * fs_gamedir. There's no need to load from other dirs since
 * fs_gamedir is the only dir written to at runtime.
 */
qboolean
FS_AddPAKFromGamedir(const char *pak)
{
	char path[MAX_OSPATH];

	Com_sprintf(path, sizeof(path), "%s/%s", fs_gamedir, pak);

	// Check of the file really exists.
	FILE *fd;

	if ((fd = Q_fopen(path, "rb")) == NULL)
	{
		assert(fd && "FS_AddPAKfromGamedir() called with nonexisting file");;
	}
	else
	{
		fclose(fd);
	}

	// Depending on filetype we must load it as .pak or .pk3.
	for (int i = 0; i < sizeof(fs_packtypes) / sizeof(fs_packtypes[0]); i++)
	{
		// Not the current filetype, next one please.
		if (strncmp(pak + strlen(pak) - strlen(fs_packtypes[i].suffix), fs_packtypes[i].suffix, strlen(fs_packtypes[i].suffix)))
		{
			continue;
		}

		fsPack_t *pakfile = NULL;

		switch (fs_packtypes[i].format)
		{
			case DAT:
				pakfile = FS_LoadDAT(path);
				break;
			case SIN:
				pakfile = FS_LoadSIN(path);
				break;
			case PAK:
				pakfile = FS_LoadPAK(path);
				break;
			case PK3:
				pakfile = FS_LoadPK3(path);
				break;
		}

		if (pakfile == NULL)
		{
			// Couldn't load it.
			return false;
		}
		else
		{
			fsSearchPath_t *search;

			FS_SortPack(pakfile);

			// Add it.
			search = Z_Malloc(sizeof(fsSearchPath_t));
			search->pack = pakfile;
			search->next = fs_searchPaths;
			fs_searchPaths = search;

			return true;
		}
	}

	// Apparently we didn't load anything.
	return false;
}

const char*
FS_GetNextRawPath(const char* lastRawPath)
{
	assert(fs_rawPath != NULL && "Don't call this if before FS_InitFilesystem()");

	if (lastRawPath == NULL)
	{
		return fs_rawPath->path;
	}

	for (fsRawPath_t* rp = fs_rawPath; rp != NULL; rp = rp->next)
	{
		if (rp->path == lastRawPath)
		{
			return (rp->next != NULL) ? rp->next->path : NULL;
		}
	}

	return NULL;
}

#ifdef _MSC_VER // looks like MSVC/the Windows CRT doesn't have basename()
// returns the last part of the given pathname, after last (back)slash
// if the last character is a (back)slash, it's removed (set to '\0')
static char* basename( char* n )
{
	size_t l = strlen(n);
	while (n[l - 1] == '\\' || n[l - 1] == '/') // cut off trailing (back)slashes, if any
	{
		--l;
		n[l] = '\0';
	}
	char* r1 = strrchr(n, '\\');
	char* r2 = strrchr(n, '/');
	if (r1 != NULL)
		return (r2 == NULL || r1 > r2) ? (r1 + 1) : (r2 + 1);
	return (r2 != NULL) ? (r2 + 1) : n;
}
#endif // _MSC_VER

static void
FS_AddDirToSearchPath(char *dir, qboolean create) {
	char *file;
	char **list;
	char path[MAX_OSPATH];
	char *tmp;
	int i, j, k;
	int nfiles;
	fsPack_t *pack = NULL;
	fsSearchPath_t *search;
	qboolean nextpak;
	size_t len = strlen(dir);

	// The directory must not end with an /. It would
	// f*ck up the logic in other parts of the game...
	if (dir[len - 1] == '/' || dir[len - 1] == '\\')
	{
		dir[len - 1] = '\0';
	}

	// Set the current directory as game directory. This
	// is somewhat fragile since the game directory MUST
	// be the last directory added to the search path.
	Q_strlcpy(fs_gamedir, dir, sizeof(fs_gamedir));

	if (create)
	{
		FS_CreatePath(fs_gamedir);
	}

	// Add the directory itself.
	search = Z_Malloc(sizeof(fsSearchPath_t));
	Q_strlcpy(search->path, dir, sizeof(search->path));
	search->next = fs_searchPaths;
	fs_searchPaths = search;

	/* remaster additional files */
	pack = FS_LoadPK3("Q2Game.kpf");
	if (pack)
	{
		pack->isProtectedPak = true;

		FS_SortPack(pack);

		search = Z_Malloc(sizeof(fsSearchPath_t));
		search->pack = pack;
		search->next = fs_searchPaths;
		fs_searchPaths = search;
	}

	// Numbered paks contain the official game data, they
	// need to be added first and are marked protected.
	// Files from protected paks are never offered for
	// download.
	for (i = 0; i < sizeof(fs_packtypes) / sizeof(fs_packtypes[0]); i++)
	{
		for (j = 0; j < MAX_PAKS; j++)
		{
			Com_sprintf(path, sizeof(path), "%s/pak%d.%s", dir, j, fs_packtypes[i].suffix);

			switch (fs_packtypes[i].format)
			{
				case DAT:
					pack = FS_LoadDAT(path);

					if (pack)
					{
						pack->isProtectedPak = true;
					}

					break;
				case SIN:
					pack = FS_LoadSIN(path);

					if (pack)
					{
						pack->isProtectedPak = true;
					}

					break;
				case PAK:
					pack = FS_LoadPAK(path);

					if (pack)
					{
						pack->isProtectedPak = true;
					}

					break;
				case PK3:
					pack = FS_LoadPK3(path);

					if (pack)
					{
						pack->isProtectedPak = false;
					}

					break;
			}

			if (pack == NULL)
			{
				continue;
			}

			FS_SortPack(pack);

			search = Z_Malloc(sizeof(fsSearchPath_t));
			search->pack = pack;
			search->next = fs_searchPaths;
			fs_searchPaths = search;
		}
	}

	// All other pak files are added after the numbered paks.
	// They aren't sorted in any way, but added in the same
	// sequence as they're returned by FS_ListFiles. This is
	// fragile and file system dependend. We cannot change
	// this, since it might break existing installations.
	for (i = 0; i < sizeof(fs_packtypes) / sizeof(fs_packtypes[0]); i++)
	{
		Com_sprintf(path, sizeof(path), "%s/*.%s", dir, fs_packtypes[i].suffix);

		// Nothing here, next pak type please.
		if ((list = FS_ListFiles(path, &nfiles, 0, 0)) == NULL)
		{
			continue;
		}

		for (j = 0; j < nfiles - 1; j++)
		{
			// Sort out numbered paks. This is as inefficient as
			// it can be, but it doesn't matter. This is done only
			// once at client or game startup.
			nextpak = false;

			for (k = 0; k < MAX_PAKS; k++)
			{
				// basename() may alter the given string.
				// We need to work around that...
				tmp = strdup(list[j]);
				file = basename(tmp);

				Com_sprintf(path, sizeof(path), "pak%d.%s", k, fs_packtypes[i].suffix);

				if (Q_strcasecmp(path, file) == 0)
				{
					nextpak = true;
					free(tmp);
					break;
				}

				free(tmp);
			}

			if (nextpak)
			{
				continue;
			}

			switch (fs_packtypes[i].format)
			{
				case DAT:
					pack = FS_LoadDAT(list[j]);
					break;
				case SIN:
					pack = FS_LoadSIN(list[j]);
					break;
				case PAK:
					pack = FS_LoadPAK(list[j]);
					break;
				case PK3:
					pack = FS_LoadPK3(list[j]);
					break;
			}

			if (pack == NULL)
			{
				continue;
			}

			FS_SortPack(pack);

			pack->isProtectedPak = false;

			search = Z_Malloc(sizeof(fsSearchPath_t));
			search->pack = pack;
			search->next = fs_searchPaths;
			fs_searchPaths = search;
		}

		FS_FreeList(list, nfiles);
	}
}

static void
FS_BuildGenericSearchPath(void) {
	// We may not use the va() function from shared.c
	// since it's buffersize is 1024 while most OS have
	// a maximum path size of 4096...
	char path[MAX_OSPATH];

	fsRawPath_t *search = fs_rawPath;

	while (search != NULL) {
		Com_sprintf(path, sizeof(path), "%s/%s", search->path, BASEDIRNAME);
		FS_AddDirToSearchPath(path, search->create);

		search = search->next;
	}

	// Until here we've added the generic directories to the
	// search path. Save the current head node so we can
	// distinguish generic and specialized directories.
	fs_baseSearchPaths = fs_searchPaths;

	// We need to create the game directory.
	Sys_Mkdir(fs_gamedir);

	// We need to create the screenshot directory since the
	// render dll doesn't link the filesystem stuff.
	Com_sprintf(path, sizeof(path), "%s/scrnshot", fs_gamedir);
	Sys_Mkdir(path);
}

// filesystem.c is used by the client and the server,
// it includes common.h only. Not the client not the
// server header. The game reset logic messes with
// client state, so we need some forwar declarations
// here.
#ifndef DEDICATED_ONLY
// Variables
extern qboolean menu_startdemoloop;

#endif

void
FS_BuildGameSpecificSearchPath(const char *dir)
{
	// We may not use the va() function from shared.c
	// since it's buffersize is 1024 while most OS have
	// a maximum path size of 4096...
	char path[MAX_OSPATH] = {0};
	int i;
	fsRawPath_t *search;

#ifndef DEDICATED_ONLY
	// Write the config. Otherwise changes made by the
	// current mod are lost.
	CL_WriteConfiguration();
#endif

	// empty string means baseq2
	if(dir[0] == '\0')
	{
		dir = BASEDIRNAME;
	}

	// This is against PEBCAK. The user may give us paths like
	// xatrix/ or even /home/stupid/quake2/xatrix.
	if (!strcmp(dir, ".") || strstr(dir, "..") || strstr(dir, "/") || strstr(dir, "\\"))
	{
		Com_Printf("Gamedir should be a single filename, not a path.\n");
		return;
	}

	// We may already have specialised directories in our search
	// path. This can happen if the server changes the mod. Let's
	// remove them.
	fs_searchPaths = FS_FreeSearchPaths(fs_searchPaths, fs_baseSearchPaths);

	/* Close open files for game dir. */
	for (i = 0; i < MAX_HANDLES; i++)
	{
		if (strstr(fs_handles[i].name, dir) && ((fs_handles[i].file != NULL) || (fs_handles[i].zip != NULL)))
		{
			FS_FCloseFile(i);
		}
	}

	// Enforce a renderer and sound backend restart to
	// purge all internal caches. This is rather hacky
	// but Quake II doesn't have a better mechanism...
	if ((dedicated != NULL) && (dedicated->value != 1))
	{
		Cbuf_AddText("vid_restart\nsnd_restart\n");
	}

	// The game was reset to baseq2. Nothing to do here.
	if (Q_stricmp(dir, BASEDIRNAME) == 0) {
		Cvar_FullSet("gamedir", "", CVAR_SERVERINFO | CVAR_NOSET);
		Cvar_FullSet("game", "", CVAR_LATCH | CVAR_SERVERINFO);

		// fs_gamedir must be reset to the last
		// dir of the generic search path.
		Com_sprintf(path, sizeof(path), "%s/%s", fs_rawPath->path, BASEDIRNAME);
		Q_strlcpy(fs_gamedir, path, sizeof(fs_gamedir));
	} else {
		Cvar_FullSet("gamedir", dir, CVAR_SERVERINFO | CVAR_NOSET);
		search = fs_rawPath;

		while (search != NULL) {
			Com_sprintf(path, sizeof(path), "%s/%s", search->path, dir);
			FS_AddDirToSearchPath(path, search->create);

			search = search->next;
		}
	}

	// Create the game directory.
	Sys_Mkdir(fs_gamedir);

	// We need to create the screenshot directory since the
	// render dll doesn't link the filesystem stuff.
	Com_sprintf(path, sizeof(path), "%s/scrnshot", fs_gamedir);
	Sys_Mkdir(path);

	// the gamedir has changed, so read in the corresponding configs
	Qcommon_ExecConfigs(false);

#ifndef DEDICATED_ONLY
	// This function is called whenever the game cvar changes =>
	// the player wants to switch to another mod. In that case the
	// list of music tracks needs to be loaded again (=> tracks
	// are possibly from the new mod dir)
	OGG_InitTrackList();

	// ...and the current list of maps in the "start network server" menu is
	// cleared so that it will be re-initialized when the menu is accessed
	if (mapnames != NULL)
	{
		for (i = 0; i < nummaps; i++)
		{
			free(mapnames[i]);
		}

		free(mapnames);
		mapnames = NULL;
	}

	// Start the demoloop, if requested. This is kind of hacky: Normaly the
	// demo loop would be started by the menu, after changeing the 'game'
	// cvar. However, the demo loop is implemented by aliases. Since the
	// game isn't changed right after the cvar is set (but when the control
	// flow enters this function) the aliases evaulate to the wrong game.
	// Work around that by injection the demo loop into the command buffer
	// here, right after the game was changed. Do it only when the game was
	// changed though the menu, otherwise we might break into the demo loop
	// after we've received a latched cvar from the server.
	if (menu_startdemoloop)
	{
		Cbuf_AddText("d1\n");
		menu_startdemoloop = false;
	}
#endif
}

// returns the filename used to open f, but (if opened from pack) in correct case
// returns NULL if f is no valid handle
const char* FS_GetFilenameForHandle(fileHandle_t f)
{
	fsHandle_t* fsh = FS_GetFileByHandle(f);
	if(fsh)
	{
		return fsh->name;
	}
	return NULL;
}

// --------

static void FS_AddDirToRawPath (const char *rawdir, qboolean create, qboolean required) {
	char dir[MAX_OSPATH] = {0};

	// Get the realpath.
	if (!Sys_Realpath(rawdir, dir, sizeof(dir)))
	{
		if (required)
		{
			Com_Error(ERR_FATAL, "Couldn't add required directory %s to search path\n", rawdir);
		}

		return;
	}

	// Convert backslashes to forward slashes.
	Q_replacebackslash(dir);

	// Make sure that the dir doesn't end with a slash.
	for (size_t s = strlen(dir) - 1; s > 0; s--)
	{
		if (dir[s] == '/')
		{
			dir[s] = '\0';
		}
		else
		{
			break;
		}
	}

	// Bail out if the dir was already added.
	for (fsRawPath_t *search = fs_rawPath; search; search = search->next)
	{
		if (strcmp(search->path, dir) == 0)
		{
			return;
		}
	}

	// Add the directory.
	fsRawPath_t *search = Z_Malloc(sizeof(fsRawPath_t));
	Q_strlcpy(search->path, dir, sizeof(search->path));
	search->create = create;
	search->next = fs_rawPath;
	fs_rawPath = search;
}


static void
FS_BuildRawPath(void)
{
	// Add $HOME/.yq2, MUST be the last dir! Required,
	// otherwise the config cannot be written.
	if (!is_portable) {
		const char *homedir = Sys_GetHomeDir();

		if (homedir != NULL) {
			FS_AddDirToRawPath(homedir, true, true);
		}
	}

	// Add binary dir. Required, because the renderer
	// libraries are loaded from it.
	const char *binarydir = Sys_GetBinaryDir();

	if(binarydir[0] != '\0')
	{
		FS_AddDirToRawPath(binarydir, false, true);
	}

	// Add data dir. Required, when the user gives us
	// a data dir he expects it in a working state.
	FS_AddDirToRawPath(datadir, false, true);

	// Add SYSTEMDIR. Optional, the user may have a
	// binary compiled with SYSTEMWIDE (installed from
	// packages), but no systemwide game data.
#ifdef SYSTEMWIDE
	FS_AddDirToRawPath(SYSTEMDIR, false, false);
#endif

	// The CD must be the last directory of the path,
	// otherwise we cannot be sure that the game won't
	// stream the videos from the CD. Required, if the
	// user sets a CD path, he expects data getting
	// read from the CD.
	if (fs_cddir->string[0] != '\0') {
		FS_AddDirToRawPath(fs_cddir->string, false, true);
	}
}

// --------

void
FS_InitFilesystem(void)
{
	// Register FS commands.
	Cmd_AddCommand("path", FS_Path_f);
	Cmd_AddCommand("link", FS_Link_f);
	Cmd_AddCommand("dir", FS_Dir_f);

	// Register cvars
	fs_basedir = Cvar_Get("basedir", ".", CVAR_NOSET);
	fs_cddir = Cvar_Get("cddir", "", CVAR_NOSET);
	fs_gamedirvar = Cvar_Get("game", "", CVAR_LATCH | CVAR_SERVERINFO);
	fs_debug = Cvar_Get("fs_debug", "0", 0);

	// Deprecation warning, can be removed at a later time.
	if (strcmp(fs_basedir->string, ".") != 0)
	{
		Com_Printf("+set basedir is deprecated, use -datadir instead\n");
		Q_strlcpy(datadir, fs_basedir->string, sizeof(datadir));
	}
	else if (strlen(datadir) == 0)
	{
		strcpy(datadir, ".");
	}

#ifdef _WIN32
	// setup minizip for Unicode compatibility
	fill_fopen_filefunc(&zlib_file_api);
	zlib_file_api.zopen_file = fopen_file_func_utf;
#endif

	// Build search path
	FS_BuildRawPath();
	FS_BuildGenericSearchPath();

	if (fs_gamedirvar->string[0] != '\0')
	{
		FS_BuildGameSpecificSearchPath(fs_gamedirvar->string);
	}
#ifndef DEDICATED_ONLY
	else
	{
		// no mod, but we still need to get the list of OGG tracks for background music
		OGG_InitTrackList();
	}
#endif

	// Debug output
	Com_Printf("Using '%s' for writing.\n", fs_gamedir);
}


void
FS_ShutdownFilesystem(void)
{
	fs_searchPaths = FS_FreeSearchPaths(fs_searchPaths, NULL);
	fs_rawPath = FS_FreeRawPaths(fs_rawPath, NULL);

	fs_baseSearchPaths = NULL;
}
