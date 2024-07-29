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
 * The collision model. Slaps "boxes" through the world and checks if
 * they collide with the world model, entities or other boxes.
 *
 * =======================================================================
 */

#include "header/common.h"

static void
Mod_LoadSkinList_MD2(const char *mod_name, const void *buffer, int modfilelen,
	char **skins, int *numskins)
{
	dmdl_t pinmodel;
	int i;

	if (modfilelen < sizeof(pinmodel))
	{
		Com_Printf("%s: %s has incorrect header size (%i should be " YQ2_COM_PRIdS ")\n",
				__func__, mod_name, modfilelen, sizeof(pinmodel));
		return;
	}

	for (i = 0; i < sizeof(pinmodel) / sizeof(int); i++)
	{
		((int *)&pinmodel)[i] = LittleLong(((int *)buffer)[i]);
	}

	if (pinmodel.version != ALIAS_VERSION)
	{
		Com_Printf("%s: %s has wrong version number (%i should be %i)\n",
				__func__, mod_name, pinmodel.version, ALIAS_VERSION);
	}

	if (pinmodel.ofs_end < 0 || pinmodel.ofs_end > modfilelen)
	{
		Com_Printf("%s: model %s file size(%d) too small, should be %d\n",
				__func__, mod_name, modfilelen, pinmodel.ofs_end);
	}

	if (pinmodel.num_skins < 0)
	{
		Com_Printf("%s: model %s file has incorrect skins count %d\n",
				__func__, mod_name, pinmodel.num_skins);
	}

	*numskins = pinmodel.num_skins;
	*skins = malloc(pinmodel.num_skins * MAX_SKINNAME);

	memcpy(*skins, (char *)buffer + pinmodel.ofs_skins,
		pinmodel.num_skins * MAX_SKINNAME);
}

static void
Mod_LoadSkinList(const char *mod_name, const void *buffer, int modfilelen,
	char **skins, int *numskins)
{
	switch (LittleLong(*(unsigned *)buffer))
	{
		case IDALIASHEADER:
			Mod_LoadSkinList_MD2(mod_name, buffer, modfilelen,
				skins, numskins);
			break;
	}
}

/* Add md5 to full file name */
static void
Mod_LoadFileInsertMD5(char *newname, const char *oldname, int size)
{
	const char *filename;

	filename = COM_SkipPath(oldname);
	memset(newname, 0, size);
	memcpy(newname, oldname, strlen(oldname) - strlen(filename));
	Q_strlcat(newname, "md5/", size);
	Q_strlcat(newname, filename, size);
}

static int
Mod_LoadFileMD5Merge(const char *namewe, void **buffer)
{
	int fullsize, filesize_anim, filesize, filesize_skins;
	char *final_buffer = NULL, *skins_list = NULL;
	void *anim_buffer = NULL, *skins_buffer = NULL;
	qboolean md5path = false;
	char newname[256];

	/* search mesh file */
	Q_strlcpy(newname, namewe, sizeof(newname));
	Q_strlcat(newname, ".md5mesh", sizeof(newname));
	filesize = FS_LoadFile(newname, buffer);

	/* check overwrite file */
	if (filesize <= 0)
	{
		char md5modelname[256];

		Mod_LoadFileInsertMD5(md5modelname, newname, sizeof(md5modelname));

		filesize = FS_LoadFile(md5modelname, buffer);
		/* no replace file */
		if (filesize <= 0)
		{
			return filesize;
		}

		md5path = true;
		strcpy(newname, md5modelname);
	}

	/* search animation file */
	memcpy(newname + strlen(newname) - strlen("mesh"), "anim", strlen("anim"));
	filesize_anim = FS_LoadFile(newname, &anim_buffer);
	if (filesize_anim <= 0)
	{
		FS_FreeFile(*buffer);
		return filesize;
	}

	/* search skins list */
	Q_strlcpy(newname, namewe, sizeof(newname));
	Q_strlcat(newname, ".md2", sizeof(newname));
	filesize_skins = FS_LoadFile(newname, &skins_buffer);
	if (filesize_skins > 0)
	{
		char *skins = NULL;
		int numskins = 0, i;

		Mod_LoadSkinList(newname, skins_buffer, filesize_skins,
			&skins, &numskins);
		FS_FreeFile(skins_buffer);

		/*
		 * 20 -> numSkins <num> | skin <num> "MAX_SKINNAME" + md5
		 */
		skins_list = malloc((numskins + 1) * (MAX_SKINNAME + 20));
		sprintf(skins_list, "\nnumSkins %d\n", numskins);
		for(i = 0; i < numskins; i++)
		{
			const char *skinname = skins + MAX_SKINNAME * i;

			if (!md5path)
			{
				sprintf(skins_list + strlen(skins_list), "skin %d \"%s\"\n",
					i, skinname);
			}
			else
			{
				char md5skinname[256];

				Mod_LoadFileInsertMD5(md5skinname, skinname, sizeof(md5skinname));

				sprintf(skins_list + strlen(skins_list), "skin %d \"%s\"\n",
					i, md5skinname);
			}
		}
	}

	/* prepare final file */
	fullsize = filesize + filesize_anim + 1;
	if (skins_list)
	{
		fullsize += strlen(skins_list);
	}

	/* allocate new buffer, ERR_FATAL on alloc fail */
	final_buffer = Z_Malloc(fullsize);

	/* copy combined information */
	memcpy(final_buffer, *buffer, filesize);
	if (skins_list)
	{
		memcpy(final_buffer + filesize, skins_list, strlen(skins_list));
		filesize += strlen(skins_list);
		free(skins_list);
	}
	final_buffer[filesize] = 0;
	memcpy(final_buffer + filesize + 1, anim_buffer, filesize_anim);

	/* Remove old buffers */
	FS_FreeFile(anim_buffer);
	FS_FreeFile(*buffer);

	*buffer = final_buffer;
	return fullsize;
}

static int
Mod_LoadFileWithoutExt(const char *namewe, void **buffer, const char* ext)
{
	char newname[256];
	size_t tlen;

	*buffer = NULL;

	tlen = strlen(namewe);

	if (!strcmp(ext, "fm") ||
		!strcmp(ext, "def") ||
		!strcmp(ext, "dkm") ||
		!strcmp(ext, "md2") ||
		!strcmp(ext, "md3") ||
		!strcmp(ext, "mdr") ||
		!strcmp(ext, "md5mesh") ||
		!strcmp(ext, "mdx") ||
		!strcmp(ext, "mdl"))
	{
		int filesize;

		/* Check ReRelease / Doom 3 / Quake 4 model */
		filesize = Mod_LoadFileMD5Merge(namewe, buffer);
		if (filesize > 0)
		{
			Com_DPrintf("%s: %s loaded as md5 (Doom 3)\n",
				__func__, namewe);
			return filesize;
		}

		/* Check Quake 3 model */
		Q_strlcpy(newname, namewe, sizeof(newname));
		Q_strlcpy(newname + tlen, ".mdr", sizeof(newname));
		filesize = FS_LoadFile(newname, buffer);
		if (filesize > 0)
		{
			Com_DPrintf("%s: %s loaded as mdr/md4 (Star Trek: Elite Force)\n",
				__func__, namewe);
			return filesize;
		}

		/* Check Quake 3 model */
		Q_strlcpy(newname, namewe, sizeof(newname));
		Q_strlcpy(newname + tlen, ".md3", sizeof(newname));
		filesize = FS_LoadFile(newname, buffer);
		if (filesize > 0)
		{
			Com_DPrintf("%s: %s loaded as md3 (Quake 3)\n",
				__func__, namewe);
			return filesize;
		}

		/* Check Heretic2 model */
		Q_strlcpy(newname, namewe, sizeof(newname));
		Q_strlcat(newname, ".fm", sizeof(newname));
		filesize = FS_LoadFile(newname, buffer);
		if (filesize > 0)
		{
			Com_DPrintf("%s: %s loaded as fm (Heretic 2)\n",
				__func__, namewe);
			return filesize;
		}

		/* Check SiN model def with include sbm/sam files */
		Q_strlcpy(newname + tlen, ".def", sizeof(newname));
		filesize = FS_LoadFile(newname, buffer);
		if (filesize > 0)
		{
			Com_DPrintf("%s: %s loaded as def/sbm/sam (SiN)\n",
				__func__, namewe);
			return filesize;
		}

		/* Check Quake 2 model */
		Q_strlcpy(newname + tlen, ".md2", sizeof(newname));
		filesize = FS_LoadFile(newname, buffer);
		if (filesize > 0)
		{
			Com_DPrintf("%s: %s loaded as md2 (Quake 2/Anachronox)\n",
				__func__, namewe);
			return filesize;
		}

		/* Check Kingpin model */
		Q_strlcpy(newname + tlen, ".mdx", sizeof(newname));
		filesize = FS_LoadFile(newname, buffer);
		if (filesize > 0)
		{
			Com_DPrintf("%s: %s loaded as mdx (Kingpin)\n",
				__func__, namewe);
			return filesize;
		}

		/* Check Daikatana model */
		Q_strlcpy(newname + tlen, ".dkm", sizeof(newname));
		filesize = FS_LoadFile(newname, buffer);
		if (filesize > 0)
		{
			Com_DPrintf("%s: %s loaded as dkm (Daikatana)\n",
				__func__, namewe);
			return filesize;
		}

		/* Check Quake model */
		Q_strlcpy(newname + tlen, ".mdl", sizeof(newname));
		filesize = FS_LoadFile(newname, buffer);
		if (filesize > 0)
		{
			Com_DPrintf("%s: %s loaded as mdl (Quake 1)\n",
				__func__, namewe);
			return filesize;
		}
	}

	if (!strcmp(ext, "bsp"))
	{
		int filesize;

		Q_strlcpy(newname, namewe, sizeof(newname));
		Q_strlcat(newname, ".", sizeof(newname));
		Q_strlcat(newname, ext, sizeof(newname));

		filesize = CM_LoadFile(newname, buffer);
		if (filesize > 0)
		{
			return filesize;
		}
	}

	Q_strlcpy(newname, namewe, sizeof(newname));
	Q_strlcat(newname, ".", sizeof(newname));
	Q_strlcat(newname, ext, sizeof(newname));

	return FS_LoadFile(newname, buffer);
}

/*
=================
Mod_LoadFile
=================
*/
int
Mod_LoadFile(const char *name, void **buffer)
{
	char namewe[256];
	const char* ext;
	int filesize, len;
	size_t tlen;

	if (!name)
	{
		return -1;
	}

	ext = COM_FileExtension(name);
	if(!ext[0])
	{
		/* file has no extension */
		return -1;
	}

	len = strlen(name);
	if (len < 5)
	{
		return -1;
	}

	/* Remove the extension */
	tlen = len - (strlen(ext) + 1);
	memset(namewe, 0, 256);
	memcpy(namewe, name, tlen);

	filesize = Mod_LoadFileWithoutExt(namewe, buffer, ext);
	if (filesize > 0)
	{
		return filesize;
	}

	/* Replacement of ReRelease models */
	if (!strcmp(namewe, "models/monsters/soldierh/tris"))
	{
		filesize = Mod_LoadFileWithoutExt("models/monsters/soldier/tris",
			buffer, ext);
	}
	else if (!strcmp(namewe, "models/monsters/gladb/tris"))
	{
		filesize = Mod_LoadFileWithoutExt("models/monsters/gladiatr/tris",
			buffer, ext);
	}
	else if (!strcmp(namewe, "models/monsters/boss5/tris"))
	{
		filesize = Mod_LoadFileWithoutExt("models/monsters/boss1/tris",
			buffer, ext);
	}
	else if (!strcmp(namewe, "models/monsters/bitch2/tris"))
	{
		filesize = Mod_LoadFileWithoutExt("models/monsters/bitch/tris",
			buffer, ext);
	}

	return filesize;
}
