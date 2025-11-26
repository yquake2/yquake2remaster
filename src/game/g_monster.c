/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (c) ZeniMax Media Inc.
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
 * Monster utility functions. While unused by the CTF code they must
 * persist here since most of the other game codes has ties to it.
 *
 * =======================================================================
 */

#include "header/local.h"

void monster_start_go(edict_t *self);

/* Monster weapons */

static void
monster_muzzleflash2(edict_t *self, vec3_t start, int flashtype)
{
	gi.WriteByte(svc_muzzleflash2);
	gi.WriteShort(self - g_edicts);
	if (flashtype < 255)
	{
		gi.WriteByte(flashtype);
	}
	else
	{
		/* hact to support effects 255 .. 511 */
		gi.WriteByte(255);
		gi.WriteByte(flashtype - 255);
	}
	gi.multicast(start, MULTICAST_PVS);
}

void
monster_fire_bullet(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int kick, int hspread, int vspread, int flashtype)
{
	if (!self)
	{
		return;
	}

	fire_bullet(self, start, dir, damage, kick, hspread, vspread, MOD_UNKNOWN);

	monster_muzzleflash2(self, start, flashtype);
}

void
monster_fire_shotgun(edict_t *self, vec3_t start, vec3_t aimdir, int damage,
		int kick, int hspread, int vspread, int count, int flashtype)
{
	if (!self)
	{
		return;
	}

	fire_shotgun(self, start, aimdir, damage, kick, hspread,
			vspread, count, MOD_UNKNOWN);

	monster_muzzleflash2(self, start, flashtype);
}

void
monster_fire_blaster(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int flashtype, int effect)
{
	if (!self)
	{
		return;
	}

	fire_blaster(self, start, dir, damage, speed, effect, false);

	monster_muzzleflash2(self, start, flashtype);
}

void
monster_fire_blueblaster(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int flashtype, int effect)
{
	if (!self)
	{
		return;
	}

	fire_blueblaster(self, start, dir, damage, speed, effect);

	monster_muzzleflash2(self, start, MZ_BLUEHYPERBLASTER);
}

void
monster_fire_blaster2(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int flashtype, int effect)
{
	if (!self)
	{
		return;
	}

	fire_blaster2(self, start, dir, damage, speed, effect, false);

	monster_muzzleflash2(self, start, flashtype);
}

void
monster_fire_ionripper(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int flashtype, int effect)
{
	if (!self)
	{
		return;
	}

	fire_ionripper(self, start, dir, damage, speed, effect);

	monster_muzzleflash2(self, start, flashtype);
}

void
monster_fire_heat(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, int flashtype)
{
	if (!self)
	{
		return;
	}

	fire_heat(self, start, dir, damage, speed, damage, damage);

	monster_muzzleflash2(self, start, flashtype);
}

void
monster_fire_tracker(edict_t *self, vec3_t start, vec3_t dir, int damage,
		int speed, edict_t *enemy, int flashtype)
{
	if (!self || !enemy)
	{
		return;
	}

	fire_tracker(self, start, dir, damage, speed, enemy);

	monster_muzzleflash2(self, start, flashtype);
}

void
monster_fire_heatbeam(edict_t *self, vec3_t start, vec3_t dir, vec3_t offset,
		int damage, int kick, int flashtype)
{
	if (!self)
	{
		return;
	}

	fire_heatbeam(self, start, dir, offset, damage, kick, true);

	monster_muzzleflash2(self, start, flashtype);
}

void
dabeam_hit(edict_t *self)
{
	edict_t *ignore;
	vec3_t start;
	vec3_t end;
	trace_t tr;

	if (!self)
	{
		return;
	}

	ignore = self;
	VectorCopy(self->s.origin, start);
	VectorMA(start, 2048, self->movedir, end);

	while (1)
	{
		tr = gi.trace(start, NULL, NULL, end, ignore,
				CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_DEADMONSTER);

		if (!tr.ent)
		{
			break;
		}

		/* hurt it if we can */
		if ((tr.ent->takedamage) && !(tr.ent->flags & FL_IMMUNE_LASER) &&
			(tr.ent != self->owner))
		{
			T_Damage(tr.ent, self, self->owner, self->movedir, tr.endpos,
					vec3_origin, self->dmg, skill->value, DAMAGE_ENERGY,
					MOD_TARGET_LASER);
		}

		if (self->dmg < 0) /* healer ray */
		{
			/* when player is at 100 health
			   just undo health fix */
			if (tr.ent->client && (tr.ent->health > 100))
			{
				tr.ent->health += self->dmg;
			}
		}

		/* if we hit something that's not a monster or
		   player or is immune to lasers, we're done */
		if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client))
		{
			if (self->spawnflags & 0x80000000)
			{
				self->spawnflags &= ~0x80000000;
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_LASER_SPARKS);
				gi.WriteByte(10);
				gi.WritePosition(tr.endpos);
				gi.WriteDir(tr.plane.normal);
				gi.WriteByte(self->s.skinnum);
				gi.multicast(tr.endpos, MULTICAST_PVS);
			}

			break;
		}

		ignore = tr.ent;
		VectorCopy(tr.endpos, start);
	}

	VectorCopy(tr.endpos, self->s.old_origin);
	self->nextthink = level.time + 0.1;
	self->think = G_FreeEdict;
}

void
monster_dabeam(edict_t *self)
{
	vec3_t last_movedir;
	vec3_t point;

	if (!self)
	{
		return;
	}

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->s.renderfx |= RF_BEAM | RF_TRANSLUCENT;
	self->s.modelindex = 1;

	self->s.frame = 2;

	if (self->owner->monsterinfo.aiflags & AI_MEDIC)
	{
		self->s.skinnum = 0xf3f3f1f1;
	}
	else
	{
		self->s.skinnum = 0xf2f2f0f0;
	}

	if (self->enemy)
	{
		VectorCopy(self->movedir, last_movedir);
		VectorMA(self->enemy->absmin, 0.5, self->enemy->size, point);

		if (self->owner->monsterinfo.aiflags & AI_MEDIC)
		{
			point[0] += sin(level.time) * 8;
		}

		VectorSubtract(point, self->s.origin, self->movedir);
		VectorNormalize(self->movedir);

		if (!VectorCompare(self->movedir, last_movedir))
		{
			self->spawnflags |= 0x80000000;
		}
	}
	else
	{
		G_SetMovedir(self->s.angles, self->movedir);
	}

	self->think = dabeam_hit;
	self->nextthink = level.time + 0.1;
	VectorSet(self->mins, -8, -8, -8);
	VectorSet(self->maxs, 8, 8, 8);
	gi.linkentity(self);

	self->spawnflags |= 0x80000001;
	self->svflags &= ~SVF_NOCLIENT;
}

void
monster_fire_grenade(edict_t *self, vec3_t start, vec3_t aimdir,
		int damage, int speed, int flashtype)
{
	if (!self)
	{
		return;
	}

	fire_grenade(self, start, aimdir, damage, speed, 2.5, damage + 40);

	monster_muzzleflash2(self, start, flashtype);
}

void
monster_fire_rocket(edict_t *self, vec3_t start, vec3_t dir,
		int damage, int speed, int flashtype)
{
	if (!self)
	{
		return;
	}

	fire_rocket(self, start, dir, damage, speed, damage + 20, damage);

	monster_muzzleflash2(self, start, flashtype);
}

void
monster_fire_railgun(edict_t *self, vec3_t start, vec3_t aimdir,
		int damage, int kick, int flashtype)
{
	if (!self)
	{
		return;
	}

	if (!(gi.pointcontents(start) & MASK_SOLID))
	{
		fire_rail(self, start, aimdir, damage, kick);
	}

	monster_muzzleflash2(self, start, flashtype);
}

void
monster_fire_bfg(edict_t *self, vec3_t start, vec3_t aimdir,
		int damage, int speed, int kick /* unused */, float damage_radius,
		int flashtype)
{
	if (!self)
	{
		return;
	}

	fire_bfg(self, start, aimdir, damage, speed, damage_radius);

	monster_muzzleflash2(self, start, flashtype);
}

/* ================================================================== */

/* Monster utility functions */

void
M_FliesOff(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->s.effects &= ~EF_FLIES;
	self->s.sound = 0;
}

void
M_FliesOn(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (self->waterlevel)
	{
		return;
	}

	self->s.effects |= EF_FLIES;
	self->s.sound = gi.soundindex("infantry/inflies1.wav");
	self->think = M_FliesOff;
	self->nextthink = level.time + 60;
}

void
M_FlyCheck(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (self->waterlevel)
	{
		return;
	}

	if (random() > 0.5)
	{
		return;
	}

	self->think = M_FliesOn;
	self->nextthink = level.time + 5 + 10 * random();
}

void
AttackFinished(edict_t *self, float time)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.attack_finished = level.time + time;
}

void
M_CheckGround(edict_t *ent)
{
	vec3_t point;
	trace_t trace;

	if (!ent)
	{
		return;
	}

	if (ent->flags & (FL_SWIM | FL_FLY))
	{
		return;
	}

	if ((ent->velocity[2] * ent->gravityVector[2]) < -100)
	{
		ent->groundentity = NULL;
		return;
	}

	/* if the hull point one-quarter unit down
	   is solid the entity is on ground */
	point[0] = ent->s.origin[0];
	point[1] = ent->s.origin[1];
	point[2] = ent->s.origin[2] + (0.25 * ent->gravityVector[2]);

	trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, point,
			ent, MASK_MONSTERSOLID);

	/* check steepness */
	if ((trace.plane.normal[2] < 0.7) && !trace.startsolid)
	{
		ent->groundentity = NULL;
		return;
	}

	if (!trace.startsolid && !trace.allsolid)
	{
		VectorCopy(trace.endpos, ent->s.origin);
		ent->groundentity = trace.ent;
		ent->groundentity_linkcount = trace.ent->linkcount;
		ent->velocity[2] = 0;
	}
}

void
M_CatagorizePosition(edict_t *ent)
{
	vec3_t point;
	int cont;

	if (!ent)
	{
		return;
	}

	/* get waterlevel */
	point[0] = (ent->absmax[0] + ent->absmin[0])/2;
	point[1] = (ent->absmax[1] + ent->absmin[1])/2;
	point[2] = ent->absmin[2] + 2;
	cont = gi.pointcontents(point);

	if (!(cont & MASK_WATER))
	{
		ent->waterlevel = 0;
		ent->watertype = 0;
		return;
	}

	ent->watertype = cont;
	ent->waterlevel = 1;
	point[2] += 26;
	cont = gi.pointcontents(point);

	if (!(cont & MASK_WATER))
	{
		return;
	}

	ent->waterlevel = 2;
	point[2] += 22;
	cont = gi.pointcontents(point);

	if (cont & MASK_WATER)
	{
		ent->waterlevel = 3;
	}
}

void
M_WorldEffects(edict_t *ent)
{
	int dmg;

	if (!ent)
	{
		return;
	}

	if (ent->health > 0)
	{
		if (!(ent->flags & FL_SWIM))
		{
			if (ent->waterlevel < 3)
			{
				ent->air_finished = level.time + 12;
			}
			else if (ent->air_finished < level.time)
			{
				/* drown! */
				if (ent->pain_debounce_time < level.time)
				{
					dmg = 2 + 2 * floor(level.time - ent->air_finished);

					if (dmg > 15)
					{
						dmg = 15;
					}

					T_Damage(ent, world, world, vec3_origin, ent->s.origin,
							vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
					ent->pain_debounce_time = level.time + 1;
				}
			}
		}
		else
		{
			if (ent->waterlevel > 0)
			{
				ent->air_finished = level.time + 9;
			}
			else if (ent->air_finished < level.time)
			{
				/* suffocate! */
				if (ent->pain_debounce_time < level.time)
				{
					dmg = 2 + 2 * floor(level.time - ent->air_finished);

					if (dmg > 15)
					{
						dmg = 15;
					}

					T_Damage(ent, world, world, vec3_origin, ent->s.origin,
							vec3_origin, dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
					ent->pain_debounce_time = level.time + 1;
				}
			}
		}
	}

	if (ent->waterlevel == 0)
	{
		if (ent->flags & FL_INWATER)
		{
			gi.sound(ent, CHAN_BODY, gi.soundindex(
							"player/watr_out.wav"), 1, ATTN_NORM, 0);
			ent->flags &= ~FL_INWATER;
		}

		return;
	}

	if ((ent->watertype & CONTENTS_LAVA) && !(ent->flags & FL_IMMUNE_LAVA))
	{
		if (ent->damage_debounce_time < level.time)
		{
			ent->damage_debounce_time = level.time + 0.2;
			T_Damage(ent, world, world, vec3_origin, ent->s.origin,
					vec3_origin, 10 * ent->waterlevel, 0, 0, MOD_LAVA);
		}
	}

	if ((ent->watertype & CONTENTS_SLIME) && !(ent->flags & FL_IMMUNE_SLIME) && !(ent->svflags & SVF_DEADMONSTER))
	{
		if (ent->damage_debounce_time < level.time)
		{
			ent->damage_debounce_time = level.time + 1;
			T_Damage(ent, world, world, vec3_origin, ent->s.origin,
					vec3_origin, 4 * ent->waterlevel, 0, 0, MOD_SLIME);
		}
	}

	if (!(ent->flags & FL_INWATER))
	{
		if (!(ent->svflags & SVF_DEADMONSTER))
		{
			if (ent->watertype & CONTENTS_LAVA)
			{
				if (random() <= 0.5)
				{
					gi.sound(ent, CHAN_BODY, gi.soundindex(
									"player/lava1.wav"), 1, ATTN_NORM, 0);
				}
				else
				{
					gi.sound(ent, CHAN_BODY, gi.soundindex(
									"player/lava2.wav"), 1, ATTN_NORM, 0);
				}
			}
			else if (ent->watertype & CONTENTS_SLIME)
			{
				gi.sound(ent, CHAN_BODY, gi.soundindex(
								"player/watr_in.wav"), 1, ATTN_NORM, 0);
			}
			else if (ent->watertype & CONTENTS_WATER)
			{
				gi.sound(ent, CHAN_BODY, gi.soundindex(
								"player/watr_in.wav"), 1, ATTN_NORM, 0);
			}
		}

		ent->flags |= FL_INWATER;
		ent->damage_debounce_time = 0;
	}
}

void
M_droptofloor(edict_t *ent)
{
	vec3_t end;
	trace_t trace;

	if (!ent)
	{
		return;
	}

	if (ent->gravityVector[2] < 0)
	{
		ent->s.origin[2] += 1;
		VectorCopy(ent->s.origin, end);
		end[2] -= 256;
	}
	else
	{
		ent->s.origin[2] -= 1;
		VectorCopy(ent->s.origin, end);
		end[2] += 256;
	}

	trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, end,
			ent, MASK_MONSTERSOLID);

	if ((trace.fraction == 1) || trace.allsolid)
	{
		return;
	}

	VectorCopy(trace.endpos, ent->s.origin);

	gi.linkentity(ent);
	M_CheckGround(ent);
	M_CatagorizePosition(ent);
}

void
M_SetEffects(edict_t *ent)
{
	int remaining;

	if (!ent)
	{
		return;
	}

	ent->s.effects &= ~(EF_COLOR_SHELL | EF_POWERSCREEN | EF_DOUBLE | EF_QUAD | EF_PENT);
	ent->s.renderfx &= ~(RF_SHELL_RED | RF_SHELL_GREEN | RF_SHELL_BLUE | RF_SHELL_DOUBLE);

	if (ent->monsterinfo.aiflags & AI_RESURRECTING)
	{
		ent->s.effects |= EF_COLOR_SHELL;
		ent->s.renderfx |= RF_SHELL_RED;
	}

	if (ent->health <= 0)
	{
		return;
	}

	if (ent->powerarmor_time > level.time)
	{
		if (ent->monsterinfo.power_armor_type == POWER_ARMOR_SCREEN)
		{
			ent->s.effects |= EF_POWERSCREEN;
		}
		else if (ent->monsterinfo.power_armor_type == POWER_ARMOR_SHIELD)
		{
			ent->s.effects |= EF_COLOR_SHELL;
			ent->s.renderfx |= RF_SHELL_GREEN;
		}
	}

	if (ent->monsterinfo.quad_framenum > level.framenum)
	{
		remaining = ent->monsterinfo.quad_framenum - level.framenum;

		if ((remaining > 30) || (remaining & 4))
		{
			ent->s.effects |= EF_QUAD;
		}
	}
	else
	{
		ent->s.effects &= ~EF_QUAD;
	}

	if (ent->monsterinfo.double_framenum > level.framenum)
	{
		remaining = ent->monsterinfo.double_framenum - level.framenum;

		if ((remaining > 30) || (remaining & 4))
		{
			ent->s.effects |= EF_DOUBLE;
		}
	}
	else
	{
		ent->s.effects &= ~EF_DOUBLE;
	}

	if (ent->monsterinfo.invincible_framenum > level.framenum)
	{
		remaining = ent->monsterinfo.invincible_framenum - level.framenum;

		if ((remaining > 30) || (remaining & 4))
		{
			ent->s.effects |= EF_PENT;
		}
	}
	else
	{
		ent->s.effects &= ~EF_PENT;
	}
}

static int
M_GetModelIndex(edict_t *self)
{
	int modelindex;

	modelindex = self->s.modelindex;

	if (modelindex == CUSTOM_PLAYER_MODEL)
	{
		char skinname[MAX_QPATH], modelname[MAX_QPATH];
		char *s;

		/* get selected player model from skin */
		Q_strlcpy(skinname, Info_ValueForKey(
			self->client->pers.userinfo, "skin"), sizeof(skinname));

		s = skinname;

		while(*s)
		{
			if (*s == '/')
			{
				*s = 0;
				break;
			}
			s++;
		}

		Com_sprintf(modelname, sizeof(modelname), "players/%s/tris.md2",
			skinname);
		modelindex = gi.modelindex(modelname);
	}

	return modelindex;
}

static qboolean
M_SetAnimGroupFrameValuesInt(edict_t *self, const char *name,
	int *ofs_frames, int *num_frames, int select)
{
	const dmdxframegroup_t * frames;
	int num, i, modelindex, skipcount;

	skipcount = 0;
	modelindex = M_GetModelIndex(self);
	frames = gi.GetModelInfo(modelindex, &num, NULL, NULL);
	if (select)
	{
		int count = 0;

		for (i = 0; i < num; i++)
		{
			if (!strcmp(frames[i].name, name))
			{
				count++;
			}
		}

		if (count > 1)
		{
			/* use random if select is negative */
			if (select < 0)
			{
				skipcount = randk() % count;
			}
			/* use exact group if positive */
			else
			{
				skipcount = select % count;
			}
		}
	}

	for (i = 0; i < num; i++)
	{
		if (!strcmp(frames[i].name, name))
		{
			if (skipcount)
			{
				skipcount--;
				continue;
			}

			*ofs_frames = frames[i].ofs;
			*num_frames = frames[i].num;
			return true;
		}
	}

	return false;
}

/* Use multy to select random group from same names */
void
M_SetAnimGroupFrameValues(edict_t *self, const char *name,
	int *ofs_frames, int *num_frames, int select)
{
	if (M_SetAnimGroupFrameValuesInt(self, name, ofs_frames, num_frames, select))
	{
		return;
	}

	if (!strcmp(name, "stand"))
	{
		/* no stand animations */
		M_SetAnimGroupFrameValuesInt(self, "idle", ofs_frames, num_frames, select);
	}
	else if (!strcmp(name, "crstnd"))
	{
		/* no crstnd animations */
		if (M_SetAnimGroupFrameValuesInt(self, "crwalk", ofs_frames, num_frames, select))
		{
			if (*num_frames > 1)
			{
				*num_frames = 1;
			}
		}
	}
	else if (!strcmp(name, "crpain"))
	{
		/* no crpain animations */
		M_SetAnimGroupFrameValuesInt(self, "pain", ofs_frames, num_frames, select);
	}
	else if (!strcmp(name, "flipoff"))
	{
		/* no flipoff animations */
		M_SetAnimGroupFrameValuesInt(self, "flip", ofs_frames, num_frames, select);
	}
	else if (!strcmp(name, "dodge"))
	{
		/* no dodge animations */
		M_SetAnimGroupFrameValuesInt(self, "duck", ofs_frames, num_frames, select);
	}
	else if (!strcmp(name, "swim"))
	{
		if (!M_SetAnimGroupFrameValuesInt(self, "run", ofs_frames, num_frames, select))
		{
			/* no swim -> run -> walk animations */
			M_SetAnimGroupFrameValuesInt(self, "walk", ofs_frames, num_frames, select);
		}
	}
	else if (!strcmp(name, "run") ||
			!strcmp(name, "fly"))
	{
		/* no run, fly, swim animations */
		M_SetAnimGroupFrameValuesInt(self, "walk", ofs_frames, num_frames, select);
	}
}

static void
M_FixStuckMonster(edict_t *self)
{
	trace_t tr;

	tr = gi.trace(self->s.origin, self->mins, self->maxs, self->s.origin, self, MASK_SOLID);
	if (!tr.startsolid)
	{
		return;
	}

	FixEntityPosition(self);

	tr = gi.trace(self->s.origin, self->mins, self->maxs, self->s.origin, self, MASK_SOLID);

	if (tr.startsolid)
	{
		gi.dprintf("%s: %s stuck in solid at %s\n",
				__func__,
				self->classname,
				vtos(self->s.origin));
	}
}

void
M_SetAnimGroupFrame(edict_t *self, const char *name, qboolean fixpos)
{
	int i, ofs_frames = 0, num_frames = 1;

	if (!self || !name)
	{
		return;
	}

	M_SetAnimGroupFrameValues(self, name, &ofs_frames, &num_frames, 0);

	i = self->s.frame - ofs_frames;

	if (i < 0)
	{
		i = 0;
	}
	i++;

	self->s.frame = ofs_frames + i % num_frames;

	if (fixpos)
	{
		gi.GetModelFrameInfo(self->s.modelindex, self->s.frame,
			self->mins, self->maxs);
		M_FixStuckMonster(self);
	}
}

static void
M_MoveFrame(edict_t *self)
{
	mmove_t *move;
	int index, firstframe, lastframe;

	if (!self)
	{
		return;
	}

	move = self->monsterinfo.currentmove;
	if (move)
	{
		firstframe = move->firstframe;
		lastframe = move->lastframe;
	}
	else if (self->monsterinfo.action)
	{
		firstframe = self->monsterinfo.firstframe;
		lastframe = self->monsterinfo.lastframe;
		lastframe += firstframe - 1;
	}
	else
	{
		return;
	}

	self->nextthink = level.time + FRAMETIME;

	if ((self->monsterinfo.nextframe) &&
		(self->monsterinfo.nextframe >= firstframe) &&
		(self->monsterinfo.nextframe <= lastframe))
	{
		if (self->s.frame != self->monsterinfo.nextframe)
		{
			self->s.frame = self->monsterinfo.nextframe;
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
		}

		self->monsterinfo.nextframe = 0;
	}
	else
	{
		/* prevent nextframe from leaking into a future move */
		self->monsterinfo.nextframe = 0;

		if (self->s.frame == lastframe)
		{
			if (move && move->endfunc)
			{
				move->endfunc(self);

				/* regrab move, endfunc is very likely to change it */
				move = self->monsterinfo.currentmove;

				/* check for death */
				if (self->svflags & SVF_DEADMONSTER)
				{
					return;
				}
			}
			else if (self->monsterinfo.action &&
				self->monsterinfo.run &&
				(!strcmp(self->monsterinfo.action, "attack") ||
				 !strcmp(self->monsterinfo.action, "pain") ||
				 !strcmp(self->monsterinfo.action, "dodge") ||
				 !strcmp(self->monsterinfo.action, "melee")))
			{
				/* last frame in pain / attack go to run action */
				self->monsterinfo.run(self);
			}
			else if (self->monsterinfo.action &&
				!strcmp(self->monsterinfo.action, "death"))
			{
				/* last frame in pain go to death action */
				monster_dynamic_dead(self);
			}
		}

		if ((self->s.frame < firstframe) ||
			(self->s.frame > lastframe))
		{
			self->monsterinfo.aiflags &= ~AI_HOLD_FRAME;
			self->s.frame = firstframe;
		}
		else
		{
			if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			{
				self->s.frame++;

				if (self->s.frame > lastframe)
				{
					self->s.frame = firstframe;
				}
			}
		}
	}

	index = self->s.frame - firstframe;

	if (move && move->frame[index].aifunc)
	{
		if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
		{
			move->frame[index].aifunc(self,
					move->frame[index].dist * self->monsterinfo.scale);
		}
		else
		{
			move->frame[index].aifunc(self, 0);
		}
	}
	else if (self->monsterinfo.action)
	{
		if (!strcmp(self->monsterinfo.action, "run") ||
			!strcmp(self->monsterinfo.action, "swim") ||
			!strcmp(self->monsterinfo.action, "fly"))
		{
			if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			{
				ai_run(self,
					self->monsterinfo.run_dist * self->monsterinfo.scale);
			}
			else
			{
				ai_run(self, 0);
			}
		}
		else if (!strcmp(self->monsterinfo.action, "walk"))
		{
			if (!(self->monsterinfo.aiflags & AI_HOLD_FRAME))
			{
				ai_walk(self,
					self->monsterinfo.walk_dist * self->monsterinfo.scale);
			}
			else
			{
				ai_walk(self, 0);
			}
		}
		else if (!strcmp(self->monsterinfo.action, "stand") ||
			!strcmp(self->monsterinfo.action, "hover") ||
			!strcmp(self->monsterinfo.action, "idle"))
		{
			ai_stand(self, 0);
		}
		else if (!strcmp(self->monsterinfo.action, "pain") ||
			!strcmp(self->monsterinfo.action, "death") ||
			!strcmp(self->monsterinfo.action, "dodge"))
		{
			ai_move(self, 0);
		}
		else if (!strcmp(self->monsterinfo.action, "attack") ||
			!strcmp(self->monsterinfo.action, "melee"))
		{
			ai_charge(self, 0);
		}
	}

	if (move && move->frame[index].thinkfunc)
	{
		move->frame[index].thinkfunc(self);
	}
}

static void
monster_dynamic_setframes(edict_t *self, int select)
{
	if (!self || !self->monsterinfo.action)
	{
		return;
	}

	M_SetAnimGroupFrameValues(self, self->monsterinfo.action,
		&self->monsterinfo.firstframe, &self->monsterinfo.lastframe, select);
}

void
monster_dynamic_walk(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.currentmove = NULL;

	if (self->flags & FL_FLY)
	{
		self->monsterinfo.action = "fly";
	}
	else if (self->flags & FL_SWIM)
	{
		self->monsterinfo.action = "swim";
	}
	else
	{
		self->monsterinfo.action = "walk";
	}

	monster_dynamic_setframes(self, 0);
}

void
monster_dynamic_run(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.currentmove = NULL;

	if (self->flags & FL_FLY)
	{
		self->monsterinfo.action = "fly";
	}
	else if (self->flags & FL_SWIM)
	{
		self->monsterinfo.action = "swim";
	}
	else
	{
		self->monsterinfo.action = "run";
	}

	monster_dynamic_setframes(self, 0);
}

void
monster_dynamic_idle(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.currentmove = NULL;
	self->monsterinfo.action = "idle";
	monster_dynamic_setframes(self, -1);
}

void
monster_dynamic_attack(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.currentmove = NULL;
	self->monsterinfo.action = "attack";
	monster_dynamic_setframes(self, -1);
}

void
monster_dynamic_dead(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->movetype = MOVETYPE_TOSS;
	self->svflags |= SVF_DEADMONSTER;
	self->nextthink = 0;
	gi.linkentity(self);
}

void
monster_dynamic_die_noanim(edict_t *self, edict_t *inflictor, edict_t *attacker,
	int damage, vec3_t point)
{
	monster_dynamic_dead(self);
}

void
monster_dynamic_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
	int damage, vec3_t point)
{
	if (!self)
	{
		return;
	}

	if (self->health <= self->gib_health)
	{
		gi.sound(self, CHAN_VOICE, gi.soundindex("misc/udeath.wav"), 1, ATTN_NORM, 0);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowHead(self, "models/objects/gibs/head2/tris.md2", damage, GIB_ORGANIC);
		self->deadflag = DEAD_DEAD;
		return;
	}

	if (self->deadflag == DEAD_DEAD)
	{
		return;
	}

	self->deadflag = DEAD_DEAD;
	self->takedamage = DAMAGE_YES;

	self->monsterinfo.currentmove = NULL;
	self->monsterinfo.action = "death";
	monster_dynamic_setframes(self, -1);
}

void
monster_dynamic_melee(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.currentmove = NULL;
	self->monsterinfo.action = "melee";
	monster_dynamic_setframes(self, 0);
}

void
monster_dynamic_dodge(edict_t *self, edict_t *attacker, float eta,
	trace_t *tr /* unused */)
{
	if (!self || !attacker)
	{
		return;
	}

	if (random() > 0.25)
	{
		return;
	}

	if (!self->enemy)
	{
		self->enemy = attacker;
		FoundTarget(self);
	}

	self->monsterinfo.currentmove = NULL;
	self->monsterinfo.action = "dodge";
	monster_dynamic_setframes(self, 0);
}

void
monster_dynamic_pain_noanim(edict_t *self, edict_t *other /* unused */,
		float kick /* unused */, int damage)
{
}

void
monster_dynamic_pain(edict_t *self, edict_t *other /* unused */,
		float kick /* unused */, int damage)
{
	if (!self)
	{
		return;
	}

	if (skill->value == SKILL_HARDPLUS)
	{
		return; /* no pain anims in nightmare */
	}

	self->monsterinfo.currentmove = NULL;

	self->monsterinfo.action = "pain";
	monster_dynamic_setframes(self, -1);
}

void
monster_dynamic_stand(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.currentmove = NULL;

	if (self->flags & FL_FLY)
	{
		self->monsterinfo.action = "hover";
	}
	else if (self->flags & FL_SWIM)
	{
		self->monsterinfo.action = "swim";
	}
	else
	{
		self->monsterinfo.action = "stand";
	}

	monster_dynamic_setframes(self, 0);
}

void
monster_dynamic_search(edict_t *self)
{
}

void
monster_dynamic_sight(edict_t *self, edict_t *other /* unused */)
{
}

void
monster_dynamic_setinfo(edict_t *self)
{
	const dmdxframegroup_t * frames;
	int num;

	if (!self)
	{
		return;
	}

	self->monsterinfo.walk = monster_dynamic_walk;
	self->monsterinfo.run = monster_dynamic_run;
	self->monsterinfo.stand = monster_dynamic_stand;
	self->monsterinfo.search = monster_dynamic_search;
	self->monsterinfo.sight = monster_dynamic_sight;
	self->monsterinfo.attack = monster_dynamic_attack;
	self->pain = monster_dynamic_pain_noanim;
	self->die = monster_dynamic_die_noanim;

	/* Check frame names for optional move animation */
	frames = gi.GetModelInfo(self->s.modelindex, &num, NULL, NULL);
	if (frames && num)
	{
		size_t i;

		for (i = 0; i < num; i++)
		{
			if (!strcmp(frames[i].name, "idle"))
			{
				self->monsterinfo.idle = monster_dynamic_idle;
			}
			else if (!strcmp(frames[i].name, "pain"))
			{
				self->pain = monster_dynamic_pain;
			}
			else if (!strcmp(frames[i].name, "attack"))
			{
				self->monsterinfo.attack = monster_dynamic_attack;
			}
			else if (!strcmp(frames[i].name, "death"))
			{
				self->die = monster_dynamic_die;
			}
			else if (!strcmp(frames[i].name, "dodge") ||
				!strcmp(frames[i].name, "duck"))
			{
				self->monsterinfo.dodge = monster_dynamic_dodge;
			}
			else if (!strcmp(frames[i].name, "melee"))
			{
				self->monsterinfo.melee = monster_dynamic_melee;
			}
		}
	}
}

void
object_think(edict_t *self)
{
	M_SetAnimGroupFrame(self, self->monsterinfo.action, false);
	self->nextthink = level.time + FRAMETIME;
}

static const char *object_actions[] = {
	"banner",
	"flagg",
	"flame",
	"poly",
};

void
object_spawn(edict_t *self)
{
	const dmdxframegroup_t * frames;
	int i, num;

	if (!self)
	{
		return;
	}

	/* Check frame names for optional move animation */
	frames = gi.GetModelInfo(self->s.modelindex, &num, NULL, NULL);
	if (!frames || num != 1)
	{
		gi.dprintf("no known frame groups in %s\n", self->classname);
		return;
	}

	/* need to use static strings */
	for (i = 0; i < sizeof(object_actions) / sizeof(*object_actions); i ++)
	{
		if (!strcmp(frames[0].name, object_actions[i]))
		{
			self->monsterinfo.action = object_actions[i];
		}
	}

	if (!self->monsterinfo.action)
	{
		gi.dprintf("no known action in %s\n", self->classname);
		return;
	}

	self->movetype = MOVETYPE_NONE;
	self->nextthink = level.time + FRAMETIME;
	self->think = object_think;
	gi.linkentity(self);
}

void
monster_think(edict_t *self)
{
	if (!self)
	{
		return;
	}

	M_MoveFrame(self);

	if (self->linkcount != self->monsterinfo.linkcount)
	{
		self->monsterinfo.linkcount = self->linkcount;
		M_CheckGround(self);
	}

	M_CatagorizePosition(self);
	M_WorldEffects(self);
	M_SetEffects(self);
}

/*
 * Using a monster makes it angry
 * at the current activator
 */
void
monster_use(edict_t *self, edict_t *other /* unused */, edict_t *activator)
{
	if (!self || !activator)
	{
		return;
	}

	if (self->enemy)
	{
		return;
	}

	if (self->health <= 0)
	{
		return;
	}

	if (activator->flags & FL_NOTARGET)
	{
		return;
	}

	if (!(activator->client) && !(activator->monsterinfo.aiflags & AI_GOOD_GUY))
	{
		return;
	}

	if (activator->flags & FL_DISGUISED)
	{
		return;
	}

	/* delay reaction so if the monster is
	   teleported, its sound is still heard */
	self->enemy = activator;
	FoundTarget(self);
}

void
monster_triggered_spawn(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->s.origin[2] += 1;
	KillBox(self);

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_STEP;
	self->svflags &= ~SVF_NOCLIENT;
	self->air_finished = level.time + 12;
	gi.linkentity(self);

	monster_start_go(self);

	if (strcmp(self->classname, "monster_fixbot") == 0)
	{
		if (self->spawnflags & 16 || self->spawnflags & 8 || self->spawnflags &
			4)
		{
			self->enemy = NULL;
			return;
		}
	}

	if (self->enemy && !(self->spawnflags & 1) &&
		!(self->enemy->flags & FL_NOTARGET))
	{
		if (!(self->enemy->flags & FL_DISGUISED))
		{
			FoundTarget(self);
		}
		else
		{
			self->enemy = NULL;
		}
	}
	else
	{
		self->enemy = NULL;
	}
}

void
monster_triggered_spawn_use(edict_t *self, edict_t *other /* unused */, edict_t *activator)
{
	if (!self || !activator)
	{
		return;
	}

	/* we have a one frame delay here so we
	   don't telefrag the guy who activated us */
	self->think = monster_triggered_spawn;
	self->nextthink = level.time + FRAMETIME;

	if (activator->client)
	{
		self->enemy = activator;
	}

	self->use = monster_use;
}

void
monster_triggered_start(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_NONE;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0;
	self->use = monster_triggered_spawn_use;
}

/*
 * When a monster dies, it fires all of its targets
 * with the current enemy as activator.
 */
void
monster_death_use(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->flags &= ~(FL_FLY | FL_SWIM);
	self->monsterinfo.aiflags &= AI_GOOD_GUY;

	if (self->item)
	{
		Drop_Item(self, self->item);
		self->item = NULL;
	}

	if (self->deathtarget)
	{
		self->target = self->deathtarget;
	}

	if (!self->target)
	{
		return;
	}

	G_UseTargets(self, self->enemy);
}

/* ================================================================== */

static qboolean
monster_start(edict_t *self)
{
	float scale;
	int i;

	if (!self)
	{
		return false;
	}

	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return false;
	}

	if ((self->spawnflags & 4) && !(self->monsterinfo.aiflags & AI_GOOD_GUY))
	{
		self->spawnflags &= ~4;
		self->spawnflags |= 1;
	}

	if ((self->spawnflags & 2) && !self->targetname)
	{
		if (g_fix_triggered->value)
		{
			self->spawnflags &= ~2;
		}

		gi.dprintf ("triggered %s at %s has no targetname\n", self->classname, vtos (self->s.origin));
	}

	if ((!(self->monsterinfo.aiflags & AI_GOOD_GUY)) &&
		(!(self->monsterinfo.aiflags & AI_DO_NOT_COUNT)))
	{
		level.total_monsters++;
	}

	self->nextthink = level.time + FRAMETIME;
	self->svflags |= SVF_MONSTER;
	self->s.renderfx |= RF_FRAMELERP;
	self->takedamage = DAMAGE_AIM;
	self->air_finished = level.time + 12;
	self->use = monster_use;

	if (!self->max_health)
	{
		self->max_health = self->health;
	}

	self->clipmask = MASK_MONSTERSOLID;

	self->s.skinnum = 0;
	self->deadflag = DEAD_NO;
	self->svflags &= ~SVF_DEADMONSTER;

	if (!self->monsterinfo.checkattack)
	{
		self->monsterinfo.checkattack = M_CheckAttack;
	}

	if (ai_model_scale->value > 0)
	{
		scale = ai_model_scale->value;
		VectorSet(self->rrs.scale, scale, scale, scale);
	}

	scale = 0;

	for (i = 0; i < 3; i++)
	{
		if (!self->rrs.scale[i])
		{
			/* fix empty scale */
			self->rrs.scale[i] = 1.0f;
		}

		scale += self->rrs.scale[i];
	}

	scale /= 3;

	/* non default scale */
	if (scale != 1.0)
	{
		int i;

		self->monsterinfo.scale *= scale;
		self->mass *= scale;

		for (i = 0; i < 3; i++)
		{
			self->mins[i] *= self->rrs.scale[i];
			self->maxs[i] *= self->rrs.scale[i];
		}
	}

	VectorCopy(self->s.origin, self->s.old_origin);

	M_FixStuckMonster(self);

	if (st.item)
	{
		self->item = FindItemByClassname(st.item);

		if (!self->item)
		{
			gi.dprintf("%s at %s has bad item: %s\n", self->classname,
					vtos(self->s.origin), st.item);
		}
	}

	/* randomize what frame they start on */
	if (self->monsterinfo.currentmove)
	{
		self->s.frame = self->monsterinfo.currentmove->firstframe +
			(randk() % (self->monsterinfo.currentmove->lastframe -
					   self->monsterinfo.currentmove->firstframe + 1));
	}
	else if (self->monsterinfo.action)
	{
		int ofs_frames = 0, num_frames = 1;

		M_SetAnimGroupFrameValues(self, self->monsterinfo.action,
			&ofs_frames, &num_frames, 0);

		self->s.frame = ofs_frames + (randk() % num_frames);
	}

	self->monsterinfo.base_height = self->maxs[2];
	self->monsterinfo.quad_framenum = 0;
	self->monsterinfo.double_framenum = 0;
	self->monsterinfo.invincible_framenum = 0;

	return true;
}

void
monster_start_go(edict_t *self)
{
	vec3_t v;

	if (!self)
	{
		return;
	}

	if (self->health <= 0)
	{
		return;
	}

	/* check for target to combat_point and change to combattarget */
	if (self->target)
	{
		qboolean notcombat;
		qboolean fixup;
		edict_t *target;

		target = NULL;
		notcombat = false;
		fixup = false;

		while ((target = G_Find(target, FOFS(targetname), self->target)) != NULL)
		{
			if (strcmp(target->classname, "point_combat") == 0)
			{
				self->combattarget = self->target;
				fixup = true;
			}
			else
			{
				notcombat = true;
			}
		}

		if (notcombat && self->combattarget)
		{
			gi.dprintf("%s at %s has target with mixed types\n",
					self->classname, vtos(self->s.origin));
		}

		if (fixup)
		{
			self->target = NULL;
		}
	}

	/* validate combattarget */
	if (self->combattarget)
	{
		edict_t *target;

		target = NULL;

		while ((target = G_Find(target, FOFS(targetname),
						self->combattarget)) != NULL)
		{
			if (strcmp(target->classname, "point_combat") != 0)
			{
				gi.dprintf( "%s at (%i %i %i) has a bad combattarget %s : %s at (%i %i %i)\n",
						self->classname, (int)self->s.origin[0], (int)self->s.origin[1],
						(int)self->s.origin[2], self->combattarget, target->classname,
						(int)target->s.origin[0], (int)target->s.origin[1],
						(int)target->s.origin[2]);
			}
		}
	}

	if (self->target)
	{
		self->goalentity = self->movetarget = G_PickTarget(self->target);

		if (!self->movetarget)
		{
			gi.dprintf("%s can't find target %s at %s\n", self->classname,
					self->target, vtos(self->s.origin));
			self->target = NULL;
			self->monsterinfo.pausetime = 100000000;
			self->monsterinfo.stand(self);
		}
		else if (strcmp(self->movetarget->classname, "path_corner") == 0)
		{
			VectorSubtract(self->goalentity->s.origin, self->s.origin, v);
			self->ideal_yaw = self->s.angles[YAW] = vectoyaw(v);
			self->monsterinfo.walk(self);
			self->target = NULL;
		}
		else
		{
			self->goalentity = self->movetarget = NULL;
			self->monsterinfo.pausetime = 100000000;
			self->monsterinfo.stand(self);
		}
	}
	else
	{
		self->monsterinfo.pausetime = 100000000;
		self->monsterinfo.stand(self);
	}

	self->think = monster_think;
	self->nextthink = level.time + FRAMETIME;
}

void
walkmonster_start_go(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!(self->spawnflags & 2) && (level.time < 1))
	{
		M_droptofloor(self);

		if (self->groundentity)
		{
			if (!M_walkmove(self, 0, 0))
			{
				gi.dprintf("%s in solid at %s\n", self->classname,
						vtos(self->s.origin));
			}
		}
	}

	if (!self->yaw_speed)
	{
		self->yaw_speed = 20;
	}

	if (!self->viewheight)
	{
		self->viewheight = 25;
	}

	if (self->spawnflags & 2)
	{
		monster_triggered_start(self);
	}
	else
	{
		monster_start_go(self);
	}
}

void
walkmonster_start(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->think = walkmonster_start_go;
	monster_start(self);
}

void
flymonster_start_go(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!M_walkmove(self, 0, 0))
	{
		gi.dprintf("%s in solid at %s\n", self->classname, vtos(self->s.origin));
	}

	if (!self->yaw_speed)
	{
		self->yaw_speed = 10;
	}

	if (!self->viewheight)
	{
		self->viewheight = 25;
	}

	if (self->spawnflags & 2)
	{
		monster_triggered_start(self);
	}
	else
	{
		monster_start_go(self);
	}
}

void
flymonster_start(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->flags |= FL_FLY;
	self->think = flymonster_start_go;
	monster_start(self);
}

void
swimmonster_start_go(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!self->yaw_speed)
	{
		self->yaw_speed = 10;
	}

	if (!self->viewheight)
	{
		self->viewheight = 10;
	}

	if (self->spawnflags & 2)
	{
		monster_triggered_start(self);
	}
	else
	{
		monster_start_go(self);
	}
}

void
swimmonster_start(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->flags |= FL_SWIM;
	self->think = swimmonster_start_go;
	monster_start(self);
}

void stationarymonster_start_go(edict_t *self);

void
stationarymonster_triggered_spawn(edict_t *self)
{
	if (!self)
	{
		return;
	}

	KillBox(self);

	self->solid = SOLID_BBOX;
	self->movetype = MOVETYPE_NONE;
	self->svflags &= ~SVF_NOCLIENT;
	self->air_finished = level.time + 12;
	gi.linkentity(self);

	monster_start_go(self);

	if (self->enemy && !(self->spawnflags & 1) &&
		!(self->enemy->flags & FL_NOTARGET))
	{
		if (!(self->enemy->flags & FL_DISGUISED))
		{
			FoundTarget(self);
		}
		else
		{
			self->enemy = NULL;
		}
	}
	else
	{
		self->enemy = NULL;
	}
}

void
stationarymonster_triggered_spawn_use(edict_t *self, edict_t *other /* unused */, edict_t *activator)
{
	if (!self || !activator)
	{
		return;
	}

	/* we have a one frame delay here so we don't telefrag the guy who activated us */
	self->think = stationarymonster_triggered_spawn;
	self->nextthink = level.time + FRAMETIME;

	if (activator->client)
	{
		self->enemy = activator;
	}

	self->use = monster_use;
}

void
stationarymonster_triggered_start(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->solid = SOLID_NOT;
	self->movetype = MOVETYPE_NONE;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0;
	self->use = stationarymonster_triggered_spawn_use;
}

void
stationarymonster_start_go(edict_t *self)
{

	if (!self)
	{
		return;
	}

	if (!self->yaw_speed)
	{
		self->yaw_speed = 20;
	}

	if (self->spawnflags & 2)
	{
		stationarymonster_triggered_start(self);
	}
	else
	{
		monster_start_go(self);
	}
}

void
stationarymonster_start(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->think = stationarymonster_start_go;
	monster_start(self);
}

void
monster_done_dodge(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->monsterinfo.aiflags &= ~AI_DODGING;
}
