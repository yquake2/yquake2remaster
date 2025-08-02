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
 */

#include "header/local.h"
#include "../files/stb_truetype.h"

static float sw_font_size = 8.0;
static int sw_font_height = 128;
static image_t *draw_chars; // 8*8 graphic characters
static image_t* draw_font = NULL;
static image_t* draw_font_alt = NULL;
static stbtt_bakedchar* draw_fontcodes = NULL;
static qboolean draw_chars_has_alt;

void R_LoadTTFFont(const char *ttffont, int vid_height, float *r_font_size,
	int *r_font_height, stbtt_bakedchar **draw_fontcodes,
	struct image_s **draw_font, struct image_s **draw_font_alt,
	loadimage_t R_LoadPic);

//=============================================================================

/*
================
RE_Draw_FindPic
================
*/
image_t *
RE_Draw_FindPic (const char *name)
{
	return R_FindPic(name, (findimage_t)R_FindImage);
}

/*
===============
Draw_InitLocal
===============
*/
void
Draw_InitLocal(void)
{
	R_LoadTTFFont(r_ttffont->string, vid.height, &sw_font_size, &sw_font_height,
		&draw_fontcodes, &draw_font, &draw_font_alt, (loadimage_t)R_LoadPic);

	draw_chars = R_LoadConsoleChars((findimage_t)R_FindImage);
	/* Heretic 2 uses more than 128 symbols in image */
	draw_chars_has_alt = !(draw_chars && !strcmp(draw_chars->name, "pics/misc/conchars.m32"));
}

/*
================
Draw_Char

Draws one 8*8 graphics character
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void
RE_Draw_CharScaled(int x, int y, int c, float scale)
{
	int drawline, row, col, v, iscale, sscale, width, height;
	const byte *pic_pixels;
	pixel_t *dest;

	iscale = (int) scale;

	if (iscale < 1)
	{
		return;
	}

	c &= 255;

	if ((c&127) == 32)
	{
		return;
	}

	if (y <= -8)
	{
		return;	// totally off screen
	}

	if ( ( y + 8 ) > vid_buffer_height )	// status text was missing in sw...
	{
		return;
	}

	row = c>>4;
	col = c&15;

	width = draw_chars->asset_width * iscale;
	height = draw_chars->asset_height * iscale;
	pic_pixels = Get_BestImageSize(draw_chars, &width, &height);

	sscale = width / draw_chars->asset_width;
	pic_pixels += ((row<<10) * sscale + (col<<3)) * sscale;

	if (y < 0)
	{	// clipped
		drawline = 8 + y;
		pic_pixels -= width * y * sscale;
		y = 0;
	}
	else
	{
		drawline = 8;
	}

	dest = vid_buffer + y * vid_buffer_width + x;

	// clipped last lines
	if ((y + iscale * (drawline + 1)) > vid_buffer_height)
	{
		drawline = (vid_buffer_height - y) / scale;
	}

	VID_DamageBuffer(x, y);
	VID_DamageBuffer(x + (scale * 8), y + (scale * drawline));

	drawline = drawline * scale;

	for (v=0 ; v < drawline; v++, dest += vid_buffer_width)
	{
		int f, fstep, u, sv;
		const byte *source;

		sv = v * height / (iscale * draw_chars->asset_height);
		source = pic_pixels + sv * width;
		f = 0;
		fstep = (width << SHIFT16XYZ) / (scale * draw_chars->asset_width);
		for (u = 0; u < (8 * iscale); u++)
		{
			if (source[f>>16] != TRANSPARENT_COLOR)
			{
				dest[u] = source[f>>16];
			}
			f += fstep;
		}
	}
}

/*
================
RE_Draw_TexRect

Draws a sub-rectangle from a source image to the screen buffer.
x, y: destination top-left in screen pixels
w, h: destination size in screen pixels
s0, t0: top-left UV in [0,1]
sw, th: width/height in UV (e.g. s1-s0, t1-t0)
src: source image (font atlas)
================
*/
void
RE_Draw_TexRect(int x, int y, int w, int h, float s0, float t0, float sw, float th,
	const image_t* src)
{
	int pic_height, pic_width, dy;
	const byte *pic_pixels;

	if (!src || !src->pixels[0])
	{
		return;
	}

	pic_width = src->width;
	pic_height = src->height;
	pic_pixels = src->pixels[0];

	for (dy = 0; dy < h; dy++)
	{
		const byte *source;
		int sy, src_y, dx;
		pixel_t *dst;
		float v;

		sy = y + dy;
		if (sy < 0 || sy >= vid_buffer_height)
		{
			continue;
		}

		v = t0 + th * ((float)dy / h);
		src_y = (int)(v * pic_height);
		if (src_y < 0)
		{
			src_y = 0;
		}

		if (src_y >= pic_height)
		{
			src_y = pic_height - 1;
		}

		dst = vid_buffer + sy * vid_buffer_width + x;
		source = pic_pixels + src_y * pic_width;

		for (dx = 0; dx < w; dx++)
		{
			byte src_pixel;
			int sx, src_x;
			float u;

			sx = x + dx;

			if (sx < 0 || sx >= vid_buffer_width)
			{
				continue;
			}

			u = s0 + sw * ((float)dx / w);
			src_x = (int)(u * pic_width);

			if (src_x < 0)
			{
				src_x = 0;
			}

			if (src_x >= pic_width)
			{
				src_x = pic_width - 1;
			}

			src_pixel = source[src_x];
			if (src_pixel != TRANSPARENT_COLOR)
			{
				dst[dx] = src_pixel;
			}
		}
	}
}

/*
=============
RE_Draw_GetPicSize
=============
*/
void
RE_Draw_GetPicSize(int *w, int *h, const char *name)
{
	const image_t *image;

	image = R_FindPic (name, (findimage_t)R_FindImage);
	if (!image)
	{
		*w = *h = -1;
		return;
	}

	*w = image->asset_width;
	*h = image->asset_height;
}

/*
=============
RE_Draw_StretchPicImplementation
=============
*/
static void
RE_Draw_StretchPicImplementation(int x, int y, int w, int h, const image_t *pic)
{
	int height, skip, pic_height, pic_width;
	const byte *pic_pixels, *source;
	byte *dest;

	if ((x < 0) ||
		(x + w > vid_buffer_width) ||
		(y + h > vid_buffer_height))
	{
		Com_Printf("%s: bad coordinates %dx%d[%dx%d]",
			__func__, x, y, w, h);
		return;
	}

	VID_DamageBuffer(x, y);
	VID_DamageBuffer(x + w, y + h);

	height = h;
	if (y < 0)
	{
		skip = -y;
		height += y;
		y = 0;
	}
	else
	{
		skip = 0;
	}

	dest = vid_buffer + y * vid_buffer_width + x;

	pic_height = h;
	pic_width = w;
	pic_pixels = Get_BestImageSize(pic, &pic_width, &pic_height);

	if (!pic->transparent)
	{
		if (w == pic_width)
		{
			int v;

			for (v = 0; v < height; v++, dest += vid_buffer_width)
			{
				int sv;

				sv = (skip + v) * pic_height / h;
				memcpy (dest, pic_pixels + sv*pic_width, w);
			}
		}
		else
		{
			int v;

			// size of screen tile to pic pixel
			int picupscale = h / pic_height;

			for (v = 0; v < height; v++, dest += vid_buffer_width)
			{
				int f, fstep, u, sv;

				sv = (skip + v) * pic_height/h;
				source = pic_pixels + sv*pic_width;
				f = 0;
				fstep = (pic_width << SHIFT16XYZ) / w;
				for (u = 0; u < w; u++)
				{
					dest[u] = source[f>>16];
					f += fstep;
				}

				if (picupscale > 1)
				{
					const pixel_t *dest_orig;
					int pu, i;

					pu = Q_min(height-v, picupscale);
					dest_orig = dest;

					// copy first line to fill whole sector
					for (i = 1; i < pu; i++)
					{
						// go to next line
						dest += vid_buffer_width;
						memcpy (dest, dest_orig, w);
					}

					// skip updated lines
					v += (picupscale - 1);
				}
			}
		}
	}
	else
	{
		source = pic_pixels;

		if (h == pic_height && w == pic_width)
		{
			int v;

			for (v = 0; v < pic_height; v++)
			{
				int u;

				for (u = 0; u < pic_width; u++)
				{
					if (source[u] != TRANSPARENT_COLOR)
						dest[u] = source[u];
				}
				dest += vid_buffer_width;
				source += pic_width;
			}
		}
		else
		{
			int v;

			for (v = 0; v < height; v++, dest += vid_buffer_width)
			{
				int f, fstep, u, sv;

				sv = (skip + v) * pic_height/h;
				source = pic_pixels + sv*pic_width;
				f = 0;
				fstep = (pic_width << SHIFT16XYZ) / w;

				for (u = 0; u < w; u++)
				{
					if (source[f>>16] != TRANSPARENT_COLOR)
					{
						dest[u] = source[f>>16];
					}
					f += fstep;
				}
			}
		}
	}
}

/*
=============
RE_Draw_StretchPic
=============
*/
void
RE_Draw_StretchPic(int x, int y, int w, int h, const char *name)
{
	const image_t *pic;

	pic = R_FindPic (name, (findimage_t)R_FindImage);
	if (!pic)
	{
		Com_Printf("Can't find pic: %s\n", name);
		return;
	}

	RE_Draw_StretchPicImplementation (x, y, w, h, pic);
}

/*
=============
RE_Draw_StretchRaw
=============
*/
void
RE_Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int bits)
{
	byte *image_scaled;
	image_t pic = {0};

	/* we have only one image size */
	pic.mip_levels = 1;

	if (bits == 32)
	{
		image_scaled = malloc(cols * rows);
		R_Convert32To8bit(data, image_scaled, cols * rows, false);
	}
	else if (r_retexturing->value)
	{
		if (cols < (w / 3) || rows < (h / 3))
		{
			image_scaled = malloc(cols * rows * 9);

			scale3x(data, image_scaled, cols, rows);

			cols = cols * 3;
			rows = rows * 3;
		}
		else
		{
			image_scaled = malloc(cols * rows * 4);

			scale2x(data, image_scaled, cols, rows);

			cols = cols * 2;
			rows = rows * 2;
		}
	}
	else
	{
		image_scaled = (byte *)data;
	}

	pic.pixels[0] = image_scaled;
	pic.width = cols;
	pic.height = rows;
	pic.asset_width = cols;
	pic.asset_height = rows;

	RE_Draw_StretchPicImplementation(x, y, w, h, &pic);

	if (image_scaled != (byte *)data)
	{
		free(image_scaled);
	}
}

void
RE_Draw_StringScaled(int x, int y, float scale, qboolean alt, const char *message)
{
	while (*message)
	{
		unsigned value = R_NextUTF8Code(&message);

		if (draw_fontcodes && draw_font && draw_font_alt)
		{
			float font_scale;

			font_scale = sw_font_size / 8.0;

			if (value >= 32 && value < MAX_FONTCODE)
			{
				float xf = 0, yf = 0, xdiff;
				stbtt_aligned_quad q;

				stbtt_GetBakedQuad(draw_fontcodes, sw_font_height, sw_font_height,
					value - 32, &xf, &yf, &q, 1);

				xdiff = (8 - xf / font_scale) / 2;
				if (xdiff < 0)
				{
					xdiff = 0;
				}

				RE_Draw_TexRect(
					x + (xdiff + q.x0 / font_scale) * scale,
					y + q.y0 * scale / font_scale + 8 * scale,
					(q.x1 - q.x0) * scale / font_scale,
					(q.y1 - q.y0) * scale / font_scale,
					q.s0, q.t0, q.s1 - q.s0, q.t1 - q.t0,
					alt ? draw_font_alt : draw_font);
				x += Q_max(8, xf / font_scale) * scale;
			}
			else
			{
				x += 8 * scale;
			}
		}
		else
		{
			int xor;

			xor = (alt && draw_chars_has_alt) ? 0x80 : 0;

			if (value > ' ' && value < 128)
			{
				RE_Draw_CharScaled(x, y, value ^ xor, scale);
			}

			x += 8 * scale;
		}
	}
}

/*
=============
Draw_Pic
=============
*/
void
RE_Draw_PicScaled(int x, int y, const char *name, float scale, const char *alttext)
{
	const image_t *pic;

	pic = R_FindPic (name, (findimage_t)R_FindImage);
	if (!pic)
	{
		if (alttext && alttext[0])
		{
			/* Show alttext if provided */
			RE_Draw_StringScaled(x, y, scale, false, alttext);
			return;
		}

		Com_Printf("Can't find pic: %s\n", name);
		return;
	}

	RE_Draw_StretchPicImplementation (
		x, y,
		scale * pic->asset_width, scale * pic->asset_height,
		pic);
}

/*
=============
RE_Draw_TileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down
refresh window.
=============
*/
void
RE_Draw_TileClear (int x, int y, int w, int h, const char *name)
{
	const byte *psrc;
	pixel_t *pdest;
	int i, j, x2;
	image_t *pic;

	if (x < 0)
	{
		w += x;
		x = 0;
	}
	if (y < 0)
	{
		h += y;
		y = 0;
	}
	if (x + w > vid_buffer_width)
	{
		w = vid_buffer_width - x;
	}

	if (y + h > vid_buffer_height)
	{
		h = vid_buffer_height - y;
	}

	if (w <= 0 || h <= 0)
	{
		return;
	}

	VID_DamageBuffer(x, y);
	VID_DamageBuffer(x + w, y + h);

	pic = R_FindPic (name, (findimage_t)R_FindImage);
	if (!pic)
	{
		Com_Printf("Can't find pic: %s\n", name);
		return;
	}

	x2 = x + w;
	pdest = vid_buffer + y * vid_buffer_width;
	for (i=0 ; i<h ; i++, pdest += vid_buffer_width)
	{
		psrc = pic->pixels[0] + pic->width * ((i+y) % pic->height);
		for (j=x ; j<x2 ; j++)
			pdest[j] = psrc[j % pic->width];
	}
}


/*
=============
RE_Draw_Fill

Fills a box of pixels with a single color
=============
*/
void
RE_Draw_Fill(int x, int y, int w, int h, int c)
{
	pixel_t *dest;
	int v;

	if (x + w > vid_buffer_width)
	{
		w = vid_buffer_width - x;
	}

	if (y + h > vid_buffer_height)
	{
		h = vid_buffer_height - y;
	}

	if (x < 0)
	{
		w += x;
		x = 0;
	}

	if (y < 0)
	{
		h += y;
		y = 0;
	}

	if (w < 0 || h < 0)
	{
		return;
	}

	VID_DamageBuffer(x, y);
	VID_DamageBuffer(x + w, y + h);

	dest = vid_buffer + y * vid_buffer_width + x;
	for (v = 0 ; v < h; v++, dest += vid_buffer_width)
	{
		memset(dest, c, w);
	}
}
//=============================================================================

/*
================
RE_Draw_FadeScreen

================
*/
void
RE_Draw_FadeScreen(void)
{
	int x,y;

	VID_DamageBuffer(0, 0);
	VID_DamageBuffer(vid_buffer_width, vid_buffer_height);

	for (y = 0 ; y < vid_buffer_height; y++)
	{
		pixel_t *pbuf;
		int t;

		pbuf = vid_buffer + vid_buffer_width * y;
		t = (y & 1) << 1;

		for (x = 0; x < vid_buffer_width; x++)
		{
			if ((x & 3) != t)
			{
				pbuf[x] = 0;
			}
		}
	}
}
