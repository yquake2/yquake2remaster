#include <stdio.h>
#include <stdarg.h>
#include "render.h"

int skin_count = 0, iskin = 0, yrotate = 0, xrotate = -90, zrotate = -90,
	imesh = 0, irender = 0, zdist = -150.0;
dmdx_t *mem_mod;

refimport_t ri;

void R_Printf(int level, const char* msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	vprintf(msg, argptr);
	va_end(argptr);
}

void
Com_Printf (const char *msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	vprintf(msg, argptr);
	va_end(argptr);
}

void
Com_DPrintf (const char *msg, ...)
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

	exit (0);
}

void
Com_Error(int error_code, const char *error, ...)
{
	va_list argptr;
	va_start(argptr, error);
	vprintf(error, argptr);
	va_end(argptr);

	exit (0);
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
		return -1;
	}

	*buffer = malloc(size);
	fseek(fp, 0, SEEK_SET);
	fread(*buffer, size, 1, fp);
	fclose(fp);

	return size;
}

void
FS_FreeFile(void *buffer)
{
	free(buffer);
}

void
model_load(const char *filename)
{
	struct image_s **skins;
	modtype_t type;
	byte *buffer;
	int numskins;
	clock_t starttime;
	FILE *fp_mesh, *fp_anim;
	vec3_t mins, maxs;
	int mesh_size = 0, anim_size = 0, full_size = 0;

	fp_mesh = fopen(filename, "rb");
	if (!fp_mesh)
	{
		R_Printf(PRINT_ALL, "Error: couldn't open \"%s\"!\n", filename);
		exit (EXIT_FAILURE);
	}

	fseek (fp_mesh, 0, SEEK_END);
	mesh_size = ftell(fp_mesh);
	if (mesh_size <= 0)
	{
		R_Printf(PRINT_ALL, "Error: empty file \"%s\"!\n", filename);

		fclose(fp_mesh);
		exit (EXIT_FAILURE);
	}

	if (strstr(filename, ".md5mesh"))
	{
		char animfile[256];

		strncpy(animfile, filename, sizeof(animfile));
		memcpy(animfile + strlen(animfile) - strlen("mesh"),
			"anim",
			strlen("anim"));
		fp_anim = fopen(animfile, "rb");
		if (fp_anim)
		{
			fseek (fp_anim, 0, SEEK_END);
			anim_size = ftell(fp_anim);
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

	fseek(fp_mesh, 0, SEEK_SET);
	fread(buffer, mesh_size, 1, fp_mesh);
	fclose(fp_mesh);

	if (anim_size)
	{
		buffer[mesh_size] = 0;
		fseek(fp_anim, 0, SEEK_SET);
		fread(buffer + mesh_size + 1, anim_size, 1, fp_anim);
		fclose(fp_anim);
	}

	starttime = clock();
	mem_mod = (dmdx_t *)Mod_LoadModel(filename, buffer, full_size,
		mins, maxs, &skins, &numskins, find_image, insert_skin, FS_LoadFile, &type);
	printf("Load takes %.3fs\n", (double)(clock() - starttime) / CLOCKS_PER_SEC);
	free(buffer);
}
