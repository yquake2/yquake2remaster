/*
 * Copyright (C) 1997-2005 Id Software, Inc.
 * Copyright (C) 2010 Matthew Baranowski, Sander van Rossen & Raven software.
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
 * The MDR models file format
 *
 * =======================================================================
 */

#include "models.h"

#define MC_BITS_X (16)
#define MC_BITS_Y (16)
#define MC_BITS_Z (16)
#define MC_BITS_VECT (16)

#define MC_SCALE_X (1.0f/64)
#define MC_SCALE_Y (1.0f/64)
#define MC_SCALE_Z (1.0f/64)

#define MC_MASK_X ((1<<(MC_BITS_X))-1)
#define MC_MASK_Y ((1<<(MC_BITS_Y))-1)
#define MC_MASK_Z ((1<<(MC_BITS_Z))-1)
#define MC_MASK_VECT ((1<<(MC_BITS_VECT))-1)

#define MC_SCALE_VECT (1.0f/(float)((1<<(MC_BITS_VECT-1))-2))

#define MC_POS_X (0)
#define MC_SHIFT_X (0)

#define MC_POS_Y ((((MC_BITS_X))/8))
#define MC_SHIFT_Y ((((MC_BITS_X)%8)))

#define MC_POS_Z ((((MC_BITS_X+MC_BITS_Y))/8))
#define MC_SHIFT_Z ((((MC_BITS_X+MC_BITS_Y)%8)))

#define MC_POS_V11 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z))/8))
#define MC_SHIFT_V11 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z)%8)))

#define MC_POS_V12 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT))/8))
#define MC_SHIFT_V12 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT)%8)))

#define MC_POS_V13 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*2))/8))
#define MC_SHIFT_V13 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*2)%8)))

#define MC_POS_V21 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*3))/8))
#define MC_SHIFT_V21 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*3)%8)))

#define MC_POS_V22 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*4))/8))
#define MC_SHIFT_V22 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*4)%8)))

#define MC_POS_V23 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*5))/8))
#define MC_SHIFT_V23 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*5)%8)))

#define MC_POS_V31 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*6))/8))
#define MC_SHIFT_V31 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*6)%8)))

#define MC_POS_V32 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*7))/8))
#define MC_SHIFT_V32 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*7)%8)))

#define MC_POS_V33 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*8))/8))
#define MC_SHIFT_V33 ((((MC_BITS_X+MC_BITS_Y+MC_BITS_Z+MC_BITS_VECT*8)%8)))

static void
MC_UnCompress(float mat[3][4],const unsigned char * comp)
{
	int val;

	val=(int)((unsigned short *)(comp))[0];
	val-=1<<(MC_BITS_X-1);
	mat[0][3]=((float)(val))*MC_SCALE_X;

	val=(int)((unsigned short *)(comp))[1];
	val-=1<<(MC_BITS_Y-1);
	mat[1][3]=((float)(val))*MC_SCALE_Y;

	val=(int)((unsigned short *)(comp))[2];
	val-=1<<(MC_BITS_Z-1);
	mat[2][3]=((float)(val))*MC_SCALE_Z;

	val=(int)((unsigned short *)(comp))[3];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[4];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[5];
	val-=1<<(MC_BITS_VECT-1);
	mat[0][2]=((float)(val))*MC_SCALE_VECT;


	val=(int)((unsigned short *)(comp))[6];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[7];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[8];
	val-=1<<(MC_BITS_VECT-1);
	mat[1][2]=((float)(val))*MC_SCALE_VECT;


	val=(int)((unsigned short *)(comp))[9];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][0]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[10];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][1]=((float)(val))*MC_SCALE_VECT;

	val=(int)((unsigned short *)(comp))[11];
	val-=1<<(MC_BITS_VECT-1);
	mat[2][2]=((float)(val))*MC_SCALE_VECT;
}

/*
=================
Mod_LoadModel_MDR

=================
*/
void *
Mod_LoadModel_MDR(const char *mod_name, const void *buffer, int modfilelen,
	struct image_s ***skins, int *numskins, modtype_t *type)
{
	int framesize, ofs_skins, ofs_frames, ofs_glcmds, ofs_meshes, ofs_tris,
		ofs_st, ofs_end;
	int num_xyz = 0, num_tris = 0, num_glcmds = 0, num_skins = 0, meshofs = 0;
	mdr_header_t pinmodel;
	dmdxmesh_t *mesh_nodes;
	dtriangle_t *tris;
	dstvert_t *st;
	void *extradata = NULL;
	dmdx_t *pheader = NULL;
	dmdx_vert_t *vertx;
	char *skin;
	int i;

	if (modfilelen < sizeof(pinmodel))
	{
		R_Printf(PRINT_ALL, "%s: %s has incorrect header size (%i should be " YQ2_COM_PRIdS ")\n",
				__func__, mod_name, modfilelen, sizeof(pinmodel));
		return NULL;
	}

	for (i = 0; i < sizeof(pinmodel) / sizeof(int); i++)
	{
		((int *)&pinmodel)[i] = LittleLong(((int *)buffer)[i]);
	}

	if (pinmodel.version != MDR_VERSION)
	{
		R_Printf(PRINT_ALL, "%s: %s has wrong version number (%i should be %i)\n",
				__func__, mod_name, pinmodel.version, MDR_VERSION);
		return NULL;
	}

	int unframesize = sizeof(mdr_frame_t) + sizeof(mdr_bone_t) * (pinmodel.num_bones - 1);
	char *frames = malloc(unframesize * pinmodel.num_frames);

	if (pinmodel.ofs_frames < 0)
	{
		int compframesize = sizeof(mdr_compframe_t) + sizeof(mdr_compbone_t) * (pinmodel.num_bones - 1);
		for (i = 0; i < pinmodel.num_frames; i++)
		{
			mdr_compframe_t * inframe = (mdr_compframe_t*)(
				(byte*)buffer + -pinmodel.ofs_frames +
				(i * compframesize));
			mdr_frame_t *outframe = (mdr_frame_t *)(frames + i * unframesize);
			int j;

			memset(outframe->name, 0, sizeof(outframe->name));
			for (j = 0; j < 3; j++)
			{
				outframe->bounds[0][j] = LittleFloat(inframe->bounds[0][j]);
				outframe->bounds[1][j] = LittleFloat(inframe->bounds[1][j]);
				outframe->origin[j] = LittleFloat(inframe->origin[j]);
				outframe->radius = LittleFloat(inframe->radius);
			}
			for (j = 0; j < pinmodel.num_bones; j ++)
			{
				MC_UnCompress(outframe->bones[j].matrix, inframe->bones[j].Comp);
			}
		}
	}
	else
	{
		for (i = 0; i < pinmodel.num_frames; i++)
		{
			mdr_frame_t * inframe = (mdr_frame_t*)(
				(byte*)buffer + pinmodel.ofs_frames +
				(i * unframesize));
			mdr_frame_t *outframe = (mdr_frame_t *)(frames + i * unframesize);
			int j;

			memcpy(outframe->name, inframe->name, sizeof(outframe->name));
			for (j = 0; j < 3; j++)
			{
				outframe->bounds[0][j] = LittleFloat(inframe->bounds[0][j]);
				outframe->bounds[1][j] = LittleFloat(inframe->bounds[1][j]);
				outframe->origin[j] = LittleFloat(inframe->origin[j]);
				outframe->radius = LittleFloat(inframe->radius);
			}

			int float_count = pinmodel.num_bones * sizeof(mdr_bone_t);
			float *infloat, *outfloat;
			infloat = (float *)&inframe->bones;
			outfloat = (float *)&outframe->bones;
			for (j = 0; j < float_count; j ++)
			{
				outfloat[j] = LittleFloat(infloat[j]);
			}
		}
	}

	mdr_lod_t *inlod;
	inlod = (mdr_lod_t*)(buffer + pinmodel.ofs_lods);

	meshofs = inlod->ofs_surfaces;
	for (i = 0; i < inlod->num_surfaces; i++)
	{
		mdr_surface_t* insurf;

		insurf = (mdr_surface_t*)((char*)inlod + meshofs);
		num_tris += LittleLong(insurf->num_tris);
		num_xyz += LittleLong(insurf->num_verts);
		meshofs += LittleLong(insurf->ofs_end);
	}

	num_skins = inlod->num_surfaces;

	/* (count vert + 3 vert * (2 float + 1 int)) + final zero; */
	num_glcmds = (10 * num_tris) + 1 * inlod->num_surfaces;

	framesize = sizeof(daliasxframe_t) + sizeof(dxtrivertx_t) * num_xyz;
	ofs_skins = sizeof(dmdx_t);
	ofs_frames = ofs_skins + num_skins * MAX_SKINNAME;
	ofs_glcmds = ofs_frames + framesize * pinmodel.num_frames;
	ofs_meshes = ofs_glcmds + num_glcmds * sizeof(int);
	ofs_tris = ofs_meshes + inlod->num_surfaces * sizeof(dmdxmesh_t);
	ofs_st = ofs_tris + num_tris * sizeof(dtriangle_t);
	ofs_end = ofs_st + num_xyz * sizeof(dstvert_t);

	*numskins = num_skins;
	extradata = Hunk_Begin(ofs_end + Q_max(*numskins, MAX_MD2SKINS) * sizeof(struct image_s *));
	pheader = Hunk_Alloc(ofs_end);
	*skins = Hunk_Alloc((*numskins) * sizeof(struct image_s *));

	pheader->framesize = framesize;
	pheader->skinheight = 256;
	pheader->skinwidth = 256;
	pheader->num_skins = *numskins;
	pheader->num_glcmds = num_glcmds;
	pheader->num_frames = pinmodel.num_frames;
	pheader->num_xyz = num_xyz;
	pheader->num_meshes = inlod->num_surfaces;
	pheader->num_st = num_xyz;
	pheader->num_tris = num_tris;
	pheader->ofs_meshes = ofs_meshes;
	pheader->ofs_skins = ofs_skins;
	pheader->ofs_st = ofs_st;
	pheader->ofs_tris = ofs_tris;
	pheader->ofs_frames = ofs_frames;
	pheader->ofs_glcmds = ofs_glcmds;
	pheader->ofs_end = ofs_end;

	mesh_nodes = (dmdxmesh_t *)((byte *)pheader + pheader->ofs_meshes);
	tris = (dtriangle_t*)((byte *)pheader + pheader->ofs_tris);
	st = (dstvert_t*)((byte *)pheader + pheader->ofs_st);
	vertx = malloc(pinmodel.num_frames * pheader->num_xyz * sizeof(dmdx_vert_t));
	skin = (char *)pheader + pheader->ofs_skins;

	num_xyz = 0;
	num_tris = 0;
	meshofs = inlod->ofs_surfaces;

	for (i = 0; i < inlod->num_surfaces; i++)
	{
		mdr_surface_t* insurf;
		const int *p;
		int j;

		insurf = (mdr_surface_t*)((char*)inlod + meshofs);

		/* load shaders */
		memcpy(skin, insurf->shader, Q_min(sizeof(insurf->shader), MAX_SKINNAME));
		skin += MAX_SKINNAME;

		/* load triangles */
		p = (const int*)((byte*)insurf + LittleLong(insurf->ofs_tris));

		mesh_nodes[i].ofs_tris = num_tris;
		mesh_nodes[i].num_tris = LittleLong(insurf->num_tris);

		for (j = 0; j < mesh_nodes[i].num_tris; j++)
		{
			int k;

			for (k = 0; k < 3; k++)
			{
				int vert_id;

				/* index */
				vert_id = LittleLong(p[j * 3 + k]) + num_xyz;
				tris[num_tris + j].index_xyz[k] = vert_id;
				tris[num_tris + j].index_st[k] = vert_id;
			}
		}

		/* load vertex */
		mdr_vertex_t * inVert = (mdr_vertex_t *)((char*)insurf + insurf->ofs_verts);
		for (j = 0; j < insurf->num_verts; j ++)
		{
			int f;

			st[j + num_xyz].s = LittleFloat(inVert->texcoords[0]) * pheader->skinwidth;
			st[j + num_xyz].t = LittleFloat(inVert->texcoords[1]) * pheader->skinheight;

			for (f = 0; f < pinmodel.num_frames; f ++)
			{
				int k, vert_pos;
				mdr_frame_t *outframe = (mdr_frame_t *)(frames + f * unframesize);
				vec3_t tempVert, tempNormal;
				mdr_weight_t *w;

				vert_pos = num_xyz + f * pheader->num_xyz + j;

				VectorClear(tempVert);
				VectorClear(tempNormal);

				w = inVert->weights;
				for ( k = 0; k < LittleLong(inVert->num_weights); k++, w++)
				{
					mdr_bone_t *bone;
					float bone_weight;
					vec3_t innorm, inoffset;
					int n;

					bone = outframe->bones + LittleLong(w->bone_index);

					VectorCopy(w->offset, inoffset);
					VectorCopy(inVert->normal, innorm);
					for (n = 0; n < 3; n++)
					{
						inoffset[n] = LittleFloat(inoffset[n]);
						innorm[n] = LittleFloat(innorm[n]);
					}
					bone_weight = LittleFloat(w->bone_weight);

					for (n = 0; n < 3; n++)
					{
						tempVert[n] += bone_weight * (DotProduct(bone->matrix[n], inoffset) + bone->matrix[n][3]);
						tempNormal[n] += bone_weight * DotProduct(bone->matrix[n], innorm);
					}
				}

				VectorCopy(tempVert, vertx[vert_pos].xyz);
				VectorCopy(tempNormal, vertx[vert_pos].norm);
			}
			inVert = (mdr_vertex_t *)((char *)inVert +
				sizeof(mdr_vertex_t) + sizeof(mdr_weight_t) * (inVert->num_weights - 1));
		}

		num_tris += LittleLong(insurf->num_tris);
		num_xyz += LittleLong(insurf->num_verts);
		meshofs += LittleLong(insurf->ofs_end);
	}

	for (i = 0; i < pheader->num_frames; i ++)
	{
		daliasxframe_t *frame = (daliasxframe_t *)(
			(byte *)pheader + pheader->ofs_frames + i * pheader->framesize);
		const mdr_frame_t *outframe = (mdr_frame_t *)(frames + i * unframesize);

		if (outframe->name[0])
		{
			strncpy(frame->name, outframe->name, sizeof(frame->name) - 1);
		}
		else
		{
			snprintf(frame->name, 15, "%d", i);
		}

		PrepareFrameVertex(vertx + i * pheader->num_xyz,
			pheader->num_xyz, frame);
	}

	free(vertx);
	free(frames);

	Mod_LoadCmdGenerate(pheader);

	Mod_LoadFixImages(mod_name, pheader, false);

	*type = mod_alias;

	return extradata;
}
