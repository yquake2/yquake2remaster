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

#include "header/client.h"

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
#include "refresh/files/stb_image.h"

/* ATD types */
typedef struct
{
	char *file;
} atd_bitmap_t;

typedef struct
{
	int bitmap;
	int next;
	float wait;
	int x;
	int y;
} atd_frame_t;

typedef struct
{
	int colortype;
	int width;
	int height;
	int bilinear;
	int clamp;
	char* type;
	atd_bitmap_t *bitmaps;
	size_t bitmap_count;
	atd_frame_t *frames;
	size_t frame_count;
} atd_sprites_t;

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

static byte *colormap_cache = NULL;
static unsigned d_8to24table_cache[256];
static qboolean colormap_loaded = false;


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
PCX_RLE_Decode(byte *pix, byte *pix_max, const byte *raw, const byte *raw_max,
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
	int full_size;
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
		(pcx->color_planes <= 0) ||
		(pcx->bits_per_pixel <= 0))
	{
		Com_Printf("%s: Bad pcx file %s: version: %d:%d, encoding: %d\n",
			__func__, name, pcx->manufacturer, pcx->version, pcx->encoding);
		return;
	}

	full_size = (pcx_height + 1) * (pcx_width + 1);
	if ((pcx->color_planes == 3 || pcx->color_planes == 4)
		&& pcx->bits_per_pixel == 8)
	{
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
				return;
			}

			memcpy(*palette, pcx->palette, sizeof(pcx->palette));
		}

		line = malloc(bytes_per_line * pcx->color_planes);

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
				Com_Printf("%s: Can't allocate for %s\n", __func__, name);
				free(out);
				return;
			}

			memcpy(*palette, pcx->palette, sizeof(pcx->palette));
		}

		line = malloc(bytes_per_line);

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
		Com_DPrintf("%s: %s file was malformed\n", __func__, name);
		free(*pic);
		*pic = NULL;
	}

	if (image_issues)
	{
		Com_Printf("%s: %s file has possible size issues.\n", __func__, name);
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
	memcpy(*pic, (byte *)mt + ofs, len - ofs);

	if (palette)
	{
		int i;

		*palette = malloc(768);
		for (i = 0; i < 256; i ++)
		{
			(*palette)[i * 3 + 0] =  mt->palette[i * 4 + 0];
			(*palette)[i * 3 + 1] =  mt->palette[i * 4 + 1];
			(*palette)[i * 3 + 2] =  mt->palette[i * 4 + 2];
		}
	}
}

static void
M32_Decode(const char *name, const byte *raw, int len, byte **pic, int *width, int *height)
{
	m32tex_t *mt;
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
	memcpy(*pic, (byte *)mt + ofs, len - ofs);
}

static void
M8_Decode(const char *name, const byte *raw, int len, byte **pic, byte **palette,
	int *width, int *height)
{
	m8tex_t *mt;
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
	memcpy(*pic, (byte *)mt + ofs, len - ofs);
	if (palette)
	{
		*palette = malloc(768);
		memcpy(*palette, mt->palette, 768);
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
	memcpy(*pic, (byte *)mt + ofs, len - ofs);
}

static void
LoadWalDKM(const char *name, const byte *raw, int len, byte **pic, byte **palette,
	int *width, int *height)
{
	dkmtex_t *mt;
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
	memcpy(*pic, (byte *)mt + ofs, len - ofs);

	if (palette)
	{
		*palette = malloc(768);
		memcpy(*palette, mt->palette, 768);
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

static void
LMP_Decode(const char *name, const byte *raw, int len, byte **pic,
	int *width, int *height)
{
	unsigned lmp_width = 0, lmp_height = 0, lmp_size = 0;
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
	memcpy(*pic, raw + sizeof(int) * 2, lmp_size);

	if (width)
	{
		*width = lmp_width;
	}

	if (height)
	{
		*height = lmp_height;
	}
}

static byte
Convert24to8(const byte *d_8to24table, const int rgb[3])
{
	int i, best, diff;

	best = 255;
	diff = 1 << 20;

	for (i = 0; i < 256; i ++)
	{
		int j, curr_diff;

		curr_diff = 0;

		for (j = 0; j < 3; j++)
		{
			int v;

			v = d_8to24table[i * 4 + j] - rgb[j];
			curr_diff += v * v;
		}

		if (curr_diff < diff)
		{
			diff = curr_diff;
			best = i;
		}
	}

	return best;
}

static void
GenerateColormap(const byte *palette, byte *out_colormap)
{
	// https://quakewiki.org/wiki/Quake_palette
	int num_fullbrights = 32; /* the last 32 colours will be full bright */
	int x;

	for (x = 0; x < 256; x++)
	{
		int y;

		for (y = 0; y < 64; y++)
		{
			if (x < 256 - num_fullbrights)
			{
				int rgb[3], i;

				for (i = 0; i < 3; i++)
				{
					/* divide by 32, rounding to nearest integer */
					rgb[i] = (palette[x * 4 + i] * (63 - y) + 16) >> 5;
					if (rgb[i] > 255)
					{
						rgb[i] = 255;
					}
				}

				out_colormap[y * 256 + x] = Convert24to8(palette, rgb);
			}
			else
			{
				/* this colour is a fullbright, just keep the original colour */
				out_colormap[y*256 + x] = x;
			}
		}
	}
}

static void
Convert24to32(unsigned *d_8to24table, byte *pal)
{
	int i;

	for (i = 0; i < 256; i++)
	{
		unsigned v;
		int	r, g, b;

		r = pal[i*3+0];
		g = pal[i*3+1];
		b = pal[i*3+2];

		v = (255U<<24) + (r<<0) + (g<<8) + (b<<16);
		d_8to24table[i] = LittleLong(v);
	}

	d_8to24table[255] &= LittleLong(0xffffff);	// 255 is transparent
}

/*
 * Load only static images without animation support
 */
static void
LoadImageWithPaletteStatic(const char *filename, byte **pic, byte **palette,
	int *width, int *height, int *bitsPerPixel)
{
	const char* ext;
	int len, ident;
	byte *raw;

	ext = COM_FileExtension(filename);
	*pic = NULL;

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

	ident = LittleShort(*((short*)raw));
	if (!strcmp(ext, "pcx") && (ident == PCX_IDENT))
	{
		PCX_Decode(filename, raw, len, pic, palette, width, height, bitsPerPixel);

		if(*pic && width && height
			&& *width == 319 && *height == 239 && *bitsPerPixel == 8
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

	FS_FreeFile(raw);
}

static void
LoadImageATDFree(atd_sprites_t* anim)
{
	for (size_t i = 0; i < anim->bitmap_count; i++)
	{
		free(anim->bitmaps[i].file);
	}
	free(anim->bitmaps);
	free(anim->frames);
	free(anim->type);
}

static void
LoadImageATD(atd_sprites_t* anim, char *tmp_buf, int len)
{
	char *curr_buff;

	/* get lines count */
	curr_buff = tmp_buf;
	while(curr_buff && *curr_buff && (curr_buff < (tmp_buf + len)))
	{
		const char *token;

		token = COM_Parse(&curr_buff);
		if (!token)
		{
			continue;
		}

		if (token[0] == '#')
		{
			size_t linesize;

			/* skip empty */
			linesize = strcspn(curr_buff, "\n\r");
			curr_buff += linesize;
		}
		else if (!strcmp(token, "type") ||
				 !strcmp(token, "width") ||
				 !strcmp(token, "height") ||
				 !strcmp(token, "bilinear") ||
				 !strcmp(token, "clamp") ||
				 !strcmp(token, "colortype"))
		{
			char token_section[MAX_TOKEN_CHARS];

			strncpy(token_section, token, sizeof(token_section) - 1);

			token = COM_Parse(&curr_buff);
			if (strcmp(token, "="))
			{
				/* should = after token */
				return;
			}

			if (!strcmp(token_section, "type") && !anim->type)
			{
				anim->type = strdup(COM_Parse(&curr_buff));
			}
			else
			{
				int value;

				token = COM_Parse(&curr_buff);
				value = (int)strtol(token, (char **)NULL, 10);

				if (!strcmp(token_section, "colortype"))
				{
					anim->colortype = value;
				}
				else if (!strcmp(token_section, "width"))
				{
					anim->width = value;
				}
				else if (!strcmp(token_section, "height"))
				{
					anim->height = value;
				}
				else if (!strcmp(token_section, "bilinear"))
				{
					anim->bilinear = value;
				}
				else if (!strcmp(token_section, "clamp"))
				{
					anim->clamp = value;
				}
			}
		}
		else if (!strcmp(token, "!bitmap"))
		{
			token = COM_Parse(&curr_buff);
			if (strcmp(token, "file"))
			{
				/* should file after token */
				return;
			}

			token = COM_Parse(&curr_buff);
			if (strcmp(token, "="))
			{
				/* should = after token */
				return;
			}

			/* save bitmap file */
			anim->bitmap_count++;
			anim->bitmaps = realloc(anim->bitmaps, anim->bitmap_count * sizeof(atd_bitmap_t));
			anim->bitmaps[anim->bitmap_count - 1].file = strdup(COM_Parse(&curr_buff));
		}
		else if (!strcmp(token, "!frame"))
		{
			atd_frame_t *frame;

			anim->frame_count++;
			anim->frames = realloc(anim->frames, anim->frame_count * sizeof(frame_t));
			frame = &anim->frames[anim->frame_count - 1];
			frame->next = -1;
			frame->wait = 0.0f;
			frame->x = frame->y = 0;
			frame->bitmap = -1;

			while(curr_buff && *curr_buff && (curr_buff < (tmp_buf + len)))
			{
				size_t linesize;

				/* skip empty */
				linesize = strspn(curr_buff, "\n\r\t ");
				curr_buff += linesize;

				/* new frame? */
				if (curr_buff[0] == '!')
				{
					break;
				}

				token = COM_Parse(&curr_buff);
				if (token[0] == '#')
				{
					/* skip empty */
					linesize = strcspn(curr_buff, "\n\r");
					curr_buff += linesize;
				}
				else if (!strcmp(token, "bitmap") ||
						 !strcmp(token, "next") ||
						 !strcmp(token, "wait") ||
						 !strcmp(token, "x") ||
						 !strcmp(token, "y"))
				{
					char token_section[MAX_TOKEN_CHARS];

					strncpy(token_section, token, sizeof(token_section) - 1);

					token = COM_Parse(&curr_buff);
					if (strcmp(token, "="))
					{
						/* should = after token */
						return;
					}

					if (!strcmp(token_section, "wait"))
					{
						token = COM_Parse(&curr_buff);
						frame->wait = (float)strtod(token, (char **)NULL);
					}
					else
					{
						int value;

						token = COM_Parse(&curr_buff);
						value = (int)strtol(token, (char **)NULL, 10);

						if (!strcmp(token_section, "bitmap"))
						{
							frame->bitmap = value;
						}
						else if (!strcmp(token_section, "next"))
						{
							frame->next = value;
						}
						else if (!strcmp(token_section, "x"))
						{
							frame->x = value;
						}
						else if (!strcmp(token_section, "y"))
						{
							frame->y = value;
						}
					}
				}
			}
		}
	}
}

/* Load images with sprites */
static void
LoadImageWithPalette(const char *filename, byte **pic, byte **palette,
	int *width, int *height, int *bitsPerPixel)
{
	const char* ext;

	ext = COM_FileExtension(filename);

	if (!strcmp(ext, "atd"))
	{
		char *tmp_buf, *raw;
		int lindent, len;

		*pic = NULL;

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

		lindent = LittleLong(*((int*)raw));
		len -= 4;

		if (lindent == IDATDSPRITEHEADER)
		{
			atd_sprites_t *anim = malloc(sizeof(atd_sprites_t));
			memset(anim, 0, sizeof(atd_sprites_t));

			tmp_buf = malloc(len + 1);
			memcpy(tmp_buf, raw + 4, len);
			tmp_buf[len] = 0;
			LoadImageATD(anim, tmp_buf, len);
			free(tmp_buf);

			if (anim->bitmap_count &&
				anim->frame_count &&
				(anim->frames[0].bitmap >= 0) &&
				(anim->frames[0].bitmap < anim->bitmap_count))
			{
				int bitmap;

				bitmap = anim->frames[0].bitmap;
				LoadImageWithPaletteStatic(anim->bitmaps[bitmap].file,
					pic, palette, width, height, bitsPerPixel);
			}
			LoadImageATDFree(anim);
			free(anim);
		}

		FS_FreeFile(raw);
		return;
	}
	else
	{
		LoadImageWithPaletteStatic(filename, pic, palette, width, height, bitsPerPixel);
	}
}

void
SCR_LoadImageWithPalette(const char *filename, byte **pic, byte **palette,
	int *width, int *height, int *bitsPerPixel)
{
	LoadImageWithPalette(filename, pic, palette, width, height, bitsPerPixel);

	if (palette && *palette && colormap_cache && !colormap_loaded)
	{
		/* Stil have no palette, lets use palete from first loaded image */
		Convert24to32(d_8to24table_cache, *palette);
		GenerateColormap((const byte *)d_8to24table_cache, colormap_cache);
		colormap_loaded = true;
	}
}

static void
LoadPalette(byte **colormap, unsigned *d_8to24table)
{
	const char * filename;
	int bitsPerPixel;
	byte *pal = NULL;

	filename = "pics/colormap.pcx";

	/* get the palette and colormap */
	LoadImageWithPalette(filename, colormap, &pal, NULL, NULL,
		&bitsPerPixel);
	if (!*colormap || !pal)
	{
		int width = 0, height = 0;
		byte *pic = NULL;

		filename = "pics/colormap.bmp";

		LoadImageWithPalette(filename, &pic, NULL, &width, &height, &bitsPerPixel);
		if (pic && width == 256 && height == 320)
		{
			int i;

			memcpy(d_8to24table, pic, 256 * 4);
			d_8to24table[255] &= LittleLong(0xffffff);	/* 255 is transparent */

			/* generate colormap */
			*colormap = malloc(256 * 320);
			if (!(*colormap))
			{
				Com_Error(ERR_FATAL, "%s: Couldn't allocate memory for colormap", __func__);
				/* code never returns after ERR_FATAL */
				return;
			}

			for (i = 0; i < (256 * 320); i ++)
			{
				int j, rgb[3];

				for(j = 0; j < 3; j++)
				{
					rgb[j] = pic[i * 4 + j];
				}
				(*colormap)[i] = Convert24to8((byte *)d_8to24table, rgb);
			}

			free(pic);
			colormap_loaded = true;
			return;
		}

		if (pic)
		{
			free(pic);
		}
	}

	if (!*colormap || !pal)
	{
		int i;

		Com_Printf("%s: Couldn't load %s, use generated palette\n",
			__func__, filename);

		/* palette r:2bit, g:3bit, b:3bit */
		for (i = 0; i < 256; i++)
		{
			unsigned v;

			v = (255U<<24) + (((i >> 0) & 0x3) << (6 + 0)) +
							 (((i >> 2) & 0x7) << (5 + 8)) +
							 (((i >> 5) & 0x7) << (5 + 16));
			d_8to24table[i] = LittleLong(v);
		}

		d_8to24table[255] &= LittleLong(0xffffff);	/* 255 is transparent */

		/* generate colormap */
		*colormap = malloc(256 * 320);
		if (!(*colormap))
		{
			Com_Error(ERR_FATAL, "%s: Couldn't allocate memory for colormap", __func__);
			/* code never returns after ERR_FATAL */
			return;
		}

		GenerateColormap((const byte *)d_8to24table, *colormap);
		colormap_loaded = false;
		return;
	}
	else
	{
		Convert24to32(d_8to24table, pal);
		free(pal);
		colormap_loaded = true;
	}
}

/*
===============
VID_GetPalette
===============
*/
void
VID_GetPalette(byte **colormap, unsigned *d_8to24table)
{
	if (!colormap_cache)
	{
		LoadPalette(&colormap_cache, d_8to24table_cache);
	}

	if (!colormap_cache)
	{
		return;
	}

	if (colormap)
	{
		*colormap = malloc(256 * 320);
		memcpy(*colormap, colormap_cache, 256 * 320);
	}
	memcpy(d_8to24table, d_8to24table_cache, sizeof(d_8to24table_cache));
}

/*
 * Get rgb color from palette
 */
unsigned
VID_PaletteColor(byte color)
{
	if (!colormap_cache)
	{
		LoadPalette(&colormap_cache, d_8to24table_cache);
	}

	return d_8to24table_cache[color & 0xFF];
}

void
VID_ImageInit(void)
{
	int i;

	colormap_cache = NULL;
	colormap_loaded = false;

	for (i = 0; i < 256; i++)
	{
		/* fake grey colors */
		d_8to24table_cache[i] = (255U<<24) + (i << 16) + (i << 8) + (i << 0);
	}
}

void
VID_ImageDestroy(void)
{
	if (colormap_cache)
	{
		free(colormap_cache);
	}
}

void
VID_GetPalette24to8(const byte *d_8to24table, byte** d_16to8table)
{
	unsigned char * table16to8;
	const char tablefile[] = "pics/16to8.dat";

	*d_16to8table = NULL;
	FS_LoadFile(tablefile, (void **)&table16to8);

	if (!table16to8)
	{
		Com_Printf("%s: Couldn't load %s\n", __func__, tablefile);
	}

	*d_16to8table = malloc(0x10000);
	if (!(*d_16to8table))
	{
		Com_Error(ERR_FATAL, "%s: Couldn't allocate memory for d_16to8table", __func__);
		/* code never returns after ERR_FATAL */
		return;
	}

	if (table16to8)
	{
		// Use predefined convert map
		memcpy(*d_16to8table, table16to8, 0x10000);
		FS_FreeFile((void *)table16to8);
	}
	else
	{
		// create new one
		unsigned int r;

		Com_Printf("%s: Generate 16 to 8 bit table\n", __func__);

		for (r = 0; r < 32; r++)
		{
			int g;

			for (g = 0; g < 64; g++)
			{
				int b;

				for (b = 0; b < 32; b++)
				{
					int c, rgb[3];

					rgb[0] = r << 3;
					rgb[1] = g << 2;
					rgb[2] = b << 3;

					c = r | ( g << 5 ) | ( b << 11 );

					// set color with minimal difference
					(*d_16to8table)[c & 0xFFFF] = Convert24to8(d_8to24table, rgb);
				}
			}
		}
	}
}
