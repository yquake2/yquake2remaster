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
 * =======================================================================
 *
 * Header for the model stuff.
 *
 * =======================================================================
 */

#ifndef REF_MODEL_H
#define REF_MODEL_H

/*

d*_t structures are on-disk representations
m*_t structures are in-memory

*/

/*
==============================================================================

BRUSH MODELS

==============================================================================
*/

//
// in memory representation
//

/* Whole model */

typedef struct model_s
{
	/* shared model definition */
	smodel_t s;

	int numsubmodels;
	struct model_s *submodels;

	/* for alias models and skins */
	struct image_s **skins;
	int numskins;
} model_t;

void Mod_Init(void);
const byte *Mod_ClusterPVS(int cluster, const model_t *model);

void Mod_Modellist_f(void);

void *Hunk_Begin(int maxsize);
void *Hunk_Alloc(int size);
int Hunk_End(void);
void Hunk_Free(void *base);

void Mod_FreeAll(void);
void	Mod_FreeModelsKnown (void);

#endif
