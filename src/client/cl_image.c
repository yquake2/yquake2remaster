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
 * Shared image load logic
 *
 * =======================================================================
 */

#include "header/client.h"

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

static byte *colormap_cache = NULL;
static unsigned d_8to24table_cache[256];
static qboolean colormap_loaded = false;

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

			Q_strlcpy(token_section, token, sizeof(token_section));

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
			atd_bitmap_t *tmp;

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
			tmp = realloc(anim->bitmaps, anim->bitmap_count * sizeof(atd_bitmap_t));
			YQ2_COM_CHECK_OOM(tmp, "realloc()",
				anim->bitmap_count * sizeof(atd_bitmap_t))
			if (!tmp)
			{
				/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
				return;
			}
			anim->bitmaps = tmp;
			anim->bitmaps[anim->bitmap_count - 1].file = strdup(COM_Parse(&curr_buff));
		}
		else if (!strcmp(token, "!frame"))
		{
			atd_frame_t *frame, *tmp;

			anim->frame_count++;
			tmp = realloc(anim->frames, anim->frame_count * sizeof(atd_frame_t));
			YQ2_COM_CHECK_OOM(tmp, "realloc()",
				anim->frame_count * sizeof(atd_frame_t))
			if (!tmp)
			{
				/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
				return;
			}

			anim->frames = tmp;
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

					Q_strlcpy(token_section, token, sizeof(token_section));

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
			atd_sprites_t *anim;

			anim = malloc(sizeof(*anim));
			YQ2_COM_CHECK_OOM(anim, "malloc()", sizeof(*anim))
			if (!anim)
			{
				FS_FreeFile(raw);
				/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
				return;
			}

			memset(anim, 0, sizeof(*anim));

			tmp_buf = malloc(len + 1);
			YQ2_COM_CHECK_OOM(tmp_buf, "malloc()", len + 1)
			if (!tmp_buf)
			{
				FS_FreeFile(raw);
				/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
				return;
			}

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
				Mod_LoadImageWithPalette(anim->bitmaps[bitmap].file,
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
		Mod_LoadImageWithPalette(filename, pic, palette, width, height, bitsPerPixel);
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
