#include "rendergl.h"
#include <math.h>

void VID_GetPalette(byte **colormap, unsigned *d_8to24table);

cvar_t *r_skeletalanimation;

static GLuint skins[16];
static unsigned char colormap[256][3];
static qboolean palette_ready = false;
static cvar_t viewer_skeletal_animation;
static vec4_t *lerped_vertices;
static const dmdx_t *lerped_model;
static int lerped_frame = -1;
static float lerped_interp = -1.0f;
GLuint ftex;
vec3_t model_center;
static float model_radius = 1.0f;
static qboolean model_bounds_ready;
static qboolean camera_fitted;

static void
update_model_bounds(void)
{
	float mins[3] = { 0.0f, 0.0f, 0.0f };
	float maxs[3] = { 0.0f, 0.0f, 0.0f };
	double radius_squared = 0.0;
	int i, j;

	if (model_bounds_ready || !mem_mod || mem_mod->num_frames <= 0 ||
		mem_mod->num_xyz <= 0)
	{
		return;
	}

	for (i = 0; i < mem_mod->num_frames; i++)
	{
		const daliasxframe_t *frame = (const daliasxframe_t *)
			((const byte *)mem_mod + mem_mod->ofs_frames +
			i * mem_mod->framesize);

		for (j = 0; j < mem_mod->num_xyz; j++)
		{
			int axis;

			for (axis = 0; axis < 3; axis++)
			{
				float coordinate = frame->translate[axis] +
					frame->scale[axis] * frame->verts[j].v[axis];

				if (i == 0 && j == 0)
				{
					mins[axis] = coordinate;
					maxs[axis] = coordinate;
				}
				else
				{
					if (coordinate < mins[axis])
					{
						mins[axis] = coordinate;
					}
					if (coordinate > maxs[axis])
					{
						maxs[axis] = coordinate;
					}
				}
			}
		}
	}

	for (i = 0; i < 3; i++)
	{
		model_center[i] = (mins[i] + maxs[i]) * 0.5f;
	}

	for (i = 0; i < mem_mod->num_frames; i++)
	{
		const daliasxframe_t *frame = (const daliasxframe_t *)
			((const byte *)mem_mod + mem_mod->ofs_frames +
			i * mem_mod->framesize);

		for (j = 0; j < mem_mod->num_xyz; j++)
		{
			double distance_squared = 0.0;
			int axis;

			for (axis = 0; axis < 3; axis++)
			{
				double coordinate = frame->translate[axis] +
					frame->scale[axis] * frame->verts[j].v[axis] -
					model_center[axis];

				distance_squared += coordinate * coordinate;
			}

			if (distance_squared > radius_squared)
			{
				radius_squared = distance_squared;
			}
		}
	}

	model_radius = (float)sqrt(radius_squared);
	if (model_radius < 1.0f)
	{
		model_radius = 1.0f;
	}
	model_bounds_ready = true;
}

void
PaletteInit(void)
{
	unsigned d_8to24table[256];
	int i;

	if (palette_ready)
	{
		return;
	}

	VID_GetPalette(NULL, d_8to24table);
	for (i = 0; i < 256; i++)
	{
		unsigned value;

		value = d_8to24table[i];
		colormap[i][0] = (value >> 0) & 0xFF;
		colormap[i][1] = (value >> 8) & 0xFF;
		colormap[i][2] = (value >> 16) & 0xFF;
	}

	palette_ready = true;
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

void
SCR_LoadImageWithPalette(const char *filename, byte **pic, byte **palette,
	int *width, int *height, int *bitsPerPixel);

byte *
image_load(const char* name, int *w, int *h, int *b)
{
	int bytesPerPixel, i;
	byte *pic = NULL, *pic_decoded, *palette_data = NULL;

	SCR_LoadImageWithPalette(name, &pic, &palette_data, w, h, &bytesPerPixel);

	if (bytesPerPixel == 32 || !pic)
	{
		return pic;
	}

	pic_decoded = malloc((*w) * (*h) * 4);

	PaletteInit();

	/* Convert indexed 8 bits texture to RGB 24 bits */
	for (i = 0; i < (*w) * (*h); ++i)
	{
		unsigned char index = pic[i];

		pic_decoded[(i * 4) + 0] = colormap[index][0];
		pic_decoded[(i * 4) + 1] = colormap[index][1];
		pic_decoded[(i * 4) + 2] = colormap[index][2];
		pic_decoded[(i * 4) + 3] = 0;
	}
	free(pic);
	if (palette_data)
	{
		free(palette_data);
	}

	return pic_decoded;
}

struct image_s *
insert_skin(const char *name, byte *data, int width, int realwidth,
	int height, int realheight, size_t size, imagetype_t type, int bits)
{
	int i;

	GLubyte *pixels = (GLubyte *)malloc(width * height * 4);

	PaletteInit();

	/* Convert indexed 8 bits texture to RGB 24 bits */
	for (i = 0; i < width * height; ++i)
	{
		pixels[(i * 4) + 0] = colormap[data[i]][0];
		pixels[(i * 4) + 1] = colormap[data[i]][1];
		pixels[(i * 4) + 2] = colormap[data[i]][2];
		pixels[(i * 4) + 3] = 0;
	}

	upload_texture(pixels, width, height);

	/* OpenGL has its own copy of image data */
	free(pixels);

	return NULL;
}

static void
RenderPacket(const dmdx_t *mod, int frame, float interp, mem_glcmd_t *packet,
	const daliasxframe_t *pframe1, const daliasxframe_t *pframe2)
{
	vec3_t norm, n_curr, n_next;
	static const vec3_t base_move = { 0.0f, 0.0f, 0.0f };
	static const vec3_t scale = { 1.0f, 1.0f, 1.0f };
	const dxtrivertx_t *pvert1, *pvert2;
	int i;

	pvert1 = &pframe1->verts[packet->index];
	pvert2 = &pframe2->verts[packet->index];
	if (lerped_model != mod || lerped_frame != frame || lerped_interp != interp)
	{
		if (!r_skeletalanimation)
		{
			r_skeletalanimation = &viewer_skeletal_animation;
			r_skeletalanimation->value = 1.0f;
		}

		lerped_vertices = R_VertBufferRealloc(mod->num_xyz);
		R_LerpVerts(mod, (frame + 1) % mod->num_frames, frame, interp,
			1.0f - interp, (float *)lerped_vertices, base_move, scale, false);
		lerped_model = mod;
		lerped_frame = frame;
		lerped_interp = interp;
	}

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

	glVertex3fv(lerped_vertices[packet->index]);
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

			RenderPacket(mod, n, interp, packet, pframe1, pframe2);
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

						RenderPacket(mod, n, interp, &packet, pframe1, pframe2);
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
	R_VertBufferFree();
}

void
keyboard(unsigned char key, int x, int y)
{
	if (skin_count > 0)
	{
		iskin += skin_count;
	}
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

	if (skin_count > 0)
	{
		iskin %= skin_count;
	}
	else
	{
		iskin = 0;
	}
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
	const double pi = 3.1415926535897932384626433832795;
	const double vertical_half_fov = 45.0 * pi / 360.0;
	double aspect, half_fov, fit_distance, camera_distance;
	double z_near, z_far;

	if (w == 0)
	{
		w = 1;
	}

	if (h == 0)
	{
		h = 1;
	}

	aspect = w / (double)h;
	update_model_bounds();
	half_fov = atan(tan(vertical_half_fov) * aspect);
	if (half_fov > vertical_half_fov)
	{
		half_fov = vertical_half_fov;
	}

	fit_distance = model_radius / sin(half_fov) * 1.1;
	if (!camera_fitted || -zdist < fit_distance)
	{
		zdist = (float)-fit_distance;
		camera_fitted = true;
	}

	camera_distance = -zdist;
	z_near = camera_distance - model_radius * 1.1;
	if (z_near < 0.1)
	{
		z_near = 0.1;
	}
	z_far = camera_distance + model_radius * 1.1;
	if (z_far <= z_near)
	{
		z_far = z_near + 1.0;
	}

	glViewport (0, 0, (GLsizei)w, (GLsizei)h);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	perspectiveGL(45.0, aspect, z_near, z_far);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
}
