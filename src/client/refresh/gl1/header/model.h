/*
 * Copyright (C) 1997-2001 Id Software, Inc.
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
void Mod_ClearAll(void);

void Mod_Modellist_f(void);
void Mod_FreeAll(void);

#endif
