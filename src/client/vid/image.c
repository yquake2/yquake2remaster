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
 * Shared image decode logic
 *
 * =======================================================================
 */

#include "../../client/header/client.h"

#define PCX_IDENT ((0x08 << 24) + (0x01 << 16) + (0x05 << 8) + 0x0a)
// don't need HDR stuff
#define STBI_NO_LINEAR
#define STBI_NO_HDR
// make sure STB_image uses standard malloc(), as we'll use standard free() to deallocate
#define STBI_MALLOC(sz)    malloc(sz)
#define STBI_REALLOC(p,sz) realloc(p,sz)
#define STBI_FREE(p)       free(p)
// Switch of the thread local stuff. Breaks mingw under Windows.
#define STBI_NO_THREAD_LOCALS
// include implementation part of stb_image into this file
#define STB_IMAGE_IMPLEMENTATION
#include "../refresh/files/stb_image.h"

static void
PCX_Decode(const byte *raw, int len, byte **pic, byte **palette,
	int *width, int *height, int *bytesPerPixel)
{
	pcx_t *pcx;
	int x, y, full_size;
	int dataByte, runLength;
	byte *out, *pix;

	*pic = NULL;
	*bytesPerPixel = 1;

	if (len < sizeof(pcx_t))
	{
		return;
	}

	/* parse the PCX file */
	pcx = (pcx_t *)raw;
	raw = &pcx->data;

	if ((pcx->manufacturer != 0x0a) ||
		(pcx->version != 5) ||
		(pcx->encoding != 1) ||
		(pcx->bits_per_pixel != 8))
	{
		return;
	}

	full_size = (pcx->ymax + 1) * (pcx->xmax + 1);
	out = malloc(full_size);

	*pic = out;

	pix = out;

	if (palette)
	{
		*palette = malloc(768);
		memcpy(*palette, (byte *)pcx + len - 768, 768);
	}

	if (width)
	{
		*width = pcx->xmax + 1;
	}

	if (height)
	{
		*height = pcx->ymax + 1;
	}

	for (y = 0; y <= pcx->ymax; y++, pix += pcx->xmax + 1)
	{
		for (x = 0; x <= pcx->xmax; )
		{
			dataByte = *raw++;

			if ((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				dataByte = *raw++;
			}
			else
			{
				runLength = 1;
			}

			while (runLength-- > 0)
			{
				if ((*pic + full_size) <= (pix + x))
				{
					x += runLength;
					runLength = 0;
				}
				else
				{
					pix[x++] = dataByte;
				}
			}
		}
	}

	if (raw - (byte *)pcx > len)
	{
		free(*pic);
		*pic = NULL;
	}
}


void
VID_ImageDecode(const char *filename, byte **pic, byte **palette,
	int *width, int *height, int *bytesPerPixel)
{
	int len, ident;
	byte *raw;

	/* load the file */
	len = FS_LoadFile(filename, (void **)&raw);

	if (!raw || len <= 0)
	{
		return;
	}

	if (len <= sizeof(int))
	{
		FS_FreeFile(raw);
		return;
	}

	ident = LittleLong(*((int*)raw));
	if (ident == PCX_IDENT)
	{
		PCX_Decode(raw, len, pic, palette, width, height, bytesPerPixel);
	}
	else
	{
		/* other formats does not have palette directly */
		*palette = NULL;

		*pic = stbi_load_from_memory(raw, len, width, height,
			bytesPerPixel, STBI_rgb_alpha);
	}

	FS_FreeFile(raw);
}
