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

void
GetPCXPalette24to8(const byte *d_8to24table, byte** d_16to8table)
{
	unsigned char * table16to8;
	char tablefile[] = "pics/16to8.dat";

	*d_16to8table = NULL;
	ri.FS_LoadFile(tablefile, (void **)&table16to8);

	if (!table16to8)
	{
		R_Printf(PRINT_ALL, "%s: Couldn't load %s\n", __func__, tablefile);
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
		ri.FS_FreeFile((void *)table16to8);
	}
	else
	{
		// create new one
		unsigned int r;

		R_Printf(PRINT_ALL, "%s: Generate 16 to 8 bit table\n", __func__);

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

/*
===============
GetPCXPalette
===============
*/
void
GetPCXPalette(byte **colormap, unsigned *d_8to24table)
{
	char	filename[] = "pics/colormap.pcx";
	byte	*pal;

	/* get the palette and colormap */
	LoadPCX(filename, colormap, &pal, NULL, NULL);
	if (!*colormap || !pal)
	{
		int width = 0, height = 0;
		byte *pic = NULL;

		if (LoadSTB("pics/colormap", "bmp", &pic, &width, &height) &&
			width == 256 && height == 320)
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

		R_Printf(PRINT_ALL, "%s: Couldn't load %s, use generated palette\n",
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
