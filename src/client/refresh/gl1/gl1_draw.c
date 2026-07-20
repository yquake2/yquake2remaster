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
 * Drawing of all images that are not textures
 *
 * =======================================================================
 */

#include "header/local.h"
#include "../files/stb_truetype.h"

static float gl_font_size = 8.0;
static int gl_font_height = 128;
image_t *draw_chars = NULL;
static image_t *draw_font = NULL;
static stbtt_bakedchar *draw_fontcodes = NULL;

static qboolean draw_chars_has_alt;

extern unsigned r_rawpalette[256];

void R_LoadTTFFont(const char *ttffont, int vid_height, float *r_font_size,
	int *r_font_height, stbtt_bakedchar **draw_fontcodes,
	struct image_s **draw_font,
	loadimage_t R_LoadPic);

void
Draw_InitLocal(void)
{
	R_LoadTTFFont(r_ttffont->string, vid.height, &gl_font_size, &gl_font_height,
		&draw_fontcodes, &draw_font, R_LoadPic);

	draw_chars = R_LoadConsoleChars((findimage_t)R_FindImage);
	/* Heretic 2 uses more than 128 symbols in image */
	draw_chars_has_alt = (draw_chars && (
		strcmp(draw_chars->name, "pics/misc/conchars.m8") &&
		strcmp(draw_chars->name, "pics/misc/conchars.m32")));
}

void
RDraw_FreeLocal(void)
{
	free(draw_fontcodes);
}

static void
Scrap_Update(void)
{
	qboolean default2Dnolerp;
	int texnum;

	default2Dnolerp = r_2D_unfiltered->value != 0.0f;

	for (texnum = 0; texnum < MAX_SCRAPS; texnum++)
	{
		unsigned *scrap_texels;

		scrap_texels = Scrap_Upload(texnum);
		if (scrap_texels)
		{
			R_Bind(TEXNUM_SCRAPS + texnum);
			R_Upload32(scrap_texels, SCRAP_WIDTH, SCRAP_HEIGHT, false);

			if (default2Dnolerp || (texnum < MAX_SCRAPS_NOLERP))
			{
				// 2D textures shouldn't be filtered by default (r_2D_unfiltered),
				// so the scrap shouldn't be filtered
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}
			else // 2D textures should be filtered by default => filter the scrap
			{
				// we can't use gl_filter_min which might be GL_*_MIPMAP_*
				// also, there's no anisotropic filtering for textures w/o mipmaps
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, gl_filter_max);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, gl_filter_max);
			}
		}
	}
}

/*
 * Draws one 8*8 graphics character with 0 being transparent.
 * It can be clipped to the top of the screen to allow the console to be
 * smoothly scrolled off.
 */
void
RDraw_CharScaled(int x, int y, int num, float scale)
{
	int row, col;
	float frow, fcol, size, scaledSize;

	num &= 255;

	if ((num & 127) == 32)
	{
		return; /* space */
	}

	if (y <= -8)
	{
		return; /* totally off screen */
	}

	row = num >> 4;
	col = num & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

	scaledSize = 8 * scale;

	if (draw_chars->scrap)
	{
		Scrap_Update();
	}

	R_UpdateGLBuffer(buf_2d, draw_chars->texnum, 0, 0, 1);

	R_Buffer2DQuad(x, y, x + scaledSize, y + scaledSize,
		draw_chars->sl + fcol * (draw_chars->sh - draw_chars->sl),
		draw_chars->tl + frow * (draw_chars->th - draw_chars->tl),
		draw_chars->sl + (fcol + size) * (draw_chars->sh - draw_chars->sl),
		draw_chars->tl + (frow + size) * (draw_chars->th - draw_chars->tl));
}

void
RDraw_StringScaled(int x, int y, float scale, qboolean alt, const char *message)
{
	while (*message)
	{
		unsigned value = R_NextUTF8Code(&message);

		if (draw_fontcodes && draw_font)
		{
			float font_scale;

			font_scale = gl_font_size / 8.0;

			if (value >= 32 && value < MAX_FONTCODE)
			{
				float xf = 0, yf = 0, xdiff;
				stbtt_aligned_quad q;

				stbtt_GetBakedQuad(draw_fontcodes, gl_font_height, gl_font_height,
					value - 32, &xf, &yf, &q, 1);

				if (alt)
				{
					q.t0 += 0.5;
					q.t1 += 0.5;
				}

				xdiff = (8 - xf / font_scale) / 2;
				if (xdiff < 0)
				{
					xdiff = 0;
				}

				R_UpdateGLBuffer(buf_2d, draw_font->texnum, 0, 0, 1);

				/* If the font texture is packed inside a scrap atlas,
				 * map the 0.0-1.0 glyph coordinates into the scrap sub-region. */
				if (draw_font->scrap)
				{
					q.s0 = draw_font->sl + q.s0 * (draw_font->sh - draw_font->sl);
					q.s1 = draw_font->sl + q.s1 * (draw_font->sh - draw_font->sl);
					q.t0 = draw_font->tl + q.t0 * (draw_font->th - draw_font->tl);
					q.t1 = draw_font->tl + q.t1 * (draw_font->th - draw_font->tl);
				}

				R_Buffer2DQuad(
					(float)(x + (xdiff + q.x0 / font_scale) * scale),
					(float)(y + q.y0 * scale / font_scale + 8 * scale),
					x + (xdiff + q.x1 / font_scale) * scale,
					y + q.y1 * scale / font_scale + 8 * scale,
					q.s0, q.t0, q.s1, q.t1);
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
				RDraw_CharScaled(x, y, value ^ xor, scale);
			}

			x += 8 * scale;
		}
	}
}

image_t *
RDraw_FindPic(const char *name)
{
	return R_FindPic(name, (findimage_t)R_FindImage);
}

void
RDraw_GetPicSize(int *w, int *h, const char *pic)
{
	const image_t *gl;

	gl = R_FindPic(pic, (findimage_t)R_FindImage);

	if (!gl)
	{
		*w = *h = -1;
		return;
	}

	*w = gl->width;
	*h = gl->height;
}

void
RDraw_StretchPic(int x, int y, int w, int h, const char *pic)
{
	image_t *gl;

	gl = R_FindPic(pic, (findimage_t)R_FindImage);

	if (!gl)
	{
		Com_Printf("%s(): Can't find pic: %s\n", __func__, pic);
		return;
	}

	if (gl->scrap)
	{
		Scrap_Update();
	}

	R_UpdateGLBuffer(buf_2d, gl->texnum, 0, 0, 1);

	R_Buffer2DQuad(x, y, x + w, y + h,
		gl->sl, gl->tl, gl->sh, gl->th);
}

void
RDraw_PicScaled(int x, int y, const char *pic, float factor, const char *alttext)
{
	image_t *gl;

	gl = R_FindPic(pic, (findimage_t)R_FindImage);

	if (!gl)
	{
		if (alttext && alttext[0])
		{
			/* Show alttext if provided */
			RDraw_StringScaled(x, y, factor, false, alttext);
			return;
		}

		Com_Printf("Can't find pic: %s\n", pic);
		return;
	}

	if (gl->scrap)
	{
		Scrap_Update();
	}

	if (gl->texnum >= TEXNUM_SCRAPS && gl->texnum < TEXNUM_IMAGES)
	{
		R_UpdateGLBuffer(buf_2d, gl->texnum, 0, 0, 1);
		R_Buffer2DQuad(x, y, x + gl->width * factor, y + gl->height * factor,
			gl->sl, gl->tl, gl->sh, gl->th);
		return;
	}

	if (gl->scrap)
	{
		Scrap_Update();
	}

	R_Bind(gl->texnum);

	GLfloat vtx[] = {
		x, y,
		x + gl->width * factor, y,
		x + gl->width * factor, y + gl->height * factor,
		x, y + gl->height * factor
	};

	GLfloat tex[] = {
		gl->sl, gl->tl,
		gl->sh, gl->tl,
		gl->sh, gl->th,
		gl->sl, gl->th
	};

	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	glVertexPointer( 2, GL_FLOAT, 0, vtx );
	glTexCoordPointer( 2, GL_FLOAT, 0, tex );
	glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
}

void
RDraw_PicScaledCol(int x, int y, const char *pic, float factor, const vec3_t color,
	const char *alttext)
{
	image_t *gl;

	gl = R_FindPic(pic, (findimage_t)R_FindImage);

	if (!gl)
	{
		if (alttext && alttext[0])
		{
			/* Show alttext if provided */
			RDraw_StringScaled(x, y, factor, false, alttext);
			return;
		}

		Com_Printf("Can't find pic: %s\n", pic);
		return;
	}

	if (gl->scrap)
	{
		Scrap_Update();
	}

	R_ApplyGLBuffer();

	R_TexEnv(GL_MODULATE);
	glColor4f(color[0], color[1], color[2], 1);

	R_Bind(gl->texnum);

	GLfloat vtx[] = {
		x, y,
		x + gl->width * factor, y,
		x + gl->width * factor, y + gl->height * factor,
		x, y + gl->height * factor
	};

	GLfloat tex[] = {
		gl->sl, gl->tl,
		gl->sh, gl->tl,
		gl->sh, gl->th,
		gl->sl, gl->th
	};

	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	glVertexPointer( 2, GL_FLOAT, 0, vtx );
	glTexCoordPointer( 2, GL_FLOAT, 0, tex );
	glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );

	glColor4f(1, 1, 1, 1);
	R_TexEnv(GL_REPLACE);
}

/*
 * This repeats a 64*64 tile graphic to fill
 * the screen around a sized down
 * refresh window.
 */
void
RDraw_TileClear(int x, int y, int w, int h, const char *pic)
{
	const image_t *image;

	image = R_FindPic(pic, (findimage_t)R_FindImage);

	if (!image)
	{
		Com_Printf("%s(): Can't find pic: %s\n", __func__, pic);
		return;
	}

	if (image->scrap)
	{
		Scrap_Update();
	}

	R_UpdateGLBuffer(buf_2d, image->texnum, 0, 0, 1);

	R_Buffer2DQuad(x, y, x + w, y + h, x / 64.0, y / 64.0,
		( x + w ) / 64.0, ( y + h ) / 64.0);
}

/*
 * Fills a box of pixels with a single color
 */
void
RDraw_Fill(int x, int y, int w, int h, int c)
{
	union
	{
		unsigned c;
		byte v[4];
	} color;

	if ((unsigned)c > 255)
	{
		Com_Error(ERR_FATAL, "%s: bad color", __func__);
		return;
	}

	glDisable(GL_TEXTURE_2D);

	color.c = d_8to24table[c];
	glColor4f(color.v [ 0 ] / 255.0, color.v [ 1 ] / 255.0,
			   color.v [ 2 ] / 255.0, 1);

	GLfloat vtx[] = {
		x, y,
		x + w, y,
		x + w, y + h,
		x, y + h
	};

	glEnableClientState( GL_VERTEX_ARRAY );

	glVertexPointer( 2, GL_FLOAT, 0, vtx );
	glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );

	glDisableClientState( GL_VERTEX_ARRAY );

	glColor4f( 1, 1, 1, 1 );
	glEnable(GL_TEXTURE_2D);
}

void
RDraw_FadeScreen(void)
{
	R_ApplyGLBuffer();	// draw what needs to be hidden
	glEnable(GL_BLEND);
	glDisable(GL_TEXTURE_2D);
	glColor4f(0, 0, 0, 0.8);

	GLfloat vtx[] = {
		0, 0,
		vid.width, 0,
		vid.width, vid.height,
		0, vid.height
	};

	glEnableClientState( GL_VERTEX_ARRAY );

	glVertexPointer( 2, GL_FLOAT, 0, vtx );
	glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );

	glDisableClientState( GL_VERTEX_ARRAY );

	glColor4f(1, 1, 1, 1);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
}

void
RDraw_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int bits)
{
	GLfloat tex[8];
	float hscale = 1.0f;
	int frac, fracstep;
	int row;

	R_Bind(0);

	if (gl_config.npottextures || rows <= 256 || bits == 32)
	{
		// X, X
		tex[0] = 0;
		tex[1] = 0;

		// X, Y
		tex[2] = 1;
		tex[3] = 0;

		// Y, X
		tex[4] = 1;
		tex[5] = 1;

		// Y, Y
		tex[6] = 0;
		tex[7] = 1;
	}
	else
	{
		// Scale params
		hscale = rows / 256.0;

		// X, X
		tex[0] = 1.0 / 512.0;
		tex[1] = 1.0 / 512.0;

		// X, Y
		tex[2] = 511.0 / 512.0;
		tex[3] = 1.0 / 512.0;

		// Y, X
		tex[4] = 511.0 / 512.0;
		tex[5] = rows * hscale / 256 - 1.0 / 512.0;

		// Y, Y
		tex[6] = 1.0 / 512.0;
		tex[7] = rows * hscale / 256 - 1.0 / 512.0;
	}

	GLfloat vtx[] = {
			x, y,
			x + w, y,
			x + w, y + h,
			x, y + h
	};

	if (!gl_config.palettedtexture || bits == 32)
	{
		/* .. because now if non-power-of-2 textures are supported, we just load
		 * the data into a texture in the original format, without skipping any
		 * pixels to fit into a 256x256 texture.
		 * This causes text in videos (which are 320x240) to not look broken anymore.
		 */
		if (bits == 32)
		{
			glTexImage2D(GL_TEXTURE_2D, 0, gl_tex_solid_format,
					cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE,
					data);
		}
		else if (gl_config.npottextures || rows <= 256)
		{
			unsigned image32[320*240]; /* was 256 * 256, but we want a bit more space */
			unsigned* img = image32;
			size_t i;

			if (cols * rows > 320 * 240)
			{
				size_t img_size = (size_t)cols * rows * 4;

				/* in case there is a bigger video after all,
				 * malloc enough space to hold the frame */
				img = (unsigned*)malloc(img_size);

				YQ2_COM_CHECK_OOM(img, "malloc()", img_size)
				if (!img)
				{
					/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
					return;
				}
			}

			for (i = 0; i < rows; ++i)
			{
				size_t j, rowOffset = i * cols;

				for (j = 0; j < cols; ++j)
				{
					byte palIdx = data[rowOffset+j];
					img[rowOffset+j] = r_rawpalette[palIdx];
				}
			}

			glTexImage2D(GL_TEXTURE_2D, 0, gl_tex_solid_format,
								cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE,
								img);

			if (img != image32)
			{
				free(img);
			}
		}
		else
		{
			unsigned int image32[320 * 240];
			int trows = 256;
			size_t i;

			for (i = 0; i < trows; i++)
			{
				const byte *source;
				unsigned *dest;
				size_t j;

				row = (int)(i * hscale);

				if (row > rows)
				{
					break;
				}

				source = data + cols * row;
				dest = &image32[i * 256];
				fracstep = cols * 0x10000 / 256;
				frac = fracstep >> 1;

				for (j = 0; j < 256; j++)
				{
					dest[j] = r_rawpalette[source[frac >> 16]];
					frac += fracstep;
				}
			}

			glTexImage2D(GL_TEXTURE_2D, 0, gl_tex_solid_format,
					256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE,
					image32);
		}
	}
	else
	{
		byte image8[256 * 256];
		int trows = 256;
		size_t i;

		for (i = 0; i < trows; i++)
		{
			const byte *source;
			byte *dest;
			size_t j;

			row = (int)(i * hscale);

			if (row > rows)
			{
				break;
			}

			source = data + cols * row;
			dest = &image8[i * 256];
			fracstep = cols * 0x10000 / 256;
			frac = fracstep >> 1;

			for (j = 0; j < 256; j++)
			{
				dest[j] = source[frac >> 16];
				frac += fracstep;
			}
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_COLOR_INDEX8_EXT, 256, 256,
				0, GL_COLOR_INDEX, GL_UNSIGNED_BYTE, image8);
	}

	// Note: gl_filter_min could be GL_*_MIPMAP_* so we can't use it for min filter here (=> no mipmaps)
	//       but gl_filter_max (either GL_LINEAR or GL_NEAREST) should do the trick.
	GLint filter = (r_videos_unfiltered->value == 0) ? gl_filter_max : GL_NEAREST;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	glVertexPointer( 2, GL_FLOAT, 0, vtx );
	glTexCoordPointer( 2, GL_FLOAT, 0, tex );
	glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );

	glDisableClientState( GL_VERTEX_ARRAY );
	glDisableClientState( GL_TEXTURE_COORD_ARRAY );
}
