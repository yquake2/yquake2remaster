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
 * The Wal image format
 *
 * =======================================================================
 */

#include "../ref_shared.h"

void
GetWalInfo(const char *origname, int *width, int *height)
{
	byte *data;
	int size;
	char filename[256];

	FixFileExt(origname, "wal", filename, sizeof(filename));

	size = ri.FS_LoadFile(filename, (void **)&data);

	if (!data)
	{
		return;
	}

	if (size > sizeof(dkmtex_t) && *data == DKM_WAL_VERSION)
	{
		const dkmtex_t *mt;

		mt = (dkmtex_t *)data;

		*width = LittleLong(mt->width);
		*height = LittleLong(mt->height);
	}
	else if (size > sizeof(miptex_t))
	{
		const miptex_t *mt;

		mt = (miptex_t *)data;

		if (size > sizeof(miptex_t))
		{
			*width = LittleLong(mt->width);
			*height = LittleLong(mt->height);
		}
	}

	ri.FS_FreeFile((void *)data);

	return;
}

void
GetM8Info(const char *origname, int *width, int *height)
{
	m8tex_t *mt;
	int size;
	char filename[256];

	FixFileExt(origname, "m8", filename, sizeof(filename));

	size = ri.FS_LoadFile(filename, (void **)&mt);

	if (!mt)
	{
		return;
	}

	if (size < sizeof(*mt) || LittleLong (mt->version) != M8_VERSION)
	{
		ri.FS_FreeFile((void *)mt);
		return;
	}

	*width = LittleLong(mt->width[0]);
	*height = LittleLong(mt->height[0]);

	ri.FS_FreeFile((void *)mt);

	return;
}

void
GetM32Info(const char *origname, int *width, int *height)
{
	m32tex_t *mt;
	int size;
	char filename[256];

	FixFileExt(origname, "m32", filename, sizeof(filename));

	size = ri.FS_LoadFile(filename, (void **)&mt);

	if (!mt)
	{
		return;
	}

	if (size < sizeof(*mt) || LittleLong (mt->version) != M32_VERSION)
	{
		ri.FS_FreeFile((void *)mt);
		return;
	}

	*width = LittleLong(mt->width[0]);
	*height = LittleLong(mt->height[0]);

	ri.FS_FreeFile((void *)mt);

	return;
}

void
GetSWLInfo(const char *origname, int *width, int *height)
{
	sinmiptex_t *mt;
	int size;
	char filename[256];

	FixFileExt(origname, "swl", filename, sizeof(filename));

	size = ri.FS_LoadFile(filename, (void **)&mt);

	if (!mt)
	{
		return;
	}

	if (size < sizeof(*mt))
	{
		ri.FS_FreeFile((void *)mt);
		return;
	}

	*width = LittleLong(mt->width);
	*height = LittleLong(mt->height);

	ri.FS_FreeFile((void *)mt);

	return;
}

void
GetPCXInfo(const char *origname, int *width, int *height)
{
	const pcx_t *pcx;
	byte *raw;
	char filename[256];

	FixFileExt(origname, "pcx", filename, sizeof(filename));

	ri.FS_LoadFile(filename, (void **)&raw);

	if (!raw)
	{
		return;
	}

	pcx = (pcx_t *)raw;

	*width = pcx->xmax + 1;
	*height = pcx->ymax + 1;

	ri.FS_FreeFile(raw);

	return;
}
