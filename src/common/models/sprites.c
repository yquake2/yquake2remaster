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
 * The sprites file format
 *
 * =======================================================================
 */

#include "models.h"

/*
=================
Mod_LoadSprite_SP2

support for .sp2 sprites
=================
*/
void *
Mod_LoadSprite_SP2(const char *mod_name, const void *buffer, int modfilelen)
{
	const dsprite_t *sprin;
	dsprite_t *sprout;
	int i, numframes;
	void *extradata;

	sprin = (dsprite_t *)buffer;
	numframes = LittleLong(sprin->numframes);

	extradata = Hunk_Begin(modfilelen);
	sprout = Hunk_Alloc(modfilelen);

	sprout->ident = LittleLong(sprin->ident);
	sprout->version = LittleLong(sprin->version);
	sprout->numframes = numframes;

	if (sprout->version != SPRITE_VERSION)
	{
		Com_Printf("%s has wrong version number (%i should be %i)\n",
				mod_name, sprout->version, SPRITE_VERSION);
		return NULL;
	}

	/* byte swap everything */
	for (i = 0; i < sprout->numframes; i++)
	{
		sprout->frames[i].width = LittleLong(sprin->frames[i].width);
		sprout->frames[i].height = LittleLong(sprin->frames[i].height);
		sprout->frames[i].origin_x = LittleLong(sprin->frames[i].origin_x);
		sprout->frames[i].origin_y = LittleLong(sprin->frames[i].origin_y);
		memcpy(sprout->frames[i].name, sprin->frames[i].name, MAX_SKINNAME);
	}

	for (i = 0; i < sprout->numframes; i++)
	{
		Com_DPrintf("%s: %s #%d: Should load external '%s'\n",
			__func__, mod_name, i,
			sprout->frames[i].name);
	}

	return extradata;
}

/*
=================
Mod_LoadSprite_SPR

support for .spr sprites
=================
*/
void *
Mod_LoadSprite_SPR(const char *mod_name, const void *buffer, int modfilelen)
{
	const dq1sprite_t pinsprite;
	const byte *curr_pos;
	dsprite_t *sprout;
	void *extradata;
	int i, size;

	if (modfilelen < sizeof(pinsprite))
	{
		Com_Printf("%s: %s has incorrect header size (%i should be " YQ2_COM_PRIdS ")\n",
				__func__, mod_name, modfilelen, sizeof(pinsprite));
		return NULL;
	}

	Mod_LittleHeader((int *)buffer, sizeof(pinsprite) / sizeof(int),
		(int *)&pinsprite);

	if (pinsprite.version != IDQ1SPRITE_VERSION)
	{
		Com_Printf("%s: %s has wrong version number (%i should be %i)\n",
				__func__, mod_name, pinsprite.version, ALIAS_VERSION);
		return NULL;
	}

	size = sizeof(dsprite_t) + sizeof(dsprframe_t) * (pinsprite.nframes - 1);
	extradata = Hunk_Begin(size);
	sprout = Hunk_Alloc(size);

	sprout->ident = IDSPRITEHEADER;
	sprout->version = SPRITE_VERSION;
	sprout->numframes = pinsprite.nframes;

	curr_pos = (byte*)buffer + sizeof(dq1sprite_t);
	for (i = 0; i < pinsprite.nframes; i++)
	{
		int skin_type;

		/* skip type / int */
		/* 0 = simple, !0 = group */
		/* this program can't read models composed of group frames! */
		skin_type = LittleLong(((int *)curr_pos)[0]);
		if (skin_type)
		{
			Com_Printf("%s: model %s has unsupported skin type %d\n",
				__func__, mod_name, skin_type);
			return NULL;
		}

		snprintf(sprout->frames[i].name, sizeof(sprout->frames[i].name),
			"%s#%d.lmp", mod_name, i);
		sprout->frames[i].origin_x = LittleLong(((int *)curr_pos)[1]);
		sprout->frames[i].origin_y = LittleLong(((int *)curr_pos)[2]);
		sprout->frames[i].width = LittleLong(((int *)curr_pos)[3]);
		sprout->frames[i].height = LittleLong(((int *)curr_pos)[4]);

		curr_pos += sizeof(int) * 5 + (
			sprout->frames[i].width * sprout->frames[i].height
		);

		if (curr_pos > ((byte *)buffer + modfilelen))
		{
			Com_Printf("%s: model %s is too short\n",
				__func__, mod_name);
			return NULL;
		}
	}

	for (i = 0; i < sprout->numframes; i++)
	{
		Com_DPrintf("%s: %s #%d: Should load internal '%s'\n",
			__func__, mod_name, i,
			sprout->frames[i].name);
	}

	return extradata;
}
