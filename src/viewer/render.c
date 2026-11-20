#include <stdio.h>
#include <stdarg.h>
#include "render.h"

int skin_count = 0, iskin = 0, yrotate = 0, xrotate = -90, zrotate = -90,
	imesh = 0, irender = 0, zdist = -150.0;
dmdx_t *mem_mod;

cvar_t *r_skeletalanimation;

void
R_Printf(int level, const char* msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	vprintf(msg, argptr);
	va_end(argptr);
}

void
Com_Printf(const char *msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	vprintf(msg, argptr);
	va_end(argptr);
}

void
Com_DPrintf(const char *msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	vprintf(msg, argptr);
	va_end(argptr);
}

void
Sys_Error(const char *error, ...)
{
	va_list argptr;
	va_start(argptr, error);
	vprintf(error, argptr);
	va_end(argptr);

	exit(0);
}

void
Com_Error(int error_code, const char *error, ...)
{
	va_list argptr;
	va_start(argptr, error);
	vprintf(error, argptr);
	va_end(argptr);

	exit(0);
}

int
FS_LoadFile(const char *path, void **buffer)
{
	int size;
	FILE *fp;

	fp = fopen(path, "rb");
	if (!fp)
	{
		return -1;
	}

	fseek (fp, 0, SEEK_END);
	size = ftell(fp);
	if (size <= 0)
	{
		fclose(fp);
		return -1;
	}

	*buffer = malloc(size);
	if (!*buffer)
	{
		fclose(fp);
		return -1;
	}

	fseek(fp, 0, SEEK_SET);
	if (fread(*buffer, size, 1, fp) != 1)
	{
		free(*buffer);
		*buffer = NULL;
		fclose(fp);
		return -1;
	}

	fclose(fp);

	return size;
}

int
CM_LoadFile(const char *path, void **buffer)
{
	return FS_LoadFile(path, buffer);
}

void
FS_FreeFile(void *buffer)
{
	free(buffer);
}

void
model_load(const char *filename)
{
	byte *buffer;
	clock_t starttime;
	FILE *fp_mesh = NULL, *fp_anim = NULL;
	size_t mesh_size = 0, anim_size = 0, full_size = 0;
	int filesize = 0;

	fp_mesh = fopen(filename, "rb");
	if (!fp_mesh)
	{
		R_Printf(PRINT_ALL, "Error: couldn't open \"%s\"!\n", filename);
		exit(EXIT_FAILURE);
	}

	fseek (fp_mesh, 0, SEEK_END);
	filesize = ftell(fp_mesh);
	if (filesize <= 0)
	{
		R_Printf(PRINT_ALL, "Error: empty file \"%s\"!\n", filename);

		fclose(fp_mesh);
		exit(EXIT_FAILURE);
	}

	mesh_size = filesize;

	if (strstr(filename, ".md5mesh"))
	{
		char animfile[256];

		Q_strlcpy(animfile, filename, sizeof(animfile));
		memcpy(animfile + strlen(animfile) - strlen("mesh"),
			"anim",
			strlen("anim"));
		fp_anim = fopen(animfile, "rb");
		if (fp_anim)
		{
			fseek(fp_anim, 0, SEEK_END);
			filesize = ftell(fp_anim);
			if (filesize <= 0)
			{
				R_Printf(PRINT_ALL, "Error: empty file \"%s\"!\n", animfile);
				fclose(fp_anim);
				fclose(fp_mesh);
				exit(EXIT_FAILURE);
			}
			anim_size = filesize;
		}
		else
		{
			R_Printf(PRINT_ALL, "Error: couldn't open \"%s\"!\n", animfile);
		}
	}

	full_size = mesh_size;
	if (anim_size)
	{
		full_size += anim_size + 1;
	}

	buffer = malloc(full_size);
	if (!buffer)
	{
		fclose(fp_mesh);
		if (fp_anim)
		{
			fclose(fp_anim);
		}

		exit(EXIT_FAILURE);
	}

	fseek(fp_mesh, 0, SEEK_SET);
	if (fread(buffer, mesh_size, 1, fp_mesh) != 1)
	{
		R_Printf(PRINT_ALL, "Cant read mesh data!\n");
		free(buffer);
		fclose(fp_mesh);
		if (fp_anim)
		{
			fclose(fp_anim);
		}

		exit(EXIT_FAILURE);
	}

	fclose(fp_mesh);

	if (fp_anim)
	{
		if (anim_size)
		{
			buffer[mesh_size] = 0;
			fseek(fp_anim, 0, SEEK_SET);
			if (fread(buffer + mesh_size + 1, anim_size, 1, fp_anim) != 1)
			{
				R_Printf(PRINT_ALL, "Cant read anim data!\n");
				free(buffer);
				fclose(fp_anim);
				exit(EXIT_FAILURE);
			}
		}

		fclose(fp_anim);
	}

	starttime = clock();
	mem_mod = (dmdx_t *)Mod_LoadModelFile(filename, buffer, full_size);
	Com_Printf("Load takes %.3fs\n", (double)(clock() - starttime) / CLOCKS_PER_SEC);
	free(buffer);
}
