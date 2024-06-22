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
 * The PCX file format
 *
 * =======================================================================
 */

#include "../ref_shared.h"

void
LoadPCX(const char *origname, byte **pic, byte **palette, int *width, int *height)
{
	char filename[256];
	int bytesPerPixel;

	FixFileExt(origname, "pcx", filename, sizeof(filename));

	*pic = NULL;

	if (palette)
	{
		*palette = NULL;
	}

	ri.VID_ImageDecode(filename, pic, palette, width, height, &bytesPerPixel);

	if (!(*pic))
	{
		R_Printf(PRINT_DEVELOPER, "Bad pcx file %s\n", filename);
	}
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
