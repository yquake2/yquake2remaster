/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2016-2017 Daniel Gibson
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
#include "../files/DG_dynarr.h"

unsigned d_8to24table[256];

static float gl3_font_size = 8.0;
static int gl3_font_height = 128;
gl3image_t *draw_chars = NULL;
GLint oldViewPort[4];
static gl3image_t *draw_font = NULL;
static stbtt_bakedchar *draw_fontcodes = NULL;
static qboolean draw_chars_has_alt;

static GLuint vbo2D = 0, ebo2D = 0, vao2D = 0, vao2Dcolor = 0; // vao2D is for textured rendering, vao2Dcolor for color-only
static qboolean bloomInitialized = false;

#define BLOOM_TEXTURES 2
static GLuint bloomTex[BLOOM_TEXTURES] = {0, 0};
static GLuint bloomFBO[BLOOM_TEXTURES] = {0, 0};

void R_LoadTTFFont(const char *ttffont, int vid_height, float *r_font_size,
	int *r_font_height, stbtt_bakedchar **draw_fontcodes,
	struct image_s **draw_font,
	loadimage_t R_LoadPic);

int gl3_num3Ddraws = 0, gl3_num2Ddraws = 0, gl3_numBufferVtxData = 0, gl3_numBufferUniforms = 0;

typedef struct gl3_drawVert2D_s {
	float x, y, s, t;
} gl3_drawVert2D;

DA_TYPEDEF(gl3_drawVert2D, Vtx2DArray_t);
DA_TYPEDEF(GLushort, UShortArray_t);
// dynamic arrays to batch all consecutive 2D draws with same texture to reduce drawcalls
static Vtx2DArray_t vtxBuf = {0};
static UShortArray_t idxBuf = {0};
static GLuint lastBatchTexture = 0;

void
GL3_Draw_InitLocal(void)
{
	R_LoadTTFFont(r_ttffont->string, vid.height, &gl3_font_size, &gl3_font_height,
		&draw_fontcodes, &draw_font, (loadimage_t)GL3_LoadPic);

	draw_chars = R_LoadConsoleChars((findimage_t)GL3_FindImage);
	/* Heretic 2 uses more than 128 symbols in image */
	draw_chars_has_alt = (draw_chars && (
		strcmp(draw_chars->name, "pics/misc/conchars.m8") &&
		strcmp(draw_chars->name, "pics/misc/conchars.m32")));

	// set up attribute layout for 2D textured rendering
	glGenVertexArrays(1, &vao2D);
	glBindVertexArray(vao2D);

	glGenBuffers(1, &vbo2D);
	GL3_BindVBO(vbo2D);
	glGenBuffers(1, &ebo2D);

	GL3_UseProgram(gl3state.si2D.shaderProgram);

	glEnableVertexAttribArray(GL3_ATTRIB_POSITION);
	// Note: the glVertexAttribPointer() configuration is stored in the VAO, not the shader or sth
	//       (that's why I use one VAO per 2D shader)
	qglVertexAttribPointer(GL3_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), 0);

	glEnableVertexAttribArray(GL3_ATTRIB_TEXCOORD);
	qglVertexAttribPointer(GL3_ATTRIB_TEXCOORD, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float), 2*sizeof(float));

	// set up attribute layout for 2D flat color rendering

	glGenVertexArrays(1, &vao2Dcolor);
	glBindVertexArray(vao2Dcolor);

	GL3_BindVBO(vbo2D); // yes, both VAOs share the same VBO

	GL3_UseProgram(gl3state.si2Dcolor.shaderProgram);

	glEnableVertexAttribArray(GL3_ATTRIB_POSITION);
	qglVertexAttribPointer(GL3_ATTRIB_POSITION, 2, GL_FLOAT, GL_FALSE, 2*sizeof(float), 0);

	GL3_BindVAO(0);
}

void
GL3_Draw_ShutdownLocal(void)
{
	glDeleteBuffers(1, &ebo2D);
	ebo2D = 0;
	glDeleteBuffers(1, &vbo2D);
	vbo2D = 0;
	glDeleteVertexArrays(1, &vao2D);
	vao2D = 0;
	glDeleteVertexArrays(1, &vao2Dcolor);
	vao2Dcolor = 0;

	da_free(vtxBuf);
	da_free(idxBuf);

	free(draw_fontcodes);
}

void
GL3_DrawCurrent2Dbatch()
{
	int numVtx = da_count(vtxBuf);

	if (numVtx == 0)
	{
		return;
	}

	GL3_UseProgram(gl3state.si2D.shaderProgram);
	GL3_Bind(lastBatchTexture);

	GL3_BindVAO(vao2D);

	// Note: while vao2D "remembers" its vbo for drawing, binding the vao does *not*
	//       implicitly bind the vbo, so I need to explicitly bind it before glBufferData()
	GL3_BindVBO(vbo2D);
	glBufferData(GL_ARRAY_BUFFER, sizeof(gl3_drawVert2D)*numVtx, vtxBuf.p, GL_STREAM_DRAW);

	GL3_BindEBO(ebo2D);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, da_count(idxBuf)*sizeof(GLushort), idxBuf.p, GL_STREAM_DRAW);
	glDrawElements(GL_TRIANGLES, da_count(idxBuf), GL_UNSIGNED_SHORT, NULL);

	++gl3_numBufferVtxData;
	++gl3_num2Ddraws;

	lastBatchTexture = 0;
	da_clear(vtxBuf);
	da_clear(idxBuf);
}

static void
drawTexturedRectangle(GLuint texNum, float x, float y, float w, float h,
                      float sl, float tl, float sh, float th)
{
	/*
	 *  x,y+h      x+w,y+h
	 * sl,th--------sh,th
	 *  |             |
	 *  |             |
	 *  |             |
	 * sl,tl--------sh,tl
	 *  x,y        x+w,y
	 */

	if ((lastBatchTexture != 0 && texNum != lastBatchTexture) || da_count(vtxBuf) + 4 > UINT16_MAX)
	{
		GL3_DrawCurrent2Dbatch();
	}

	lastBatchTexture = texNum;

	GLushort firstIdx = da_count(vtxBuf);

	gl3_drawVert2D* addVtx = da_addn_uninit(vtxBuf, 4);
	//                            X,   Y,   S,  T
	addVtx[0] = (gl3_drawVert2D){ x,   y+h, sl, th };
	addVtx[1] = (gl3_drawVert2D){ x,   y,   sl, tl };
	addVtx[2] = (gl3_drawVert2D){ x+w, y+h, sh, th };
	addVtx[3] = (gl3_drawVert2D){ x+w, y,   sh, tl };

	GLushort* addIdx = da_addn_uninit(idxBuf, 6);
	addIdx[0] = firstIdx;  // first triangle of rectangle
	addIdx[1] = firstIdx+1;
	addIdx[2] = firstIdx+2;
	addIdx[3] = firstIdx+1; // second triangle
	addIdx[4] = firstIdx+3;
	addIdx[5] = firstIdx+2;
}

// bind the texture before calling this
static void
drawTexturedRectangleNow(float x, float y, float w, float h,
                      float sl, float tl, float sh, float th)
{
	/*
	 *  x,y+h      x+w,y+h
	 * sl,th--------sh,th
	 *  |             |
	 *  |             |
	 *  |             |
	 * sl,tl--------sh,tl
	 *  x,y        x+w,y
	 */

	// in case some batched 2D draws are outstanding, draw them now
	// to preserve draw order
	GL3_DrawCurrent2Dbatch();

	gl3_drawVert2D vBuf[4] = {
	//    X,   Y,   S,  T
		{ x,   y+h, sl, th },
		{ x,   y,   sl, tl },
		{ x+w, y+h, sh, th },
		{ x+w, y,   sh, tl }
	};

	GL3_BindVAO(vao2D);

	// Note: while vao2D "remembers" its vbo for drawing, binding the vao does *not*
	//       implicitly bind the vbo, so I need to explicitly bind it before glBufferData()
	GL3_BindVBO(vbo2D);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vBuf), vBuf, GL_STREAM_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	++gl3_numBufferVtxData;
	++gl3_num2Ddraws;

	//glMultiDrawArrays(mode, first, count, drawcount) ??
}

/*
 * Draws one 8*8 graphics character with 0 being transparent.
 * It can be clipped to the top of the screen to allow the console to be
 * smoothly scrolled off.
 */
void
GL3_Draw_CharScaled(int x, int y, int num, float scale)
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
		GL3_Scrap_Upload();
	}

	drawTexturedRectangle(draw_chars->texnum, x, y, scaledSize, scaledSize,
		draw_chars->sl + fcol * (draw_chars->sh - draw_chars->sl),
		draw_chars->tl + frow * (draw_chars->th - draw_chars->tl),
		draw_chars->sl + (fcol + size) * (draw_chars->sh - draw_chars->sl),
		draw_chars->tl + (frow + size) * (draw_chars->th - draw_chars->tl));
}

void
GL3_Draw_StringScaled(int x, int y, float scale, qboolean alt, const char *message)
{
	while (*message)
	{
		unsigned value = R_NextUTF8Code(&message);

		if (draw_fontcodes && draw_font)
		{
			float font_scale;

			font_scale = gl3_font_size / 8.0;

			if (value >= 32 && value < MAX_FONTCODE)
			{
				float xf = 0, yf = 0, xdiff;
				stbtt_aligned_quad q;

				stbtt_GetBakedQuad(draw_fontcodes, gl3_font_height, gl3_font_height,
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

				/* If the font texture is packed inside a scrap atlas,
				 * map the 0.0-1.0 glyph coordinates into the scrap sub-region. */
				if (draw_font->scrap)
				{
					q.s0 = draw_font->sl + q.s0 * (draw_font->sh - draw_font->sl);
					q.s1 = draw_font->sl + q.s1 * (draw_font->sh - draw_font->sl);
					q.t0 = draw_font->tl + q.t0 * (draw_font->th - draw_font->tl);
					q.t1 = draw_font->tl + q.t1 * (draw_font->th - draw_font->tl);
				}

				drawTexturedRectangle(
					draw_font->texnum,
					(float)(x + (xdiff + q.x0 / font_scale) * scale),
					(float)(y + q.y0 * scale / font_scale + 8 * scale),
					(q.x1 - q.x0) * scale / font_scale,
					(q.y1 - q.y0) * scale / font_scale,
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
				GL3_Draw_CharScaled(x, y, value ^ xor, scale);
			}

			x += 8 * scale;
		}
	}
}

gl3image_t *
GL3_Draw_FindPic(const char *name)
{
	return R_FindPic(name, (findimage_t)GL3_FindImage);
}

void
GL3_Draw_GetPicSize(int *w, int *h, const char *pic)
{
	const gl3image_t *gl;

	gl = R_FindPic(pic, (findimage_t)GL3_FindImage);

	if (!gl)
	{
		*w = *h = -1;
		return;
	}

	*w = gl->width;
	*h = gl->height;
}

void
GL3_Draw_StretchPic(int x, int y, int w, int h, const char *pic)
{
	const gl3image_t *gl;

	gl = R_FindPic(pic, (findimage_t)GL3_FindImage);

	if (!gl)
	{
		Com_Printf("%s(): Can't find pic: %s\n", __func__, pic);
		return;
	}

	if (gl->scrap)
	{
		/* Upload any pending scrap textures before rendering 2D elements */
		GL3_Scrap_Upload();
	}

	drawTexturedRectangle(gl->texnum, x, y, w, h, gl->sl, gl->tl, gl->sh, gl->th);
}

void
GL3_Draw_PicScaled(int x, int y, const char *pic, float factor, const char *alttext)
{
	const gl3image_t *gl;

	gl = R_FindPic(pic, (findimage_t)GL3_FindImage);
	if (!gl)
	{
		if (alttext && alttext[0])
		{
			/* Show alttext if provided */
			GL3_Draw_StringScaled(x, y, factor, false, alttext);
			return;
		}

		Com_Printf("Can't find pic: %s\n", pic);
		return;
	}

	if (gl->scrap)
	{
		/* Upload any pending scrap textures before rendering 2D elements */
		GL3_Scrap_Upload();
	}

	drawTexturedRectangle(gl->texnum, x, y, gl->width * factor, gl->height * factor,
		gl->sl, gl->tl, gl->sh, gl->th);
}

void
GL3_Draw_PicScaledCol(int x, int y, const char *pic, float factor, const vec3_t color,
	const char *alttext)
{
	const gl3image_t *gl;

	gl = R_FindPic(pic, (findimage_t)GL3_FindImage);
	if (!gl)
	{
		if (alttext && alttext[0])
		{
			/* Show alttext if provided */
			GL3_Draw_StringScaled(x, y, factor, false, alttext);
			return;
		}

		Com_Printf("Can't find pic: %s\n", pic);
		return;
	}

	if (gl->scrap)
	{
		/* Upload any pending scrap textures before rendering 2D elements */
		GL3_Scrap_Upload();
	}

	gl3state.uniCommonData.color = HMM_Vec4(color[0], color[1], color[2], 1.0f);
	GL3_UpdateUBOCommon();

	GL3_UseProgram(gl3state.si2Dtinted.shaderProgram);
	GL3_Bind(gl->texnum);

	// NOTE: this function (and this shader) are only used for the crosshair
	//       so use the simple immediate (unbatched) draw function
	drawTexturedRectangleNow(x, y, gl->width * factor, gl->height*factor, gl->sl, gl->tl, gl->sh, gl->th);

	gl3state.uniCommonData.color = HMM_Vec4(1, 1, 1, 1);
	GL3_UpdateUBOCommon();
}

/*
 * This repeats a 64*64 tile graphic to fill
 * the screen around a sized down
 * refresh window.
 */
void
GL3_Draw_TileClear(int x, int y, int w, int h, const char *pic)
{
	const gl3image_t *image;

	if(w <= 0 || h <= 0)
	{
		return;
	}

	image = R_FindPic(pic, (findimage_t)GL3_FindImage);
	if (!image)
	{
		Com_Printf("%s(): Can't find pic: %s\n", __func__, pic);
		return;
	}

	if (image->scrap)
	{
		/* Upload any pending scrap textures before rendering 2D elements */
		GL3_Scrap_Upload();
	}

	drawTexturedRectangle(image->texnum, x, y, w, h,
		x / 64.0f, y / 64.0f, (x + w) / 64.0f, (y + h) / 64.0f);
}

void
GL3_DrawFrameBufferObject(int x, int y, int w, int h, GLuint fboTexture, const float v_blend[4])
{
	GLuint finalTex = fboTexture;
	qboolean bloomActive = false;

	/* check for r_bloom */
	if (r_bloom && r_bloom->value)
	{
		GLuint bloom = GL3_ApplyBloom(fboTexture, w, h);
		if (bloom != 0)
		{
			finalTex = bloom;
			bloomActive = true;
		}
	}

	qboolean underwater = (r_newrefdef.rdflags & RDF_UNDERWATER) != 0;
	gl3ShaderInfo_t* shader = underwater ? &gl3state.si2DpostProcessWater
	                                     : &gl3state.si2DpostProcess;

	/* select shader and bind scene texture */
	GL3_UseProgram(shader->shaderProgram);
	GL3_Bind(finalTex);

	/* set shader uniforms if present */
	if (underwater && shader->uniLmScalesOrTime != -1)
	{
		glUniform1f(shader->uniLmScalesOrTime, r_newrefdef.time);
	}

	if (shader->uniVblend != -1)
	{
		glUniform4fv(shader->uniVblend, 1, v_blend);
	}

	if (!r_bloom || !r_bloom->value)
	{
		drawTexturedRectangleNow(x, y, w, h, 0, 1, 1, 0);
		return;
	}

	/*
	 * Build a small fullscreen quad vertex array and upload it into the
	 * shared VBO. We use the same VBO/VAO used by other 2D draw helpers
	 * (vao2D / vbo2D). The VAO already has pointers configured
	 * in GL3_Draw_InitLocal(), so we only need to upload the vertex data.
	 *
	 * Vertex layout (matches VAO setup): X, Y, S, T
	 */
	GLfloat fsQuad[16];

	if (bloomActive && underwater)
	{
		/* invert T coordinates to compensate for FBO orientation underwater */
		fsQuad[0]  = (GLfloat)x;       fsQuad[1]  = (GLfloat)(y + h); fsQuad[2]  = 0.0f; fsQuad[3]  = 0.0f;
		fsQuad[4]  = (GLfloat)x;       fsQuad[5]  = (GLfloat)y;       fsQuad[6]  = 0.0f; fsQuad[7]  = 1.0f;
		fsQuad[8]  = (GLfloat)(x + w); fsQuad[9]  = (GLfloat)(y + h); fsQuad[10] = 1.0f; fsQuad[11] = 0.0f;
		fsQuad[12] = (GLfloat)(x + w); fsQuad[13] = (GLfloat)y;       fsQuad[14] = 1.0f; fsQuad[15] = 1.0f;
	}
	else
	{
		fsQuad[0]  = (GLfloat)x;       fsQuad[1]  = (GLfloat)(y + h); fsQuad[2]  = 0.0f; fsQuad[3]  = 1.0f;
		fsQuad[4]  = (GLfloat)x;       fsQuad[5]  = (GLfloat)y;       fsQuad[6]  = 0.0f; fsQuad[7]  = 0.0f;
		fsQuad[8]  = (GLfloat)(x + w); fsQuad[9]  = (GLfloat)(y + h); fsQuad[10] = 1.0f; fsQuad[11] = 1.0f;
		fsQuad[12] = (GLfloat)(x + w); fsQuad[13] = (GLfloat)y;       fsQuad[14] = 1.0f; fsQuad[15] = 0.0f;
	}

	GL3_BindVAO(vao2D);
	GL3_BindVBO(vbo2D);

	glBufferData(GL_ARRAY_BUFFER, sizeof(fsQuad), fsQuad, GL_STREAM_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	GL3_BindVAO(0);
	if (finalTex != fboTexture)
	{
		glDeleteTextures(1, &finalTex);
	}
}

/*
 * Fills a box of pixels with a single color
 */
void
GL3_Draw_Fill(int x, int y, int w, int h, int c)
{
	union
	{
		unsigned c;
		byte v[4];
	} color;
	size_t i;

	if ((unsigned)c > 255)
	{
		Com_Error(ERR_FATAL, "%s: bad color", __func__);
		return;
	}

	color.c = d_8to24table[c];

	// in case some batched 2D draws are outstanding, draw them now
	// to preserve draw order
	GL3_DrawCurrent2Dbatch();

	GLfloat vBuf[8] = {
	//  X,   Y
		x,   y+h,
		x,   y,
		x+w, y+h,
		x+w, y
	};

	for (i = 0; i < 3; ++i)
	{
		gl3state.uniCommonData.color.Elements[i] = color.v[i] * (1.0f/255.0f);
	}

	gl3state.uniCommonData.color.A = 1.0f;

	GL3_UpdateUBOCommon();

	GL3_UseProgram(gl3state.si2Dcolor.shaderProgram);
	GL3_BindVAO(vao2Dcolor);

	GL3_BindVBO(vbo2D);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vBuf), vBuf, GL_STREAM_DRAW);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	++gl3_numBufferVtxData;
	++gl3_num2Ddraws;
}

// in GL1 this is called R_Flash() (which just calls R_PolyBlend())
// now implemented in 2D mode and called after SetGL2D() because
// it's pretty similar to GL3_Draw_FadeScreen()
void
GL3_Draw_Flash(const float color[4], float x, float y, float w, float h)
{
	size_t i = 0;

	if (gl_polyblend->value == 0)
	{
		return;
	}

	// in case some batched 2D draws are outstanding, draw them now
	// to preserve draw order
	GL3_DrawCurrent2Dbatch();

	GLfloat vBuf[8] = {
	//  X,   Y
		x,   y+h,
		x,   y,
		x+w, y+h,
		x+w, y
	};

	glEnable(GL_BLEND);

	if (r_bloom && r_bloom->value)
	{
		/* this blends the screen flash while bloom is enabled
		 * TODO: disable broke fixing window on disable bloom */
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}

	for (i = 0; i < 4; ++i)
	{
		gl3state.uniCommonData.color.Elements[i] = color[i];
	}

	GL3_UpdateUBOCommon();

	GL3_UseProgram(gl3state.si2Dcolor.shaderProgram);

	GL3_BindVAO(vao2Dcolor);

	GL3_BindVBO(vbo2D);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vBuf), vBuf, GL_STREAM_DRAW);

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	++gl3_numBufferVtxData;
	++gl3_num2Ddraws;

	glDisable(GL_BLEND);
}

void
GL3_Draw_FadeScreen(void)
{
	float color[4] = {0, 0, 0, 0.6f};
	GL3_Draw_Flash(color, 0, 0, vid.width, vid.height);
}

void
GL3_Draw_StretchRaw(int x, int y, int w, int h, int cols, int rows, const byte *data, int bits)
{
	GL3_Bind(0);

	unsigned image32[320*240]; /* was 256 * 256, but we want a bit more space */

	unsigned* img = image32;

	if (bits == 32)
	{
		img = (unsigned *)data;
	}
	else
	{
		size_t i;

		if (cols * rows > 320 * 240)
		{
			/* in case there is a bigger video after all,
			 * malloc enough space to hold the frame */
			img = (unsigned*)malloc(cols*rows*4);
		}

		for (i = 0; i < rows; ++i)
		{
			size_t j, rowOffset;

			rowOffset = i * cols;

			for (j = 0; j < cols; ++j)
			{
				byte palIdx = data[rowOffset+j];
				img[rowOffset+j] = gl3_rawpalette[palIdx];
			}
		}
	}

	GL3_UseProgram(gl3state.si2D.shaderProgram);

	GLuint glTex;
	glGenTextures(1, &glTex);
	GL3_SelectTMU(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, glTex);

	glTexImage2D(GL_TEXTURE_2D, 0, gl3_tex_solid_format,
	             cols, rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);

	if (img != image32 && img != (unsigned *)data)
	{
		free(img);
	}

	// Note: gl_filter_min could be GL_*_MIPMAP_* so we can't use it for min filter here (=> no mipmaps)
	//       but gl_filter_max (either GL_LINEAR or GL_NEAREST) should do the trick.
	GLint filter = (r_videos_unfiltered->value == 0) ? gl_filter_max : GL_NEAREST;
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);

	// NOTE: this is only used for videos and only called once per frame (or not at all)
	drawTexturedRectangleNow(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f);

	glDeleteTextures(1, &glTex);

	GL3_Bind(0);
}

/*
 * Called at the end of the frame, after 2D (UI) rendering is done.
 * Does some internal housekeeping, then swaps the buffers
 * and shows the next frame.
 */
void GL3_EndFrame(void)
{
	GL3_DrawCurrent2Dbatch();

	// by saving those values into a variable and setting them to 0 afterwards,
	// gl3_show_draw_stats can include its own drawcalls (from previous frame)
	int num3D = gl3_num3Ddraws;
	int num2D = gl3_num2Ddraws;
	int numBufVtx = gl3_numBufferVtxData;
	int numBufUni = gl3_numBufferUniforms;
	gl3_num3Ddraws = 0;
	gl3_num2Ddraws = 0;
	gl3_numBufferVtxData = 0;
	gl3_numBufferUniforms = 0;

	if(gl3_show_draw_stats->value)
	{
		float factor = 1.0f; // TODO: like SCR_GetConsoleScale()
		char stbuf[128] = {0};
		snprintf(stbuf, sizeof(stbuf), "3D drawcalls: %d - 2D drawcalls: %d - buffer vtx data: %d - buffer uniforms: %d",
		         num3D, num2D, numBufVtx, numBufUni);

		GL3_Draw_StringScaled(10, 5, factor, true, stbuf);
		GL3_DrawCurrent2Dbatch();
	}

#if 0
	if(gl3config.useBigVBO)
	{
		// I think this is a good point to orphan the VBO and get a fresh one
		GL3_BindVAO(gl3state.vao3D);
		GL3_BindVBO(gl3state.vbo3D);
		glBufferData(GL_ARRAY_BUFFER, gl3state.vbo3Dsize, NULL, GL_STREAM_DRAW);
		gl3state.vbo3DcurOffset = 0;
	}
#endif

	GL3_SwapWindow();
}

/* draw a fullscreen quad using the existing vao2D/vbo2D */
static void
GL3_DrawFullscreenQuadFromArray(const GLfloat fsQuad[16])
{
	GL3_BindVAO(vao2D);
	GL3_BindVBO(vbo2D);
	glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 16, fsQuad, GL_STREAM_DRAW);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GL3_BindVAO(0);
}

/* Shutdown bloom resources */
void
GL3_BloomShutdown(void)
{
	size_t i;

	for (i = 0; i < BLOOM_TEXTURES; i++)
	{
		if (bloomFBO[i])
		{
			glDeleteFramebuffers(1, &bloomFBO[i]);
			bloomFBO[i] = 0;
		}

		if (bloomTex[i])
		{
			glDeleteTextures(1, &bloomTex[i]);
			bloomTex[i] = 0;
		}
	}

	bloomInitialized = false;
}

/*
 * Apply a simple bloom effect
 *
 * Returns: GLuint of the composite bloom texture.
 * Caller must delete the returned texture when done.
 */
GLuint
GL3_ApplyBloom(GLuint sceneTex, int sceneW, int sceneH)
{
	GLint locTex, locDir;

	if (!r_bloom || !r_bloom->value)
	{
		return 0;
	}

	int w = Q_max(sceneW, 1);
	int h = Q_max(sceneH, 1);

	int downscale = 2;
	int bw = (w / downscale) > 0 ? (w / downscale) : 1;
	int bh = (h / downscale) > 0 ? (h / downscale) : 1;

	/* create FBOs + textures */
	GLuint fboBright = 0, fboPing = 0, fboComp = 0;
	GLuint texBright = 0, texPing = 0, texComp = 0;

	glGenFramebuffers(1, &fboBright);
	glGenFramebuffers(1, &fboPing);
	glGenFramebuffers(1, &fboComp);

	glGenTextures(1, &texBright);
	glGenTextures(1, &texPing);
	glGenTextures(1, &texComp);

	GLenum internalFmt = gl3_tex_solid_format;
	GLenum fmt = GL_RGBA;
	GLenum type = GL_UNSIGNED_BYTE;

	/* texBright */
	GL3_SelectTMU(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texBright);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, bw, bh, 0, fmt, type, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	/* texPing */
	glBindTexture(GL_TEXTURE_2D, texPing);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, bw, bh, 0, fmt, type, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	/* texComp */
	glBindTexture(GL_TEXTURE_2D, texComp);
	glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, w, h, 0, fmt, type, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	/* attach textures to FBOs */
	glBindFramebuffer(GL_FRAMEBUFFER, fboBright);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texBright, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, fboPing);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texPing, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, fboComp);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texComp, 0);

	/* check completeness */
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);

	glBindFramebuffer(GL_FRAMEBUFFER, fboBright);
	glBindFramebuffer(GL_FRAMEBUFFER, fboPing);
	glBindFramebuffer(GL_FRAMEBUFFER, fboComp);

	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		goto fail;
	}

	/* save old viewport */
	glGetIntegerv(GL_VIEWPORT, oldViewPort);

	/* fullscreen quads for downsampled and full sizes (X,Y,S,T) */
	GLfloat fsQuadDown[16] = {
		-1.0f,  1.0f, 0.0f, 1.0f,
		-1.0f, -1.0f, 0.0f, 0.0f,
		1.0f,  1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f, 0.0f
	};
	GLfloat fsQuadFull[16] = {
		0.0f, (GLfloat)h, 0.0f, 1.0f,
		0.0f, 0.0f, 0.0f, 0.0f,
		(GLfloat)w, (GLfloat)h, 1.0f, 1.0f,
		(GLfloat)w, 0.0f, 1.0f, 0.0f
	};

	/* Bright pass */
	glBindFramebuffer(GL_FRAMEBUFFER, fboBright);
	glViewport(0, 0, bw, bh);
	glClear(GL_COLOR_BUFFER_BIT);

	GL3_UseProgram(gl3state.si2DbloomBright.shaderProgram);

	GL3_SelectTMU(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sceneTex);
	locTex = glGetUniformLocation(gl3state.si2DbloomBright.shaderProgram, "tex");

	if (locTex != -1)
	{
		glUniform1i(locTex, 0);
	}

	/* playable default value */
	float threshold = 0.75f;
	GLint locThreshold = glGetUniformLocation(gl3state.si2DbloomBright.shaderProgram, "threshold");
	if (locThreshold != -1)
	{
		glUniform1f(locThreshold, threshold);
	}

	GL3_DrawFullscreenQuadFromArray(fsQuadDown);
	glBindTexture(GL_TEXTURE_2D, 0);

	/* blur pass */
	locTex = glGetUniformLocation(gl3state.si2DbloomBlur.shaderProgram, "tex");
	locDir = glGetUniformLocation(gl3state.si2DbloomBlur.shaderProgram, "dir");

	glBindFramebuffer(GL_FRAMEBUFFER, fboPing);
	glViewport(0, 0, bw, bh);
	glClear(GL_COLOR_BUFFER_BIT);

	GL3_UseProgram(gl3state.si2DbloomBlur.shaderProgram);
	GL3_SelectTMU(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texBright);

	if (locTex != -1)
	{
		glUniform1i(locTex, 0);
	}

	if (locDir != -1)
	{
		glUniform2f(locDir, 1.0f / (float)bw, 0.0f);
	}

	GL3_DrawFullscreenQuadFromArray(fsQuadDown);
	glBindTexture(GL_TEXTURE_2D, 0);

	/* vertical blur */
	glBindFramebuffer(GL_FRAMEBUFFER, fboBright);
	glViewport(0, 0, bw, bh);
	glClear(GL_COLOR_BUFFER_BIT);

	GL3_UseProgram(gl3state.si2DbloomBlur.shaderProgram);
	GL3_SelectTMU(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texPing);

	if (locTex != -1)
	{
		glUniform1i(locTex, 0);
	}

	if (locDir != -1)
	{
		glUniform2f(locDir, 0.0f, 1.0f / (float)bh);
	}

	GL3_DrawFullscreenQuadFromArray(fsQuadDown);
	glBindTexture(GL_TEXTURE_2D, 0);

	/* composite pass*/
	glBindFramebuffer(GL_FRAMEBUFFER, fboComp);
	glViewport(0, 0, w, h);
	glClear(GL_COLOR_BUFFER_BIT);

	/* render base scene */
	glDisable(GL_BLEND);
	GL3_UseProgram(gl3state.si2D.shaderProgram);
	GL3_SelectTMU(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, sceneTex);
	GL3_DrawFullscreenQuadFromArray(fsQuadFull);

	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);

	GL3_UpdateUBOCommon();

	GL3_UseProgram(gl3state.si2Dtinted.shaderProgram);
	GL3_SelectTMU(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texBright);
	GL3_DrawFullscreenQuadFromArray(fsQuadFull);

	/* restore engine blending */
	glDisable(GL_BLEND);
	gl3state.uniCommonData.color = HMM_Vec4(1.0f, 1.0f, 1.0f, 1.0f);
	GL3_UpdateUBOCommon();

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(oldViewPort[0], oldViewPort[1], oldViewPort[2], oldViewPort[3]);

	/* cleanup */
	glDeleteFramebuffers(1, &fboBright);
	glDeleteFramebuffers(1, &fboPing);
	glDeleteFramebuffers(1, &fboComp);

	glDeleteTextures(1, &texPing);
	glDeleteTextures(1, &texBright);

	return texComp;

fail:
	if (fboBright)
	{
		glDeleteFramebuffers(1, &fboBright);
	}

	if (fboPing)
	{
		glDeleteFramebuffers(1, &fboPing);
	}

	if (fboComp)
	{
		glDeleteFramebuffers(1, &fboComp);
	}

	if (texBright)
	{
		glDeleteTextures(1, &texBright);
	}

	if (texPing)
	{
		glDeleteTextures(1, &texPing);
	}

	if (texComp)
	{
		glDeleteTextures(1, &texComp);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	return 0;
}
