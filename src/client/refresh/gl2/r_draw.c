/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// r_draw.c

#include "r_local.h"

typedef enum
{
	XALIGN_LEFT = 0,
	XALIGN_RIGHT = 1,
	XALIGN_CENTER = 2
} UI_AlignX;

vertexbuffer_t vb_gui;
int guiVertCount;
glvert_t guiVerts[2048];


void ClearVertexBuffer()
{
	guiVertCount = 0;
}

void PushVert(float x, float y, float z)
{
	VectorSet(guiVerts[guiVertCount].xyz, x, y, z);
	guiVertCount++;
}

void SetTexCoords(float s, float t)
{
	guiVerts[guiVertCount - 1].st[0] = s;
	guiVerts[guiVertCount - 1].st[1] = t;
}

void SetNormal(float x, float y, float z)
{
	VectorSet(guiVerts[guiVertCount - 1].normal, x, y, z);
}

void SetColor(float r, float g, float b, float a)
{
	Vector4Set(guiVerts[guiVertCount - 1].rgba, r, g, b, a);
}


/*
==============================================================================
 fonts
==============================================================================
*/
typedef struct
{
	char		name[MAX_QPATH];
	image_t		*image;
	qboolean	filtered;
} rfont_t;

static rfont_t *fonts[2];
rfont_t* font_current;

rfont_t *R_LoadFont( const char *name, qboolean filtered )
{
	char	fullname[MAX_QPATH];
	rfont_t* f;

	f = ri.MemAlloc(sizeof(rfont_t));
	if (!f)
	{
		ri.Error(ERR_FATAL, "Cannot allocate font %s\n", name);
		return NULL;
	}

	Com_sprintf(fullname, sizeof(fullname), "gfx/fonts/%s.tga", name);

	f->image = R_FindTexture(fullname, it_gui, true);
	if (f->image == NULL || f->image == r_texture_missing)
	{
		ri.Error(ERR_FATAL, "Missing image %s for font %s\n", fullname, name);
		return NULL;
	}

	strcpy(f->name, name);
	f->filtered = filtered;

	R_MultiTextureBind(TMU_DIFFUSE, f->image->texnum);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	if (filtered)
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else
	{
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}
	R_MultiTextureBind(TMU_DIFFUSE, 0);
	return f;

}

/*
===============
R_LoadFonts
===============
*/
extern qboolean R_LoadFontBFF(char* name);
void R_LoadFonts()
{
	fonts[0] = R_LoadFont("default", true);
	font_current = fonts[0];

	R_LoadFontBFF("arial");
}



/*
================
R_DrawSingleChar

Draws one 8*8 graphics character with 0 being transparent.
It can be clipped to the top of the screen to allow the console to be
smoothly scrolled off.
================
*/
void R_DrawSingleChar(int x, int y, int num, int charSize)
{
	int				row, col;
	float			frow, fcol, size;

	num &= 255;
	
	if ( (num&127) == 32 )
		return;		// space

	if (y <= -charSize)
		return;			// totally off screen

	row = num>>4;
	col = num>>4;

	frow = row*0.0625;
	fcol = col*0.0625;
	size = 0.0625;

	ClearVertexBuffer();
	PushVert(x, y, 0);
	SetTexCoords(fcol, frow);
	PushVert(x + charSize, y, 0);
	SetTexCoords(fcol + size, frow);
	PushVert(x + charSize, y + charSize, 0);
	SetTexCoords(fcol + size, frow + size);
	PushVert(x, y, 0);
	SetTexCoords(fcol, frow);
	PushVert(x + charSize, y + charSize, 0);
	SetTexCoords(fcol + size, frow + size);
	PushVert(x, y + charSize, 0);
	SetTexCoords(fcol, frow + size);

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, V_UV);
	R_MultiTextureBind(TMU_DIFFUSE, font_current->image->texnum);
	R_DrawVertexBuffer(&vb_gui, 0, 0);
}

/*
=============
Draw_FindPic
=============
*/
image_t	*R_RegisterPic(char *name)
{
	image_t *image;
	char	fullname[MAX_QPATH];

	if (name[0] != '/' && name[0] != '\\')
	{
		Com_sprintf (fullname, sizeof(fullname), "gfx/%s.tga", name);
		image = R_FindTexture (fullname, it_gui, true);
	}
	else
		image = R_FindTexture (name+1, it_gui, true);

	return image;
}

/*
=============
R_GetImageSize
=============
*/
void R_GetImageSize (int *w, int *h, char *name)
{
	image_t *image;

	//image = R_RegisterPic (pic);
	image = R_FindTexture (name, it_gui, false);
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
R_DrawStretchImage

Draw stretched image
=============
*/
void R_DrawStretchImage (int x, int y, int w, int h, char * name)
{
	image_t *image;

	image = R_RegisterPic (name);
	//image = R_FindTexture(name, it_gui, false);
	if (!image)
	{
		ri.Printf(PRINT_ALL, "Image not precached: %s\n", name);
		return;
	}

	ClearVertexBuffer();
	PushVert(x, y, 0);
	SetTexCoords(image->sl, image->tl);
	PushVert(x + w, y, 0);
	SetTexCoords(image->sh, image->tl);
	PushVert(x + w, y + h, 0);
	SetTexCoords(image->sh, image->th);
	PushVert(x, y, 0);
	SetTexCoords(image->sl, image->tl);
	PushVert(x + w, y + h, 0);
	SetTexCoords(image->sh, image->th);
	PushVert(x, y + h, 0);
	SetTexCoords(image->sl, image->th);

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, V_UV);
	R_MultiTextureBind(TMU_DIFFUSE, image->texnum);
	R_Blend(true);
	R_DrawVertexBuffer(&vb_gui, 0, 0);
	R_Blend(false);
}


/*
=============
R_DrawImage

Draw image at given coords
=============
*/
void R_DrawImage(int x, int y, char *name)
{
	image_t *image;

	//image = R_RegisterPic (pic);
	image = R_FindTexture(name, it_gui, false);
	if (!image)
	{
		ri.Printf(PRINT_ALL, "Image not precached: %s\n", name);
		return;
	}

	ClearVertexBuffer();
	PushVert(x, y, 0);
	SetTexCoords(image->sl, image->tl);
	PushVert(x + image->width, y, 0);
	SetTexCoords(image->sh, image->tl);
	PushVert(x + image->width, y + image->height, 0);
	SetTexCoords(image->sh, image->th);	
	PushVert(x, y, 0);
	SetTexCoords(image->sl, image->tl);
	PushVert(x + image->width, y + image->height, 0);
	SetTexCoords(image->sh, image->th);
	PushVert(x, y + image->height, 0);
	SetTexCoords(image->sl, image->th);

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, V_UV);
	R_MultiTextureBind(TMU_DIFFUSE, image->texnum);
	R_DrawVertexBuffer(&vb_gui, 0, 0);
}

/*
=============
R_DrawTileClear

This repeats a 64*64 tile graphic to fill the screen around a sized down refresh window.
=============
*/
void R_DrawTileClear (int x, int y, int w, int h, char *name)
{
	image_t	*image;

	//image = R_RegisterPic (pic);
	image = R_FindTexture(name, it_gui, false);
	if (!image)
	{
		ri.Printf(PRINT_ALL, "Image not precached: %s\n", name);
		return;
	}

	ClearVertexBuffer();
	PushVert(x, y, 0);
	SetTexCoords(x / 64.0f, y / 64.0f);	
	PushVert(x + w, y, 0);
	SetTexCoords((x + w) / 64.0f, y / 64.0f);
	PushVert(x + w, y + h, 0);
	SetTexCoords((x + w) / 64.0f, (y + h) / 64.0f);	
	PushVert(x, y, 0);
	SetTexCoords(x / 64.0f, y / 64.0f);
	PushVert(x + w, y + h, 0);
	SetTexCoords((x + w) / 64.0f, (y + h) / 64.0f);
	PushVert(x, y + h, 0);
	SetTexCoords(x / 64.0f, (y + h) / 64.0f);

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, V_UV);
	R_MultiTextureBind(TMU_DIFFUSE, image->texnum);
	R_DrawVertexBuffer(&vb_gui, 0, 0);
}


/*
=============
R_DrawFill

Fills a box of pixels with a single color
=============
*/
void R_DrawFill(int x, int y, int w, int h)
{
	ClearVertexBuffer();
	PushVert(x, y, 0);
	PushVert(x + w, y, 0);
	PushVert(x + w, y + h, 0);
	PushVert(x, y, 0);
	PushVert(x + w, y + h, 0);
	PushVert(x, y + h, 0);

	R_Blend(true);

	R_MultiTextureBind(TMU_DIFFUSE, r_texture_white->texnum);
//	R_ProgUniform4f(LOC_COLOR4, color[0], color[1], color[2], color[3]);

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, 0);
	R_DrawVertexBuffer(&vb_gui, 0, 0);

	R_Blend(false);
}

void R_AdjustToVirtualScreenSize(float* x, float* y)
{
	float    xscale;
	float    yscale;

	xscale = vid.width / 800.0;
	yscale = vid.height / 600.0;

	if (x) *x *= xscale;
	if (y) *y *= yscale;
}

/*
===============
AddCharToString
===============
*/
static void AddCharToString(float x, float y, float w, float h, int num)
{
	int				row, col;
	float			frow, fcol, size;

	num &= 255;

	if ((num & 127) == 32)
		return;		// space

	if (y <= -8)
		return;			// totally off screen

	row = num >> 4;
	col = num & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

	PushVert(x, y, 0);
	SetTexCoords(fcol, frow);

	PushVert(x + w, y, 0);
	SetTexCoords(fcol + size, frow);

	PushVert(x + w, y + h, 0);
	SetTexCoords(fcol + size, frow + size);

	PushVert(x, y, 0);
	SetTexCoords(fcol, frow);

	PushVert(x + w, y + h, 0);
	SetTexCoords(fcol + size, frow + size);

	PushVert(x, y + h, 0);
	SetTexCoords(fcol, frow + size);
}

/*
===============
R_DrawStringOld

DEPRECATED
===============
*/
void R_DrawString(float x, float y, int alignx, int charSize, int fontId, vec4_t color, const char* str)
{
	int ofs_x = 0;

	if (!str)
		return;

	// align text
	int strX = (strlen(str) * charSize);
	if (alignx == XALIGN_CENTER)
		ofs_x -= (strX / 2);
	else if (alignx == XALIGN_RIGHT)
		ofs_x -= ((strlen(str) * charSize));

	ClearVertexBuffer();
	while (*str)
	{
		AddCharToString(ofs_x + x, y, charSize, charSize, *str);
		x += charSize;
		str++;
	}

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, V_UV);
	R_ProgUniform4f(LOC_COLOR4, color[0], color[1], color[2], color[3]);
	R_MultiTextureBind(TMU_DIFFUSE, fonts[fontId]->image->texnum);
	R_DrawVertexBuffer(&vb_gui, 0, 0);
}




static void AddCharToStringOld(float x, float y, float w, float h, int num)
{
	int				row, col;
	float			frow, fcol, size;

	num &= 255;

	if ((num & 127) == 32)
		return;		// space

	if (y <= -8)
		return;			// totally off screen

	row = num >> 4;
	col = num & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

	PushVert(x, y, 0);
	SetTexCoords(fcol, frow);

	PushVert(x + w, y, 0);
	SetTexCoords(fcol + size, frow);

	PushVert(x + w, y + h, 0);
	SetTexCoords(fcol + size, frow + size);

	PushVert(x, y, 0);
	SetTexCoords(fcol, frow);

	PushVert(x + w, y + h, 0);
	SetTexCoords(fcol + size, frow + size);

	PushVert(x, y + h, 0);
	SetTexCoords(fcol, frow + size);
}

/*
===============
R_DrawStringOld

DEPRECATED
===============
*/
void R_DrawStringOld(char* string, float x, float y, float fontSize, int alignx, rgba_t color)
{
	float CHAR_SIZEX = 8 * fontSize;
	float CHAR_SIZEY = 8 * fontSize;
	int ofs_x = 0;

	if (!string)
		return;

	R_AdjustToVirtualScreenSize(&x, &y); // pos
	R_AdjustToVirtualScreenSize(&CHAR_SIZEX, &CHAR_SIZEY); // pos

	CHAR_SIZEX = CHAR_SIZEY = (int)CHAR_SIZEX; // hack so text isnt blurry

	// align text
	int strX = (strlen(string) * CHAR_SIZEX);
	if (alignx == XALIGN_CENTER)
		ofs_x -= (strX / 2);
	else if (alignx == XALIGN_RIGHT)
		ofs_x -= ((strlen(string) * CHAR_SIZEX));

	ClearVertexBuffer();
	while (*string)
	{
		AddCharToStringOld(ofs_x + x, y, CHAR_SIZEX, CHAR_SIZEY, *string);
		x += CHAR_SIZEX;
		string++;
	}

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, V_UV);
	R_ProgUniform4f(LOC_COLOR4, color[0], color[1], color[2], color[3]);
	R_MultiTextureBind(TMU_DIFFUSE, font_current->image->texnum);
	R_DrawVertexBuffer(&vb_gui, 0, 0);
}


/*
===============
AddCharTo3DString
===============
*/
void AddCharTo3DString(float x, float y, float z, float fontSize, int num)
{
	int row, col;
	float frow, fcol, size;

	num &= 255;

	if ((num & 127) == 32)
		return; // space

	row = num >> 4;
	col = num & 15;

	frow = row * 0.0625;
	fcol = col * 0.0625;
	size = 0.0625;

	PushVert(x + (-fontSize / 2), -fontSize / 2, 0);
	SetTexCoords(fcol, frow);

	PushVert(x + (fontSize / 2), -fontSize / 2, 0);
	SetTexCoords(fcol + size, frow);

	PushVert(x + (fontSize / 2), fontSize / 2, 0);
	SetTexCoords(fcol + size, frow + size);

	PushVert(x + (-fontSize / 2), -fontSize / 2, 0);
	SetTexCoords(fcol, frow);

	PushVert(x + (fontSize / 2), fontSize / 2, 0);
	SetTexCoords(fcol + size, frow + size);

	PushVert(x + (-fontSize / 2), fontSize / 2, 0);
	SetTexCoords(fcol, frow + size);
}

/*
===============
R_DrawString3D
Draw text in 3D space always oriented towards camera
===============
*/
void R_DrawString3D(char* string, vec3_t pos, float fontSize, int alignx, vec3_t color)
{
	// this does support more than the code uses, leaving for future use
	float CHAR_SIZEX = 8 * fontSize;
	float CHAR_SIZEY = 8 * fontSize;
	int ofs_x = 0;

	if (!string || !string[0] | !strlen(string))
		return;


	alignx = XALIGN_CENTER;
	// align text
	int strX = (strlen(string) * CHAR_SIZEX);
	if (alignx == XALIGN_CENTER)
		ofs_x -= (strX / 2);
	else if (alignx == XALIGN_RIGHT)
		ofs_x -= ((strlen(string) * CHAR_SIZEX));

	mat4_t mat;
	Mat4MakeIdentity(mat);
	Mat4Translate(mat, pos[0], pos[1], pos[2]);
	Mat4RotateAroundX(mat, -90);
	Mat4RotateAroundY(mat, (-r_newrefdef.view.angles[1]) + 90); //rotate to always face the camera
	Mat4RotateAroundX(mat, -r_newrefdef.view.angles[0]);

	// create vertex buffer for string
	ClearVertexBuffer();
	while (*string)
	{
		AddCharTo3DString(ofs_x, pos[1], pos[2], CHAR_SIZEX, *string);
		ofs_x += CHAR_SIZEX;
		string++;
	}

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, V_UV);

	R_BindProgram(GLPROG_DEBUGSTRING);
	R_ProgUniform4f(LOC_COLOR4, color[0], color[1], color[2], 1.0);
	R_ProgUniformMatrix4fv(LOC_LOCALMODELVIEW, 1, mat);

	R_MultiTextureBind(TMU_DIFFUSE, font_current->image->texnum);
	R_DrawVertexBuffer(&vb_gui, 0, 0);

	R_UnbindProgram();
}


/*
===============
R_DrawStretchedImage
DEPRECATED
===============
*/
void R_DrawStretchedImage(rect_t pos, rgba_t color, char* pic)
{
	image_t* gl;

	gl = R_RegisterPic(pic);
	if (!gl)
	{
		ri.Printf(PRINT_ALL, "R_DrawStretchedImage: no %s\n", pic);
		return;
	}

	rect_t rect;

	rect[0] = pos[0];
	rect[1] = pos[1];
	rect[2] = pos[2];
	rect[3] = pos[3];

//	R_AdjustToVirtualScreenSize(&rect[0], &rect[1]); // pos
//	R_AdjustToVirtualScreenSize(&rect[2], &rect[3]); // w/h

	ClearVertexBuffer();

	PushVert(rect[0], rect[1], 0);
	SetTexCoords(gl->sl, gl->tl);
	PushVert(rect[0] + rect[2], rect[1], 0);
	SetTexCoords(gl->sh, gl->tl);
	PushVert(rect[0] + rect[2], rect[1] + rect[3], 0);
	SetTexCoords(gl->sh, gl->th);
	PushVert(rect[0], rect[1], 0);
	SetTexCoords(gl->sl, gl->tl);
	PushVert(rect[0] + rect[2], rect[1] + rect[3], 0);
	SetTexCoords(gl->sh, gl->th);
	PushVert(rect[0], rect[1] + rect[3], 0);
	SetTexCoords(gl->sl, gl->th);

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, V_UV);

	R_Blend(true);
	R_MultiTextureBind(TMU_DIFFUSE, gl->texnum);
	R_ProgUniform4f(LOC_COLOR4, color[0], color[1], color[2], color[3]);
	R_DrawVertexBuffer(&vb_gui, 0, 0);
	R_Blend(false);
}

/*
===============
R_NewDrawFill
DEPRECATED
===============
*/
void R_NewDrawFill(rect_t pos, rgba_t color)
{
	rect_t rect;
	rect[0] = pos[0];
	rect[1] = pos[1];
	rect[2] = pos[2];
	rect[3] = pos[3];

//	R_AdjustToVirtualScreenSize(&rect[0], &rect[1]); // pos
//	R_AdjustToVirtualScreenSize(&rect[2], &rect[3]); // w/h

	ClearVertexBuffer();
	PushVert(rect[0], rect[1], 0);
	PushVert(rect[0] + rect[2], rect[1], 0);
	PushVert(rect[0] + rect[2], rect[1] + rect[3], 0);
	PushVert(rect[0], rect[1], 0);
	PushVert(rect[0] + rect[2], rect[1] + rect[3], 0);
	PushVert(rect[0], rect[1] + rect[3], 0);

	R_Blend(true);

	R_MultiTextureBind(TMU_DIFFUSE, r_texture_white->texnum);
	R_ProgUniform4f(LOC_COLOR4, color[0], color[1], color[2], color[3]);

	R_UpdateVertexBuffer(&vb_gui, guiVerts, guiVertCount, 0);
	R_DrawVertexBuffer(&vb_gui, 0, 0);

	R_Blend(false);
}