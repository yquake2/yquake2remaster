/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_font.c - BFF2 bitmap fonts

#include "r_local.h"

//#define FONT_TEXPTR 1

#define MAX_FONTS 6
#define MAX_DRAWSTRING_LEN 255

int R_FindFont(char* name);
int R_GetFontHeight(int fontId);
int R_GetTextWidth(int fontId, char* text);
void R_DrawText(int x, int y, int alignX, int fontId, float scale, vec4_t color, char* text);

typedef struct font_s
{
	qboolean inuse;
	char	name[MAX_QPATH];
	int		CellX, CellY, YOffset, RowPitch;
	char	Base;
	char	charWidth[256];
	float	RowFactor, ColFactor;
#if FONT_TEXPTR
	image_t* tex; // no need to reference object
#else
	unsigned int texnum;
#endif
} font_t;

static font_t r_fonts[MAX_FONTS];
static int r_numfonts = 0;

/*
===============
R_FontByIndex

Returns font_t by index in r_fonts[] or NULL if index is wrong
===============
*/
static font_t *R_FontByIndex(int index)
{
	if (index < 0 || index >= r_numfonts)
		return NULL;
	if (!r_fonts[index].inuse)
		return NULL;
	return &r_fonts[index];
}

/*
===============
R_FindFont

Returns the index of font or -1 if not found
===============
*/
int R_FindFont(char* name)
{
	for (int i = 0; i < r_numfonts; i++)
	{
		if (!r_fonts[i].inuse)
			continue;
		if (!strcmp(name, r_fonts[i].name))
		{
			return i;
		}
	}
	return -1;
}

/*
===============
R_LoadFontBFF

Loads BFF font file
===============
*/
void R_LoadFontBFF(char* name)
{
	char	fullname[MAX_QPATH];
	byte	*buf, *pixels;
	int		len;
	char	bpp;
	int		picwidth, picheight;
	font_t	*f;

	static const WIDTH_DATA_OFFSET = 20; // Offset to width data with BFF file
	static const MAP_DATA_OFFSET = 276; // Offset to texture image data with BFF file

	if (R_FindFont(name) != -1)
		return;

	Com_sprintf(fullname, sizeof(fullname), "fonts/%s.bff", name);

	len = ri.LoadFile(fullname, &buf);
	if (!buf)
	{
		ri.Error(ERR_DROP, "Font file %s does not exist\n", fullname);
		return;
	}

	// check for 'BFF2' header
	if (buf[0] != 0xBF || buf[1] != 0xF2)
	{
		ri.FreeFile(buf);
		ri.Error(ERR_DROP, "%s is not a font file\n", fullname);
		return;
	}

	if (r_numfonts == MAX_FONTS)
	{
		ri.Error(ERR_DROP, "Too many fonts, only %i allowed!\n", name, MAX_FONTS);
		return;
	}

	f = &r_fonts[r_numfonts];

	// Grab the rest of the header
	memcpy(&picwidth, &buf[2], sizeof(int));
	memcpy(&picheight, &buf[6], sizeof(int));
	memcpy(&f->CellX, &buf[10], sizeof(int));
	memcpy(&f->CellY, &buf[14], sizeof(int));
	bpp = buf[18];
	f->Base = buf[19];

	// Check filesize
	if (len != ((MAP_DATA_OFFSET)+((picwidth * picheight) * (bpp / 8))))
	{
		ri.FreeFile(buf);
		ri.Error(ERR_DROP, "%s has wrong size, file is corrupt.\n", fullname);
		return;
	}

	// calculate font params
	f->RowPitch = picwidth / f->CellX;
	f->ColFactor = (float)f->CellX / (float)picwidth;
	f->RowFactor = (float)f->CellY / (float)picheight;
	f->YOffset = f->CellY;

	if (bpp != 32 && bpp != 24)
	{
		ri.FreeFile(buf);
		ri.Error(ERR_DROP, "Font file %s is not RGB or RGBA format\n", fullname);
		return;
	}

	pixels = malloc((picwidth * picheight) * (bpp / 8));
	if (pixels == NULL)
	{
		ri.Error(ERR_DROP, "Failed to allocate font pixels\n");
		return;
	}

	memcpy(f->charWidth, &buf[WIDTH_DATA_OFFSET], 256); // Grab char widths
	memcpy(pixels, &buf[MAP_DATA_OFFSET], (picwidth * picheight) * (bpp / 8)); 	// Grab image data

#if FONT_TEXPTR
	f->tex = R_LoadTexture( va("$font_%s", name), pixels, picwidth, picheight, it_gui, bpp);
#else
	image_t* img;
	img = R_LoadTexture(va("$font_%s", name), pixels, picwidth, picheight, it_gui, bpp);
	f->texnum = img->texnum;
#endif

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	R_BindTexture(0);

	free(pixels);
	ri.FreeFile(buf);

	strncpy(f->name, name, sizeof(f->name));
	f->inuse = true;
	r_numfonts++;
}

/*
===============
R_GetTextWidthInternal
===============
*/
static int R_GetTextWidthInternal(font_t* f, int strLen, char* text)
{
	int i, width = 0;
	strLen = (int)strnlen(text, MAX_DRAWSTRING_LEN);
	for (i = 0, i = 0; i != strLen; i++)
		width += f->charWidth[text[i]];
	return width;
}

/*
===============
R_GetFontHeight

Returns glyph height in font
===============
*/
int R_GetFontHeight(int fontId)
{
	font_t* f;
	if ((f = R_FontByIndex(fontId)) == NULL)
		return -1;
	return f->CellY;
}

/*
===============
R_GetTextWidth

Returns the width of text or -1 if wrong font
===============
*/
int R_GetTextWidth(int fontId, char* text)
{
	int i, strLen, width = 0;
	font_t* f;

	if ((f = R_FontByIndex(fontId)) == NULL)
		return -1;

	strLen = (int)strnlen(text, MAX_DRAWSTRING_LEN);
	for (i = 0, i = 0; i != strLen; i++)
		width += f->charWidth[text[i]];

	return width;
}

extern vertexbuffer_t vb_gui;
extern int guiVertCount;
extern glvert_t guiVerts[2048];
extern void ClearVertexBuffer();
extern void PushVert(float x, float y, float z);
extern void SetTexCoords(float s, float t);

/*
===============
R_DrawTextInternal
===============
*/
static void R_DrawTextInternal(float x, float y, font_t* f, float scale, vec4_t color, int alignX, char* text, int strLen)
{
	int pos;
	float CurX, CurY;
	int Row, Col;
	float U, V, U1, V1;

	CurX = x;
	CurY = y;

	ClearVertexBuffer();

	for (pos = 0; pos != strLen; pos++)
	{
		//if (text[pos] == '\n' && alignX == 0)
		//{
		//	CurX = x;
		//	CurY += f->CellY; //R_GetFontHeight
		//	continue;
		//}

		Row = (text[pos] - f->Base) / f->RowPitch;
		Col = (text[pos] - f->Base) - Row * f->RowPitch;

		U = Col * f->ColFactor;
		V = Row * f->RowFactor;
		U1 = U + f->ColFactor;
		V1 = V + f->RowFactor;

		PushVert(CurX, CurY, 0);
		SetTexCoords(U, V);

		PushVert(CurX + f->CellX * scale, CurY, 0);
		SetTexCoords(U1, V);

		PushVert(CurX, CurY + f->CellY * scale, 0);
		SetTexCoords(U, V1);

		PushVert(CurX + f->CellX * scale, CurY, 0);
		SetTexCoords(U1, V);

		PushVert(CurX + f->CellX * scale, CurY + f->CellY * scale, 0);
		SetTexCoords(U1, V1);
		PushVert(CurX, CurY + f->CellY * scale, 0);
		SetTexCoords(U, V1);
		CurX += f->charWidth[text[pos]] * scale;
	}

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, V_UV);
	R_ProgUniform4f(LOC_COLOR4, color[0], color[1], color[2], color[3]);
	R_MultiTextureBind(TMU_DIFFUSE, f->texnum);
	R_DrawVertexBuffer(&vb_gui, 0, 0);
}

/*
===============
R_DrawText
===============
*/
void R_DrawText(int x, int y, int alignX, int fontId, float scale, vec4_t color, char* text)
{
	int strLen, CurX, CurY, w;
	font_t* f;

	if (scale <= 0.1f)
		return;

	if ((f = R_FontByIndex(fontId)) == NULL)
		f = R_FontByIndex(0); // it is never null

	strLen = (int)strnlen(text, MAX_DRAWSTRING_LEN);
	if (strLen <= 0)
		return;

	CurX = x;
	CurY = y;

	// align text
	if (alignX > 0)
	{
		w = R_GetTextWidth(fontId, text) * scale;
		if (alignX == 1) // center
			CurX -= (w / 2);
		else if (alignX == 2) // right
			CurX -= w;
	}

	R_Blend(true);
	R_DrawTextInternal(CurX, CurY, f, scale, color, alignX, text, strLen);
	R_Blend(false);
}



#if 0
/*
===============
R_DrawText
===============
*/
void R_DrawText(int x, int y, int fontId, float scale, int alignX, char* text)
{
	int strLen, pos, CurX, CurY;
	int Row, Col, w;
	float U, V, U1, V1;
	font_t* f;

	if (scale <= 0.1f)
		return;

	if ((f = R_FontByIndex(fontId)) == NULL)
		f = R_FontByIndex(0); // it is never null

	strLen = (int)strnlen(text, MAX_DRAWSTRING_LEN);
	if (strLen <= 0)
		return;

	CurX = x;
	CurY = y;

	// align text
	if (alignX > 0)
	{
		w = R_GetTextWidth(fontId, text) * scale;
		if (alignX == 1) // center
			CurX -= (w / 2);
		else if (alignX == 2) // right
			CurX -= w;
	}

	//R_DrawTextInternal(CurX, CurY, fontId, scale, alignX, text);

#ifdef FONT_TEXPTR
	R_BindTexture(f->tex->texnum);
#else
	R_BindTexture(f->texnum);
#endif

	glBegin(GL_TRIANGLES);
	for (pos = 0; pos != strLen; pos++)
	{
		if (text[pos] == '\n' && alignX == 0) // left
		{
			CurX = x;
			CurY += R_GetFontHeight(fontId);
			continue;
		}

		Row = (text[pos] - f->Base) / f->RowPitch;
		Col = (text[pos] - f->Base) - Row * f->RowPitch;

		U = Col * f->ColFactor;
		V = Row * f->RowFactor;
		U1 = U + f->ColFactor;
		V1 = V + f->RowFactor;

		glTexCoord2f(U, V);  glVertex3f(CurX, CurY, 0);
		glTexCoord2f(U1, V);  glVertex3f(CurX + f->CellX * scale, CurY, 0);
		glTexCoord2f(U, V1); glVertex3f(CurX, CurY + f->CellY * scale, 0);

		glTexCoord2f(U1, V);  glVertex3f(CurX + f->CellX * scale, CurY, 0);
		glTexCoord2f(U1, V1); glVertex3f(CurX + f->CellX * scale, CurY + f->CellY * scale, 0);
		glTexCoord2f(U, V1); glVertex3f(CurX, CurY + f->CellY * scale, 0);

		CurX += f->charWidth[text[pos]] * scale;
	}
	glEnd();
}
#endif