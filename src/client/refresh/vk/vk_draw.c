/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2018-2019 Krzysztof Kondrak
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

static int vk_rawTexture_height = 0;
static int vk_rawTexture_width = 0;
static float vk_font_size = 8.0;
static int vk_font_height = 128;
static image_t *draw_chars = NULL;
static image_t *draw_font = NULL;
static image_t *draw_font_alt = NULL;
static stbtt_bakedchar *draw_fontcodes = NULL;
static qboolean draw_chars_has_alt;

void R_LoadTTFFont(const char *ttffont, int vid_height, float *r_font_size,
	int *r_font_height, stbtt_bakedchar **draw_fontcodes,
	struct image_s **draw_font, struct image_s **draw_font_alt,
	loadimage_t R_LoadPic);

void
Draw_InitLocal(void)
{
	R_LoadTTFFont(r_ttffont->string, vid.height, &vk_font_size, &vk_font_height,
		&draw_fontcodes, &draw_font, &draw_font_alt, Vk_LoadPic);

	draw_chars = R_LoadConsoleChars((findimage_t)Vk_FindImage);
	/* Heretic 2 uses more than 128 symbols in image */
	draw_chars_has_alt = !(draw_chars && !strcmp(draw_chars->name, "pics/misc/conchars.m32"));
}

void
Draw_FreeLocal(void)
{
	free(draw_fontcodes);
}

/*
 * Draws one 8*8 graphics character with 0 being transparent.
 * It can be clipped to the top of the screen to allow the console to be
 * smoothly scrolled off.
 */
void
RE_Draw_CharScaled(int x, int y, int num, float scale)
{
	int row, col;
	float frow, fcol, size, scaledSize;

	if (!vk_frameStarted)
	{
		return;
	}

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

	QVk_DrawTexRect((float)x / vid.width, (float)y / vid.height,
					scaledSize / vid.width, scaledSize / vid.height,
					fcol, frow, size, size, &draw_chars->vk_texture);
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

			font_scale = vk_font_size / 8.0;

			if (value >= 32 && value < MAX_FONTCODE)
			{
				float xf = 0, yf = 0, xdiff;
				stbtt_aligned_quad q;

				stbtt_GetBakedQuad(draw_fontcodes, vk_font_height, vk_font_height,
					value - 32, &xf, &yf, &q, 1);

				xdiff = (8 - xf / font_scale) / 2;
				if (xdiff < 0)
				{
					xdiff = 0;
				}

				QVk_DrawTexRect((float)(x + (xdiff + q.x0 / font_scale) * scale) / vid.width,
								(float)(y + q.y0 * scale / font_scale + 8 * scale) / vid.height,
								(q.x1 - q.x0) * scale / font_scale / vid.width,
								(q.y1 - q.y0) * scale / font_scale / vid.height,
								q.s0, q.t0, q.s1 - q.s0, q.t1 - q.t0,
								alt ? &draw_font_alt->vk_texture : &draw_font->vk_texture);
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
RE_Draw_FindPic
=============
*/
image_t *
RE_Draw_FindPic(const char *name)
{
	return R_FindPic(name, (findimage_t)Vk_FindImage);
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

	image = R_FindPic(name, (findimage_t)Vk_FindImage);
	if (!image)
	{
		*w = *h = -1;
		return;
	}

	*w = image->width;
	*h = image->height;
}

/*
=============
RE_Draw_StretchPic
=============
*/
void
RE_Draw_StretchPic(int x, int y, int w, int h, const char *name)
{
	image_t *vk;

	if (!vk_frameStarted)
	{
		return;
	}

	vk = R_FindPic(name, (findimage_t)Vk_FindImage);
	if (!vk)
	{
		Com_Printf("%s(): Can't find pic: %s\n", __func__, name);
		return;
	}

	QVk_DrawTexRect((float)x / vid.width, (float)y / vid.height,
					(float)w / vid.width, (float)h / vid.height,
					0, 0, 1, 1, &vk->vk_texture);
}


/*
=============
RE_Draw_PicScaled
=============
*/
void
RE_Draw_PicScaled(int x, int y, const char *name, float scale, const char *alttext)
{
	image_t *vk;

	vk = R_FindPic(name, (findimage_t)Vk_FindImage);
	if (!vk)
	{
		if (alttext && alttext[0])
		{
			/* Show alttext if provided */
			RE_Draw_StringScaled(x, y, scale, false, alttext);
			return;
		}

		Com_Printf("%s(): Can't find pic: %s\n", __func__, name);
		return;
	}

	RE_Draw_StretchPic(x, y, vk->width * scale, vk->height * scale, name);
}

/*
 * This repeats a 64*64 tile graphic to fill
 * the screen around a sized down
 * refresh window.
 */
void
RE_Draw_TileClear(int x, int y, int w, int h, const char *name)
{
	const image_t *image;
	float divisor;

	if (!vk_frameStarted)
	{
		return;
	}

	image = R_FindPic(name, (findimage_t)Vk_FindImage);

	if (!image)
	{
		Com_Printf("%s(): Can't find pic: %s\n", __func__, name);
		return;
	}

	/* draw before change viewport */
	QVk_Draw2DCallsRender();

	// Change viewport and scissor to draw in the top left corner as the world view.
	VkViewport tileViewport = vk_viewport;
	VkRect2D tileScissor = vk_scissor;

	tileViewport.x = 0.f;
	tileViewport.y = 0.f;
	tileScissor.offset.x = 0;
	tileScissor.offset.y = 0;

	vkCmdSetViewport(vk_activeCmdbuffer, 0u, 1u, &tileViewport);
	vkCmdSetScissor(vk_activeCmdbuffer, 0u, 1u, &tileScissor);

	divisor = (vk_pixel_size->value < 1.0f ? 1.0f : vk_pixel_size->value);
	QVk_DrawTexRect((float)x / (vid.width * divisor),	(float)y / (vid.height * divisor),
					(float)w / (vid.width * divisor),	(float)h / (vid.height * divisor),
					(float)x / (64.0 * divisor),		(float)y / (64.0 * divisor),
					(float)w / (64.0 * divisor),		(float)h / (64.0 * divisor),
					&image->vk_texture);

	/* force draw before change viewport */
	QVk_Draw2DCallsRender();

	// Restore viewport and scissor.
	vkCmdSetViewport(vk_activeCmdbuffer, 0u, 1u, &vk_viewport);
	vkCmdSetScissor(vk_activeCmdbuffer, 0u, 1u, &vk_scissor);

}

/*
 * Fills a box of pixels with a single color
 */
void
RE_Draw_Fill(int x, int y, int w, int h, int c)
{
	union
	{
		unsigned c;
		byte v[4];
	} color;

	if (!vk_frameStarted)
	{
		return;
	}

	if ((unsigned)c > 255)
	{
		Com_Error(ERR_FATAL, "%s: bad color", __func__);
		return;
	}

	color.c = d_8to24table[c];

	QVk_DrawColorRect(
		(float)x / vid.width, (float)y / vid.height,
		(float)w / vid.width, (float)h / vid.height,
		(float)color.v[0] / 255.f,
		(float)color.v[1] / 255.f,
		(float)color.v[2] / 255.f,
		1.f,
		RP_UI);
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
	if (!vk_frameStarted)
	{
		return;
	}

	QVk_DrawColorRect(
		0.0f, 0.0f, 1.0f, 1.0f,
		0.f, 0.f, 0.f, .8f,
		RP_UI);
}


//====================================================================


/*
=============
RE_Draw_StretchRaw
=============
*/
void
RE_Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int bits)
{

	int i, j;
	unsigned *dest;
	byte *source;
	byte *image_scaled = NULL;
	unsigned *raw_image32;

	if (!vk_frameStarted)
	{
		return;
	}

	if (bits == 32)
	{
		raw_image32 = malloc(cols * rows * sizeof(unsigned));
		if (!raw_image32)
		{
			return;
		}

		memcpy(raw_image32, data, cols * rows * sizeof(unsigned));
	}
	else
	{
		if (r_retexturing->value)
		{
			// triple scaling
			if (cols < (vid.width / 3) || rows < (vid.height / 3))
			{
				image_scaled = malloc(cols * rows * 9);

				scale3x(data, image_scaled, cols, rows);

				cols = cols * 3;
				rows = rows * 3;
			}
			else
			// double scaling
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

		raw_image32 = malloc(cols * rows * sizeof(unsigned));
		YQ2_COM_CHECK_OOM(raw_image32, "malloc()",
			cols * rows * sizeof(unsigned))
		if (!raw_image32)
		{
			/* unaware about YQ2_ATTR_NORETURN_FUNCPTR? */
			return;
		}

		source = image_scaled;
		dest = raw_image32;
		for (i = 0; i < rows; ++i)
		{
			int rowOffset = i * cols;
			for (j = 0; j < cols; ++j)
			{
				byte palIdx = source[rowOffset + j];
				dest[rowOffset + j] = r_rawpalette[palIdx];
			}
		}

		if (r_retexturing->value)
		{
			int scaled_size = cols * rows;

			free(image_scaled);
			SmoothColorImage(raw_image32, scaled_size, cols);
		}
	}

	if (vk_rawTexture.resource.image != VK_NULL_HANDLE &&
	    (vk_rawTexture_width != cols || vk_rawTexture_height != rows))
	{
		QVk_ReleaseTexture(&vk_rawTexture);
		QVVKTEXTURE_CLEAR(vk_rawTexture);
	}

	if (vk_rawTexture.resource.image != VK_NULL_HANDLE)
	{
		QVk_UpdateTextureData(&vk_rawTexture, (unsigned char*)raw_image32, 0, 0, cols, rows);
	}
	else
	{
		vk_rawTexture_width = cols;
		vk_rawTexture_height = rows;

		QVVKTEXTURE_CLEAR(vk_rawTexture);
		QVk_CreateTexture(&vk_rawTexture, (unsigned char*)raw_image32, cols, rows,
			(r_videos_unfiltered->value == 0) ? vk_current_sampler : S_NEAREST,
			false);
		QVk_DebugSetObjectName((uint64_t)vk_rawTexture.resource.image,
			VK_OBJECT_TYPE_IMAGE, "Image: raw texture");
		QVk_DebugSetObjectName((uint64_t)vk_rawTexture.imageView,
			VK_OBJECT_TYPE_IMAGE_VIEW, "Image View: raw texture");
		QVk_DebugSetObjectName((uint64_t)vk_rawTexture.descriptorSet,
			VK_OBJECT_TYPE_DESCRIPTOR_SET, "Descriptor Set: raw texture");
		QVk_DebugSetObjectName((uint64_t)vk_rawTexture.resource.memory,
			VK_OBJECT_TYPE_DEVICE_MEMORY, "Memory: raw texture");
	}

	free(raw_image32);

	QVk_DrawTexRect((float)x / vid.width, (float)y / vid.height,
					(float)w / vid.width, (float)h / vid.height,
					0.f, 0.f, 1.f, 1.0f, &vk_rawTexture);
	QVk_Draw2DCallsRender();
}
