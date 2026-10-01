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
#include "../header/common.h"
#include "models.h"

#define PCX_IDENT ((0x05 << 8) + 0x0a)

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
#include "../../client/refresh/files/stb_image.h"

// Fix Jennell Jaquays' name in the Quitscreen
// this is 98x11 pixels, each value an index
// into the standard baseq2/pak0/pics/quit.pcx colormap
static unsigned char quitscreenfix[] = {
	191,191,191,47,28,39,4,4,39,1,47,28,47,28,29,1,
	28,28,47,31,31,1,29,31,1,28,47,47,47,47,29,28,
	47,31,30,28,40,40,4,28,28,40,39,40,29,102,102,245,
	28,39,4,4,39,103,40,40,1,1,102,94,47,47,1,94,
	94,94,94,47,102,245,103,103,103,47,1,102,1,102,29,29,
	29,29,47,28,245,31,31,31,47,1,28,1,28,47,1,102, 102,102,
	191,191,142,47,4,8,8,8,8,4,47,28,1,28,29,28,
	29,29,31,1,47,245,47,47,28,28,31,47,28,1,31,1,
	1,245,47,39,8,8,8,40,39,8,8,8,39,1,1,47,
	4,8,8,8,8,4,47,29,28,31,28,28,29,28,28,28,
	29,28,31,28,47,29,1,28,31,47,1,28,1,1,29,29,
	29,47,28,1,28,28,245,28,28,28,28,47,29,28,47,102,102,103,
	191,191,142,31,29,36,8,8,36,31,40,39,40,4,1,1,
	39,40,39,40,40,31,28,40,40,4,39,40,28,47,31,40,
	39,40,4,1,36,8,8,4,47,36,8,8,39,1,1,1,
	29,36,8,8,36,4,4,39,40,4,47,1,47,40,40,39,
	39,40,28,40,40,47,45,39,40,28,4,39,40,4,39,1,
	28,4,40,28,28,4,39,28,47,40,40,39,40,39,28,28,1,103,
	1,142,29,142,28,39,8,8,36,36,8,8,8,8,36,1,
	8,8,8,8,8,36,39,8,8,8,8,8,36,40,36,8,
	8,8,8,36,40,8,8,40,1,4,8,8,40,1,1,31,
	28,39,8,8,36,8,8,8,8,8,36,31,36,8,8,8,
	8,8,36,8,8,4,40,8,8,36,8,8,8,8,8,36,
	40,8,8,40,39,8,8,40,36,8,8,8,8,8,39,29,28,29,
	103,191,142,47,28,40,8,8,40,8,8,33,33,8,8,36,
	8,8,36,36,8,8,36,8,8,36,36,8,8,36,8,8,
	33,33,8,8,36,8,8,4,47,40,8,8,39,47,28,245,
	28,40,8,8,40,40,36,36,33,8,8,36,8,8,36,36,
	8,8,36,8,8,40,40,8,8,40,4,36,36,33,8,8,
	36,8,8,39,39,8,8,36,8,8,33,36,36,39,28,1,47,28,
	103,246,1,47,1,39,8,8,40,8,8,8,8,8,8,36,
	8,8,4,40,8,8,36,8,8,40,4,8,8,36,8,8,
	8,8,8,8,36,8,8,40,29,39,8,8,39,1,1,47,
	1,39,8,8,40,36,8,8,8,8,8,36,8,8,4,40,
	8,8,36,8,8,40,39,8,8,40,36,8,8,8,8,8,
	36,8,8,39,40,8,8,40,36,8,8,8,8,36,28,1,1,29,
	103,47,40,40,4,36,8,8,36,8,8,33,36,36,36,4,
	8,8,39,4,8,8,36,8,8,4,40,8,8,36,8,8,
	33,36,36,36,36,8,8,40,31,40,8,8,40,47,40,40,
	4,36,8,8,36,8,8,33,33,8,8,36,8,8,36,36,
	8,8,36,8,8,36,36,8,8,36,8,8,33,33,8,8,
	36,8,8,36,36,8,8,4,39,36,36,33,8,8,4,40,4,31,
	191,40,8,8,8,8,8,36,29,36,8,8,8,8,8,40,
	8,8,40,4,8,8,36,8,8,40,39,8,8,39,36,8,
	8,8,8,8,39,8,8,39,45,4,8,8,40,40,8,8,
	8,8,8,36,29,36,8,8,8,8,8,40,36,8,8,8,
	8,8,40,36,8,8,8,8,8,40,36,8,8,8,8,8,
	40,36,8,8,8,8,8,36,8,8,8,8,8,36,4,8,8,4,
	47,45,40,39,40,39,39,245,246,1,40,40,40,39,4,47,
	40,4,28,29,39,40,30,39,39,1,28,40,4,28,1,40,
	40,40,39,4,29,40,39,1,1,1,4,4,47,45,40,39,
	40,39,39,245,246,29,39,40,40,40,4,47,28,39,39,36,
	8,8,4,1,39,40,4,40,40,1,29,4,39,4,40,39,
	1,39,36,36,33,8,8,4,39,4,39,4,40,47,36,8,8,40,
	1,28,47,28,28,29,1,28,47,28,31,28,28,27,47,28,
	45,246,30,28,245,29,47,47,29,30,28,47,27,1,246,47,
	47,47,1,28,47,28,47,1,47,47,1,29,29,47,47,28,
	28,29,1,47,1,47,47,28,31,47,47,31,47,47,47,4,
	8,8,39,245,1,47,28,245,28,47,31,28,47,28,28,28,
	40,8,8,8,8,8,36,47,28,1,246,47,1,40,8,8,36,1,
	47,1,102,1,102,102,47,94,94,102,47,47,102,102,102,102,
	94,1,94,47,102,1,102,47,30,30,102,27,47,102,94,1,
	102,47,1,94,102,103,1,102,103,103,47,47,47,29,1,29,
	28,28,29,28,1,47,28,31,29,1,47,29,28,1,1,47,
	4,39,1,47,47,1,28,28,28,47,1,28,45,28,47,47,
	1,40,4,4,40,4,29,28,31,45,47,28,47,47,4,40,28,28
};

static void
fixQuitScreen(byte* px)
{
	// overwrite 11 lines, 98 pixels each, from quitscreenfix[]
	// starting at line 140, column 188
	// quitscreen is 320x240 px
	int r, qsIdx = 0;

	px += 140*320; // go to line 140
	px += 188; // to colum 188
	for(r=0; r<11; ++r)
	{
		memcpy(px, quitscreenfix+qsIdx, 98);
		qsIdx += 98;
		px += 320;
	}
}

static const byte *
PCX_RLE_Decode(byte *pix, const byte *pix_max, const byte *raw, const byte *raw_max,
	int bytes_per_line, qboolean *image_issues)
{
	int x;

	for (x = 0; x < bytes_per_line; )
	{
		int runLength;
		byte dataByte;

		if (raw >= raw_max)
		{
			// no place for read
			*image_issues = true;
			return raw;
		}
		dataByte = *raw++;

		if ((dataByte & 0xC0) == 0xC0)
		{
			runLength = dataByte & 0x3F;
			if (raw >= raw_max)
			{
				// no place for read
				*image_issues = true;
				return raw;
			}
			dataByte = *raw++;
		}
		else
		{
			runLength = 1;
		}

		while (runLength-- > 0)
		{
			if (pix_max <= (pix + x))
			{
				// no place for write
				*image_issues = true;
				return raw;
			}
			else
			{
				pix[x++] = dataByte;
			}
		}
	}
	return raw;
}

static void
PCX_Decode(const char *name, const byte *raw, int len, byte **pic, byte **palette,
	int *width, int *height, int *bitsPerPixel)
{
	const pcx_t *pcx;
	size_t full_size;
	int pcx_width, pcx_height, bytes_per_line;
	qboolean image_issues = false;
	byte *out, *pix;
	const byte *data;

	*pic = NULL;
	*bitsPerPixel = 8;

	if (palette)
	{
		*palette = NULL;
	}

	if (len < sizeof(pcx_t))
	{
		return;
	}

	/* parse the PCX file */
	pcx = (const pcx_t *)raw;

	data = &pcx->data;

	bytes_per_line = LittleShort(pcx->bytes_per_line);
	pcx_width = LittleShort(pcx->xmax) - LittleShort(pcx->xmin);
	pcx_height = LittleShort(pcx->ymax) - LittleShort(pcx->ymin);

	if ((pcx->manufacturer != 0x0a) ||
		(pcx->version != 5) ||
		(pcx->encoding != 1) ||
		(pcx_width <= 0) ||
		(pcx_height <= 0) ||
		(bytes_per_line <= 0) ||
		(pcx->color_planes == 0) ||
		(pcx->bits_per_pixel == 0))
	{
		Com_Printf("%s: Bad pcx file %s: version: %d:%d, encoding: %d\n",
			__func__, name, pcx->manufacturer, pcx->version, pcx->encoding);
		return;
	}

	full_size = (size_t)(pcx_height + 1) * (pcx_width + 1);
	if ((pcx->color_planes == 3 || pcx->color_planes == 4)
		&& pcx->bits_per_pixel == 8)
	{
		if (full_size > SIZE_MAX / 4)
		{
			Com_Printf("%s: %s dimensions overflow\n", __func__, name);
			return;
		}

		full_size *= 4;
		*bitsPerPixel = 32;
	}

	out = malloc(full_size);
	if (!out)
	{
		Com_Printf("%s: Can't allocate for %s\n", __func__, name);
		return;
	}

	*pic = out;

	pix = out;

	if (width)
	{
		*width = pcx_width + 1;
	}

	if (height)
	{
		*height = pcx_height + 1;
	}

	if ((pcx->color_planes == 3 || pcx->color_planes == 4)
		&& pcx->bits_per_pixel == 8)
	{
		int x, y, linesize;
		byte *line;

		if (bytes_per_line <= pcx_width)
		{
			image_issues = true;
		}

		/* clean image alpha */
		memset(pix, 255, full_size);

		linesize = Q_max(bytes_per_line, pcx_width + 1) * pcx->color_planes;
		line = malloc(linesize);
		YQ2_COM_CHECK_OOM(line, "malloc()", linesize)
		if (!line)
		{
			return;
		}

		for (y = 0; y <= pcx_height; y++, pix += (pcx_width + 1) * 4)
		{
			data = PCX_RLE_Decode(line, line + linesize,
				data, (byte *)pcx + len,
				bytes_per_line * pcx->color_planes, &image_issues);

			for (x = 0; x <= pcx_width; x++) {
				int j;

				for (j = 0; j < pcx->color_planes; j++)
				{
					pix[4 * x + j] = line[x + bytes_per_line * j];
				}
			}
		}

		free(line);
	}
	else if (pcx->bits_per_pixel == 1)
	{
		byte *line;
		int y;

		if (palette)
		{
			*palette = malloc(768);

			if (!(*palette))
			{
				Com_Printf("%s: Can't allocate for %s\n", __func__, name);
				free(out);
				*pic = NULL;
				return;
			}

			memcpy(*palette, pcx->palette, sizeof(pcx->palette));
		}

		line = malloc(bytes_per_line * pcx->color_planes);
		YQ2_COM_CHECK_OOM(line, "malloc()", bytes_per_line * pcx->color_planes)
		if (!line)
		{
			return;
		}

		for (y = 0; y <= pcx_height; y++, pix += pcx_width + 1)
		{
			int x;

			data = PCX_RLE_Decode(line, line + bytes_per_line * pcx->color_planes,
				data, (byte *)pcx + len,
				bytes_per_line * pcx->color_planes, &image_issues);

			for (x = 0; x <= pcx_width; x++)
			{
				int m, i, v;

				m = 0x80 >> (x & 7);
				v = 0;

				for (i = pcx->color_planes - 1; i >= 0; i--) {
					v <<= 1;
					v += (line[i * bytes_per_line + (x >> 3)] & m) ? 1 : 0;
				}
				pix[x] = v;
			}
		}
		free(line);
	}
	else if (pcx->color_planes == 1 && pcx->bits_per_pixel == 8)
	{
		int y, linesize;
		byte *line;

		if (palette)
		{
			*palette = malloc(768);

			if (!(*palette))
			{
				Com_Printf("%s: Can't allocate for %s\n", __func__, name);
				free(out);
				*pic = NULL;
				return;
			}

			if (len > 768)
			{
				memcpy(*palette, (byte *)pcx + len - 768, 768);

				if ((((byte *)pcx)[len - 769] != 0x0C))
				{
					Com_DPrintf("%s: %s has no palette marker\n",
						__func__, name);
				}
			}
			else
			{
				image_issues = true;
			}
		}

		if (bytes_per_line <= pcx_width)
		{
			image_issues = true;
		}

		linesize = Q_max(bytes_per_line, pcx_width + 1);
		line = malloc(linesize);
		YQ2_COM_CHECK_OOM(line, "malloc()", linesize)
		if (!line)
		{
			return;
		}

		for (y = 0; y <= pcx_height; y++, pix += pcx_width + 1)
		{
			data = PCX_RLE_Decode(line, line + linesize,
				data, (byte *)pcx + len,
				bytes_per_line, &image_issues);
			/* copy only visible part */
			memcpy(pix, line, pcx_width + 1);
		}
		free(line);
	}
	else if (pcx->color_planes == 1 &&
		(pcx->bits_per_pixel == 2 || pcx->bits_per_pixel == 4))
	{
		int y;

		byte *line;

		if (palette)
		{
			*palette = malloc(768);

			if (!(*palette))
			{
				free(out);
				YQ2_COM_CHECK_OOM(*palette, "malloc()", 768)
				return;
			}

			memcpy(*palette, pcx->palette, sizeof(pcx->palette));
		}

		line = malloc(bytes_per_line);
		if (!line)
		{
			free(out);
			YQ2_COM_CHECK_OOM(line, "malloc()", bytes_per_line)
			return;
		}

		for (y = 0; y <= pcx_height; y++, pix += pcx_width + 1)
		{
			int x, mask, div;

			data = PCX_RLE_Decode(line, line + bytes_per_line,
				data, (byte *)pcx + len,
				bytes_per_line, &image_issues);

			mask = (1 << pcx->bits_per_pixel) - 1;
			div = 8 / pcx->bits_per_pixel;

			for (x = 0; x <= pcx_width; x++)
			{
				unsigned v, shift;

				v = line[x / div] & 0xFF;
				/* for 2 bits:
				 * 0 -> 6
				 * 1 -> 4
				 * 3 -> 2
				 * 4 -> 0
				 */
				shift = pcx->bits_per_pixel * ((div - 1) - x % div);
				pix[x] = (v >> shift) & mask;
			}
		}

		free(line);
	}
	else
	{
		Com_Printf("%s: Bad pcx file %s: planes: %d, bits: %d\n",
			__func__, name, pcx->color_planes, pcx->bits_per_pixel);
		free(*pic);
		*pic = NULL;
	}

	if (pcx->color_planes != 1 || pcx->bits_per_pixel != 8)
	{
		Com_DPrintf("%s: %s has uncommon flags, "
			"could be unsupported by other engines\n",
			__func__, name);
	}

	if (data - (byte *)pcx > len)
	{
		Com_DPrintf("%s: %s file was malformed\n",
			__func__, name);
		free(*pic);
		*pic = NULL;
	}

	if (image_issues)
	{
		Com_Printf("%s: %s file has possible size issues.\n",
			__func__, name);
	}
}

static void
SWL_Decode(const char *name, const byte *raw, int len, byte **pic, byte **palette,
	int *width, int *height)
{
	sinmiptex_t *mt;
	int ofs;

	mt = (sinmiptex_t *)raw;

	*pic = NULL;

	if (!mt)
	{
		return;
	}

	if (len < sizeof(*mt))
	{
		Com_DPrintf("%s: can't load %s small header\n", __func__, name);
		return;
	}

	*width = LittleLong(mt->width);
	*height = LittleLong(mt->height);
	ofs = LittleLong(mt->offsets[0]);

	if ((ofs <= 0) || (*width <= 0) || (*height <= 0) ||
	    (((len - ofs) / *height) < *width))
	{
		Com_DPrintf("%s: can't load %s small body\n", __func__, name);
		return;
	}

	*pic = malloc(len - ofs);
	YQ2_COM_CHECK_OOM(*pic, "malloc()", len - ofs)
	if (*pic)
	{
		memcpy(*pic, (byte *)mt + ofs, len - ofs);
	}

	if (palette)
	{
		*palette = malloc(768);
		YQ2_COM_CHECK_OOM(*palette, "malloc()", 768)
		if (*palette)
		{
			size_t i;

			for (i = 0; i < 256; i ++)
			{
				(*palette)[i * 3 + 0] =  mt->palette[i * 4 + 0];
				(*palette)[i * 3 + 1] =  mt->palette[i * 4 + 1];
				(*palette)[i * 3 + 2] =  mt->palette[i * 4 + 2];
			}
		}
	}
}

static void
M32_Decode(const char *name, const byte *raw, int len, byte **pic, int *width, int *height)
{
	const m32tex_t *mt;
	int ofs;

	mt = (m32tex_t *)raw;

	if (!mt)
	{
		return;
	}

	if (len < sizeof(m32tex_t))
	{
		Com_DPrintf("%s: can't load %s small header\n", __func__, name);
		return;
	}

	if (LittleLong (mt->version) != M32_VERSION)
	{
		Com_DPrintf("%s: can't load %s wrong magic value.\n", __func__, name);
		return;
	}

	*width = LittleLong (mt->width[0]);
	*height = LittleLong (mt->height[0]);
	ofs = LittleLong (mt->offsets[0]);

	if ((ofs <= 0) || (*width <= 0) || (*height <= 0) ||
	    (((len - ofs) / *height) < (*width * 4)))
	{
		Com_DPrintf("%s: can't load %s small body\n", __func__, name);
	}

	*pic = malloc(len - ofs);
	YQ2_COM_CHECK_OOM(*pic, "malloc()", len - ofs)
	if (*pic)
	{
		memcpy(*pic, (byte *)mt + ofs, len - ofs);
	}
}

static void
M8_Decode(const char *name, const byte *raw, int len, byte **pic, byte **palette,
	int *width, int *height)
{
	const m8tex_t *mt;
	int ofs;

	mt = (m8tex_t *)raw;

	if (!mt)
	{
		return;
	}

	if (len < sizeof(*mt))
	{
		Com_Printf("%s: can't load %s, small header\n", __func__, name);
		return;
	}

	if (LittleLong (mt->version) != M8_VERSION)
	{
		Com_Printf("%s: can't load %s, wrong magic value.\n", __func__, name);
		return;
	}

	*width = LittleLong(mt->width[0]);
	*height = LittleLong(mt->height[0]);
	ofs = LittleLong(mt->offsets[0]);

	if ((ofs <= 0) || (*width <= 0) || (*height <= 0) ||
	    (((len - ofs) / *height) < *width))
	{
		Com_Printf("%s: can't load %s, small body\n", __func__, name);
		return;
	}

	*pic = malloc(len - ofs);
	YQ2_COM_CHECK_OOM(*pic, "malloc()", len - ofs)
	if (*pic)
	{
		memcpy(*pic, (byte *)mt + ofs, len - ofs);
	}

	if (palette)
	{
		*palette = malloc(768);
		YQ2_COM_CHECK_OOM(*palette, "malloc()", 768)
		if (*palette)
		{
			memcpy(*palette, mt->palette, 768);
		}
	}
}

static void
LoadWalQ2(const char *name, const byte *raw, int len, byte **pic, byte **palette,
	int *width, int *height)
{
	const miptex_t *mt;
	int ofs;

	mt = (miptex_t *)raw;

	if (len < sizeof(*mt))
	{
		Com_Printf("%s: can't load %s, small header\n", __func__, name);
		return;
	}

	*width = LittleLong(mt->width);
	*height = LittleLong(mt->height);
	ofs = LittleLong(mt->offsets[0]);

	if ((ofs <= 0) || (*width <= 0) || (*height <= 0) ||
	    (((len - ofs) / *height) < *width))
	{
		Com_Printf("%s: can't load %s, small body\n", __func__, name);
		return;
	}

	*pic = malloc(len - ofs);
	YQ2_COM_CHECK_OOM(*pic, "malloc()", len - ofs)
	if (*pic)
	{
		memcpy(*pic, (byte *)mt + ofs, len - ofs);
	}
}

static void
LoadWalDKM(const char *name, const byte *raw, int len, byte **pic, byte **palette,
	int *width, int *height)
{
	const dkmtex_t *mt;
	int ofs;

	mt = (dkmtex_t *)raw;

	if (len < sizeof(*mt))
	{
		Com_Printf("%s: can't load %s, small header\n", __func__, name);
		return;
	}

	if (mt->version != DKM_WAL_VERSION)
	{
		Com_Printf("%s: can't load %s, wrong magic value.\n", __func__, name);
		return;
	}

	*width = LittleLong(mt->width);
	*height = LittleLong(mt->height);
	ofs = LittleLong(mt->offsets[0]);

	if ((ofs <= 0) || (*width <= 0) || (*height <= 0) ||
	    (((len - ofs) / *height) < *width))
	{
		Com_Printf("%s: can't load %s, small body\n", __func__, name);
		return;
	}

	*pic = malloc(len - ofs);
	YQ2_COM_CHECK_OOM(*pic, "malloc()", len - ofs)
	if (*pic)
	{
		memcpy(*pic, (byte *)mt + ofs, len - ofs);
	}

	if (palette)
	{
		*palette = malloc(768);
		YQ2_COM_CHECK_OOM(*palette, "malloc()", 768)
		if (*palette)
		{
			memcpy(*palette, mt->palette, 768);
		}
	}
}

static void
WAL_Decode(const char *name, const byte *raw, int len, byte **pic, byte **palette,
	int *width, int *height)
{
	if (*raw == DKM_WAL_VERSION)
	{
		LoadWalDKM(name, raw, len, pic, palette, width, height);
	}
	else
	{
		LoadWalQ2(name, raw, len, pic, palette, width, height);
	}
}

/*
 * Doom flats have no header at all, just raw square (or rectangular)
 * 8bpp pixel data, so the size alone has to reveal the dimensions.
 */
static void
LMP_DecodeFlat(const char *name, const byte *raw, int len, byte **pic,
	int *width, int *height)
{
	int w, h;

	switch (len)
	{
		case 64 * 64: w = 64; h = 64; break;
		case 64 * 128: w = 64; h = 128; break;
		case 128 * 128: w = 128; h = 128; break;
		case 256 * 256: w = 256; h = 256; break;
		default:
			Com_Printf("%s: can't load %s, unsupported flat size %d\n",
				__func__, name, len);
			return;
	}

	*pic = malloc(len);
	YQ2_COM_CHECK_OOM(*pic, "malloc()", len)
	if (*pic)
	{
		memcpy(*pic, raw, len);
	}

	if (width)
	{
		*width = w;
	}

	if (height)
	{
		*height = h;
	}
}

typedef struct
{
	short width, height;
	short leftoffset, topoffset;
} doompatch_header_t;

/*
 * Draws the RLE encoded posts of a Doom patch column onto a canvas at
 * the given offset, used both for standalone patches and for compositing
 * multi-patch wall textures (TEXTURE1/TEXTURE2 + PNAMES).
 */
static void
Doom_BlitPatchColumns(const byte *raw, int len, byte *canvas,
	int canvas_w, int canvas_h, int origin_x, int origin_y)
{
	const doompatch_header_t *hdr;
	const int *columnofs;
	int w, x;

	if (len < (int)sizeof(*hdr))
	{
		return;
	}

	hdr = (const doompatch_header_t *)raw;
	w = LittleShort(hdr->width);

	if ((w <= 0) || (w > 4096) ||
		(len < (int)(sizeof(*hdr) + (size_t)w * sizeof(int))))
	{
		return;
	}

	columnofs = (const int *)(raw + sizeof(*hdr));

	for (x = 0; x < w; x++)
	{
		int ofs, canvas_x;

		canvas_x = origin_x + x;
		if ((canvas_x < 0) || (canvas_x >= canvas_w))
		{
			continue;
		}

		ofs = LittleLong(columnofs[x]);
		if ((ofs < 0) || (ofs >= len))
		{
			continue;
		}

		while ((ofs + 2) < len)
		{
			int topdelta, count, y;
			const byte *post;

			topdelta = raw[ofs];
			if (topdelta == 0xFF)
			{
				break;
			}

			count = raw[ofs + 1];
			if ((ofs + 3 + count) > len)
			{
				break;
			}

			post = raw + ofs + 3;
			for (y = 0; y < count; y++)
			{
				int canvas_y;

				canvas_y = origin_y + topdelta + y;
				if ((canvas_y >= 0) && (canvas_y < canvas_h))
				{
					canvas[canvas_y * canvas_w + canvas_x] = post[y];
				}
			}

			ofs += 4 + count;
		}
	}
}

/*
 * Doom patches store columns of RLE encoded posts (topdelta, length,
 * pixels) referenced by a per-column offset table, unlike the flat
 * headerless format or the Quake width/height + raw pixels format.
 */
static void
LMP_DecodePatch(const char *name, const byte *raw, int len, byte **pic,
	int *width, int *height)
{
	const doompatch_header_t *hdr;
	int w, h;
	byte *out;

	if (len < (int)sizeof(*hdr))
	{
		Com_Printf("%s: can't load %s, small header\n", __func__, name);
		return;
	}

	hdr = (const doompatch_header_t *)raw;
	w = LittleShort(hdr->width);
	h = LittleShort(hdr->height);

	if ((w <= 0) || (h <= 0) || (w > 4096) || (h > 4096) ||
		(len < (int)(sizeof(*hdr) + (size_t)w * sizeof(int))))
	{
		Com_Printf("%s: can't load %s, bad patch dimensions %dx%d ? %d\n",
			__func__, name, w, h, len);
		return;
	}

	out = malloc((size_t)w * h);
	YQ2_COM_CHECK_OOM(out, "malloc()", (size_t)w * h)
	if (!out)
	{
		return;
	}

	/* 0xFF marks "no pixel here", used as transparency placeholder */
	memset(out, 0xFF, (size_t)w * h);
	Doom_BlitPatchColumns(raw, len, out, w, h, 0, 0);

	*pic = out;

	if (width)
	{
		*width = w;
	}

	if (height)
	{
		*height = h;
	}
}

/*
 * Looks up the name of patch 'index' inside a loaded PNAMES lump.
 */
static qboolean
Doom_LoadPatchName(const byte *pnames, int pnames_len, int index, char name[9])
{
	int count;

	if (pnames_len < 4)
	{
		return false;
	}

	count = LittleLong(((const int *)pnames)[0]);
	if ((index < 0) || (index >= count) ||
		(pnames_len < (int)(4 + (size_t)count * 8)))
	{
		return false;
	}

	memcpy(name, pnames + 4 + (size_t)index * 8, 8);
	name[8] = 0;
	return true;
}

typedef struct
{
	short originx, originy, patch, stepdir, colormap;
} doommappatch_t;

/*
 * Wall textures referenced by Doom sidedefs are not raw patch lumps,
 * they are composed at load time from one or more patches placed at
 * given offsets, as described by the TEXTURE1/TEXTURE2 lumps and named
 * via PNAMES.
 */
static byte *
Doom_ComposeTexture(const char *texturelump, const byte *pnames, int pnames_len,
	const char *name, int *out_w, int *out_h)
{
	byte *raw, *canvas = NULL;
	int len, numtextures, i;

	len = FS_LoadFile(texturelump, (void **)&raw);
	if (!raw || (len < 4))
	{
		if (raw)
		{
			FS_FreeFile(raw);
		}
		return NULL;
	}

	numtextures = LittleLong(((const int *)raw)[0]);
	if ((numtextures < 0) || (len < (int)(4 + (size_t)numtextures * 4)))
	{
		FS_FreeFile(raw);
		return NULL;
	}

	for (i = 0; i < numtextures; i++)
	{
		int offset, width, height, patchcount, p;
		const byte *tex;
		const doommappatch_t *patches;
		char texname[9];

		offset = LittleLong(((const int *)(raw + 4))[i]);
		if ((offset < 0) || ((offset + 22) > len))
		{
			continue;
		}

		tex = raw + offset;
		memcpy(texname, tex, 8);
		texname[8] = 0;

		if (Q_strcasecmp(texname, name) != 0)
		{
			continue;
		}

		width = LittleShort(*(const short *)(tex + 12));
		height = LittleShort(*(const short *)(tex + 14));
		patchcount = LittleShort(*(const short *)(tex + 20));

		if ((width <= 0) || (height <= 0) || (width > 4096) || (height > 4096) ||
			(patchcount < 0) ||
			((offset + 22 + patchcount * (int)sizeof(doommappatch_t)) > len))
		{
			continue;
		}

		canvas = malloc((size_t)width * height);
		if (!canvas)
		{
			break;
		}
		memset(canvas, 0xFF, (size_t)width * height);

		patches = (const doommappatch_t *)(tex + 22);
		for (p = 0; p < patchcount; p++)
		{
			int originx, originy, patch_index, patchlen;
			char patchname[9], patchpath[MAX_QPATH];
			byte *patchraw;

			originx = LittleShort(patches[p].originx);
			originy = LittleShort(patches[p].originy);
			patch_index = LittleShort(patches[p].patch);

			if (!Doom_LoadPatchName(pnames, pnames_len, patch_index, patchname))
			{
				continue;
			}

			Com_sprintf(patchpath, sizeof(patchpath), "patches/%s.lmp", patchname);
			patchlen = FS_LoadFile(patchpath, (void **)&patchraw);
			if (patchraw && (patchlen > 0))
			{
				Doom_BlitPatchColumns(patchraw, patchlen, canvas, width, height,
					originx, originy);
			}

			if (patchraw)
			{
				FS_FreeFile(patchraw);
			}
		}

		*out_w = width;
		*out_h = height;
		break;
	}

	FS_FreeFile(raw);
	return canvas;
}

/*
 * Tries TEXTURE1 then TEXTURE2 to build a composed Doom wall texture.
 */
static byte *
Doom_LoadComposedTexture(const char *name, int *out_w, int *out_h)
{
	byte *pnames, *pic;
	int pnames_len;

	pnames_len = FS_LoadFile("custom/PNAMES.lmp", (void **)&pnames);
	if (!pnames || (pnames_len <= 0))
	{
		if (pnames)
		{
			FS_FreeFile(pnames);
		}
		return NULL;
	}

	pic = Doom_ComposeTexture("custom/TEXTURE1.lmp", pnames, pnames_len,
		name, out_w, out_h);
	if (!pic)
	{
		pic = Doom_ComposeTexture("custom/TEXTURE2.lmp", pnames, pnames_len,
			name, out_w, out_h);
	}

	FS_FreeFile(pnames);
	return pic;
}

static void
LMP_Decode(const char *name, const byte *raw, int len, byte **pic,
	int *width, int *height)
{
	unsigned lmp_width = 0, lmp_height = 0;
	size_t lmp_size = 0;

	if (!strncmp(name, "flat/", 5))
	{
		LMP_DecodeFlat(name, raw, len, pic, width, height);
		return;
	}

	if (!strncmp(name, "patches/", 8))
	{
		LMP_DecodePatch(name, raw, len, pic, width, height);
		return;
	}

	if (len < (sizeof(int) * 3))
	{
		/* looks too small */
		return;
	}

	lmp_width = LittleLong(((int*)raw)[0]) & 0xFFFF;
	lmp_height = LittleLong(((int*)raw)[1]) & 0xFFFF;
	lmp_size = lmp_height * lmp_width;
	if ((lmp_size + sizeof(int) * 2) > len)
	{
		Com_Printf("%s: can't load %s, small body %dx%d ? %d\n",
			__func__, name, lmp_width, lmp_height, len);
		return;
	}

	*pic = malloc(lmp_size);
	YQ2_COM_CHECK_OOM(*pic, "malloc()", sizeof(lmp_size))
	if (*pic)
	{
		memcpy(*pic, raw + sizeof(int) * 2, lmp_size);
	}

	if (width)
	{
		*width = lmp_width;
	}

	if (height)
	{
		*height = lmp_height;
	}
}

static void
Mod_LoadQuakePalette(byte **palette)
{
	int len;
	byte *raw;

	/* found some image, try to load default colormap */
	len = FS_LoadFile("gfx/palette.lmp", (void **)&raw);
	if (!raw || len <= 0)
	{
		/* no palette */
		*palette = NULL;
		return;
	}

	if (len == 768)
	{
		*palette = malloc(len);
		YQ2_COM_CHECK_OOM(*palette, "malloc()", sizeof(len))
		if (*palette)
		{
			memcpy(*palette, raw, len);
			Com_DPrintf("%s: Loaded custom palette\n", __func__);
		}
	}
	else
	{
		Com_DPrintf("%s: Unexpected palette size %d\n",
			__func__, len);
	}

	FS_FreeFile(raw);
}

/* Does not check to embeded images */
void
Mod_RawDecodeImageWithPalette(const char *filename, const byte *raw, int len,
	byte **pic, byte **palette, int *width, int *height, int *bitsPerPixel)
{
	const char* ext;
	int ident;

	ext = COM_FileExtension(filename);
	ident = LittleShort(*((short*)raw));
	if (!strcmp(ext, "pcx") && (ident == PCX_IDENT))
	{
		PCX_Decode(filename, raw, len, pic, palette, width, height, bitsPerPixel);

		if (*pic && width && height
			&& *width == 320 && *height == 240 && *bitsPerPixel == 8
			&& Q_strcasecmp(filename, "pics/quit.pcx") == 0
			&& Com_BlockChecksum(raw, len) == 3329419434u)
		{
			// it's the quit screen, and the baseq2 one (identified by checksum)
			// so fix it
			fixQuitScreen(*pic);
		}
	}
	else if (!strcmp(ext, "m8"))
	{
		M8_Decode(filename, raw, len, pic, palette, width, height);
		*bitsPerPixel = 8;
	}
	else if (!strcmp(ext, "swl"))
	{
		SWL_Decode(filename, raw, len, pic, palette, width, height);
		*bitsPerPixel = 8;
	}
	else if (!strcmp(ext, "wal"))
	{
		WAL_Decode(filename, raw, len, pic, palette, width, height);
		*bitsPerPixel = 8;
	}
	else if (!strcmp(ext, "lmp"))
	{
		LMP_Decode(filename, raw, len, pic, width, height);
		*bitsPerPixel = 8;

		if (palette)
		{
			*palette = NULL;
		}
	}
	else
	{
		int sourcebitsPerPixel = 0;

		/* other formats does not have palette directly */
		if (palette)
		{
			*palette = NULL;
		}

		if (!strcmp(ext, "m32"))
		{
			M32_Decode(filename, raw, len, pic, width, height);
		}
		else
		{
			*pic = stbi_load_from_memory(raw, len, width, height,
				&sourcebitsPerPixel, STBI_rgb_alpha);

			if (*pic == NULL)
			{
				Com_DPrintf("%s couldn't load data from %s: %s!\n",
					__func__, filename, stbi_failure_reason());
			}
		}

		*bitsPerPixel = 32;
	}
}

typedef struct
{
	char *old;
	char *new;
} img_replacement_t;

/* Replacement of ReRelease images */
static const img_replacement_t img_replacements[] = {
	{"pics/ctfsb1.pcx", "pics/tag4.pcx"},
	{"pics/ctfsb2.pcx", "pics/tag5.pcx"}
};

/*
 * Load only static images without animation support
 */
void
Mod_LoadImageWithPalette(const char *filename, byte **pic, byte **palette,
	int *width, int *height, int *bitsPerPixel)
{
	int len;
	byte *raw;

	*pic = NULL;

	/* load the file */
	len = FS_LoadFile(filename, (void **)&raw);
	if (!raw || len <= 0)
	{
		size_t i;

		/* Replace to other one if load failed */
		for (i = 0; i < ARRLEN(img_replacements); i++)
		{
			if (!strcmp(filename, img_replacements[i].old))
			{
				Com_DPrintf("%s: tring to replace %s to %s.\n",
					__func__, filename, img_replacements[i].new);
				len = FS_LoadFile(img_replacements[i].new, (void **)&raw);
				break;
			}
		}
	}

	if (!raw || len <= 0)
	{
		const char* ext;

		ext = COM_FileExtension(filename);

		/* map wall texture */
		if (strcmp(ext, "lmp"))
		{
			return;
		}

		*pic = Mod_LoadEmbededLMP(filename, width, height, bitsPerPixel);

		/*
		 * Bare names (no directory) are Doom wall textures composed
		 * from patches via TEXTURE1/TEXTURE2 + PNAMES, not raw lumps.
		 */
		if (!*pic && !strchr(filename, '/'))
		{
			*pic = Doom_LoadComposedTexture(filename, width, height);
			if (*pic)
			{
				*bitsPerPixel = 8;
			}
		}

		/* Get Quake palette */
		if (palette && *pic && *bitsPerPixel == 8)
		{
			Mod_LoadQuakePalette(palette);
		}

		return;
	}

	if (len <= sizeof(int))
	{
		FS_FreeFile(raw);
		return;
	}

	Mod_RawDecodeImageWithPalette(filename, raw, len, pic, palette, width, height,
		bitsPerPixel);

	FS_FreeFile(raw);
}
