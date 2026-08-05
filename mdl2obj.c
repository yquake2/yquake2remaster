// Simple MDL (hlmdl_header_t) -> OBJ exporter
// Usage: mdl2obj <input.mdl> [output.obj] [frame]
// - By default exports frame 0 (first frame). Supports applying bone transforms for that frame.
// - Minimal implementation: extracts vertices, normals, and faces from models and applies bone transforms from the first sequence
// - Does NOT handle encrypted/compressed MDLs; only supports seqgroup==0 (embedded anims)

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "src/common/header/shared.h"
#include "src/common/header/files.h"

void R_Printf(int level, const char* msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	vprintf(msg, argptr);
	va_end(argptr);
}

void
Com_Printf (const char *msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	vprintf(msg, argptr);
	va_end(argptr);
}

void
Com_DPrintf (const char *msg, ...)
{
	va_list argptr;
	va_start(argptr, msg);
	vprintf(msg, argptr);
	va_end(argptr);
}

void
Sys_Error(const char *error, ...)
{
	va_list argptr;
	va_start(argptr, error);
	vprintf(error, argptr);
	va_end(argptr);

	exit (0);
}

void
Com_Error(int error_code, const char *error, ...)
{
	va_list argptr;
	va_start(argptr, error);
	vprintf(error, argptr);
	va_end(argptr);

	exit (0);
}

// Basic types that mirror the original studio types

/* bone controllers */
typedef struct
{
	int bone;	/* -1 == 0 */
	int type;	/* X, Y, Z, XR, YR, ZR, M */
	float start;
	float end;
	int rest;	/* byte index value at rest */
	int index;	/* 0-3 user set controller, 4 mouth */
} hlmdl_bonecontroller_t;

typedef unsigned short hlmdl_anim_t[6];

// animation frames
typedef union
{
	struct {
		byte valid;
		byte total;
	} num;
	short		value;
} hlmdl_animvalue_t;

// Minimal math helpers copied from engine mathlib
static void
AngleQuaternion(const vec3_t angles, vec4_t quaternion)
{
	float angle, sr, sp, sy, cr, cp, cy;

	angle = angles[2] * 0.5;
	sy = sinf(angle);
	cy = cosf(angle);
	angle = angles[1] * 0.5;
	sp = sinf(angle);
	cp = cosf(angle);
	angle = angles[0] * 0.5;
	sr = sinf(angle);
	cr = cosf(angle);

	quaternion[0] = sr*cp*cy-cr*sp*sy; // X
	quaternion[1] = cr*sp*cy+sr*cp*sy; // Y
	quaternion[2] = cr*cp*sy-sr*sp*cy; // Z
	quaternion[3] = cr*cp*cy+sr*sp*sy; // W
}

static void
QuaternionMatrix(const vec4_t quaternion, float (*matrix)[4])
{
	matrix[0][0] = 1.0f - 2.0f * quaternion[1] * quaternion[1] - 2.0f * quaternion[2] * quaternion[2];
	matrix[1][0] = 2.0f * quaternion[0] * quaternion[1] + 2.0f * quaternion[3] * quaternion[2];
	matrix[2][0] = 2.0f * quaternion[0] * quaternion[2] - 2.0f * quaternion[3] * quaternion[1];

	matrix[0][1] = 2.0f * quaternion[0] * quaternion[1] - 2.0f * quaternion[3] * quaternion[2];
	matrix[1][1] = 1.0f - 2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[2] * quaternion[2];
	matrix[2][1] = 2.0f * quaternion[1] * quaternion[2] + 2.0f * quaternion[3] * quaternion[0];

	matrix[0][2] = 2.0f * quaternion[0] * quaternion[2] + 2.0f * quaternion[3] * quaternion[1];
	matrix[1][2] = 2.0f * quaternion[1] * quaternion[2] - 2.0f * quaternion[3] * quaternion[0];
	matrix[2][2] = 1.0f - 2.0f * quaternion[0] * quaternion[0] - 2.0f * quaternion[1] * quaternion[1];
}

static void
QuaternionSlerp(const vec4_t p, vec4_t q, float t, vec4_t qt)
{
	int i;
	float omega, cosom, sinom, sclp, sclq;

	float a = 0;
	float b = 0;
	for (i = 0; i < 4; i++)
	{
		a += (p[i]-q[i])*(p[i]-q[i]);
		b += (p[i]+q[i])*(p[i]+q[i]);
	}

	if (a > b)
	{
		for (i = 0; i < 4; i++)
		{
			((float*)q)[i] = -((float*)q)[i];
		}
	}

	cosom = p[0]*q[0] + p[1]*q[1] + p[2]*q[2] + p[3]*q[3];

	if ((1.0f + cosom) > 0.00000001f)
	{
		if ((1.0f - cosom) > 0.00000001f)
		{
			omega = acosf(cosom);
			sinom = sinf( omega );
			sclp = sinf((1.0f - t) * omega) / sinom;
			sclq = sinf(t * omega ) / sinom;
		}
		else
		{
			sclp = 1.0f - t;
			sclq = t;
		}

		for (i = 0; i < 4; i++)
		{
			qt[i] = sclp * p[i] + sclq * q[i];
		}
	}
	else
	{
		qt[0] = -p[1];
		qt[1] = p[0];
		qt[2] = -p[3];
		qt[3] = p[2];
		sclp = sinf( (1.0f - t) * 0.5f * M_PI);
		sclq = sinf( t * 0.5f * M_PI);
		for (i = 0; i < 3; i++)
		{
			qt[i] = sclp * p[i] + sclq * qt[i];
		}
	}
}

static void
VectorTransform(const vec3_t in1, const float in2[3][4], vec3_t out)
{
	out[0] = DotProduct(in1, in2[0]) + in2[0][3];
	out[1] = DotProduct(in1, in2[1]) + in2[1][3];
	out[2] = DotProduct(in1, in2[2]) + in2[2][3];
}

static void
VectorRotate(const vec3_t in1, const float in2[3][4], vec3_t out)
{
	out[0] = DotProduct(in1, in2[0]);
	out[1] = DotProduct(in1, in2[1]);
	out[2] = DotProduct(in1, in2[2]);
}

// Calculate quaternion from anim (port of StudioModel functions, assumes panim points to array per bone)
static void
CalcBoneQuaternion(int frame, float s, hlmdl_bone_t *pbone, hlmdl_anim_t *panim, float *q_out)
{
	vec4_t q1, q2;
	vec3_t angle1, angle2;
	hlmdl_animvalue_t *panimvalue;
	int j;

	for (j = 0; j < 3; j++)
	{
		// default
		angle1[j] = angle2[j] = pbone->value[j + 3];
		if (panim && (*panim)[j + 3] != 0)
		{
			panimvalue = (hlmdl_animvalue_t *)((byte*)panim + (*panim)[j + 3]);
			int k = frame;
			if (panimvalue->num.total < panimvalue->num.valid) k = 0;
			while (panimvalue->num.total <= k) {
				k -= panimvalue->num.total;
				panimvalue = (hlmdl_animvalue_t *)((byte*)panimvalue + (panimvalue->num.valid + 1) * sizeof(hlmdl_animvalue_t));
				if (panimvalue->num.total < panimvalue->num.valid) k = 0;
			}
			if (panimvalue->num.valid > k)
			{
				angle1[j] = panimvalue[k+1].value;

				if (panimvalue->num.valid > k + 1)
				{
					angle2[j] = panimvalue[k+2].value;
				}
				else
				{
					if (panimvalue->num.total > k + 1)
					{
						angle2[j] = angle1[j];
					}
					else
					{
						angle2[j] = panimvalue[panimvalue->num.valid+2].value;
					}
				}
			}
			else
			{
				angle1[j] = panimvalue[panimvalue->num.valid].value;

				if (panimvalue->num.total > k + 1)
				{
					angle2[j] = angle1[j];
				}
				else
				{
					angle2[j] = panimvalue[panimvalue->num.valid + 2].value;
				}
			}

			angle1[j] = pbone->value[j+3] + angle1[j] * pbone->scale[j+3];
			angle2[j] = pbone->value[j+3] + angle2[j] * pbone->scale[j+3];
		}
		// no controllers support; m_adj assumed 0
	}

	if (angle1[0] != angle2[0] || angle1[1] != angle2[1] || angle1[2] != angle2[2])
	{
		AngleQuaternion(angle1, q1);
		AngleQuaternion(angle2, q2);
		QuaternionSlerp(q1, q2, s, q_out);
	}
	else
	{
		AngleQuaternion(angle1, q_out);
	}
}

// Calculate bone position from anim
static void
CalcBonePosition(int frame, float s, hlmdl_bone_t *pbone, hlmdl_anim_t *panim, float *pos)
{
	hlmdl_animvalue_t *panimvalue;

	for (int j = 0; j < 3; j++) {
		pos[j] = pbone->value[j];
		if (panim && (*panim)[j] != 0) {
			panimvalue = (hlmdl_animvalue_t *)((byte*)panim + (*panim)[j]);
			int k = frame;
			if (panimvalue->num.total < panimvalue->num.valid) k = 0;
			while (panimvalue->num.total <= k) {
				k -= panimvalue->num.total;
				panimvalue = (hlmdl_animvalue_t *)((byte*)panimvalue + (panimvalue->num.valid + 1) * sizeof(hlmdl_animvalue_t));
				if (panimvalue->num.total < panimvalue->num.valid) k = 0;
			}
			if (panimvalue->num.valid > k) {
				if (panimvalue->num.valid > k + 1)
					pos[j] += (panimvalue[k+1].value * (1.0f - s) + s * panimvalue[k+2].value) * pbone->scale[j];
				else
					pos[j] += panimvalue[k+1].value * pbone->scale[j];
			} else {
				if (panimvalue->num.total <= k + 1)
					pos[j] += (panimvalue[panimvalue->num.valid].value * (1.0f - s) + s * panimvalue[panimvalue->num.valid + 2].value) * pbone->scale[j];
				else
					pos[j] += panimvalue[panimvalue->num.valid].value * pbone->scale[j];
			}
		}
	}
}

static void *
load_file(const char *path, size_t *out_size)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return NULL;
	if (fseek(f, 0, SEEK_END) != 0)
	{
		fclose(f);
		return NULL;
	}

	long size = ftell(f);
	if (size < 0)
	{
		fclose(f);
		return NULL;
	}

	rewind(f);
	void *buf = malloc((size_t)size);
	if (!buf)
	{
		fclose(f);
		return NULL;
	}

	if (fread(buf, 1, (size_t)size, f) != (size_t)size)
	{
		free(buf);
		fclose(f);
		return NULL;
	}

	fclose(f);
	*out_size = (size_t)size;
	return buf;
}

int
main(int argc, char **argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "Usage: %s <input.mdl> [output.obj] [frame]\n", argv[0]);
		return 1;
	}

	const char *inpath = argv[1];
	const char *outpath = NULL;
	char default_out[1024];
	if (argc >= 3 && argv[2][0] != '-' )
	{
		outpath = argv[2];
	}
	else
	{
		const char *dot = strrchr(inpath, '.');
		size_t n = (dot ? (size_t)(dot - inpath) : strlen(inpath));
		snprintf(default_out, sizeof(default_out), "%.*s.obj", (int)n, inpath);
		outpath = default_out;
	}

	int frame = 0;
	if (argc >= 4)
	{
		frame = atoi(argv[3]);
	}

	size_t size;
	void *buf = load_file(inpath, &size);
	if (!buf)
	{
		fprintf(stderr, "Error: cannot read file '%s'\n", inpath);
		return 2;
	}

	if (size < (size_t)sizeof(hlmdl_header_t))
	{
		fprintf(stderr, "Error: file too small to be MDL\n");
		free(buf);
		return 3;
	}

	hlmdl_header_t *hdr = (hlmdl_header_t *)buf;

	if (hdr->num_bodyparts <= 0 || hdr->ofs_bodyparts <= 0)
	{
		fprintf(stderr, "Error: no bodyparts in model or unsupported file.\n");
		free(buf);
		return 4;
	}

	// Prepare bone transforms for the requested frame
	int nBones = hdr->num_bones;
	float (*bonetransform)[3][4] = malloc(sizeof(float) * 12 * (nBones > 0 ? nBones:1));
	if (!bonetransform)
	{
		fprintf(stderr, "OOM\n");
		free(buf);
		return 7;
	}

	// zero defaults
	float *m_adj = NULL;
	if (hdr->num_bonecontrollers > 0)
	{
		m_adj = malloc(sizeof(float) * hdr->num_bonecontrollers);
		memset(m_adj, 0, sizeof(float) * hdr->num_bonecontrollers);
	}

	// Use first sequence
	hlmdl_sequence_t *pseq = NULL;
	if (hdr->num_seq > 0 && hdr->ofs_seq > 0)
	{
		pseq = (hlmdl_sequence_t *)((byte*)hdr + hdr->ofs_seq);
	}

	if (!pseq)
	{
		// No sequences: identity bones
		for (int i = 0; i < nBones; ++i)
		{
			memset(bonetransform[i], 0, sizeof(bonetransform[i]));

			bonetransform[i][0][0]=1;
			bonetransform[i][1][1]=1;
			bonetransform[i][2][2]=1;
		}
	}
	else
	{
		hlmdl_anim_t *panim = NULL;

		if (pseq->seqgroup != 0)
		{
			fprintf(stderr, "Warning: seqgroup != 0 not supported; using seqgroup 0 assumptions\n");
		}

		if (pseq->seqgroup == 0)
		{
			panim = (hlmdl_anim_t *)((byte*)hdr + pseq->animindex);
		}
		else
		{
			panim = (hlmdl_anim_t *)((byte*)hdr + pseq->animindex); // fallback
		}

		vec4_t *q = malloc(sizeof(vec4_t) * nBones);
		vec3_t *pos = malloc(sizeof(vec3_t) * nBones);
		if (!q || !pos)
		{
			fprintf(stderr, "OOM\n");
			free(buf);
			free(bonetransform);
			free(q);
			free(pos);
			return 8;
		}

		int frame_int = frame;
		float s = 0.0f; // integer frame

		hlmdl_anim_t *panim_perbone = panim;
		hlmdl_bone_t *pbones = (hlmdl_bone_t *)((byte*)hdr + hdr->ofs_bones);

		for (int i = 0; i < nBones; ++i)
		{
			// note: we pass file size/base if later needed
			// If panim is not valid we'll default to bone values in Calc functions
			vec4_t qtmp; // will be placed into q[i]
			// reuse local helpers
			// Call simplified calculators
			// compute quaternion
			// default behavior when panim is invalid handled in helper
			// We implement an inline-like call for quaternion/pos
			// (helpers are defined above in file)
			// We simply call CalcBoneQuaternion/CalcBonePosition (implemented earlier)
			// To avoid duplication, call them directly
			CalcBoneQuaternion(frame_int, s, &pbones[i], (panim_perbone? &panim_perbone[i] : NULL), q[i]);
			CalcBonePosition(frame_int, s, &pbones[i], (panim_perbone? &panim_perbone[i] : NULL), pos[i]);
		}

		for (int i = 0; i < nBones; ++i)
		{
			float bonematrix[3][4];
			QuaternionMatrix(q[i], bonematrix);
			bonematrix[0][3] = pos[i][0];
			bonematrix[1][3] = pos[i][1];
			bonematrix[2][3] = pos[i][2];

			int parent = pbones[i].parent;
			if (parent == -1)
			{
				memcpy(bonetransform[i], bonematrix, sizeof(float) * 12);
			} else {
				R_ConcatTransforms(bonetransform[parent], bonematrix, bonetransform[i]);
			}
		}

		free(q); free(pos);
	}

	// Collect transformed vertices and normals
	float *g_verts = NULL; size_t g_verts_count = 0;
	float *g_norms = NULL; size_t g_norms_count = 0;
	byte *g_boneids = NULL; size_t g_boneids_count = 0;
	typedef struct { int model_base_v; int model_base_n; } baseoff_t;
	baseoff_t *bases = NULL; size_t nbases = 0;

	hlmdl_bodypart_t *bparts = (hlmdl_bodypart_t *)((byte *)hdr + hdr->ofs_bodyparts);
	for (int bp = 0; bp < hdr->num_bodyparts; ++bp)
	{
		hlmdl_bodymodel_t *models = (hlmdl_bodymodel_t *)((byte *)hdr + bparts[bp].ofs_model);

		for (int m = 0; m < bparts[bp].num_models; ++m)
		{
			hlmdl_bodymodel_t *pm = &models[m];
			if (pm->num_verts > 0 &&
				pm->ofs_vert > 0 &&
				(size_t)pm->ofs_vert + pm->num_verts * sizeof(vec3_t) <= size)
			{
				vec3_t *verts = (vec3_t *)((byte *)hdr + pm->ofs_vert);
				byte *vertbone = NULL;
				if (pm->ofs_vertinfo > 0 &&
					(size_t)pm->ofs_vertinfo + pm->num_verts <= size)
					vertbone = (byte *)hdr + pm->ofs_vertinfo;
				size_t old = g_verts_count;
				g_verts = realloc(g_verts, sizeof(float) * 3 * (g_verts_count + pm->num_verts));
				g_boneids = realloc(g_boneids, sizeof(byte) * (g_boneids_count + pm->num_verts));
				for (int k = 0; k < pm->num_verts; ++k)
				{
					vec3_t outv;
					int bone = 0;
					if (vertbone) bone = vertbone[k];
					if (bone < 0 || bone >= nBones)
						bone = 0;
					VectorTransform(verts[k], bonetransform[bone], outv);
					memcpy(g_verts + 3*(old + k), outv, sizeof(float)*3);
					g_boneids[g_boneids_count + k] = bone;
				}

				g_verts_count += pm->num_verts;
				g_boneids_count += pm->num_verts;
			}

			if (pm->num_norms > 0 &&
				pm->ofs_norm > 0 &&
				(size_t)pm->ofs_norm + pm->num_norms * sizeof(vec3_t) <= size)
			{
				vec3_t *norms = (vec3_t *)((byte *)hdr + pm->ofs_norm);
				byte *normbone = NULL;
				if (pm->ofs_norminfo > 0 &&
					(size_t)pm->ofs_norminfo + pm->num_norms <= size)
				{
					normbone = (byte *)hdr + pm->ofs_norminfo;
				}

				size_t oldn = g_norms_count;
				g_norms = realloc(g_norms, sizeof(float) * 3 * (g_norms_count + pm->num_norms));
				for (int k = 0; k < pm->num_norms; ++k) {
					vec3_t outn;
					int bone = 0;
					if (normbone) bone = normbone[k];
					if (bone < 0 || bone >= nBones) bone = 0;
					VectorRotate(norms[k], bonetransform[bone], outn);
					memcpy(g_norms + 3*(oldn + k), outn, sizeof(float)*3);
				}
				g_norms_count += pm->num_norms;
			}

			bases = realloc(bases, sizeof(baseoff_t) * (nbases + 1));
			bases[nbases].model_base_v = (int)g_verts_count - pm->num_verts;
			bases[nbases].model_base_n = (int)g_norms_count - pm->num_norms;
			nbases++;
		}
	}

	// Dump first 10 vertices
	for (size_t i = 0; i < g_verts_count; ++i) {
		printf("mdl2obj vert %zu bone %d: %f %f %f\n", i, (int)g_boneids[i], g_verts[3*i], g_verts[3*i+1], g_verts[3*i+2]);
	}

	// Prepare texture info if present
	hlmdl_texture_t *ptexture = NULL;
	short *pskinref = NULL;

	if (hdr->num_skins > 0 && hdr->ofs_texture > 0)
	{
		ptexture = (hlmdl_texture_t *)((byte*)hdr + hdr->ofs_texture);
	}

	if (hdr->num_skinref > 0 && hdr->ofs_skins > 0)
	{
		pskinref = (short *)((byte*)hdr + hdr->ofs_skins);
	}

	// We'll build UV list and faces in memory first
	float *uv_coords = NULL; size_t uv_count = 0; // pairs (u,v)
	struct { int gv; int s; int t; int texid; int vt; } *uvkeys = NULL; size_t uvkeys_count = 0;
	int *faces = NULL; size_t faces_count = 0; // each triangle = 9 ints: v,vt,vn x3

	int have_normals = (g_norms_count > 0);

	size_t base_index = 0; // index into bases
	for (int bp = 0; bp < hdr->num_bodyparts; ++bp)
	{
		hlmdl_bodymodel_t *models = (hlmdl_bodymodel_t *)((byte *)hdr + bparts[bp].ofs_model);
		for (int m = 0; m < bparts[bp].num_models; ++m) {
			hlmdl_bodymodel_t *pm = &models[m];
			int base_v = bases[base_index].model_base_v;
			int base_n = bases[base_index].model_base_n;
			base_index++;

			if (pm->num_mesh <= 0 || pm->ofs_mesh <= 0) continue;
			hlmdl_bodymesh_t *meshes = (hlmdl_bodymesh_t *)((byte *)hdr + pm->ofs_mesh);
			for (int ms = 0; ms < pm->num_mesh; ++ms) {
				hlmdl_bodymesh_t *pmesh = &meshes[ms];
				if (pmesh->ofs_tris <= 0) continue;
				if ((size_t)pmesh->ofs_tris >= size) continue;

				// determine texture index and scales
				int texid = 0;
				float s_scale = 1.0f, t_scale = 1.0f;
				if (pskinref) {
					int idx = pmesh->skinref;
					if (idx >= 0 && idx < hdr->num_skinref) {
						texid = pskinref[idx];
					}
				} else {
					texid = pmesh->skinref;
				}
				if (ptexture && texid >= 0 && texid < hdr->num_skins) {
					int w = ptexture[texid].width; if (w <= 0) w = 1;
					int h = ptexture[texid].height; if (h <= 0) h = 1;
					s_scale = 1.0f / (float)w;
					t_scale = 1.0f / (float)h;
				}

				int16_t *ptricmds = (int16_t *)((byte *)hdr + pmesh->ofs_tris);
				int offset = 0; // index within the tri commands
				while (1) {
					int16_t cnt = ptricmds[offset++];
					if (cnt == 0) break;
					int tri_type; // 0=strip,1=fan
					if (cnt < 0) { tri_type = 1; cnt = -cnt; } else tri_type = 0;
					if (cnt <= 0) break;
					int *vlist = (int*)malloc(sizeof(int) * cnt);
					int *nlist = (int*)malloc(sizeof(int) * cnt);
					int *vtlist = (int*)malloc(sizeof(int) * cnt);
					for (int vi = 0; vi < cnt; ++vi) {
						int16_t v = ptricmds[offset++];
						int16_t n = ptricmds[offset++];
						int16_t sraw = ptricmds[offset++];
						int16_t traw = ptricmds[offset++];
						int gv = base_v + v; // 0-based global vertex index
						int gn = base_n + n; // 0-based global normal index
						vlist[vi] = gv;
						nlist[vi] = gn;

						// find or create vt index for (gv, sraw, traw, texid)
						int found = -1;
						for (size_t k = 0; k < uvkeys_count; ++k) {
							if (uvkeys[k].gv == gv && uvkeys[k].s == sraw && uvkeys[k].t == traw && uvkeys[k].texid == texid) { found = uvkeys[k].vt; break; }
						}
						if (found == -1) {
							// add uv coord
							float u = sraw * s_scale;
							float vcoord = traw * t_scale;
							uv_coords = realloc(uv_coords, sizeof(float) * 2 * (uv_count + 1));
							uv_coords[2*uv_count + 0] = u;
							uv_coords[2*uv_count + 1] = vcoord;
							// add key
							uvkeys = realloc(uvkeys, sizeof(*uvkeys) * (uvkeys_count + 1));
							uvkeys[uvkeys_count].gv = gv;
							uvkeys[uvkeys_count].s = sraw;
							uvkeys[uvkeys_count].t = traw;
							uvkeys[uvkeys_count].texid = texid;
							uvkeys[uvkeys_count].vt = (int)(uv_count + 1); // OBJ vt indices are 1-based
							found = uvkeys[uvkeys_count].vt;
							uvkeys_count++;
							uv_count++;
						}
						vtlist[vi] = found;
					}

					// triangulate and store faces (v,vt,vn)
					if (cnt >= 3) {
						if (tri_type == 0) {
							for (int k = 2; k < cnt; ++k) {
								int a,b,c, avt,bvt,cvt, an,bn,cn;
								if ((k & 1) == 0) {
									a = vlist[k-2]; b = vlist[k-1]; c = vlist[k];
									avt = vtlist[k-2]; bvt = vtlist[k-1]; cvt = vtlist[k];
									an = nlist[k-2]; bn = nlist[k-1]; cn = nlist[k];
								} else {
									a = vlist[k-1]; b = vlist[k-2]; c = vlist[k];
									avt = vtlist[k-1]; bvt = vtlist[k-2]; cvt = vtlist[k];
									an = nlist[k-1]; bn = nlist[k-2]; cn = nlist[k];
								}
								faces = realloc(faces, sizeof(int) * 9 * (faces_count + 1));
								int idx = faces_count*9;
								faces[idx+0] = a+1; faces[idx+1] = avt; faces[idx+2] = an+1;
								faces[idx+3] = b+1; faces[idx+4] = bvt; faces[idx+5] = bn+1;
								faces[idx+6] = c+1; faces[idx+7] = cvt; faces[idx+8] = cn+1;
								faces_count++;
							}
						}
						else
						{
							for (int k = 2; k < cnt; ++k)
							{
								int a = vlist[0]; int b = vlist[k-1]; int c = vlist[k];
								int avt = vtlist[0]; int bvt = vtlist[k-1]; int cvt = vtlist[k];
								int an = nlist[0]; int bn = nlist[k-1]; int cn = nlist[k];
								faces = realloc(faces, sizeof(int) * 9 * (faces_count + 1));
								int idx = faces_count*9;
								faces[idx+0] = a+1; faces[idx+1] = avt; faces[idx+2] = an+1;
								faces[idx+3] = b+1; faces[idx+4] = bvt; faces[idx+5] = bn+1;
								faces[idx+6] = c+1; faces[idx+7] = cvt; faces[idx+8] = cn+1;
								faces_count++;
							}
						}
					}

					free(vlist); free(nlist); free(vtlist);
				}
			}
		}
	}

	// Now open OBJ for writing
	FILE *out = fopen(outpath, "w");
	if (!out)
	{
		fprintf(stderr, "Error: cannot open output '%s'\n", outpath);
		free(buf); free(bonetransform); free(m_adj); free(g_verts); free(g_norms); free(bases);
		return 6;
	}

	fprintf(out, "# OBJ exported from %s (frame %d)\n", inpath, frame);

	// write transformed vertices
	for (size_t i = 0; i < g_verts_count; ++i)
	{
		float *v = &g_verts[3*i];
		fprintf(out, "v %g %g %g\n", v[0], v[1], v[2]);
	}

	// write UVs (vt) if present
	int have_uvs = (uv_count > 0);
	if (have_uvs)
	{
		for (size_t i = 0; i < uv_count; ++i)
		{
			float u = uv_coords[2*i + 0];
			float v = uv_coords[2*i + 1];
			fprintf(out, "vt %g %g\n", u, v);
		}
	}

	// write normals (vn)
	if (have_normals)
	{
		for (size_t i = 0; i < g_norms_count; ++i)
		{
			float *n = &g_norms[3*i];
			fprintf(out, "vn %g %g %g\n", n[0], n[1], n[2]);
		}
	}

	// write faces using v/vt/vn (vt optional)
	for (size_t fi = 0; fi < faces_count; ++fi)
	{
		int *f = &faces[fi*9];

		printf("mdl2obj tri %zu: %d %d %d\n", fi, f[0]-1, f[3]-1, f[6]-1);

		if (have_uvs && have_normals)
		{
			fprintf(out, "f %d/%d/%d %d/%d/%d %d/%d/%d\n",
				f[0], f[1], f[2], f[3], f[4], f[5], f[6], f[7], f[8]);
		}
		else if (have_uvs)
		{
			fprintf(out, "f %d/%d %d/%d %d/%d\n", f[0], f[1], f[3], f[4], f[6], f[7]);
		}
		else if (have_normals)
		{
			fprintf(out, "f %d//%d %d//%d %d//%d\n", f[0], f[2], f[3], f[5], f[6], f[8]);
		} else
		{
			fprintf(out, "f %d %d %d\n", f[0], f[3], f[6]);
		}
	}

	fclose(out);

	fprintf(stderr, "Exported OBJ to %s (frame %d)\n", outpath, frame);

	free(buf); free(bonetransform); free(m_adj); free(g_verts); free(g_norms); free(bases);
	free(uv_coords); free(uvkeys); free(faces);
	return 0;
}
