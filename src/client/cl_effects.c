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
 * This file implements all specialized client side effects.  E.g.
 * weapon effects, enemy effects, flash, etc.
 *
 * =======================================================================
 */

#include "header/client.h"

void CL_LogoutEffect(vec3_t org, int type);
void CL_ItemRespawnParticles(vec3_t org);
void CL_ClearLightStyles(void);
void CL_ClearDlights(void);
void CL_ClearParticles(void);

static vec3_t avelocities[NUMVERTEXNORMALS];
extern struct model_s *cl_mod_smoke;
extern struct model_s *cl_mod_flash;

extern cparticle_t *active_particles, *free_particles;

void
CL_AddMuzzleFlash(void)
{
	vec3_t fv, rv;
	cdlight_t *dl;
	int i, weapon;
	centity_t *pl;
	int silenced;
	float volume;
	char soundname[64];

	i = MSG_ReadShort(&net_message);

	if ((i < 1) || (i >= MAX_EDICTS))
	{
		Com_Error(ERR_DROP, "%s: bad entity %d >= %d\n",
			__func__, i, MAX_EDICTS);
	}

	weapon = MSG_ReadByte(&net_message);
	if (weapon < 0)
	{
		Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
	}

	silenced = weapon & MZ_SILENCED;
	weapon &= ~MZ_SILENCED;

	pl = &cl_entities[i];

	dl = CL_AllocDlight(i);
	VectorCopy(pl->current.origin, dl->origin);
	AngleVectors(pl->current.angles, fv, rv, NULL);
	VectorMA(dl->origin, 18, fv, dl->origin);
	VectorMA(dl->origin, 16, rv, dl->origin);

	if (silenced)
	{
		dl->radius = 100.0f + (randk() & 31);
	}

	else
	{
		dl->radius = 200.0f + (randk() & 31);
	}

	dl->minlight = 32;
	dl->die = cl.time;

	if (silenced)
	{
		volume = 0.2f;
	}

	else
	{
		volume = 1;
	}

	switch (weapon)
	{
		case MZ_BLASTER:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);
			break;
		case MZ_BLUEHYPERBLASTER:
			dl->color[0] = 0;
			dl->color[1] = 0;
			dl->color[2] = 1;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);
			break;
		case MZ_HYPERBLASTER:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/hyprbf1a.wav"), volume, ATTN_NORM, 0);
			break;
		case MZ_MACHINEGUN:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%db.wav",
				(randk() % 5) + 1);
			S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(
						soundname), volume, ATTN_NORM, 0);
			break;
		case MZ_SHOTGUN:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/shotgf1b.wav"), volume, ATTN_NORM, 0);
			S_StartSound(NULL, i, CHAN_AUTO,
				S_RegisterSound("weapons/shotgr1b.wav"), volume, ATTN_NORM, 0.1f);
			break;
		case MZ_SSHOTGUN:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/sshotf1b.wav"), volume, ATTN_NORM, 0);
			break;
		case MZ_CHAINGUN1:
			dl->radius = 200.0f + (randk() & 31);
			dl->color[0] = 1;
			dl->color[1] = 0.25;
			dl->color[2] = 0;
			Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%db.wav",
				(randk() % 5) + 1);
			S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(
						soundname), volume, ATTN_NORM, 0);
			break;
		case MZ_CHAINGUN2:
			dl->radius = 225.0f + (randk() & 31);
			dl->color[0] = 1;
			dl->color[1] = 0.5;
			dl->color[2] = 0;
			dl->die = cl.time + 0.1;  /* long delay */
			Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%db.wav",
				(randk() % 5) + 1);
			S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(
						soundname), volume, ATTN_NORM, 0);
			Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%db.wav",
				(randk() % 5) + 1);
			S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(
						soundname), volume, ATTN_NORM, 0.05);
			break;
		case MZ_CHAINGUN3:
			dl->radius = 250.0f + (randk() & 31);
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			dl->die = cl.time + 0.1;  /* long delay */
			Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%db.wav",
				(randk() % 5) + 1);
			S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(
						soundname), volume, ATTN_NORM, 0);
			Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%db.wav",
				(randk() % 5) + 1);
			S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(
						soundname), volume, ATTN_NORM, 0.033f);
			Com_sprintf(soundname, sizeof(soundname), "weapons/machgf%db.wav",
				(randk() % 5) + 1);
			S_StartSound(NULL, i, CHAN_WEAPON, S_RegisterSound(
						soundname), volume, ATTN_NORM, 0.066f);
			break;
		case MZ_RAILGUN:
			dl->color[0] = 0.5;
			dl->color[1] = 0.5;
			dl->color[2] = 1.0;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/railgf1a.wav"), volume, ATTN_NORM, 0);
			break;
		case MZ_ROCKET:
			dl->color[0] = 1;
			dl->color[1] = 0.5;
			dl->color[2] = 0.2;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/rocklf1a.wav"), volume, ATTN_NORM, 0);
			S_StartSound(NULL, i, CHAN_AUTO,
				S_RegisterSound("weapons/rocklr1b.wav"), volume, ATTN_NORM, 0.1f);
			break;
		case MZ_GRENADE:
			dl->color[0] = 1;
			dl->color[1] = 0.5;
			dl->color[2] = 0;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/grenlf1a.wav"), volume, ATTN_NORM, 0);
			S_StartSound(NULL, i, CHAN_AUTO,
				S_RegisterSound("weapons/grenlr1b.wav"), volume, ATTN_NORM, 0.1f);
			break;
		case MZ_BFG:
			dl->color[0] = 0;
			dl->color[1] = 1;
			dl->color[2] = 0;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/bfg__f1y.wav"), volume, ATTN_NORM, 0);
			break;

		case MZ_LOGIN:
			dl->color[0] = 0;
			dl->color[1] = 1;
			dl->color[2] = 0;
			dl->die = cl.time + 1;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
			CL_LogoutEffect(pl->current.origin, weapon);
			break;
		case MZ_LOGOUT:
			dl->color[0] = 1;
			dl->color[1] = 0;
			dl->color[2] = 0;
			dl->die = cl.time + 1;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
			CL_LogoutEffect(pl->current.origin, weapon);
			break;
		case MZ_RESPAWN:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			dl->die = cl.time + 1.0;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/grenlf1a.wav"), 1, ATTN_NORM, 0);
			CL_LogoutEffect(pl->current.origin, weapon);
			break;
		case MZ_PHALANX:
			dl->color[0] = 1;
			dl->color[1] = 0.5;
			dl->color[2] = 0.5;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/plasshot.wav"), volume, ATTN_NORM, 0);
			break;
		case MZ_IONRIPPER:
			dl->color[0] = 1;
			dl->color[1] = 0.5;
			dl->color[2] = 0.5;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/rippfire.wav"), volume, ATTN_NORM, 0);
			break;
		case MZ_ETF_RIFLE:
			dl->color[0] = 0.9f;
			dl->color[1] = 0.7f;
			dl->color[2] = 0;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/nail1.wav"), volume, ATTN_NORM, 0);
			break;
		case MZ_SHOTGUN2:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/shotg2.wav"), volume, ATTN_NORM, 0);
			break;
		case MZ_HEATBEAM:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			dl->die = cl.time + 100;
			break;
		case MZ_BLASTER2:
			dl->color[0] = 0;
			dl->color[1] = 1;
			dl->color[2] = 0;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/blastf1a.wav"), volume, ATTN_NORM, 0);
			break;
		case MZ_TRACKER:
			/* negative flashes handled the same in gl/soft until CL_AddDLights */
			dl->color[0] = -1;
			dl->color[1] = -1;
			dl->color[2] = -1;
			S_StartSound(NULL, i, CHAN_WEAPON,
				S_RegisterSound("weapons/disint2.wav"), volume, ATTN_NORM, 0);
			break;
		case MZ_NUKE1:
			dl->color[0] = 1;
			dl->color[1] = 0;
			dl->color[2] = 0;
			dl->die = cl.time + 100;
			break;
		case MZ_NUKE2:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			dl->die = cl.time + 100;
			break;
		case MZ_NUKE4:
			dl->color[0] = 0;
			dl->color[1] = 0;
			dl->color[2] = 1;
			dl->die = cl.time + 100;
			break;
		case MZ_NUKE8:
			dl->color[0] = 0;
			dl->color[1] = 1;
			dl->color[2] = 1;
			dl->die = cl.time + 100;
			break;
	}
}

void
CL_AddMuzzleFlash2(void)
{
	int ent;
	vec3_t origin;
	int flash_number;
	cdlight_t *dl;
	vec3_t forward, right;
	char soundname[64];

	ent = MSG_ReadShort(&net_message);

	if ((ent < 1) || (ent >= MAX_EDICTS))
	{
		Com_Error(ERR_DROP, "%s: bad entity %d >= %d\n",
			__func__, ent, MAX_EDICTS);
	}

	flash_number = MSG_ReadByte(&net_message);
	if (flash_number < 0)
	{
		Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
	}

	if (flash_number > MZ2_EFFECT_MAX)
	{
		Com_DPrintf("%s: bad offset\n", __func__);
		return;
	}

	/* locate the origin */
	AngleVectors(cl_entities[ent].current.angles, forward, right, NULL);
	origin[0] = cl_entities[ent].current.origin[0] + forward[0] *
				monster_flash_offset[flash_number][0] + right[0] *
				monster_flash_offset[flash_number][1];
	origin[1] = cl_entities[ent].current.origin[1] + forward[1] *
				monster_flash_offset[flash_number][0] + right[1] *
				monster_flash_offset[flash_number][1];
	origin[2] = cl_entities[ent].current.origin[2] + forward[2] *
				monster_flash_offset[flash_number][0] + right[2] *
				monster_flash_offset[flash_number][1] +
				monster_flash_offset[flash_number][2];

	dl = CL_AllocDlight(ent);
	VectorCopy(origin, dl->origin);
	dl->radius = 200.0f + (randk() & 31);
	dl->minlight = 32;
	dl->die = cl.time;

	switch (flash_number)
	{
		case MZ2_INFANTRY_MACHINEGUN_1:
		case MZ2_INFANTRY_MACHINEGUN_2:
		case MZ2_INFANTRY_MACHINEGUN_3:
		case MZ2_INFANTRY_MACHINEGUN_4:
		case MZ2_INFANTRY_MACHINEGUN_5:
		case MZ2_INFANTRY_MACHINEGUN_6:
		case MZ2_INFANTRY_MACHINEGUN_7:
		case MZ2_INFANTRY_MACHINEGUN_8:
		case MZ2_INFANTRY_MACHINEGUN_9:
		case MZ2_INFANTRY_MACHINEGUN_10:
		case MZ2_INFANTRY_MACHINEGUN_11:
		case MZ2_INFANTRY_MACHINEGUN_12:
		case MZ2_INFANTRY_MACHINEGUN_13:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			CL_ParticleEffect(origin, vec3_origin, 0xff000000, 0xff6b6b6b, 40);
			CL_SmokeAndFlash(origin);
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_SOLDIER_MACHINEGUN_1:
		case MZ2_SOLDIER_MACHINEGUN_2:
		case MZ2_SOLDIER_MACHINEGUN_3:
		case MZ2_SOLDIER_MACHINEGUN_4:
		case MZ2_SOLDIER_MACHINEGUN_5:
		case MZ2_SOLDIER_MACHINEGUN_6:
		case MZ2_SOLDIER_MACHINEGUN_7:
		case MZ2_SOLDIER_MACHINEGUN_8:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			CL_ParticleEffect(origin, vec3_origin, 0xff000000, 0xff6b6b6b, 40);
			CL_SmokeAndFlash(origin);
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("soldier/solatck3.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_GUNNER_MACHINEGUN_1:
		case MZ2_GUNNER_MACHINEGUN_2:
		case MZ2_GUNNER_MACHINEGUN_3:
		case MZ2_GUNNER_MACHINEGUN_4:
		case MZ2_GUNNER_MACHINEGUN_5:
		case MZ2_GUNNER_MACHINEGUN_6:
		case MZ2_GUNNER_MACHINEGUN_7:
		case MZ2_GUNNER_MACHINEGUN_8:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			CL_ParticleEffect(origin, vec3_origin, 0xff000000, 0xff6b6b6b, 40);
			CL_SmokeAndFlash(origin);
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("gunner/gunatck2.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_ACTOR_MACHINEGUN_1:
		case MZ2_SUPERTANK_MACHINEGUN_1:
		case MZ2_SUPERTANK_MACHINEGUN_2:
		case MZ2_SUPERTANK_MACHINEGUN_3:
		case MZ2_SUPERTANK_MACHINEGUN_4:
		case MZ2_SUPERTANK_MACHINEGUN_5:
		case MZ2_SUPERTANK_MACHINEGUN_6:
		case MZ2_TURRET_MACHINEGUN:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;

			CL_ParticleEffect(origin, vec3_origin, 0xff000000, 0xff6b6b6b, 40);
			CL_SmokeAndFlash(origin);
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_BOSS2_MACHINEGUN_L1:
		case MZ2_BOSS2_MACHINEGUN_L2:
		case MZ2_BOSS2_MACHINEGUN_L3:
		case MZ2_BOSS2_MACHINEGUN_L4:
		case MZ2_BOSS2_MACHINEGUN_L5:
		case MZ2_CARRIER_MACHINEGUN_L1:
		case MZ2_CARRIER_MACHINEGUN_L2:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;

			CL_ParticleEffect(origin, vec3_origin, 0xff000000, 0xff6b6b6b, 40);
			CL_SmokeAndFlash(origin);
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("infantry/infatck1.wav"), 1, ATTN_NONE, 0);
			break;

		case MZ2_SOLDIER_BLASTER_1:
		case MZ2_SOLDIER_BLASTER_2:
		case MZ2_SOLDIER_BLASTER_3:
		case MZ2_SOLDIER_BLASTER_4:
		case MZ2_SOLDIER_BLASTER_5:
		case MZ2_SOLDIER_BLASTER_6:
		case MZ2_SOLDIER_BLASTER_7:
		case MZ2_SOLDIER_BLASTER_8:
		case MZ2_TURRET_BLASTER:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("soldier/solatck2.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_FLYER_BLASTER_1:
		case MZ2_FLYER_BLASTER_2:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("flyer/flyatck3.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_MEDIC_BLASTER_1:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("medic/medatck1.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_HOVER_BLASTER_1:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("hover/hovatck1.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_FLOAT_BLASTER_1:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("floater/fltatck1.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_SOLDIER_SHOTGUN_1:
		case MZ2_SOLDIER_SHOTGUN_2:
		case MZ2_SOLDIER_SHOTGUN_3:
		case MZ2_SOLDIER_SHOTGUN_4:
		case MZ2_SOLDIER_SHOTGUN_5:
		case MZ2_SOLDIER_SHOTGUN_6:
		case MZ2_SOLDIER_SHOTGUN_7:
		case MZ2_SOLDIER_SHOTGUN_8:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			CL_SmokeAndFlash(origin);
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("soldier/solatck1.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_TANK_BLASTER_1:
		case MZ2_TANK_BLASTER_2:
		case MZ2_TANK_BLASTER_3:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_TANK_MACHINEGUN_1:
		case MZ2_TANK_MACHINEGUN_2:
		case MZ2_TANK_MACHINEGUN_3:
		case MZ2_TANK_MACHINEGUN_4:
		case MZ2_TANK_MACHINEGUN_5:
		case MZ2_TANK_MACHINEGUN_6:
		case MZ2_TANK_MACHINEGUN_7:
		case MZ2_TANK_MACHINEGUN_8:
		case MZ2_TANK_MACHINEGUN_9:
		case MZ2_TANK_MACHINEGUN_10:
		case MZ2_TANK_MACHINEGUN_11:
		case MZ2_TANK_MACHINEGUN_12:
		case MZ2_TANK_MACHINEGUN_13:
		case MZ2_TANK_MACHINEGUN_14:
		case MZ2_TANK_MACHINEGUN_15:
		case MZ2_TANK_MACHINEGUN_16:
		case MZ2_TANK_MACHINEGUN_17:
		case MZ2_TANK_MACHINEGUN_18:
		case MZ2_TANK_MACHINEGUN_19:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			CL_ParticleEffect(origin, vec3_origin, 0xff000000, 0xff6b6b6b, 40);
			CL_SmokeAndFlash(origin);
			Com_sprintf(soundname, sizeof(soundname), "tank/tnkatk2%c.wav",
				'a' + (char)(randk() % 5));
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound(soundname), 1, ATTN_NORM, 0);
			break;

		case MZ2_CHICK_ROCKET_1:
		case MZ2_TURRET_ROCKET:
			dl->color[0] = 1;
			dl->color[1] = 0.5f;
			dl->color[2] = 0.2f;
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("chick/chkatck2.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_TANK_ROCKET_1:
		case MZ2_TANK_ROCKET_2:
		case MZ2_TANK_ROCKET_3:
			dl->color[0] = 1;
			dl->color[1] = 0.5f;
			dl->color[2] = 0.2f;
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("tank/tnkatck1.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_SUPERTANK_ROCKET_1:
		case MZ2_SUPERTANK_ROCKET_2:
		case MZ2_SUPERTANK_ROCKET_3:
		case MZ2_BOSS2_ROCKET_1:
		case MZ2_BOSS2_ROCKET_2:
		case MZ2_BOSS2_ROCKET_3:
		case MZ2_BOSS2_ROCKET_4:
		case MZ2_CARRIER_ROCKET_1:
			dl->color[0] = 1;
			dl->color[1] = 0.5f;
			dl->color[2] = 0.2f;
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("tank/rocket.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_GUNNER_GRENADE_1:
		case MZ2_GUNNER_GRENADE_2:
		case MZ2_GUNNER_GRENADE_3:
		case MZ2_GUNNER_GRENADE_4:
			dl->color[0] = 1;
			dl->color[1] = 0.5;
			dl->color[2] = 0;
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("gunner/gunatck3.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_GLADIATOR_RAILGUN_1:
		case MZ2_CARRIER_RAILGUN:
		case MZ2_WIDOW_RAIL:
			dl->color[0] = 0.5;
			dl->color[1] = 0.5;
			dl->color[2] = 1.0;
			break;

		case MZ2_MAKRON_BFG:
			dl->color[0] = 0.5;
			dl->color[1] = 1;
			dl->color[2] = 0.5;
			break;

		case MZ2_MAKRON_BLASTER_1:
		case MZ2_MAKRON_BLASTER_2:
		case MZ2_MAKRON_BLASTER_3:
		case MZ2_MAKRON_BLASTER_4:
		case MZ2_MAKRON_BLASTER_5:
		case MZ2_MAKRON_BLASTER_6:
		case MZ2_MAKRON_BLASTER_7:
		case MZ2_MAKRON_BLASTER_8:
		case MZ2_MAKRON_BLASTER_9:
		case MZ2_MAKRON_BLASTER_10:
		case MZ2_MAKRON_BLASTER_11:
		case MZ2_MAKRON_BLASTER_12:
		case MZ2_MAKRON_BLASTER_13:
		case MZ2_MAKRON_BLASTER_14:
		case MZ2_MAKRON_BLASTER_15:
		case MZ2_MAKRON_BLASTER_16:
		case MZ2_MAKRON_BLASTER_17:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("makron/blaster.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_JORG_MACHINEGUN_L1:
		case MZ2_JORG_MACHINEGUN_L2:
		case MZ2_JORG_MACHINEGUN_L3:
		case MZ2_JORG_MACHINEGUN_L4:
		case MZ2_JORG_MACHINEGUN_L5:
		case MZ2_JORG_MACHINEGUN_L6:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			CL_ParticleEffect(origin, vec3_origin, 0xff000000, 0xff6b6b6b, 40);
			CL_SmokeAndFlash(origin);
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("boss3/xfire.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_JORG_MACHINEGUN_R1:
		case MZ2_JORG_MACHINEGUN_R2:
		case MZ2_JORG_MACHINEGUN_R3:
		case MZ2_JORG_MACHINEGUN_R4:
		case MZ2_JORG_MACHINEGUN_R5:
		case MZ2_JORG_MACHINEGUN_R6:
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			CL_ParticleEffect(origin, vec3_origin, 0xff000000, 0xff6b6b6b, 40);
			CL_SmokeAndFlash(origin);
			break;

		case MZ2_JORG_BFG_1:
			dl->color[0] = 0.5;
			dl->color[1] = 1;
			dl->color[2] = 0.5;
			break;

		case MZ2_BOSS2_MACHINEGUN_R1:
		case MZ2_BOSS2_MACHINEGUN_R2:
		case MZ2_BOSS2_MACHINEGUN_R3:
		case MZ2_BOSS2_MACHINEGUN_R4:
		case MZ2_BOSS2_MACHINEGUN_R5:
		case MZ2_CARRIER_MACHINEGUN_R1:
		case MZ2_CARRIER_MACHINEGUN_R2:

			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;

			CL_ParticleEffect(origin, vec3_origin, 0xff000000, 0xff6b6b6b, 40);
			CL_SmokeAndFlash(origin);
			break;

		case MZ2_STALKER_BLASTER:
		case MZ2_DAEDALUS_BLASTER:
		case MZ2_MEDIC_BLASTER_2:
		case MZ2_WIDOW_BLASTER:
		case MZ2_WIDOW_BLASTER_SWEEP1:
		case MZ2_WIDOW_BLASTER_SWEEP2:
		case MZ2_WIDOW_BLASTER_SWEEP3:
		case MZ2_WIDOW_BLASTER_SWEEP4:
		case MZ2_WIDOW_BLASTER_SWEEP5:
		case MZ2_WIDOW_BLASTER_SWEEP6:
		case MZ2_WIDOW_BLASTER_SWEEP7:
		case MZ2_WIDOW_BLASTER_SWEEP8:
		case MZ2_WIDOW_BLASTER_SWEEP9:
		case MZ2_WIDOW_BLASTER_100:
		case MZ2_WIDOW_BLASTER_90:
		case MZ2_WIDOW_BLASTER_80:
		case MZ2_WIDOW_BLASTER_70:
		case MZ2_WIDOW_BLASTER_60:
		case MZ2_WIDOW_BLASTER_50:
		case MZ2_WIDOW_BLASTER_40:
		case MZ2_WIDOW_BLASTER_30:
		case MZ2_WIDOW_BLASTER_20:
		case MZ2_WIDOW_BLASTER_10:
		case MZ2_WIDOW_BLASTER_0:
		case MZ2_WIDOW_BLASTER_10L:
		case MZ2_WIDOW_BLASTER_20L:
		case MZ2_WIDOW_BLASTER_30L:
		case MZ2_WIDOW_BLASTER_40L:
		case MZ2_WIDOW_BLASTER_50L:
		case MZ2_WIDOW_BLASTER_60L:
		case MZ2_WIDOW_BLASTER_70L:
		case MZ2_WIDOW_RUN_1:
		case MZ2_WIDOW_RUN_2:
		case MZ2_WIDOW_RUN_3:
		case MZ2_WIDOW_RUN_4:
		case MZ2_WIDOW_RUN_5:
		case MZ2_WIDOW_RUN_6:
		case MZ2_WIDOW_RUN_7:
		case MZ2_WIDOW_RUN_8:
			dl->color[0] = 0;
			dl->color[1] = 1;
			dl->color[2] = 0;
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("tank/tnkatck3.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_WIDOW_DISRUPTOR:
			dl->color[0] = -1;
			dl->color[1] = -1;
			dl->color[2] = -1;
			S_StartSound(NULL, ent, CHAN_WEAPON,
				S_RegisterSound("weapons/disint2.wav"), 1, ATTN_NORM, 0);
			break;

		case MZ2_WIDOW_PLASMABEAM:
		case MZ2_WIDOW2_BEAMER_1:
		case MZ2_WIDOW2_BEAMER_2:
		case MZ2_WIDOW2_BEAMER_3:
		case MZ2_WIDOW2_BEAMER_4:
		case MZ2_WIDOW2_BEAMER_5:
		case MZ2_WIDOW2_BEAM_SWEEP_1:
		case MZ2_WIDOW2_BEAM_SWEEP_2:
		case MZ2_WIDOW2_BEAM_SWEEP_3:
		case MZ2_WIDOW2_BEAM_SWEEP_4:
		case MZ2_WIDOW2_BEAM_SWEEP_5:
		case MZ2_WIDOW2_BEAM_SWEEP_6:
		case MZ2_WIDOW2_BEAM_SWEEP_7:
		case MZ2_WIDOW2_BEAM_SWEEP_8:
		case MZ2_WIDOW2_BEAM_SWEEP_9:
		case MZ2_WIDOW2_BEAM_SWEEP_10:
		case MZ2_WIDOW2_BEAM_SWEEP_11:
			dl->radius = 300.0f + (randk() & 100);
			dl->color[0] = 1;
			dl->color[1] = 1;
			dl->color[2] = 0;
			dl->die = cl.time + 200;
			break;
	}
}

void
CL_TeleporterParticles(const entity_xstate_t *ent)
{
	int i, j;
	cparticle_t *p;
	float time;

	time = (float)cl.time;

	for (i = 0; i < 8; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = time;
		p->color = 0xff53ffff;

		for (j = 0; j < 2; j++)
		{
			p->org[j] = ent->origin[j] - 16 + (randk() & 31);
			p->vel[j] = crandk() * 14;
		}

		p->org[2] = ent->origin[2] - 8 + (randk() & 7);
		p->vel[2] = 80 + (randk() & 7);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.5;
	}
}

unsigned int
CL_CombineColors(unsigned int basecolor, unsigned int finalcolor, float scale)
{
	int a_beg, b_beg, c_beg, d_beg;
	int a_end, b_end, c_end, d_end;
	int a_step, b_step, c_step, d_step;

	// get colors
	a_beg = (basecolor >> 0 ) & 0xff;
	b_beg = (basecolor >> 8 ) & 0xff;
	c_beg = (basecolor >> 16) & 0xff;
	d_beg = (basecolor >> 24) & 0xff;

	a_end = (finalcolor >> 0 ) & 0xff;
	b_end = (finalcolor >> 8 ) & 0xff;
	c_end = (finalcolor >> 16) & 0xff;
	d_end = (finalcolor >> 24) & 0xff;

	a_step = (a_end - a_beg) * scale;
	b_step = (b_end - b_beg) * scale;
	c_step = (c_end - c_beg) * scale;
	d_step = (d_end - d_beg) * scale;

	return (((a_beg + a_step) << 0) & 0x000000ff) |
		   (((b_beg + b_step) << 8) & 0x0000ff00) |
		   (((c_beg + c_step) << 16) & 0x00ff0000) |
		   (((d_beg + d_step) << 24) & 0xff000000);
}

void
CL_LogoutEffect(vec3_t org, int type)
{
	int i, j;
	cparticle_t *p;
	float time;

	time = (float)cl.time;

	for (i = 0; i < 500; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = time;

		if (type == MZ_LOGIN)
		{
			p->color = CL_CombineColors(0xff00ff00, 0xffffffff,
				(float)(randk() & 15) / 15.0);
		}

		else if (type == MZ_LOGOUT)
		{
			p->color = CL_CombineColors(0xff2b3ba7, 0xff001357,
				(float)(randk() & 15) / 15.0);
		}

		else
		{
			p->color = CL_CombineColors(0xff07abff, 0xff002bab,
				(float)(randk() & 15) / 15.0);
		}

		p->org[0] = org[0] - 16 + frandk() * 32;
		p->org[1] = org[1] - 16 + frandk() * 32;
		p->org[2] = org[2] - 24 + frandk() * 56;

		for (j = 0; j < 3; j++)
		{
			p->vel[j] = crandk() * 20;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (1.0 + frandk() * 0.3);
	}
}

void
CL_ItemRespawnParticles(vec3_t org)
{
	int i, j;
	cparticle_t *p;
	float time;

	time = (float)cl.time;

	for (i = 0; i < 64; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = time;
		p->color = CL_CombineColors(0xff2fa75f, 0xffffffff,
			(float)(randk() & 15) / 15.0);
		p->org[0] = org[0] + crandk() * 8;
		p->org[1] = org[1] + crandk() * 8;
		p->org[2] = org[2] + crandk() * 8;

		for (j = 0; j < 3; j++)
		{
			p->vel[j] = crandk() * 8;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY * 0.2;
		p->alpha = 1.0;

		p->alphavel = -1.0f / (1.0f + frandk() * 0.3f);
	}
}

void
CL_ExplosionParticles(vec3_t org)
{
	int i, j;
	cparticle_t *p;
	float time;

	time = (float)cl.time;

	for (i = 0; i < 256; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = time;
		p->color = CL_CombineColors(0xff07abff, 0xff002bab,
			(float)(randk() & 15) / 15.0);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((randk() % 32) - 16);
			p->vel[j] = (randk() % 384) - 192;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.8f / (0.5f + frandk() * 0.3f);
	}
}

static int default_colortable[] = {
	0xff234b63, /* 0x10 */
	0xff4f5bb3, /* 0x68 */
	0xff0f1b4f, /* 0xa8 */
	0xff7b9f97, /* 0x90 */
};

static int nuke_colortable[] = {
	0xffc3b79f, /* 0x10 */
	0xffa79773, /* 0x68 */
	0xff8b7747, /* 0xa8 */
	0xff6f5317, /* 0x90 */
};

void
CL_BigTeleportParticles(vec3_t org)
{
	int i;
	cparticle_t *p;
	float time;

	time = (float)cl.time;
	float angle, dist;

	for (i = 0; i < 4096; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = time;
		p->color = default_colortable[randk() & 3];

		angle = M_PI * 2 * (randk() & 1023) / 1023.0f;
		dist = (float)(randk() & 31);
		p->org[0] = org[0] + (float)cos(angle) * dist;
		p->vel[0] = (float)cos(angle) * (70 + (randk() & 63));
		p->accel[0] = -(float)cos(angle) * 100;

		p->org[1] = org[1] + (float)sin(angle) * dist;
		p->vel[1] = (float)sin(angle) * (70 + (randk() & 63));
		p->accel[1] = -(float)sin(angle) * 100;

		p->org[2] = org[2] + 8 + (randk() % 90);
		p->vel[2] = -100 + (randk() & 31);
		p->accel[2] = PARTICLE_GRAVITY * 4;
		p->alpha = 1.0;

		p->alphavel = -0.3f / (0.5f + frandk() * 0.3f);
	}
}

/*
 *  Wall impact puffs
 */
void
CL_BlasterParticles(vec3_t org, vec3_t dir)
{
	int i, j;
	cparticle_t *p;
	float d;
	int count;
	float time;

	time = (float)cl.time;

	count = 40;

	for (i = 0; i < count; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = time;
		p->color = CL_CombineColors(0xff07abff, 0xff002bab,
			(float)(randk() & 15) / 15.0);
		d = randk() & 15;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((randk() & 7) - 4) + d * dir[j];
			p->vel[j] = dir[j] * 30 + crandk() * 40;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0f / (0.5f + frandk() * 0.3f);
	}
}

void
CL_BlasterTrail(vec3_t start, vec3_t end)
{
	vec3_t move;
	vec3_t vec;
	int len;
	int j;
	cparticle_t *p;
	int dec;
	float time;

	time = (float)cl.time;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = (int)VectorNormalize(vec);

	dec = 5;
	VectorScale(vec, 5, vec);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = time;

		p->alpha = 1.0;
		p->alphavel = -1.0f / (0.3f + frandk() * 0.2f);
		p->color = 0xff07abff;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crandk();
			p->vel[j] = crandk() * 5;
			p->accel[j] = 0;
		}

		VectorAdd(move, vec, move);
	}
}

void
CL_QuadTrail(vec3_t start, vec3_t end)
{
	vec3_t move;
	vec3_t vec;
	int len;
	int j;
	cparticle_t *p;
	int dec;
	float time;

	time = (float)cl.time;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = (int)VectorNormalize(vec);

	dec = 5;
	VectorScale(vec, 5, vec);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = time;

		p->alpha = 1.0;
		p->alphavel = -1.0f / (0.8f + frandk() * 0.2f);
		p->color = 0xff7f672f;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crandk() * 16;
			p->vel[j] = crandk() * 5;
			p->accel[j] = 0;
		}

		VectorAdd(move, vec, move);
	}
}

void
CL_FlagTrail(vec3_t start, vec3_t end, int color)
{
	vec3_t move;
	vec3_t vec;
	int len;
	int j;
	cparticle_t *p;
	int dec;
	float time;

	time = (float)cl.time;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = (int)VectorNormalize(vec);

	dec = 5;
	VectorScale(vec, 5, vec);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = time;

		p->alpha = 1.0;
		p->alphavel = -1.0f / (0.8f + frandk() * 0.2f);
		p->color = color;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crandk() * 16;
			p->vel[j] = crandk() * 5;
			p->accel[j] = 0;
		}

		VectorAdd(move, vec, move);
	}
}

void
CL_DiminishingTrail(vec3_t start, vec3_t end, centity_t *old, int flags)
{
	vec3_t move;
	vec3_t vec;
	int len;
	int j;
	cparticle_t *p;
	float dec;
	float orgscale;
	float velscale;
	float time;

	time = (float)cl.time;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 0.5;
	VectorScale(vec, dec, vec);

	if (old->trailcount > 900)
	{
		orgscale = 4;
		velscale = 15;
	}
	else if (old->trailcount > 800)
	{
		orgscale = 2;
		velscale = 10;
	}
	else
	{
		orgscale = 1;
		velscale = 5;
	}

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
		{
			return;
		}

		/* drop less particles as it flies */
		if ((randk() & 1023) < old->trailcount)
		{
			p = free_particles;
			free_particles = p->next;
			p->next = active_particles;
			active_particles = p;
			VectorClear(p->accel);

			p->time = time;

			if (flags & EF_GIB)
			{
				p->alpha = 1.0;
				p->alphavel = -1.0f / (1 + frandk() * 0.4f);
				p->color = CL_CombineColors(0xff001f9b, 0xff00001b,
					(float)(randk() & 15) / 15.0);

				for (j = 0; j < 3; j++)
				{
					p->org[j] = move[j] + crandk() * orgscale;
					p->vel[j] = crandk() * velscale;
					p->accel[j] = 0;
				}

				p->vel[2] -= PARTICLE_GRAVITY;
			}
			else if (flags & EF_GREENGIB)
			{
				p->alpha = 1.0;
				p->alphavel = -1.0f / (1 + frandk() * 0.4f);
				p->color = CL_CombineColors(0xff53ffff, 0xff007fef,
					(float)(randk() & 15) / 15.0);

				for (j = 0; j < 3; j++)
				{
					p->org[j] = move[j] + crandk() * orgscale;
					p->vel[j] = crandk() * velscale;
					p->accel[j] = 0;
				}

				p->vel[2] -= PARTICLE_GRAVITY;
			}
			else
			{
				p->alpha = 1.0;
				p->alphavel = -1.0f / (1 + frandk() * 0.2f);
				p->color = CL_CombineColors(0xff3f3f3f, 0xffababab,
					(float)(randk() & 15) / 15.0);

				for (j = 0; j < 3; j++)
				{
					p->org[j] = move[j] + crandk() * orgscale;
					p->vel[j] = crandk() * velscale;
				}

				p->accel[2] = 20;
			}
		}

		old->trailcount -= 5;

		if (old->trailcount < 100)
		{
			old->trailcount = 100;
		}

		VectorAdd(move, vec, move);
	}
}

void
MakeNormalVectors(vec3_t forward, vec3_t right, vec3_t up)
{
	float d;

	/* this rotate and negate guarantees a
	   vector not colinear with the original */
	right[1] = -forward[0];
	right[2] = forward[1];
	right[0] = forward[2];

	d = DotProduct(right, forward);
	VectorMA(right, -d, forward, right);
	VectorNormalize(right);
	CrossProduct(right, forward, up);
}

void
CL_RocketTrail(vec3_t start, vec3_t end, centity_t *old)
{
	vec3_t move;
	vec3_t vec;
	int len;
	int j;
	cparticle_t *p;
	int dec;
	float time;

	time = (float)cl.time;

	/* smoke */
	CL_DiminishingTrail(start, end, old, EF_ROCKET);

	/* fire */
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = (int)VectorNormalize(vec);

	dec = 1;
	VectorScale(vec, dec, vec);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
		{
			return;
		}

		if ((randk() & 7) == 0)
		{
			p = free_particles;
			free_particles = p->next;
			p->next = active_particles;
			active_particles = p;

			VectorClear(p->accel);
			p->time = time;

			p->alpha = 1.0;
			p->alphavel = -1.0f / (1 + frandk() * 0.2f);
			p->color = CL_CombineColors(0xff27ffff, 0xff0fbfff,
				(float)(randk() & 15) / 15.0);

			for (j = 0; j < 3; j++)
			{
				p->org[j] = move[j] + crandk() * 5;
				p->vel[j] = crandk() * 20;
			}

			p->accel[2] = -PARTICLE_GRAVITY;
		}

		VectorAdd(move, vec, move);
	}
}

void
CL_RailTrail(vec3_t start, vec3_t end)
{
	vec3_t move;
	vec3_t vec;
	int len;
	int j;
	cparticle_t *p;
	float dec;
	vec3_t right, up;
	int i;
	float d, c, s;
	vec3_t dir;
	float time;

	time = (float)cl.time;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	MakeNormalVectors(vec, right, up);

	for (i = 0; i < len; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = time;
		VectorClear(p->accel);

		d = i * 0.1f;
		c = (float)cos(d);
		s = (float)sin(d);

		VectorScale(right, c, dir);
		VectorMA(dir, s, up, dir);

		p->alpha = 1.0;
		p->alphavel = -1.0f / (1 + frandk() * 0.2f);
		p->color = CL_CombineColors(0xff6f5317, 0xff2b1f00,
			(float)(randk() & 15) / 15.0);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + dir[j] * 3;
			p->vel[j] = dir[j] * 6;
		}

		VectorAdd(move, vec, move);
	}

	dec = 0.75;
	VectorScale(vec, dec, vec);
	VectorCopy(start, move);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = time;
		VectorClear(p->accel);

		p->alpha = 1.0;
		p->alphavel = -1.0f / (0.6f + frandk() * 0.2f);
		p->color = CL_CombineColors(0xff000000, 0xffebebeb,
			(float)(randk() & 15) / 15.0);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crandk() * 3;
			p->vel[j] = crandk() * 3;
			p->accel[j] = 0;
		}

		VectorAdd(move, vec, move);
	}
}

void
CL_IonripperTrail(vec3_t start, vec3_t ent)
{
	vec3_t move;
	vec3_t vec;
	int len;
	int j;
	cparticle_t *p;
	int dec;
	int left = 0;
	float time;

	time = (float)cl.time;

	VectorCopy(start, move);
	VectorSubtract(ent, start, vec);
	len = (int)VectorNormalize(vec);

	dec = 5;
	VectorScale(vec, 5, vec);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = time;
		p->alpha = 0.5;
		p->alphavel = -1.0f / (0.3f + frandk() * 0.2f);
		p->color = CL_CombineColors(0xff0057d3, 0xff002bab,
			(float)(randk() & 15) / 15.0);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j];
			p->accel[j] = 0;
		}

		if (left)
		{
			left = 0;
			p->vel[0] = 10;
		}
		else
		{
			left = 1;
			p->vel[0] = -10;
		}

		p->vel[1] = 0;
		p->vel[2] = 0;

		VectorAdd(move, vec, move);
	}
}

void
CL_BubbleTrail(vec3_t start, vec3_t end)
{
	vec3_t move;
	vec3_t vec;
	int len;
	int i, j;
	cparticle_t *p;
	float dec;
	float time;

	time = (float)cl.time;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 32;
	VectorScale(vec, dec, vec);

	for (i = 0; i < len; i += 32)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		VectorClear(p->accel);
		p->time = time;

		p->alpha = 1.0;
		p->alphavel = -1.0f / (1 + frandk() * 0.2f);
		p->color = CL_CombineColors(0xff3f3f3f, 0xffababab,
			(float)(randk() & 15) / 15.0);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crandk() * 2;
			p->vel[j] = crandk() * 5;
		}

		p->vel[2] += 6;

		VectorAdd(move, vec, move);
	}
}

void
CL_FlyParticles(vec3_t origin, int count)
{
	int i;
	cparticle_t *p;
	float angle;
	float sp, sy, cp, cy;
	vec3_t forward;
	float dist = 64;
	float ltime;
	float time;

	time = (float)cl.time;

	if (count > NUMVERTEXNORMALS)
	{
		count = NUMVERTEXNORMALS;
	}

	if (!avelocities[0][0])
	{
		for (i = 0; i < NUMVERTEXNORMALS; i++)
		{
			avelocities[i][0] = (randk() & 255) * 0.01f;
			avelocities[i][1] = (randk() & 255) * 0.01f;
			avelocities[i][2] = (randk() & 255) * 0.01f;
		}
	}

	ltime = time / 1000.0f;

	for (i = 0; i < count; i += 2)
	{
		angle = ltime * avelocities[i][0];
		sy = (float)sin(angle);
		cy = (float)cos(angle);
		angle = ltime * avelocities[i][1];
		sp = (float)sin(angle);
		cp = (float)cos(angle);

		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;

		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = time;

		dist = (float)sin(ltime + i) * 64;
		p->org[0] = origin[0] + bytedirs[i][0] * dist + forward[0] * 16;
		p->org[1] = origin[1] + bytedirs[i][1] * dist + forward[1] * 16;
		p->org[2] = origin[2] + bytedirs[i][2] * dist + forward[2] * 16;

		VectorClear(p->vel);
		VectorClear(p->accel);

		p->color = 0xff000000; /* black */
		p->alpha = 1;
		p->alphavel = -100;
	}
}

void
CL_FlyEffect(centity_t *ent, vec3_t origin)
{
	int n;
	int count;
	int starttime;

	if (ent->fly_stoptime < cl.time)
	{
		starttime = cl.time;
		ent->fly_stoptime = cl.time + 60000;
	}
	else
	{
		starttime = ent->fly_stoptime - 60000;
	}

	n = cl.time - starttime;

	if (n < 20000)
	{
		count = (int)n * 162 / 20000.0;
	}

	else
	{
		n = ent->fly_stoptime - cl.time;

		if (n < 20000)
		{
			count = (int)n * 162 / 20000.0;
		}

		else
		{
			count = 162;
		}
	}

	CL_FlyParticles(origin, count);
}

void
CL_BfgParticles(entity_t *ent)
{
	int i;
	cparticle_t *p;
	float angle;
	float sp, sy, cp, cy;
	vec3_t forward;
	float dist = 64;
	vec3_t v;
	float ltime;
	float time;

	time = (float)cl.time;

	if (!avelocities[0][0])
	{
		for (i = 0; i < NUMVERTEXNORMALS; i++)
		{
			avelocities[i][0] = (randk() & 255) * 0.01f;
			avelocities[i][1] = (randk() & 255) * 0.01f;
			avelocities[i][2] = (randk() & 255) * 0.01f;
		}
	}

	ltime = time / 1000.0;

	for (i = 0; i < NUMVERTEXNORMALS; i++)
	{
		angle = ltime * avelocities[i][0];
		sy = (float)sin(angle);
		cy = (float)cos(angle);
		angle = ltime * avelocities[i][1];
		sp = (float)sin(angle);
		cp = (float)cos(angle);

		forward[0] = cp * cy;
		forward[1] = cp * sy;
		forward[2] = -sp;

		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = time;

		dist = (float)sin(ltime + i) * 64;
		p->org[0] = ent->origin[0] + bytedirs[i][0] * dist + forward[0] * 16;
		p->org[1] = ent->origin[1] + bytedirs[i][1] * dist + forward[1] * 16;
		p->org[2] = ent->origin[2] + bytedirs[i][2] * dist + forward[2] * 16;

		VectorClear(p->vel);
		VectorClear(p->accel);

		VectorSubtract(p->org, ent->origin, v);
		dist = VectorLength(v) / 90.0f;
		p->color = CL_CombineColors(0xff00ff00, 0xffffffff, dist);
		p->alpha = 1.0f - dist;
		p->alphavel = -100;
	}
}

void
CL_TrapParticles(entity_t *ent)
{
	vec3_t move;
	vec3_t vec;
	vec3_t start, end;
	int len;
	int j;
	cparticle_t *p;
	int dec;
	float time;

	time = (float)cl.time;

	ent->origin[2] -= 14;
	VectorCopy(ent->origin, start);
	VectorCopy(ent->origin, end);
	end[2] += 64;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = (int)VectorNormalize(vec);

	dec = 5;
	VectorScale(vec, 5, vec);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = time;

		p->alpha = 1.0;
		p->alphavel = -1.0f / (0.3f + frandk() * 0.2f);
		p->color = 0xff07abff;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crandk();
			p->vel[j] = crandk() * 15;
			p->accel[j] = 0;
		}

		p->accel[2] = PARTICLE_GRAVITY;

		VectorAdd(move, vec, move);
	}

	{
		int i, j, k;
		cparticle_t *p;
		float vel;
		vec3_t dir;
		vec3_t org;

		ent->origin[2] += 14;
		VectorCopy(ent->origin, org);

		for (i = -2; i <= 2; i += 4)
		{
			for (j = -2; j <= 2; j += 4)
			{
				for (k = -2; k <= 4; k += 4)
				{
					if (!free_particles)
					{
						return;
					}

					p = free_particles;
					free_particles = p->next;
					p->next = active_particles;
					active_particles = p;

					p->time = time;
					p->color = CL_CombineColors(0xff07abff, 0xff006be3,
						(float)(randk() & 15) / 15.0);
					p->alpha = 1.0;
					p->alphavel = -1.0f / (0.3f + (randk() & 7) * 0.02f);

					p->org[0] = org[0] + i + ((randk() & 23) * crandk());
					p->org[1] = org[1] + j + ((randk() & 23) * crandk());
					p->org[2] = org[2] + k + ((randk() & 23) * crandk());

					dir[0] = j * 8.0f;
					dir[1] = i * 8.0f;
					dir[2] = k * 8.0f;

					VectorNormalize(dir);
					vel = (float)(50 + (randk() & 63));
					VectorScale(dir, vel, p->vel);

					p->accel[0] = p->accel[1] = 0;
					p->accel[2] = -PARTICLE_GRAVITY;
				}
			}
		}
	}
}

void
CL_BFGExplosionParticles(vec3_t org)
{
	int i, j;
	cparticle_t *p;
	float time;

	time = (float)cl.time;

	for (i = 0; i < 256; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = time;
		p->color = CL_CombineColors(0xff00ff00, 0xffffffff,
			(float)(randk() & 15) / 15.0);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((randk() % 32) - 16);
			p->vel[j] = (randk() % 384) - 192;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.8f / (0.5f + frandk() * 0.3f);
	}
}

void
CL_TeleportParticles(vec3_t org)
{
	int i, j, k;
	cparticle_t *p;
	float vel;
	vec3_t dir;
	float time;

	time = (float)cl.time;

	for (i = -16; i <= 16; i += 4)
	{
		for (j = -16; j <= 16; j += 4)
		{
			for (k = -16; k <= 32; k += 4)
			{
				if (!free_particles)
				{
					return;
				}

				p = free_particles;
				free_particles = p->next;
				p->next = active_particles;
				active_particles = p;

				p->time = time;
				p->color = CL_CombineColors(0xff6b6b6b, 0xffdbdbdb,
					(float)(randk() & 15) / 15.0);
				p->alpha = 1.0;
				p->alphavel = -1.0f / (0.3f + (randk() & 7) * 0.02f);

				p->org[0] = org[0] + i + (randk() & 3);
				p->org[1] = org[1] + j + (randk() & 3);
				p->org[2] = org[2] + k + (randk() & 3);

				dir[0] = j * 8.0f;
				dir[1] = i * 8.0f;
				dir[2] = k * 8.0f;

				VectorNormalize(dir);
				vel = (float)(50 + (randk() & 63));
				VectorScale(dir, vel, p->vel);

				p->accel[0] = p->accel[1] = 0;
				p->accel[2] = -PARTICLE_GRAVITY;
			}
		}
	}
}

/*
 * An entity has just been parsed that has an
 * event value. the female events are there for
 * backwards compatability
 */
extern struct sfx_s *cl_sfx_footsteps[4];

void
CL_EntityEvent(entity_xstate_t *ent)
{
	switch (ent->event)
	{
		case EV_ITEM_RESPAWN:
			S_StartSound(NULL, ent->number, CHAN_WEAPON,
				S_RegisterSound("items/respawn1.wav"), 1, ATTN_IDLE, 0);
			CL_ItemRespawnParticles(ent->origin);
			break;
		case EV_PLAYER_TELEPORT:
			S_StartSound(NULL, ent->number, CHAN_WEAPON,
				S_RegisterSound("misc/tele1.wav"), 1, ATTN_IDLE, 0);
			CL_TeleportParticles(ent->origin);
			break;
		case EV_FOOTSTEP:

			if (cl_footsteps->value)
			{
				vec3_t mins = {0, 0, 0}, maxs = {0, 0, 0}, dir = { 0, 0, -1000000 };
				struct sfx_s *sfx = NULL;
				trace_t trace;
				trace = CM_BoxTrace(ent->origin, dir, mins, maxs, 0, MASK_DEADSOLID);

				/* ladder does not have separate material */
				if ((trace.contents & CONTENTS_LADDER))
				{
					char name[MAX_QPATH];
					int step;

					/* material has sometime 5 steps versions */
					step = randk() % 5;

					Com_sprintf(name, sizeof(name), "player/ladder%i.wav",
						step + 1);

					sfx = S_RegisterSound(name);
				}

				/* step sound based onb material */
				if (!sfx && trace.surface->material[0])
				{
					char name[MAX_QPATH];
					int step;

					/* material has sometime 5 steps versions */
					step = randk() % 5;

					Com_sprintf(name, sizeof(name), "player/%s%i.wav",
						trace.surface->material, step + 1);

					sfx = S_RegisterSound(name);
				}

				/* no material steps sound found */
				if (!sfx)
				{
					int step;

					step = randk() & 3;
					sfx = cl_sfx_footsteps[step];
				}

				S_StartSound(NULL, ent->number, CHAN_BODY,
						sfx, 1, ATTN_NORM, 0);
			}

			break;
		case EV_FALLSHORT:
			S_StartSound(NULL, ent->number, CHAN_AUTO,
				S_RegisterSound("player/land1.wav"), 1, ATTN_NORM, 0);
			break;
		case EV_FALL:
			S_StartSound(NULL, ent->number, CHAN_AUTO,
				S_RegisterSound("*fall2.wav"), 1, ATTN_NORM, 0);
			break;
		case EV_FALLFAR:
			S_StartSound(NULL, ent->number, CHAN_AUTO,
				S_RegisterSound("*fall1.wav"), 1, ATTN_NORM, 0);
			break;
	}
}

void
CL_ClearEffects(void)
{
	CL_ClearParticles();
	CL_ClearDlights();
	CL_ClearLightStyles();
}

void
CL_FlameEffects(vec3_t origin)
{
	int n, count;

	count = rand() & 0xF;

	/* Particles going down */
	for(n = 0; n < count; n++)
	{
		cparticle_t *p;
		int j;

		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		VectorClear(p->accel);
		p->time = cl.time;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1 + frandk() * 0.2);
		p->color = CL_CombineColors(0xff007fef, 0xff003bb7,
					(float)(randk() & 15) / 15.0);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = origin[j] + crandk() * 5;
			p->vel[j] = crandk() * 5;
		}

		p->vel[2] = crandk() * -10;
		p->accel[2] = -PARTICLE_GRAVITY;
	}

	count = rand() & 0x7;

	/* Particles go up */
	for (n = 0; n < count; n++)
	{
		cparticle_t *p;
		int j;

		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = cl.time;

		p->alpha = 1.0;
		p->alphavel = -1.0 / (1 + frandk() * 0.5);
		p->color = CL_CombineColors(0xff0000ff, 0xff002f2f,
					(float)(randk() & 15) / 15.0);
		for (j=0 ; j<3 ; j++)
		{
			p->org[j] = origin[j] + crandk() * 3;
		}
		p->vel[2] = 20 + crandk() * 5;
	}
}

void
CL_Flashlight(int ent, vec3_t pos)
{
	cdlight_t *dl;

	dl = CL_AllocDlight(ent);
	VectorCopy(pos, dl->origin);
	dl->radius = 400;
	dl->minlight = 250;
	dl->die = cl.time + 100;
	dl->color[0] = 1;
	dl->color[1] = 1;
	dl->color[2] = 1;
}

void
CL_ColorFlash(vec3_t pos, int ent, float intensity, float r, float g, float b)
{
	cdlight_t *dl;

	dl = CL_AllocDlight(ent);
	VectorCopy(pos, dl->origin);
	dl->radius = intensity;
	dl->minlight = 250;
	dl->die = cl.time + 100;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
}

void
CL_DebugTrail(vec3_t start, vec3_t end)
{
	vec3_t move;
	vec3_t vec;
	float len;
	cparticle_t *p;
	float dec;
	vec3_t right, up;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	MakeNormalVectors(vec, right, up);

	dec = 3;
	VectorScale(vec, dec, vec);
	VectorCopy(start, move);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = (float)cl.time;
		VectorClear(p->accel);
		VectorClear(p->vel);
		p->alpha = 1.0;
		p->alphavel = -0.1f;
		p->color = CL_CombineColors(0xff6f5317, 0xff2b1f00,
			(float)(randk() & 15) / 15.0);
		VectorCopy(move, p->org);
		VectorAdd(move, vec, move);
	}
}

void
CL_SmokeTrail(vec3_t start, vec3_t end, unsigned int basecolor, unsigned int finalcolor,
		int spacing)
{
	vec3_t move;
	vec3_t vec;
	float len, time;
	int j;
	cparticle_t *p;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	VectorScale(vec, spacing, vec);

	time = (float)cl.time;

	while (len > 0)
	{
		len -= spacing;

		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = time;

		p->alpha = 1.0;
		p->alphavel = -1.0f / (1 + frandk() * 0.5f);
		p->color = CL_CombineColors(basecolor, finalcolor,
			(float)(randk() & 15) / 15.0);


		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crandk() * 3;
			p->accel[j] = 0;
		}

		p->vel[2] = 20 + crandk() * 5;

		VectorAdd(move, vec, move);
	}
}

void
CL_ForceWall(vec3_t start, vec3_t end, int color)
{
	vec3_t move;
	vec3_t vec;
	int j;
	cparticle_t *p;

	float len, time;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	VectorScale(vec, 4, vec);

	time = (float)cl.time;

	while (len > 0)
	{
		len -= 4;

		if (!free_particles)
		{
			return;
		}

		if (frandk() > 0.3)
		{
			p = free_particles;
			free_particles = p->next;
			p->next = active_particles;
			active_particles = p;
			VectorClear(p->accel);

			p->time = time;

			p->alpha = 1.0;
			p->alphavel = -1.0f / (3.0 + frandk() * 0.5f);
			p->color = color;

			for (j = 0; j < 3; j++)
			{
				p->org[j] = move[j] + crandk() * 3;
				p->accel[j] = 0;
			}

			p->vel[0] = 0;
			p->vel[1] = 0;
			p->vel[2] = -40 - (crandk() * 10);
		}

		VectorAdd(move, vec, move);
	}
}

/*
 * CL_BubbleTrail2 (lets you control the # of bubbles
 * by setting the distance between the spawns)
 */
void
CL_BubbleTrail2(vec3_t start, vec3_t end, int dist)
{
	vec3_t move;
	vec3_t vec;
	float len, time;
	int i;
	int j;
	cparticle_t *p;

	time = (float)cl.time;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	VectorScale(vec, dist, vec);

	for (i = 0; i < len; i += dist)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		VectorClear(p->accel);
		p->time = time;

		p->alpha = 1.0;
		p->alphavel = -1.0f / (1 + frandk() * 0.1f);
		p->color = CL_CombineColors(0xff3f3f3f, 0xffababab,
			(float)(randk() & 15) / 15.0);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crandk() * 2;
			p->vel[j] = crandk() * 10;
		}

		p->org[2] -= 4;
		p->vel[2] += 20;
		VectorAdd(move, vec, move);
	}
}

void
CL_Heatbeam(vec3_t start, vec3_t forward)
{
	vec3_t move;
	vec3_t vec;
	float len;
	int j;
	cparticle_t *p;
	vec3_t right, up;
	float i;
	float c, s;
	vec3_t dir;
	float ltime;
	float step = 32.0, rstep;
	float start_pt;
	float rot;
	float variance;
	float time;
	vec3_t end;

	VectorMA(start, 4096, forward, end);

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	VectorCopy(cl.v_right, right);
	VectorCopy(cl.v_up, up);

	VectorMA(move, -0.5, right, move);
	VectorMA(move, -0.5, up, move);

	time = (float)cl.time;

	ltime = (float)cl.time / 1000.0f;
	start_pt = (float)fmod(ltime * 96.0f, step);
	VectorMA(move, start_pt, vec, move);

	VectorScale(vec, step, vec);

	rstep = M_PI / 10.0f;

	for (i = start_pt; i < len; i += step)
	{
		if (i > step * 5) /* don't bother after the 5th ring */
		{
			break;
		}

		for (rot = 0; rot < M_PI * 2; rot += rstep)
		{
			if (!free_particles)
			{
				return;
			}

			p = free_particles;
			free_particles = p->next;
			p->next = active_particles;
			active_particles = p;

			p->time = time;
			VectorClear(p->accel);
			variance = 0.5;
			c = (float)cos(rot) * variance;
			s = (float)sin(rot) * variance;

			/* trim it so it looks like it's starting at the origin */
			if (i < 10)
			{
				VectorScale(right, c * (i / 10.0f), dir);
				VectorMA(dir, s * (i / 10.0f), up, dir);
			}
			else
			{
				VectorScale(right, c, dir);
				VectorMA(dir, s, up, dir);
			}

			p->alpha = 0.5;
			p->alphavel = -1000.0;
			p->color = CL_CombineColors(0xff0fbfff, 0xff003bb7,
				(float)(randk() & 15) / 15.0);

			for (j = 0; j < 3; j++)
			{
				p->org[j] = move[j] + dir[j] * 3;
				p->vel[j] = 0;
			}
		}

		VectorAdd(move, vec, move);
	}
}

/*
 *Puffs with velocity along direction, with some randomness thrown in
 */
void
CL_ParticleSteamEffect(vec3_t org, vec3_t dir, unsigned int basecolor, unsigned int finalcolor,
		int count, int magnitude)
{
	int i, j;
	cparticle_t *p;
	float d, time;
	vec3_t r, u;

	time = (float)cl.time;
	MakeNormalVectors(dir, r, u);

	for (i = 0; i < count; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = time;
		p->color = CL_CombineColors(basecolor, finalcolor,
			(float)(randk() & 15) / 15.0);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + magnitude * 0.1f * crandk();
		}

		VectorScale(dir, magnitude, p->vel);
		d = crandk() * magnitude / 3;
		VectorMA(p->vel, d, r, p->vel);
		d = crandk() * magnitude / 3;
		VectorMA(p->vel, d, u, p->vel);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY / 2;
		p->alpha = 1.0;

		p->alphavel = -1.0f / (0.5f + frandk() * 0.3f);
	}
}

void
CL_ParticleSteamEffect2(cl_sustain_t *self)
{
	int i, j;
	cparticle_t *p;
	float d;
	vec3_t r, u;
	vec3_t dir;

	VectorCopy(self->dir, dir);
	MakeNormalVectors(dir, r, u);

	for (i = 0; i < self->count; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = cl.time;
		p->color = CL_CombineColors(self->basecolor, self->finalcolor,
			(float)(randk() & 15) / 15.0);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = self->org[j] + self->magnitude * 0.1 * crandk();
		}

		VectorScale(dir, self->magnitude, p->vel);
		d = crandk() * self->magnitude / 3;
		VectorMA(p->vel, d, r, p->vel);
		d = crandk() * self->magnitude / 3;
		VectorMA(p->vel, d, u, p->vel);

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY / 2;
		p->alpha = 1.0;

		p->alphavel = -1.0 / (0.5 + frandk() * 0.3);
	}

	self->nextthink += self->thinkinterval;
}

void
CL_TrackerTrail(vec3_t start, vec3_t end, unsigned int color)
{
	vec3_t move;
	vec3_t vec;
	vec3_t forward, right, up, angle_dir;
	float len;
	int j;
	cparticle_t *p;
	int dec;
	float dist;
	float time;

	time = (float)cl.time;
	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	VectorCopy(vec, forward);
	AngleVectors2(forward, angle_dir);
	AngleVectors(angle_dir, forward, right, up);

	dec = 3;
	VectorScale(vec, 3, vec);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = time;

		p->alpha = 1.0;
		p->alphavel = -2.0;
		p->color = color;
		dist = DotProduct(move, forward);
		VectorMA(move, 8 * cos(dist), up, p->org);

		for (j = 0; j < 3; j++)
		{
			p->vel[j] = 0;
			p->accel[j] = 0;
		}

		p->vel[2] = 5;

		VectorAdd(move, vec, move);
	}
}

void
CL_Tracker_Shell(vec3_t origin)
{
	vec3_t dir;
	int i;
	cparticle_t *p;
	float time;

	time = (float)cl.time;

	for (i = 0; i < 300; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = time;

		p->alpha = 1.0;
		p->alphavel = INSTANT_PARTICLE;
		p->color = 0xff000000; /* black */
		dir[0] = crandk();
		dir[1] = crandk();
		dir[2] = crandk();
		VectorNormalize(dir);

		VectorMA(origin, 40, dir, p->org);
	}
}

void
CL_MonsterPlasma_Shell(vec3_t origin)
{
	vec3_t dir;
	int i;
	cparticle_t *p;
	float time;

	time = (float)cl.time;

	for (i = 0; i < 40; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = time;

		p->alpha = 1.0;
		p->alphavel = INSTANT_PARTICLE;
		p->color = 0xff07abff;
		dir[0] = crandk();
		dir[1] = crandk();
		dir[2] = crandk();
		VectorNormalize(dir);

		VectorMA(origin, 10, dir, p->org);
	}
}

void
CL_Widowbeamout(cl_sustain_t *self)
{
	vec3_t dir;
	int i;
	cparticle_t *p;
	float ratio;
	float time;

	ratio = 1.0f - (((float)self->endtime - (float)cl.time) / 2100.0f);
	time = (float)cl.time;

	for (i = 0; i < 300; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = time;

		p->alpha = 1.0;
		p->alphavel = INSTANT_PARTICLE;
		p->color = default_colortable[randk() & 3];
		dir[0] = crandk();
		dir[1] = crandk();
		dir[2] = crandk();
		VectorNormalize(dir);

		VectorMA(self->org, (45.0 * ratio), dir, p->org);
	}
}

void
CL_Nukeblast(cl_sustain_t *self)
{
	vec3_t dir;
	int i;
	cparticle_t *p;
	float ratio;
	float time;

	ratio = 1.0f - (((float)self->endtime - (float)cl.time) / 1000.0f);
	time = (float)cl.time;

	for (i = 0; i < 700; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = time;

		p->alpha = 1.0;
		p->alphavel = INSTANT_PARTICLE;
		p->color = nuke_colortable[randk() & 3];
		dir[0] = crandk();
		dir[1] = crandk();
		dir[2] = crandk();
		VectorNormalize(dir);

		VectorMA(self->org, (200.0 * ratio), dir, p->org);
	}
}

void
CL_WidowSplash(vec3_t org)
{
	int i;
	cparticle_t *p;
	vec3_t dir;
	float time;

	time = (float)cl.time;

	for (i = 0; i < 256; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = time;
		p->color = default_colortable[randk() & 3];
		dir[0] = crandk();
		dir[1] = crandk();
		dir[2] = crandk();
		VectorNormalize(dir);
		VectorMA(org, 45.0, dir, p->org);
		VectorMA(vec3_origin, 40.0, dir, p->vel);

		p->accel[0] = p->accel[1] = 0;
		p->alpha = 1.0;

		p->alphavel = -0.8f / (0.5f + frandk() * 0.3f);
	}
}

void
CL_Tracker_Explode(vec3_t origin)
{
	vec3_t dir, backdir;
	int i;
	cparticle_t *p;
	float time;

	time = (float)cl.time;

	for (i = 0; i < 300; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = time;

		p->alpha = 1.0;
		p->alphavel = -1.0;
		p->color = 0xff000000; /* black */
		dir[0] = crandk();
		dir[1] = crandk();
		dir[2] = crandk();
		VectorNormalize(dir);
		VectorScale(dir, -1, backdir);

		VectorMA(origin, 64, dir, p->org);
		VectorScale(backdir, 64, p->vel);
	}
}

void
CL_TagTrail(vec3_t start, vec3_t end, int color)
{
	vec3_t move;
	vec3_t vec;
	float len;
	int j;
	cparticle_t *p;
	int dec;
	float time;

	time = (float)cl.time;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 5;
	VectorScale(vec, 5, vec);

	while (len >= 0)
	{
		len -= dec;

		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = time;

		p->alpha = 1.0;
		p->alphavel = -1.0f / (0.8f + frandk() * 0.2f);
		p->color = color;

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crandk() * 16;
			p->vel[j] = crandk() * 5;
			p->accel[j] = 0;
		}

		VectorAdd(move, vec, move);
	}
}

void
CL_ColorExplosionParticles(vec3_t org, unsigned int basecolor, unsigned int finalcolor)
{
	int i;
	int j;
	cparticle_t *p;
	float time;

	time = (float)cl.time;

	for (i = 0; i < 128; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = time;
		p->color = CL_CombineColors(basecolor, finalcolor,
			(float)(randk() & 15) / 15.0);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((randk() % 32) - 16);
			p->vel[j] = (randk() % 256) - 128;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -0.4f / (0.6f + frandk() * 0.2f);
	}
}

/*
 * Like the steam effect, but unaffected by gravity
 */
void
CL_ParticleSmokeEffect(vec3_t org, vec3_t dir, unsigned int basecolor, unsigned int finalcolor,
		int count, int magnitude)
{
	int i, j;
	cparticle_t *p;
	float d;
	vec3_t r, u;
	float time;

	time = (float)cl.time;

	MakeNormalVectors(dir, r, u);

	for (i = 0; i < count; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = time;
		p->color = CL_CombineColors(basecolor, finalcolor,
			(float)(randk() & 15) / 15.0);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + magnitude * 0.1f * crandk();
		}

		VectorScale(dir, magnitude, p->vel);
		d = crandk() * magnitude / 3;
		VectorMA(p->vel, d, r, p->vel);
		d = crandk() * magnitude / 3;
		VectorMA(p->vel, d, u, p->vel);

		p->accel[0] = p->accel[1] = p->accel[2] = 0;
		p->alpha = 1.0;

		p->alphavel = -1.0f / (0.5f + frandk() * 0.3f);
	}
}

/*
 * Wall impact puffs (Green)
 */
void
CL_BlasterParticles2(vec3_t org, vec3_t dir, unsigned int basecolor, unsigned int finalcolor)
{
	int i, j;
	cparticle_t *p;
	float d;
	int count;
	float time;

	time = (float)cl.time;

	count = 40;

	for (i = 0; i < count; i++)
	{
		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;

		p->time = time;
		d = (float)(randk() & 15);
		p->color = CL_CombineColors(basecolor, finalcolor,
			(float)(randk() & 15) / 15.0);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = org[j] + ((randk() & 7) - 4) + d * dir[j];
			p->vel[j] = dir[j] * 30 + crandk() * 40;
		}

		p->accel[0] = p->accel[1] = 0;
		p->accel[2] = -PARTICLE_GRAVITY;
		p->alpha = 1.0;

		p->alphavel = -1.0f / (0.5f + frandk() * 0.3f);
	}
}

/*
 * Green!
 */
void
CL_BlasterTrail2(vec3_t start, vec3_t end)
{
	vec3_t move;
	vec3_t vec;
	float len;
	int j;
	cparticle_t *p;
	int dec;
	float time;

	time = (float)cl.time;

	VectorCopy(start, move);
	VectorSubtract(end, start, vec);
	len = VectorNormalize(vec);

	dec = 5;
	VectorScale(vec, 5, vec);

	while (len > 0)
	{
		len -= dec;

		if (!free_particles)
		{
			return;
		}

		p = free_particles;
		free_particles = p->next;
		p->next = active_particles;
		active_particles = p;
		VectorClear(p->accel);

		p->time = time;

		p->alpha = 1.0;
		p->alphavel = -1.0f / (float)(0.3f + frandk() * 0.2f);

		for (j = 0; j < 3; j++)
		{
			p->org[j] = move[j] + crandk();
			p->vel[j] = crandk() * 5;
			p->accel[j] = 0;
		}

		VectorAdd(move, vec, move);
	}
}

