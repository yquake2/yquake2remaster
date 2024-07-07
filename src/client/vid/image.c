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
#include "../refresh/files/stb_image.h"

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

static void
PCX_Decode(const char *name, const byte *raw, int len, byte **pic, byte **palette,
	int *width, int *height)
{
	pcx_t *pcx;
	int x, y;
	int full_size;
	int pcx_width, pcx_height;
	qboolean image_issues = false;
	int dataByte, runLength;
	byte *out, *pix;

	*pic = NULL;

	if (palette)
	{
		*palette = NULL;
	}

	if (len < sizeof(pcx_t))
	{
		return;
	}

	/* parse the PCX file */
	pcx = (pcx_t *)raw;

	pcx->xmin = LittleShort(pcx->xmin);
	pcx->ymin = LittleShort(pcx->ymin);
	pcx->xmax = LittleShort(pcx->xmax);
	pcx->ymax = LittleShort(pcx->ymax);
	pcx->hres = LittleShort(pcx->hres);
	pcx->vres = LittleShort(pcx->vres);
	pcx->bytes_per_line = LittleShort(pcx->bytes_per_line);
	pcx->palette_type = LittleShort(pcx->palette_type);

	raw = &pcx->data;

	pcx_width = pcx->xmax - pcx->xmin;
	pcx_height = pcx->ymax - pcx->ymin;

	if ((pcx->manufacturer != 0x0a) || (pcx->version != 5) ||
		(pcx->encoding != 1) || (pcx->bits_per_pixel != 8))
	{
		Com_Printf("%s: Bad pcx file %s\n", __func__, name);
		return;
	}

	if (pcx->bytes_per_line <= pcx_width)
	{
		pcx->bytes_per_line = pcx_width + 1;
		image_issues = true;
	}

	full_size = (pcx_height + 1) * (pcx_width + 1);
	out = malloc(full_size);
	if (!out)
	{
		Com_Printf("%s: Can't allocate for %s\n", __func__, name);
		return;
	}

	*pic = out;

	pix = out;

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
		}
		else
		{
			image_issues = true;
		}
	}

	if (width)
	{
		*width = pcx_width + 1;
	}

	if (height)
	{
		*height = pcx_height + 1;
	}

	for (y = 0; y <= pcx_height; y++, pix += pcx_width + 1)
	{
		for (x = 0; x < pcx->bytes_per_line; )
		{
			if (raw - (byte *)pcx > len)
			{
				// no place for read
				image_issues = true;
				x = pcx_width;
				break;
			}
			dataByte = *raw++;

			if ((dataByte & 0xC0) == 0xC0)
			{
				runLength = dataByte & 0x3F;
				if (raw - (byte *)pcx > len)
				{
					// no place for read
					image_issues = true;
					x = pcx_width;
					break;
				}
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
					// no place for write
					image_issues = true;
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

	*pic = malloc (len - ofs);
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

	*pic = malloc (len - ofs);
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

	*pic = malloc (len - ofs);
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

	*pic = malloc (len - ofs);
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

void
VID_ImageDecode(const char *filename, byte **pic, byte **palette,
	int *width, int *height, int *bitesPerPixel)
{
	const char* ext;
	int len, ident;
	byte *raw;

	ext = COM_FileExtension(filename);

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

	*pic = NULL;

	ident = LittleShort(*((short*)raw));
	if (!strcmp(ext, "pcx") && (ident == PCX_IDENT))
	{
		PCX_Decode(filename, raw, len, pic, palette, width, height);

		if(*pic && width && height
			&& *width == 319 && *height == 239
			&& Q_strcasecmp(filename, "pics/quit.pcx") == 0
			&& Com_BlockChecksum(raw, len) == 3329419434u)
		{
			// it's the quit screen, and the baseq2 one (identified by checksum)
			// so fix it
			fixQuitScreen(*pic);
		}

		*bitesPerPixel = 8;
	}
	else if (!strcmp(ext, "m8"))
	{
		M8_Decode(filename, raw, len, pic, palette, width, height);
		*bitesPerPixel = 8;
	}
	else if (!strcmp(ext, "swl"))
	{
		SWL_Decode(filename, raw, len, pic, palette, width, height);
		*bitesPerPixel = 8;
	}
	else if (!strcmp(ext, "wal"))
	{
		WAL_Decode(filename, raw, len, pic, palette, width, height);
		*bitesPerPixel = 8;
	}
	else
	{
		int sourcebitesPerPixel = 0;

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
				&sourcebitesPerPixel, STBI_rgb_alpha);

			if (*pic == NULL)
			{
				Com_DPrintf("%s couldn't load data from %s: %s!\n",
					__func__, filename, stbi_failure_reason());
			}
		}

		*bitesPerPixel = 32;
	}

	FS_FreeFile(raw);
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

				out_colormap[y*256+x] = Convert24to8(palette, rgb);
			}
			else
			{
				/* this colour is a fullbright, just keep the original colour */
				out_colormap[y*256+x] = x;
			}
		}
	}
}

static void
LoadPalette(byte **colormap, unsigned *d_8to24table)
{
	const char * filename;
	int bitesPerPixel;
	byte *pal = NULL;

	filename = "pics/colormap.pcx";

	/* get the palette and colormap */
	VID_ImageDecode(filename, colormap, &pal, NULL, NULL,
		&bitesPerPixel);
	if (!*colormap || !pal)
	{
		int width = 0, height = 0;
		byte *pic = NULL;

		filename = "pics/colormap.bmp";

		VID_ImageDecode(filename, &pic, NULL, &width, &height, &bitesPerPixel);
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
		return;
	}
	else
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

		free(pal);
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
