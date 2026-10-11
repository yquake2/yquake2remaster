#include "rendergl.h"
#define STB_TRUETYPE_IMPLEMENTATION  // force following include to generate implementation
#include "../client/refresh/files/stb_truetype.h"

unsigned char ttf_buffer[1<<20];
unsigned char temp_bitmap[1024*1024];
cvar_t *r_skeletalanimation;

static GLuint skins[16];
stbtt_bakedchar cdata[0x500]; // ASCII 32..126 is 95 glyphs
GLuint ftex;

void
my_stbtt_initfont(void)
{
	fread(ttf_buffer, 1, 1<<20, fopen("/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf", "rb"));
	stbtt_BakeFontBitmap(ttf_buffer, 0, 16.0, temp_bitmap, 1024, 1024, 32, 0x500, cdata); // no guarantee this fits!

	// can free ttf_buffer at this point
	glGenTextures(1, &ftex);
	glBindTexture(GL_TEXTURE_2D, ftex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, 1024, 1024, 0, GL_ALPHA, GL_UNSIGNED_BYTE, temp_bitmap);
	// can free temp_bitmap at this point
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

static int
TT_get_utf8(char **curr)
{
	unsigned value = 0, size = 0, i;

	value = **curr;
	if (!(value & 0x80))
	{
		size = 1;
	}
	else if ((value & 0xE0) == 0xC0)
	{
		size = 2;
		value = (value & 0x1F) << 6;
	}
	else if ((value & 0xF0) == 0xE0)
	{
		size = 3;
		value = (value & 0x0F) << 12;
	}
	else if ((value & 0xF8) == 0xF0)
	{
		size = 4;
		value = (value & 0x07) << 18;
	}

	(*curr) ++;
	size --;

	for (i = 0; (i < size); i++)
	{
		int c;

		c = **curr;
		if ((c & 0xC0) != 0x80)
		{
			break;
		}
		value |= (c & 0x3F) << ((size - i - 1) * 6);
		(*curr) ++;
	}

	return value;
}

void
my_stbtt_print(float x, float y, char *curr)
{
	// assume orthographic projection with units = screen pixels, origin at top left
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, ftex);
	glBegin(GL_QUADS);

	while(*curr)
	{
		unsigned value = TT_get_utf8(&curr);

		if (value >= 32 && value < 0x500) {
			stbtt_aligned_quad q;
			stbtt_GetBakedQuad(cdata, 1024, 1024, value - 32, &x, &y, &q, 1);//1=opengl & d3d10+,0=d3d9
			glTexCoord2f(q.s0, q.t0); glVertex2f(q.x0, q.y0);
			glTexCoord2f(q.s1, q.t0); glVertex2f(q.x1, q.y0);
			glTexCoord2f(q.s1, q.t1); glVertex2f(q.x1, q.y1);
			glTexCoord2f(q.s0, q.t1); glVertex2f(q.x0, q.y1);
		}
	}
	glEnd();
}

void
upload_texture(GLubyte *pixels, int width, int height)
{
	GLuint id;

	if (skin_count >= 16)
	{
		return;
	}

	/* Generate OpenGL texture */
	glGenTextures (1, &id);
	glBindTexture (GL_TEXTURE_2D, id);

	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_FALSE);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width,
			height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
			pixels);

	skins[skin_count] = id;

	skin_count ++;
}

/* Palette */
static unsigned char colormap[256][3] = {
#include "colormap.h"
};

void
SCR_LoadImageWithPalette(const char *filename, byte **pic, byte **palette,
	int *width, int *height, int *bitsPerPixel);
void
VID_ImageInit(void);

byte *
image_load(const char* name, int *w, int *h, int *b)
{
	int bytesPerPixel, i;
	byte *pic = NULL, *pic_decoded, *palette = NULL;

	SCR_LoadImageWithPalette(name, &pic, &palette, w, h, &bytesPerPixel);

	if (bytesPerPixel == 32 || !pic)
	{
		return pic;
	}

	if (!palette)
	{
		palette = malloc(768);
		memcpy(palette, colormap, 768);
	}

	pic_decoded = malloc((*w) * (*h) * 4);

	/* Convert indexed 8 bits texture to RGB 24 bits */
	for (i = 0; i < (*w) * (*h); ++i)
	{
		pic_decoded[(i * 4) + 0] = palette[pic[i] * 3 + 0];
		pic_decoded[(i * 4) + 1] = palette[pic[i] * 3 + 1];
		pic_decoded[(i * 4) + 2] = palette[pic[i] * 3 + 2];
		pic_decoded[(i * 4) + 2] = 0;
	}
	free(pic);
	free(palette);

	return pic_decoded;
}

struct image_s *
insert_skin(const char *name, byte *data, int width, int realwidth,
	int height, int realheight, size_t size, imagetype_t type, int bits)
{
	int i;

	GLubyte *pixels = (GLubyte *)malloc(width * height * 4);

	/* Convert indexed 8 bits texture to RGB 24 bits */
	for (i = 0; i < width * height; ++i)
	{
		pixels[(i * 4) + 0] = colormap[data[i]][0];
		pixels[(i * 4) + 1] = colormap[data[i]][1];
		pixels[(i * 4) + 2] = colormap[data[i]][2];
		pixels[(i * 4) + 2] = 0;
	}

	upload_texture(pixels, width, height);

	/* OpenGL has its own copy of image data */
	free(pixels);

	return NULL;
}

static void
RenderPacket(float interp, mem_glcmd_t *packet,
	const daliasxframe_t *pframe1, const daliasxframe_t *pframe2)
{
	vec3_t v_curr, v_next, v, norm, n_curr, n_next;
	const dxtrivertx_t *pvert1, *pvert2;
	int i;

	pvert1 = &pframe1->verts[packet->index];
	pvert2 = &pframe2->verts[packet->index];

	/* Pass texture coordinates to OpenGL */
	glTexCoord2f (packet->s, packet->t);

	/* Interpolate normals */
	for (i = 0; i < 3; i++)
	{
		n_curr[i] = pvert1->normal[i] / 127.f;
		n_next[i] = pvert2->normal[i] / 127.f;
	}

	norm[0] = n_curr[0] + interp * (n_next[0] - n_curr[0]);
	norm[1] = n_curr[1] + interp * (n_next[1] - n_curr[1]);
	norm[2] = n_curr[2] + interp * (n_next[2] - n_curr[2]);

	glNormal3fv (norm);

	/* Interpolate vertices */
	v_curr[0] = pframe1->scale[0] * pvert1->v[0] + pframe1->translate[0];
	v_curr[1] = pframe1->scale[1] * pvert1->v[1] + pframe1->translate[1];
	v_curr[2] = pframe1->scale[2] * pvert1->v[2] + pframe1->translate[2];

	v_next[0] = pframe2->scale[0] * pvert2->v[0] + pframe2->translate[0];
	v_next[1] = pframe2->scale[1] * pvert2->v[1] + pframe2->translate[1];
	v_next[2] = pframe2->scale[2] * pvert2->v[2] + pframe2->translate[2];

	v[0] = v_curr[0] + interp * (v_next[0] - v_curr[0]);
	v[1] = v_curr[1] + interp * (v_next[1] - v_curr[1]);
	v[2] = v_curr[2] + interp * (v_next[2] - v_curr[2]);

	glVertex3fv(v);
}

static void
RenderPacketWithFrame(const dmdx_t *mod, int n, float interp, int *pglcmds)
{
	int i;
	int num_frames = mod->num_frames;

	/* Draw the model */
	while ((i = *(pglcmds++)) != 0)
	{
		if (i < 0)
		{
			glBegin(GL_TRIANGLE_FAN);
			i = -i;
		}
		else
		{
			glBegin(GL_TRIANGLE_STRIP);
		}

		/* Draw each vertex of this group */
		for (/* Nothing */; i > 0; --i, pglcmds += 3)
		{
			mem_glcmd_t *packet;
			const daliasxframe_t *pframe1, *pframe2;
			const byte *pframes;

			pframes = (byte*)mod + mod->ofs_frames;
			packet = (mem_glcmd_t *)pglcmds;
			pframe1 = (const daliasxframe_t*)(pframes + mod->framesize * n);
			pframe2 = (const daliasxframe_t*)(pframes + mod->framesize * ((n + 1) % num_frames));

			RenderPacket(interp, packet, pframe1, pframe2);
		}

		glEnd ();
	}
}

/**
 * Free resources allocated for the model.
 */
static void
FreeModel(dmdx_t *mod)
{
	Hunk_Free(mod);

	if (skin_count)
	{
		/* Delete OpenGL textures */
		glDeleteTextures(skin_count, skins);
	}
}

/**
 * Render the model with interpolation between frame n and n+1
 * using model's GL command list.
 * interp is the interpolation percent. (from 0.0 to 1.0)
 */
void
RenderFrameItp(int n, float interp, const dmdx_t *mod)
{
	int num_mesh_nodes, i;
	dmdxmesh_t *mesh_nodes;

	n += mod->num_frames;
	n %= mod->num_frames;

	/* Enable model's texture */
	if (skin_count)
	{
		glBindTexture(GL_TEXTURE_2D, skins[iskin % skin_count]);
	}

	irender %= 2;

	num_mesh_nodes = mod->num_meshes;
	mesh_nodes = (dmdxmesh_t *)((char*)mod + mod->ofs_meshes);

	imesh = (imesh + num_mesh_nodes + 1) % (num_mesh_nodes + 1);
	for (i = 0; i < num_mesh_nodes; i++)
	{
		if (imesh == 0 || (i == (num_mesh_nodes - imesh)))
		{

			if (!irender)
			{
				int *pglcmds;

				pglcmds = (int*)((byte*)mod + mod->ofs_glcmds);

				/* pglcmds points at the start of the command list */
				RenderPacketWithFrame(mod, n, interp,
					pglcmds + mesh_nodes[i].ofs_glcmds);
			}
			else
			{
				const daliasxframe_t *pframe1, *pframe2;
				dtriangle_t *ptri;
				dstvert_t *pstverts;

				const byte *pframes;
				int k, tri_max;

				pframes = (byte*)mod + mod->ofs_frames;
				pframe1 = (const daliasxframe_t*)(pframes + mod->framesize * n);
				pframe2 = (const daliasxframe_t*)(pframes + mod->framesize * ((n + 1) % mod->num_frames));
				ptri = (dtriangle_t *)((byte *)mod + mod->ofs_tris);
				pstverts = (dstvert_t *)((byte *)mod + mod->ofs_st);

				tri_max = mesh_nodes[i].ofs_tris + mesh_nodes[i].num_tris;

				for (k = mesh_nodes[i].ofs_tris; k < tri_max; ++k)
				{
					int j;

					glBegin(GL_TRIANGLE_FAN);

					for (j = 0; j < 3; j++)
					{
						mem_glcmd_t packet;

						packet.index = ptri[k].index_xyz[j];
						packet.s = (float)pstverts[ptri[k].index_st[j]].s / mod->skinwidth;
						packet.t = (float)pstverts[ptri[k].index_st[j]].t / mod->skinheight;

						RenderPacket(interp, &packet, pframe1, pframe2);
					}

					glEnd ();
				}
			}
		}
	}
}

/**
 * Calculate the current frame in animation beginning at frame
 * 'start' and ending at frame 'end', given interpolation percent.
 * interp will be reseted to 0.0 if the next frame is reached.
 */
void
Animate(int start, int end, int *frame, float *interp)
{
	if ((*frame < start) || (*frame > end))
		*frame = start;

	if (*interp >= 1.0f)
	{
		/* Move to next frame */
		*interp = 0.0f;
		(*frame)++;

		if (*frame >= end)
			*frame = start;
	}
}

struct image_s*
find_image(const char *name, imagetype_t type)
{
	int w, h, b;
	byte *data;

	if (!name || !name[0])
	{
		return NULL;
	}

	data = image_load(name, &w, &h, &b);
	if (!data)
	{
		R_Printf(PRINT_ALL, "%s: failed loading!\n", name);
	}
	else
	{
		R_Printf(PRINT_ALL, "upload %s %dx%d %d\n",name, w, h, b);
		upload_texture(data, w, h);
		free(data);
	}

	/* return false as code does not fully support image logic */
	return NULL;
}

void
cleanup(void)
{
	FreeModel(mem_mod);
}

void
keyboard(unsigned char key, int x, int y)
{
	iskin += skin_count;
	xrotate += 360;
	yrotate += 360;
	zrotate += 360;

	switch (key)
	{
		case '0':
			imesh++;
			break;

		case '=':
			iskin++;
			break;

		case '-':
			iskin--;
			break;

		case 'w':
			xrotate += 5;
			break;

		case 's':
			xrotate -= 5;
			break;

		case 'd':
			yrotate += 5;
			break;

		case 'a':
			yrotate -= 5;
			break;

		case 'z':
			zrotate += 5;
			break;

		case 'c':
			zrotate -= 5;
			break;

		case 'x':
			irender++;
			break;

		case 'v':
			zdist -= 5;
			break;

		case 'b':
			zdist += 5;
			break;


		case 'q':
			exit (0);
			break;
	}

	iskin %= skin_count;
	xrotate %= 360;
	yrotate %= 360;
	zrotate %= 360;
}

/*
 * https://nehe.gamedev.net/article/replacement_for_gluperspective/21002/
 */
void perspectiveGL( GLdouble fovY, GLdouble aspect, GLdouble zNear, GLdouble zFar )
{
	const GLdouble pi = 3.1415926535897932384626433832795;
	GLdouble fW, fH;

	//fH = tan( (fovY / 2) / 180 * pi ) * zNear;
	fH = tan( fovY / 360 * pi ) * zNear;
	fW = fH * aspect;

	glFrustum( -fW, fW, -fH, fH, zNear, zFar );
}

void
reshape(int w, int h)
{
	if (h == 0)
	{
		h = 1;
	}

	glViewport (0, 0, (GLsizei)w, (GLsizei)h);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	perspectiveGL(45.0, w/(GLdouble)h, 0.1, 10000.0);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
}
