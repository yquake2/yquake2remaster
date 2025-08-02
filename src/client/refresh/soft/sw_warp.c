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
 * Warps. Used on water surfaces und for skybox rotation.
 *
 * =======================================================================
 */

#include "header/local.h"

static vec3_t skyaxis;

/* 3dstudio environment map names */
static const char	*suf[6] = {"rt", "bk", "lf", "ft", "up", "dn"};
static const int	r_skysideimage[6] = {5, 2, 4, 1, 0, 3};
extern mtexinfo_t		r_skytexinfo[6];

void
RE_SetSky(const char *name, float rotate, int autorotate, const vec3_t axis)
{
	char	skyname[MAX_QPATH];
	int		i;

	Q_strlcpy(skyname, name, sizeof(skyname));
	VectorCopy(axis, skyaxis);

	for (i = 0; i < 6; i++)
	{
		image_t	*image;

		image = (image_t *)GetSkyImage(skyname, suf[r_skysideimage[i]],
			r_palettedtexture->value, (findimage_t)R_FindImage);

		if (!image)
		{
			Com_Printf("%s: can't load %s:%s sky\n",
				__func__, skyname, suf[r_skysideimage[i]]);
			image = r_notexture_mip;
		}

		r_skytexinfo[i].image = image;
	}
}
