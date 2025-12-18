/*
pragma
Copyright (C) 2023-2024 BraXi.

Quake 2 Engine 'Id Tech 2'
Copyright (C) 1997-2001 Id Software, Inc.

See the attached GNU General Public License v2 for more details.
*/

// vertex buffer objects


#include "r_local.h"

#define BUFFER_OFFSET(i) ((char*)NULL + (i))

extern vertexbuffer_t vb_gui;
extern int guiVertCount;
extern glvert_t guiVerts[2048];
extern void ClearVertexBuffer();
extern void PushVert(float x, float y, float z);
extern void SetTexCoords(float s, float t);

#define VBA_DEBUG 1

/*
===============
VertexBufferAttributes

Enable or disable vertex attributes
===============
*/
static qboolean VertexBufferAttributes(vertexbuffer_t* vbo, qboolean enable)
{
	int attrib;

	if (!R_UsingProgram())
	{
#ifdef VBA_DEBUG
		ri.Error(ERR_FATAL, "Tried to render vertexbuffer but not using programs!\n");
#endif
		return false;
	}

	attrib = R_GetProgAttribLoc(VALOC_POS);

#ifdef VBA_DEBUG
	if (attrib == -1)
	{
		ri.Error(ERR_FATAL, "Tried to render vertexbuffer but program %s has no %s attrib\n", R_GetCurrentProgramName(), R_GetProgAttribName(VALOC_POS));
	}
#else
	if (attrib == -1)
		return false;
#endif

	if (enable)
	{
		glEnableVertexAttribArray(attrib);
		glVertexAttribPointer(attrib, 3, GL_FLOAT, GL_FALSE, sizeof(glvert_t), BUFFER_OFFSET(0));

		if (vbo->flags & V_NORMAL)
		{
			attrib = R_GetProgAttribLoc(VALOC_NORMAL);
			if (attrib != -1)
			{
				glEnableVertexAttribArray(attrib);
				glVertexAttribPointer(attrib, 3, GL_FLOAT, GL_FALSE, sizeof(glvert_t), BUFFER_OFFSET(12));
			}
#ifdef VBA_DEBUG
			else
			{
				ri.Error(ERR_FATAL, "Vertex buffer has normals, but program %s has no %s attrib\n", R_GetCurrentProgramName(), R_GetProgAttribName(VALOC_NORMAL));
			}
#endif
		}

		if (vbo->flags & V_UV)
		{
			attrib = R_GetProgAttribLoc(VALOC_TEXCOORD);
			if (attrib != -1)
			{
				glEnableVertexAttribArray(attrib);
				glVertexAttribPointer(attrib, 2, GL_FLOAT, GL_FALSE, sizeof(glvert_t), BUFFER_OFFSET(24));
			}
#ifdef VBA_DEBUG
			else
			{
				ri.Error(ERR_FATAL, "Vertex buffer has texcoords, but program %s has no %s attrib\n", R_GetCurrentProgramName(), R_GetProgAttribName(VALOC_TEXCOORD));
			}
#endif
		}

		if (vbo->flags & V_COLOR)
		{
			attrib = R_GetProgAttribLoc(VALOC_COLOR);
			if (attrib != -1)
			{
				glEnableVertexAttribArray(attrib);
				//glBindAttribLocation(pCurrentProgram->programObject, attrib, "inColor");
				glVertexAttribPointer(attrib, 4, GL_FLOAT, GL_FALSE, sizeof(glvert_t), BUFFER_OFFSET(32));
			}
#ifdef VBA_DEBUG
			else
			{
				ri.Error(ERR_FATAL, "Vertex buffer has colors, but program %s has no %s attrib\n", R_GetCurrentProgramName(), R_GetProgAttribName(VALOC_COLOR));
			}
#endif
		}
	}
	else
	{
		attrib = R_GetProgAttribLoc(VALOC_POS);
		glDisableVertexAttribArray(attrib);

		if (vbo->flags & V_NORMAL)
		{
			attrib = R_GetProgAttribLoc(VALOC_NORMAL);
			if (attrib != -1)
				glDisableVertexAttribArray(attrib);
		}

		if (vbo->flags & V_UV)
		{
			attrib = R_GetProgAttribLoc(VALOC_TEXCOORD);
			if (attrib != -1)
				glDisableVertexAttribArray(attrib);
		}

		if (vbo->flags & V_COLOR)
		{
			attrib = R_GetProgAttribLoc(VALOC_COLOR);
			if (attrib != -1)
				glDisableVertexAttribArray(attrib);
		}
	}
	return true;
}

/*
===============
R_AllocVertexBuffer

Create vertex buffer object and allocate space for verticles
===============
*/
vertexbuffer_t* R_AllocVertexBuffer(vboFlags_t flags, unsigned int numVerts, unsigned int numIndices)
{
	vertexbuffer_t* vbo = NULL;

	vbo = ri.MemAlloc(sizeof(vertexbuffer_t));
	if (!vbo)
	{
		ri.Error(ERR_FATAL, "R_AllocVertexBuffer failed\n");
		return NULL;
	}

	vbo->flags = flags;

	glGenBuffers(1, &vbo->vboBuf);

//	glBindBuffer(GL_ARRAY_BUFFER, vbo->vboBuf);
//	glBufferData(GL_ARRAY_BUFFER, (numVerts * sizeof(glvert_t)), &vbo->verts->xyz[0], GL_STATIC_DRAW);
//	glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (numVerts)
	{
		vbo->verts = ri.MemAlloc(sizeof(glvert_t) * numVerts);
		if (!vbo->verts)
		{
			ri.Error(ERR_FATAL, "R_AllocVertexBuffer failed to allocate %i vertices\n", numVerts);
			return NULL;
		}
		vbo->numVerts = numVerts;
	}

	if ((flags & V_INDICES) && numIndices > 0)
	{
		glGenBuffers(1, &vbo->indexBuf);
		vbo->indices = ri.MemAlloc(sizeof(unsigned short) * numIndices);
		{
			ri.Error(ERR_FATAL, "R_AllocVertexBuffer failed to allocate %i indices\n", numIndices);
			return NULL;
		}

		vbo->numIndices = numIndices;
//		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo->indexBuf);
//		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned short) * 3, vbo->indices, GL_STATIC_DRAW);
	}

	return vbo;
}

/*
===============
R_UpdateVertexBuffer
===============
*/
void R_UpdateVertexBuffer(vertexbuffer_t* vbo, glvert_t* verts, unsigned int numVerts, vboFlags_t flags)
{
	if (vbo->vboBuf == 0)
		glGenBuffers(1, &vbo->vboBuf);

	vbo->numVerts = numVerts;
	vbo->flags = flags;

	glBindBuffer(GL_ARRAY_BUFFER, vbo->vboBuf);
	if (vbo->verts != NULL && vbo->numVerts != numVerts && verts != NULL)
	{
		ri.Error(ERR_FATAL, "R_UpdateVertexBuffer probably scrapping allocated verts\n", numVerts);
	}
	if(verts != NULL)
		glBufferData(GL_ARRAY_BUFFER, (numVerts * sizeof(glvert_t)), &verts->xyz[0], GL_STATIC_DRAW);
	else
		glBufferData(GL_ARRAY_BUFFER, (numVerts * sizeof(glvert_t)), &vbo->verts->xyz[0], GL_STATIC_DRAW);

	if (!(vbo->flags & V_NOFREE) && vbo->verts != NULL)
	{
		ri.MemFree(vbo->verts);
		vbo->verts = NULL;
	}
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

/*
===============
R_UpdateVertexBufferIndices
===============
*/
void R_UpdateVertexBufferIndices(vertexbuffer_t* vbo, unsigned short* indices, unsigned int numIndices)
{
	if (vbo->indexBuf == 0)
		glGenBuffers(1, &vbo->indexBuf);

	if (vbo->indices != NULL && vbo->numIndices != numIndices)
	{
		ri.Error(ERR_FATAL, "R_UpdateVertexBufferIndices probably scrapping allocated indices\n", numIndices);
	}

	vbo->numIndices = numIndices;

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo->indexBuf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, (numIndices * sizeof(unsigned short)), indices, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}


/*
===============
R_DrawVertexBuffer
===============
*/
void R_DrawVertexBuffer(vertexbuffer_t* vbo, unsigned int startVert, unsigned int numVerts)
{
	glBindBuffer(GL_ARRAY_BUFFER, vbo->vboBuf);

	VertexBufferAttributes(vbo, true);

	// case one: we have index buffer
	if (vbo->numIndices && vbo->indices != NULL && vbo->indexBuf) //if ((vbo->flags & V_INDICES) && vbo->indexBuf)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vbo->indexBuf);
		
		//glDrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const void *indices);
		if(numVerts)
			glDrawRangeElements(GL_TRIANGLES, startVert, startVert + numVerts, numVerts, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));
		else
			glDrawRangeElements(GL_TRIANGLES, 0, vbo->numVerts, vbo->numVerts, GL_UNSIGNED_SHORT, BUFFER_OFFSET(0));

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
	else // case two: we don't have index buffer and for simple primitives like gui we usualy don't
	{
		if (!numVerts)
			glDrawArrays(GL_TRIANGLES, 0, vbo->numVerts);
		else
			glDrawArrays(GL_TRIANGLES, startVert, numVerts);
	}

	VertexBufferAttributes(vbo, false);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}


/*
===============
R_DeleteVertexBuffers
===============
*/
void R_DeleteVertexBuffers(vertexbuffer_t* vbo)
{
	if (!glDeleteBuffers)
		return; // no gl context

	if (vbo->vboBuf)
		glDeleteBuffers(1, &vbo->vboBuf);

	if (vbo->indexBuf)
		glDeleteBuffers(1, &vbo->indexBuf);
}

/*
===============
R_FreeVertexBuffer

this does FREE vertexbuffer
===============
*/
void R_FreeVertexBuffer(vertexbuffer_t* vbo)
{
	if (!vbo || vbo == NULL)
		return;

	R_DeleteVertexBuffers(vbo);

	if (vbo->indices)
	{
		ri.MemFree(vbo->indices);
		vbo->indices = NULL;
	}

	if (vbo->verts)
	{
		ri.MemFree(vbo->verts);
		vbo->verts = NULL;
	}

	ri.MemFree(vbo);
	vbo = NULL;
//	memset(vbo, 0, sizeof(*vbo));
}