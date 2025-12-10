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
 * Interface between client <-> game and client calculations.
 *
 * =======================================================================
 */

#include "../header/local.h"
#include "../monster/misc/player.h"

static edict_t *pm_passent;

/*
 * The ugly as hell coop spawnpoint fixup function.
 * While coop was planed by id, it wasn't part of
 * the initial release and added later with patch
 * to version 2.00. The spawnpoints in some maps
 * were SNAFU, some have wrong targets and some
 * no name at all. Fix this by matching the coop
 * spawnpoint target names to the nearest named
 * single player spot.
 */
void
SP_FixCoopSpots(edict_t *self)
{
	edict_t *spot;
	vec3_t d;

	if (!self)
	{
		return;
	}

	/* Entity number 292 is an unnamed info_player_start
	   next to a named info_player_start. Delete it, if
	   we're in coop since it screws up the spawnpoint
	   selection heuristic in SelectCoopSpawnPoint().
	   This unnamed info_player_start is selected as
	   spawnpoint for player 0, therefor none of the
	   named info_coop_start() matches... */
	if (Q_stricmp(level.mapname, "xware") == 0)
	{
		if (self->s.number == 292)
		{
			G_FreeEdict(self);
			self = NULL;
			return;
		}
	}

	spot = NULL;

	while (1)
	{
		spot = G_Find(spot, FOFS(classname), "info_player_start");

		if (!spot)
		{
			return;
		}

		if (!spot->targetname)
		{
			continue;
		}

		VectorSubtract(self->s.origin, spot->s.origin, d);

		if (VectorLength(d) < 550)
		{
			if ((!self->targetname) || (Q_stricmp(self->targetname, spot->targetname) != 0))
			{
				self->targetname = spot->targetname;
			}

			return;
		}
	}
}

/*
 * Some maps have no coop spawnpoints at
 * all. Add these by injecting entities
 * into the map where they should have
 * been
 */
void
SP_CreateCoopSpots(edict_t *self)
{
	edict_t *spot;

	if (!self)
	{
		return;
	}

	if (Q_stricmp(level.mapname, "security") == 0)
	{
		spot = G_Spawn();
		spot->classname = "info_player_coop";
		spot->s.origin[0] = 188 - 64;
		spot->s.origin[1] = -164;
		spot->s.origin[2] = 80;
		spot->targetname = "jail3";
		spot->s.angles[YAW] = 90;

		spot = G_Spawn();
		spot->classname = "info_player_coop";
		spot->s.origin[0] = 188 + 64;
		spot->s.origin[1] = -164;
		spot->s.origin[2] = 80;
		spot->targetname = "jail3";
		spot->s.angles[YAW] = 90;

		spot = G_Spawn();
		spot->classname = "info_player_coop";
		spot->s.origin[0] = 188 + 128;
		spot->s.origin[1] = -164;
		spot->s.origin[2] = 80;
		spot->targetname = "jail3";
		spot->s.angles[YAW] = 90;

		return;
	}
}

/*
 * Some maps have no unnamed (e.g. generic)
 * info_player_start. This is no problem in
 * normal gameplay, but if the map is loaded
 * via console there is a huge chance that
 * the player will spawn in the wrong point.
 * Therefore create an unnamed info_player_start
 * at the correct point.
 */
static void
CreateUnnamedSpawnpoint(const edict_t *self, const char *mapname, const char *spotname)
{
	edict_t *spot;

	if (Q_stricmp(level.mapname, mapname) != 0)
	{
		return;
	}

	if (!self->targetname || Q_stricmp(self->targetname, spotname) != 0)
	{
		return;
	}

	spot = G_SpawnOptional();

	if (!spot)
	{
		return;
	}

	spot->classname = "info_player_start";

	VectorCopy(self->s.origin, spot->s.origin);
	spot->s.angles[YAW] = self->s.angles[YAW];
}

void
SP_CreateUnnamedSpawn(edict_t *self)
{
	if (!self)
	{
		return;
	}

	CreateUnnamedSpawnpoint(self, "mine1",  "mintro");
	CreateUnnamedSpawnpoint(self, "mine2",  "mine1");
	CreateUnnamedSpawnpoint(self, "mine3",  "mine2a");
	CreateUnnamedSpawnpoint(self, "mine4",  "mine3");
	CreateUnnamedSpawnpoint(self, "power2", "power1");
	CreateUnnamedSpawnpoint(self, "waste1", "power2");
	CreateUnnamedSpawnpoint(self, "waste2", "waste1");
	CreateUnnamedSpawnpoint(self, "city2",  "city2NL");
}

/*
 * QUAKED info_player_start (1 0 0) (-16 -16 -24) (16 16 32)
 *
 * The normal starting point for a level.
 */
void
SP_info_player_start(edict_t *self)
{
	if (!self)
	{
		return;
	}

	DynamicResetSpawnModels(self);

	/* Call function to hack unnamed spawn points */
	self->think = SP_CreateUnnamedSpawn;
	self->nextthink = level.time + FRAMETIME;

	if (coop->value &&
		Q_stricmp(level.mapname, "security") == 0)
	{
		/* invoke one of our gross, ugly, disgusting hacks */
		self->think = SP_CreateCoopSpots;
		self->nextthink = level.time + FRAMETIME;
	}

	/* Fix coop spawn points */
	SP_FixCoopSpots(self);
}

/*
 * QUAKED info_player_deathmatch (1 0 1) (-16 -16 -24) (16 16 32)
 *
 * potential spawning position for deathmatch games
 */
void
SP_info_player_deathmatch(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	DynamicResetSpawnModels(self);
	SP_misc_teleporter_dest(self);
}

/*
 * QUAKED info_player_coop (1 0 1) (-16 -16 -24) (16 16 32)
 * potential spawning position for coop games
 */
void
SP_info_player_coop(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!coop->value)
	{
		G_FreeEdict(self);
		return;
	}

	DynamicResetSpawnModels(self);

	if ((Q_stricmp(level.mapname, "jail2") == 0) ||
		(Q_stricmp(level.mapname, "jail4") == 0) ||
		(Q_stricmp(level.mapname, "mintro") == 0) ||
		(Q_stricmp(level.mapname, "mine1") == 0) ||
		(Q_stricmp(level.mapname, "mine2") == 0) ||
		(Q_stricmp(level.mapname, "mine3") == 0) ||
		(Q_stricmp(level.mapname, "mine4") == 0) ||
		(Q_stricmp(level.mapname, "lab") == 0) ||
		(Q_stricmp(level.mapname, "boss1") == 0) ||
		(Q_stricmp(level.mapname, "fact1") == 0) ||
		(Q_stricmp(level.mapname, "fact3") == 0) ||
		(Q_stricmp(level.mapname, "waste1") == 0) || /* really? */
		(Q_stricmp(level.mapname, "biggun") == 0) ||
		(Q_stricmp(level.mapname, "space") == 0) ||
		(Q_stricmp(level.mapname, "command") == 0) ||
		(Q_stricmp(level.mapname, "power2") == 0) ||
		(Q_stricmp(level.mapname, "strike") == 0) ||
		(Q_stricmp(level.mapname, "city2") == 0))
	{
		/* invoke one of our gross, ugly, disgusting hacks */
		self->think = SP_FixCoopSpots;
		self->nextthink = level.time + FRAMETIME;
	}
}

/*
 * QUAKED info_player_coop_lava (1 0 1) (-16 -16 -24) (16 16 32)
 *
 * potential spawning position for coop games on rmine2 where lava level
 * needs to be checked
 */
void
SP_info_player_coop_lava(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!coop->value)
	{
		G_FreeEdict(self);
		return;
	}

	DynamicResetSpawnModels(self);
}

/*
 * QUAKED info_player_intermission (1 0 1) (-16 -16 -24) (16 16 32)
 *
 * The deathmatch intermission point will be at one of these
 * Use 'angles' instead of 'angle', so you can set pitch or
 * roll as well as yaw.  'pitch yaw roll'
 */
void
SP_info_player_intermission(edict_t *self)
{
	/* This function cannot be removed
	 * since the info_player_intermission
	 * needs a callback function. Like
	 * every entity. */
	DynamicResetSpawnModels(self);
}

/* ======================================================================= */

void
player_pain(edict_t *self /* unused */, edict_t *other /* unused */,
		float kick /* unused */, int damage /* unused */)
{
	/* Player pain is handled at the end
	 * of the frame in P_DamageFeedback.
	 * This function is still here since
	 * the player is an entity and needs
	 * a pain callback */
}

static qboolean
IsFemale(edict_t *ent)
{
	char *info;

	if (!ent)
	{
		return false;
	}

	if (!ent->client)
	{
		return false;
	}

	info = Info_ValueForKey(ent->client->pers.userinfo, "gender");

	if (strstr(info, "crakhor"))
	{
		return true;
	}

	if ((info[0] == 'f') || (info[0] == 'F'))
	{
		return true;
	}

	return false;
}

static qboolean
IsNeutral(edict_t *ent)
{
	char *info;

	if (!ent)
	{
		return false;
	}

	if (!ent->client)
	{
		return false;
	}

	info = Info_ValueForKey(ent->client->pers.userinfo, "gender");

	if (strstr(info, "crakhor"))
	{
		return false;
	}

	if ((info[0] != 'f') && (info[0] != 'F') && (info[0] != 'm') &&
		(info[0] != 'M'))
	{
		return true;
	}

	return false;
}

void
ClientObituary(edict_t *self, edict_t *inflictor /* unused */,
		edict_t *attacker)
{
	int mod;
	char *message;
	char *message2;
	qboolean ff;

	if (!self || !attacker || !inflictor)
	{
		return;
	}

	if (coop->value && attacker && attacker->client)
	{
		meansOfDeath |= MOD_FRIENDLY_FIRE;
	}

	if (deathmatch->value || coop->value)
	{
		ff = meansOfDeath & MOD_FRIENDLY_FIRE;
		mod = meansOfDeath & ~MOD_FRIENDLY_FIRE;
		message = NULL;
		message2 = "";

		switch (mod)
		{
			case MOD_SUICIDE:
				message = "suicides";
				break;
			case MOD_FALLING:
				message = "cratered";
				break;
			case MOD_CRUSH:
				message = "was squished";
				break;
			case MOD_WATER:
				message = "sank like a rock";
				break;
			case MOD_SLIME:
				message = "melted";
				break;
			case MOD_LAVA:
				message = "does a back flip into the lava";
				break;
			case MOD_EXPLOSIVE:
			case MOD_BARREL:
				message = "blew up";
				break;
			case MOD_EXIT:
				message = "found a way out";
				break;
			case MOD_TARGET_LASER:
				message = "saw the light";
				break;
			case MOD_TARGET_BLASTER:
				message = "got blasted";
				break;
			case MOD_BOMB:
			case MOD_SPLASH:
			case MOD_TRIGGER_HURT:
				message = "was in the wrong place";
				break;
			case MOD_GEKK:
			case MOD_BRAINTENTACLE:
				message = "that's gotta hurt";
				break;
			default:
				break;
		}

		if (attacker == self)
		{
			switch (mod)
			{
				case MOD_HELD_GRENADE:
					message = "tried to put the pin back in";
					break;
				case MOD_HG_SPLASH:
				case MOD_G_SPLASH:

					if (IsNeutral(self))
					{
						message = "tripped on its own grenade";
					}
					else if (IsFemale(self))
					{
						message = "tripped on her own grenade";
					}
					else
					{
						message = "tripped on his own grenade";
					}

					break;
				case MOD_R_SPLASH:

					if (IsNeutral(self))
					{
						message = "blew itself up";
					}
					else if (IsFemale(self))
					{
						message = "blew herself up";
					}
					else
					{
						message = "blew himself up";
					}

					break;
				case MOD_BFG_BLAST:
					message = "should have used a smaller gun";
					break;
				case MOD_DOPPLE_EXPLODE:

					if (IsNeutral(self))
					{
						message = "got caught in it's own trap";
					}
					else if (IsFemale(self))
					{
						message = "got caught in her own trap";
					}
					else
					{
						message = "got caught in his own trap";
					}

					break;
				case MOD_TRAP:
					message = "sucked into his own trap";
					break;
				default:

					if (IsNeutral(self))
					{
						message = "killed itself";
					}
					else if (IsFemale(self))
					{
						message = "killed herself";
					}
					else
					{
						message = "killed himself";
					}

					break;
			}
		}

		if (message)
		{
			gi.bprintf(PRINT_MEDIUM, "%s %s.\n",
					self->client->pers.netname,
					message);

			if (deathmatch->value)
			{
				self->client->resp.score--;
			}

			self->enemy = NULL;
			return;
		}

		self->enemy = attacker;

		if (attacker && attacker->client)
		{
			switch (mod)
			{
				case MOD_BLASTER:
					message = "was blasted by";
					break;
				case MOD_SHOTGUN:
					message = "was gunned down by";
					break;
				case MOD_SSHOTGUN:
					message = "was blown away by";
					message2 = "'s super shotgun";
					break;
				case MOD_MACHINEGUN:
					message = "was machinegunned by";
					break;
				case MOD_CHAINGUN:
					message = "was cut in half by";
					message2 = "'s chaingun";
					break;
				case MOD_GRENADE:
					message = "was popped by";
					message2 = "'s grenade";
					break;
				case MOD_G_SPLASH:
					message = "was shredded by";
					message2 = "'s shrapnel";
					break;
				case MOD_ROCKET:
					message = "ate";
					message2 = "'s rocket";
					break;
				case MOD_R_SPLASH:
					message = "almost dodged";
					message2 = "'s rocket";
					break;
				case MOD_HYPERBLASTER:
					message = "was melted by";
					message2 = "'s hyperblaster";
					break;
				case MOD_RAILGUN:
					message = "was railed by";
					break;
				case MOD_BFG_LASER:
					message = "saw the pretty lights from";
					message2 = "'s BFG";
					break;
				case MOD_BFG_BLAST:
					message = "was disintegrated by";
					message2 = "'s BFG blast";
					break;
				case MOD_BFG_EFFECT:
					message = "couldn't hide from";
					message2 = "'s BFG";
					break;
				case MOD_HANDGRENADE:
					message = "caught";
					message2 = "'s handgrenade";
					break;
				case MOD_HG_SPLASH:
					message = "didn't see";
					message2 = "'s handgrenade";
					break;
				case MOD_HELD_GRENADE:
					message = "feels";
					message2 = "'s pain";
					break;
				case MOD_TELEFRAG:
					message = "tried to invade";
					message2 = "'s personal space";
					break;
				case MOD_GRAPPLE:
					message = "was caught by";
					message2 = "'s grapple";
					break;
				case MOD_RIPPER:
					message = "ripped to shreds by";
					message2 = "'s ripper gun";
					break;
				case MOD_PHALANX:
					message = "was evaporated by";
					break;
				case MOD_TRAP:
					message = "caught in trap by";
					break;
				case MOD_CHAINFIST:
					message = "was shredded by";
					message2 = "'s ripsaw";
					break;
				case MOD_DISINTEGRATOR:
					message = "lost his grip courtesy of";
					message2 = "'s disintegrator";
					break;
				case MOD_ETF_RIFLE:
					message = "was perforated by";
					break;
				case MOD_HEATBEAM:
					message = "was scorched by";
					message2 = "'s plasma beam";
					break;
				case MOD_TESLA:
					message = "was enlightened by";
					message2 = "'s tesla mine";
					break;
				case MOD_PROX:
					message = "got too close to";
					message2 = "'s proximity mine";
					break;
				case MOD_NUKE:
					message = "was nuked by";
					message2 = "'s antimatter bomb";
					break;
				case MOD_VENGEANCE_SPHERE:
					message = "was purged by";
					message2 = "'s vengeance sphere";
					break;
				case MOD_DEFENDER_SPHERE:
					message = "had a blast with";
					message2 = "'s defender sphere";
					break;
				case MOD_HUNTER_SPHERE:
					message = "was killed like a dog by";
					message2 = "'s hunter sphere";
					break;
				case MOD_TRACKER:
					message = "was annihilated by";
					message2 = "'s disruptor";
					break;
				case MOD_DOPPLE_EXPLODE:
					message = "was blown up by";
					message2 = "'s doppleganger";
					break;
				case MOD_DOPPLE_VENGEANCE:
					message = "was purged by";
					message2 = "'s doppleganger";
					break;
				case MOD_DOPPLE_HUNTER:
					message = "was hunted down by";
					message2 = "'s doppleganger";
					break;
				default:
					break;
			}

			if (message)
			{
				gi.bprintf(PRINT_MEDIUM, "%s %s %s%s\n",
						self->client->pers.netname,
						message, attacker->client->pers.netname,
						message2);

				if (gamerules && gamerules->value)
				{
					if (DMGame.Score)
					{
						if (ff)
						{
							DMGame.Score(attacker, self, -1);
						}
						else
						{
							DMGame.Score(attacker, self, 1);
						}
					}

					return;
				}

				if (deathmatch->value)
				{
					if (ff)
					{
						attacker->client->resp.score--;
					}
					else
					{
						attacker->client->resp.score++;
					}
				}

				return;
			}
		}
	}

	gi.bprintf(PRINT_MEDIUM, "%s died.\n", self->client->pers.netname);

	if (deathmatch->value)
	{
		if (gamerules && gamerules->value)
		{
			if (DMGame.Score)
			{
				DMGame.Score(self, self, -1);
			}

			return;
		}
		else
		{
			self->client->resp.score--;
		}
	}
}

void
TossClientWeapon(edict_t *self)
{
	gitem_t *item;
	edict_t *drop;
	qboolean quad;
	qboolean quadfire;
	float spread;

	if (!self)
	{
		return;
	}

	if (!deathmatch->value)
	{
		return;
	}

	item = self->client->pers.weapon;

	if (!self->client->pers.inventory[self->client->ammo_index])
	{
		item = NULL;
	}

	if (item && (strcmp(item->pickup_name, "Blaster") == 0))
	{
		item = NULL;
	}

	if (!((int)(dmflags->value) & DF_QUAD_DROP))
	{
		quad = false;
	}
	else
	{
		quad = (self->client->quad_framenum > (level.framenum + 10));
	}

	if (!((int)(dmflags->value) & DF_QUADFIRE_DROP))
	{
		quadfire = false;
	}
	else
	{
		quadfire = (self->client->quadfire_framenum > (level.framenum + 10));
	}

	if (item && quad)
	{
		spread = 22.5;
	}
	else if (item && quadfire)
	{
		spread = 12.5;
	}
	else
	{
		spread = 0.0;
	}

	if (item)
	{
		self->client->v_angle[YAW] -= spread;
		drop = Drop_Item(self, item);
		self->client->v_angle[YAW] += spread;
		drop->spawnflags = DROPPED_PLAYER_ITEM;
	}

	if (quad)
	{
		self->client->v_angle[YAW] += spread;
		drop = Drop_Item(self, FindItemByClassname("item_quad"));
		self->client->v_angle[YAW] -= spread;
		drop->spawnflags |= DROPPED_PLAYER_ITEM;

		drop->touch = Touch_Item;
		drop->nextthink = level.time +
						(self->client->quad_framenum -
						   level.framenum) * FRAMETIME;
		drop->think = G_FreeEdict;
	}

	if (quadfire)
	{
		self->client->v_angle[YAW] += spread;
		drop = Drop_Item(self, FindItemByClassname("item_quadfire"));
		self->client->v_angle[YAW] -= spread;
		drop->spawnflags |= DROPPED_PLAYER_ITEM;

		drop->touch = Touch_Item;
		drop->nextthink = level.time + (self->client->quadfire_framenum -
						   level.framenum) * FRAMETIME;
		drop->think = G_FreeEdict;
	}
}

void
LookAtKiller(edict_t *self, edict_t *inflictor, edict_t *attacker)
{
	vec3_t dir;

	if (!self)
	{
		return;
	}

	if (attacker && (attacker != world) && (attacker != self))
	{
		VectorSubtract(attacker->s.origin, self->s.origin, dir);
	}
	else if (inflictor && (inflictor != world) && (inflictor != self))
	{
		VectorSubtract(inflictor->s.origin, self->s.origin, dir);
	}
	else
	{
		self->client->killer_yaw = self->s.angles[YAW];
		return;
	}

	if (dir[0])
	{
		self->client->killer_yaw = 180 / M_PI * atan2(dir[1], dir[0]);
	}
	else if (dir[1] > 0)
	{
		self->client->killer_yaw = 90;
	}
	else if (dir[1] < 0)
	{
		self->client->killer_yaw = 270;
	}
	else
	{
		self->client->killer_yaw = 0;

		if (dir[1] > 0)
		{
			self->client->killer_yaw = 90;
		}
		else if (dir[1] < 0)
		{
			self->client->killer_yaw = -90;
		}
	}

	if (self->client->killer_yaw < 0)
	{
		self->client->killer_yaw += 360;
	}
}

void
player_die(edict_t *self, edict_t *inflictor, edict_t *attacker,
		int damage, vec3_t point /* unused */)
{
	int n;

	if (!self || !inflictor || !attacker)
	{
		return;
	}

	if (self->client->chasetoggle)
	{
		ChasecamRemove(self);
		self->client->pers.chasetoggle = 1;
	}
	else
	{
		self->client->pers.chasetoggle = 0;
	}

	VectorClear(self->avelocity);

	self->takedamage = DAMAGE_YES;
	self->movetype = MOVETYPE_TOSS;

	self->s.modelindex2 = 0; /* remove linked weapon model */
	self->s.modelindex3 = 0; /* remove linked ctf flag */

	self->s.angles[PITCH] = 0.0;
	self->s.angles[ROLL] = 0.0;

	self->s.sound = 0;
	self->client->weapon_sound = 0;

	self->maxs[2] = -8;

	self->svflags |= SVF_DEADMONSTER;

	if (!self->deadflag)
	{
		self->client->respawn_time = level.time + 1.0;
		LookAtKiller(self, inflictor, attacker);
		self->client->ps.pmove.pm_type = PM_DEAD;
		ClientObituary(self, inflictor, attacker);
		TossClientWeapon(self);

		/* if at start and same team, clear */
		if (ctf->value && (meansOfDeath == MOD_TELEFRAG) &&
			(self->client->resp.ctf_state < 2) &&
			(self->client->resp.ctf_team == attacker->client->resp.ctf_team))
		{
			attacker->client->resp.score--;
			self->client->resp.ctf_state = 0;
		}

		CTFFragBonuses(self, inflictor, attacker);
		TossClientWeapon(self);
		CTFPlayerResetGrapple(self);
		CTFDeadDropFlag(self);
		CTFDeadDropTech(self);

		if (deathmatch->value && !self->client->showscores)
		{
			Cmd_Help_f(self); /* show scores */
		}

		/* clear inventory: this is kind of ugly, but
		   it's how we want to handle keys in coop */
		for (n = 0; n < game.num_items; n++)
		{
			if (coop->value && itemlist[n].flags & IT_KEY)
			{
				self->client->resp.coop_respawn.inventory[n] =
					self->client->pers.inventory[n];
			}

			self->client->pers.inventory[n] = 0;
		}
	}

	if (gamerules && gamerules->value) /* if we're in a dm game, alert the game */
	{
		if (DMGame.PlayerDeath)
		{
			DMGame.PlayerDeath(self, inflictor, attacker);
		}
	}

	/* remove powerups */
	self->client->quad_framenum = 0;
	self->client->invincible_framenum = 0;
	self->client->invisible_framenum = 0;
	self->client->breather_framenum = 0;
	self->client->enviro_framenum = 0;
	self->flags &= ~FL_POWER_ARMOR;

	self->client->quadfire_framenum = 0;
	self->client->double_framenum = 0;

	if (self->client->owned_sphere)
	{
		edict_t *sphere;

		sphere = self->client->owned_sphere;
		sphere->die(sphere, self, self, 0, vec3_origin);
	}

	/* clear inventory */
	memset(self->client->pers.inventory, 0, sizeof(self->client->pers.inventory));

	/* if we've been killed by the tracker, GIB! */
	if ((meansOfDeath & ~MOD_FRIENDLY_FIRE) == MOD_TRACKER)
	{
		self->health = -100;
		damage = 400;
	}

	/* make sure no trackers are still hurting us. */
	if (self->client->tracker_pain_framenum)
	{
		RemoveAttackingPainDaemons(self);
	}

	/* if we got obliterated by the nuke, don't gib */
	if ((self->health < -80) && (meansOfDeath == MOD_NUKE))
	{
		self->flags |= FL_NOGIB;
	}

	if (self->health < -40)
	{
		/* don't toss gibs if we got vaped by the nuke */
		if (!(self->flags & FL_NOGIB))
		{
			/* gib (sound is played at end of server frame) */
			self->sounds = gi.soundindex("misc/udeath.wav");

			/* more meaty gibs for your dollar! */
			if ((deathmatch->value) && (self->health < -80))
			{
				for (n = 0; n < 4; n++)
				{
					ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
				}
			}

			for (n = 0; n < 4; n++)
			{
				ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
			}
		}

		self->flags &= ~FL_NOGIB;
		ThrowClientHead(self, damage);
		self->client->anim_priority = ANIM_DEATH;
		self->client->anim_end = 0;
		self->takedamage = DAMAGE_NO;
	}
	else
	{
		/* normal death */
		if (!self->deadflag)
		{
			int firstframe, lastframe, group = 0;
			const char *action;

			/* start a death animation */
			self->client->anim_priority = ANIM_DEATH;

			firstframe = FRAME_crdeath1;
			lastframe = FRAME_crdeath5;

			if (self->client->ps.pmove.pm_flags & PMF_DUCKED)
			{
				action = "crdeath";
			}
			else
			{
				group = randk() % 3;
				switch (group)
				{
					case 0:
						firstframe = FRAME_death101;
						lastframe = FRAME_death106;
						break;
					case 1:
						firstframe = FRAME_death201;
						lastframe = FRAME_death206;
						break;
					case 2:
						firstframe = FRAME_death301;
						lastframe = FRAME_death308;
						break;
				}

				action = "death";
			}

			P_SetAnimGroup(self, action, firstframe, lastframe, group);
			self->s.frame --;

			/* sound is played at end of server frame */
			if (!self->sounds)
			{
				self->sounds = gi.soundindex(va("*death%i.wav",
								(randk() % 4) + 1));
			}
		}
	}

	self->deadflag = DEAD_DEAD;

	gi.linkentity(self);
}

/* ======================================================================= */

static void
Player_GiveStartItems(edict_t *ent, const char *ptr)
{
	if (!ptr || !*ptr)
	{
		return;
	}

	while (*ptr)
	{
		char buffer[MAX_QPATH + 1] = {0};
		const char *buffer_end = NULL, *item_name = NULL;
		char *curr_buf;

		buffer_end = strchr(ptr, ';');
		if (!buffer_end)
		{
			buffer_end = ptr + strlen(ptr);
		}
		Q_strlcpy(buffer, ptr, Q_min(MAX_QPATH, buffer_end - ptr));

		curr_buf = buffer;
		item_name = COM_Parse(&curr_buf);
		if (item_name)
		{
			gitem_t *item;

			item = FindItemByClassname(item_name);
			if (!item || !item->pickup)
			{
				gi.dprintf("%s: Invalid g_start_item entry: %s\n", __func__, item_name);
			}
			else
			{
				edict_t *dummy;
				int count = 1;

				if (*curr_buf)
				{
					count = atoi(COM_Parse(&curr_buf));
				}

				if (count == 0)
				{
					ent->client->pers.inventory[ITEM_INDEX(item)] = 0;
				}
				else
				{
					dummy = G_Spawn();
					dummy->item = item;
					dummy->count = count;
					dummy->spawnflags |= DROPPED_PLAYER_ITEM;
					item->pickup(dummy, ent);
					G_FreeEdict(dummy);
				}
			}
		}

		/* skip end of section */
		ptr = buffer_end;
		if (*ptr == ';')
		{
			ptr ++;
		}
	}
}

/*
 * This is only called when the game first
 * initializes in single player, but is called
 * after each death and level change in deathmatch
 */
void
InitClientPersistant(edict_t *ent)
{
	gclient_t *client;
	gitem_t *item;

	client = ent->client;

	if (!client)
	{
		return;
	}

	memset(&client->pers, 0, sizeof(client->pers));

	item = FindItem("Blaster");
	client->pers.selected_item = ITEM_INDEX(item);
	client->pers.inventory[client->pers.selected_item] = 1;

	client->pers.weapon = item;
	client->pers.lastweapon = item;

	if (ctf->value)
	{
		/* Provide Grapple for ctf only */
		item = FindItem("Grapple");
		client->pers.inventory[ITEM_INDEX(item)] = 1;
	}

	client->pers.health = 100;
	client->pers.max_health = 100;

	client->pers.max_bullets = 200;
	client->pers.max_shells = 100;
	client->pers.max_rockets = 50;
	client->pers.max_grenades = 50;
	client->pers.max_cells = 200;
	client->pers.max_slugs = 50;

	client->pers.max_magslug = 50;
	client->pers.max_trap = 5;
	client->pers.max_prox = 50;
	client->pers.max_tesla = 50;
	client->pers.max_flechettes = 200;
	client->pers.max_rounds = 100;

	client->pers.connected = true;

	/* Default chasecam to off */
	client->pers.chasetoggle = 0;

	/* start items */
	if (*g_start_items->string)
	{
		if ((deathmatch->value || coop->value) && !sv_cheats->value)
		{
			gi.cprintf(ent, PRINT_HIGH,
				"You must run the server with '+set cheats 1' to enable 'g_start_items'.\n");
			return;
		}
		else
		{
			Player_GiveStartItems(ent, g_start_items->string);
		}
	}

	if (level.start_items && *level.start_items)
	{
		Player_GiveStartItems(ent, level.start_items);
	}
}

void
InitClientResp(gclient_t *client)
{
	if (!client)
	{
		return;
	}

	int ctf_team = client->resp.ctf_team;
	qboolean id_state = client->resp.id_state;

	memset(&client->resp, 0, sizeof(client->resp));

	client->resp.ctf_team = ctf_team;
	client->resp.id_state = id_state;

	client->resp.enterframe = level.framenum;
	client->resp.coop_respawn = client->pers;

	if (ctf->value && (client->resp.ctf_team < CTF_TEAM1))
	{
		CTFAssignTeam(client);
	}
}

/*
 * Some information that should be persistant, like health,
 * is still stored in the edict structure, so it needs to
 * be mirrored out to the client structure before all the
 * edicts are wiped.
 */
void
SaveClientData(void)
{
	int i;
	edict_t *ent;

	for (i = 0; i < game.maxclients; i++)
	{
		ent = &g_edicts[1 + i];

		if (!ent->inuse)
		{
			continue;
		}

		game.clients[i].pers.chasetoggle = ent->client->chasetoggle;
		game.clients[i].pers.health = ent->health;
		game.clients[i].pers.max_health = ent->max_health;
		game.clients[i].pers.savedFlags =
			(ent->flags & (FL_FLASHLIGHT | FL_GODMODE | FL_NOTARGET | FL_POWER_ARMOR));

		if (coop->value)
		{
			game.clients[i].pers.score = ent->client->resp.score;
		}
	}
}

static void
FetchClientEntData(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->health = ent->client->pers.health;
	ent->max_health = ent->client->pers.max_health;
	ent->flags |= ent->client->pers.savedFlags;

	if (coop->value)
	{
		ent->client->resp.score = ent->client->pers.score;
	}
}

/* ======================================================================= */

/*
 * Returns the distance to the
 * nearest player from the given spot
 */
float
PlayersRangeFromSpot(edict_t *spot)
{
	edict_t *player;
	float bestplayerdistance;
	vec3_t v;
	int n;
	float playerdistance;

	if (!spot)
	{
		return 0.0;
	}

	bestplayerdistance = 9999999;

	for (n = 1; n <= maxclients->value; n++)
	{
		player = &g_edicts[n];

		if (!player->inuse)
		{
			continue;
		}

		if (player->health <= 0)
		{
			continue;
		}

		VectorSubtract(spot->s.origin, player->s.origin, v);
		playerdistance = VectorLength(v);

		if (playerdistance < bestplayerdistance)
		{
			bestplayerdistance = playerdistance;
		}
	}

	return bestplayerdistance;
}

/*
 * go to a random point, but NOT the two
 * points closest to other players
 */
edict_t *
SelectRandomDeathmatchSpawnPoint(void)
{
	edict_t *spot, *spot1, *spot2;
	int count = 0;
	int selection;
	float range, range1, range2;

	spot = NULL;
	range1 = range2 = 99999;
	spot1 = spot2 = NULL;

	while ((spot = G_Find(spot, FOFS(classname),
					"info_player_deathmatch")) != NULL)
	{
		count++;
		range = PlayersRangeFromSpot(spot);

		if (range < range1)
		{
			range1 = range;
			spot1 = spot;
		}
		else if (range < range2)
		{
			range2 = range;
			spot2 = spot;
		}
	}

	if (!count)
	{
		return NULL;
	}

	if (count <= 2)
	{
		spot1 = spot2 = NULL;
	}
	else
	{
		if (spot1)
		{
			count--;
		}

		if (spot2)
		{
			count--;
		}
	}

	selection = randk() % count;

	spot = NULL;

	do
	{
		spot = G_Find(spot, FOFS(classname), "info_player_deathmatch");

		if ((spot == spot1) || (spot == spot2))
		{
			selection++;
		}
	}
	while (selection--);

	return spot;
}

edict_t *
SelectFarthestDeathmatchSpawnPoint(void)
{
	edict_t *bestspot;
	float bestdistance, bestplayerdistance;
	edict_t *spot;

	spot = NULL;
	bestspot = NULL;
	bestdistance = 0;

	while ((spot = G_Find(spot, FOFS(classname),
					"info_player_deathmatch")) != NULL)
	{
		bestplayerdistance = PlayersRangeFromSpot(spot);

		if (bestplayerdistance > bestdistance)
		{
			bestspot = spot;
			bestdistance = bestplayerdistance;
		}
	}

	if (bestspot)
	{
		return bestspot;
	}

	/* if there is a player just spawned on each and every start spot/
	   we have no choice to turn one into a telefrag meltdown */
	spot = G_Find(NULL, FOFS(classname), "info_player_deathmatch");

	return spot;
}

static edict_t *
SelectDeathmatchSpawnPoint(void)
{
	if ((int)(dmflags->value) & DF_SPAWN_FARTHEST)
	{
		return SelectFarthestDeathmatchSpawnPoint();
	}
	else
	{
		return SelectRandomDeathmatchSpawnPoint();
	}
}

static edict_t *
SelectLavaCoopSpawnPoint(edict_t *ent)
{
	int index;
	edict_t *spot = NULL;
	float lavatop;
	edict_t *lava;
	edict_t *pointWithLeastLava;
	float lowest;
	edict_t *spawnPoints[64];
	vec3_t center;
	int numPoints;
	edict_t *highestlava;

	if (!ent)
	{
		return NULL;
	}

	lavatop = -99999;
	highestlava = NULL;

	lava = NULL;

	while (1)
	{
		lava = G_Find(lava, FOFS(classname), "func_door");

		if (!lava)
		{
			break;
		}

		VectorAdd(lava->absmax, lava->absmin, center);
		VectorScale(center, 0.5, center);

		if (lava->spawnflags & 2 && (gi.pointcontents(center) & MASK_WATER))
		{
			if (lava->absmax[2] > lavatop)
			{
				lavatop = lava->absmax[2];
				highestlava = lava;
			}
		}
	}

	/* if we didn't find ANY lava, then return NULL */
	if (!highestlava)
	{
		return NULL;
	}

	/* find the top of the lava and include a small margin of error (plus bbox size) */
	lavatop = highestlava->absmax[2] + 64;

	/* find all the lava spawn points and store them in spawnPoints[] */
	spot = NULL;
	numPoints = 0;

	while ((spot = (G_Find(spot, FOFS(classname), "info_player_coop_lava"))))
	{
		if (numPoints == 64)
		{
			break;
		}

		spawnPoints[numPoints++] = spot;
	}

	if (numPoints < 1)
	{
		return NULL;
	}

	/* walk up the sorted list and return the lowest, open, non-lava spawn point */
	spot = NULL;
	lowest = 999999;
	pointWithLeastLava = NULL;

	for (index = 0; index < numPoints; index++)
	{
		if (spawnPoints[index]->s.origin[2] < lavatop)
		{
			continue;
		}

		if (PlayersRangeFromSpot(spawnPoints[index]) > 32)
		{
			if (spawnPoints[index]->s.origin[2] < lowest)
			{
				/* save the last point */
				pointWithLeastLava = spawnPoints[index];
				lowest = spawnPoints[index]->s.origin[2];
			}
		}
	}

	/* well, we may telefrag someone, but oh well... */
	if (pointWithLeastLava)
	{
		return pointWithLeastLava;
	}

	return NULL;
}

static edict_t *
SelectCoopSpawnPoint(edict_t *ent)
{
	int index;
	edict_t *spot = NULL;
	char *target;

	if (!ent)
	{
		return NULL;
	}

	if (!Q_stricmp(level.mapname, "rmine2p") || !Q_stricmp(level.mapname, "rmine2"))
	{
		return SelectLavaCoopSpawnPoint(ent);
	}

	index = ent->client - game.clients;

	/* player 0 starts in normal player spawn point */
	if (!index)
	{
		return NULL;
	}

	spot = NULL;

	/* assume there are four coop spots at each spawnpoint */
	while (1)
	{
		spot = G_Find(spot, FOFS(classname), "info_player_coop");

		if (!spot)
		{
			return NULL; /* we didn't have enough... */
		}

		target = spot->targetname;

		if (!target)
		{
			target = "";
		}

		if (Q_stricmp(game.spawnpoint, target) == 0)
		{
			/* this is a coop spawn point
			   for one of the clients here */
			index--;

			if (!index)
			{
				return spot; /* this is it */
			}
		}
	}

	return spot;
}

static edict_t *
SelectSpawnPointByTarget(const char *spawnpoint)
{
	edict_t *spot = NULL;
	while ((spot = G_Find(spot, FOFS(classname), "info_player_start")) != NULL)
	{
		if (!spawnpoint[0] && !spot->targetname)
		{
			break;
		}

		if (!spawnpoint[0] || !spot->targetname)
		{
			continue;
		}

		if (Q_stricmp(spawnpoint, spot->targetname) == 0)
		{
			break;
		}
	}

	return spot;
}

edict_t *
SP_GetSpawnPoint(void)
{
	edict_t *spot = NULL;

	spot = SelectSpawnPointByTarget(game.spawnpoint);
	if (!spot)
	{
		/* previous map use incorrect target, use default */
		spot = SelectSpawnPointByTarget("");
	}

	if (!spot)
	{
		if (!game.spawnpoint[0])
		{
			/* there wasn't a spawnpoint without a target, so use any */
			spot = G_Find(spot, FOFS(classname), "info_player_start");
		}

		if (!spot)
		{
			gi.error("Couldn't find spawn point '%s'\n", game.spawnpoint);
			return NULL;
		}
	}

	return spot;
}

/*
 * Chooses a player start, deathmatch start, coop start, etc
 */
void
SelectSpawnPoint(edict_t *ent, vec3_t origin, vec3_t angles)
{
	edict_t *spot = NULL;
	edict_t *coopspot = NULL;
	int dist;
	int index;
	int counter = 0;
	vec3_t d;

	if (!ent)
	{
		return;
	}

	if (deathmatch->value)
	{
		if (ctf->value)
		{
			spot = SelectCTFSpawnPoint(ent);
		}
		else
		{
			spot = SelectDeathmatchSpawnPoint();
		}
	}
	else if (coop->value)
	{
		spot = SelectCoopSpawnPoint(ent);
	}

	/* find a single player start spot */
	if (!spot)
	{
		spot = SP_GetSpawnPoint();
	}

	/* If we are in coop and we didn't find a coop
	   spawnpoint due to map bugs (not correctly
	   connected or the map was loaded via console
	   and thus no previously map is known to the
	   client) use one in 550 units radius. */
	if (coop->value)
	{
		index = ent->client - game.clients;

		if (Q_stricmp(spot->classname, "info_player_start") == 0 && index != 0)
		{
			while (counter < 3)
			{
				coopspot = G_Find(coopspot, FOFS(classname), "info_player_coop");

				if (!coopspot)
				{
					break;
				}

				VectorSubtract(coopspot->s.origin, spot->s.origin, d);

				/* In xship the coop spawnpoints are farther
				   away than in other maps. Quirk around this.
				   Oh well... */
				if (Q_stricmp(level.mapname, "xship") == 0)
				{
					dist = 2500;
				}
				else
				{
					dist = 550;
				}

				if ((VectorLength(d) < dist))
				{
					if (index == counter)
					{
						spot = coopspot;
						break;
					}
					else
					{
						counter++;
					}
				}
			}
		}
	}

	VectorCopy(spot->s.origin, origin);
	origin[2] += 9;
	VectorCopy(spot->s.angles, angles);
}

/* ====================================================================== */

void
InitBodyQue(void)
{
	if (deathmatch->value || coop->value)
	{
		int i;
		edict_t *ent;

		level.body_que = 0;

		for (i = 0; i < BODY_QUEUE_SIZE; i++)
		{
			ent = G_Spawn();
			ent->classname = "bodyque";
		}
	}
}

void
body_die(edict_t *self, edict_t *inflictor /* unused */,
		edict_t *attacker /* unused */, int damage,
		vec3_t point /* unused */)
{
	int n;

	if (!self)
	{
		return;
	}

	if (self->health < -40)
	{
		gi.sound(self, CHAN_BODY, gi.soundindex(
						"misc/udeath.wav"), 1, ATTN_NORM, 0);

		for (n = 0; n < 4; n++)
		{
			ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2",
					damage, GIB_ORGANIC);
		}

		self->s.origin[2] -= 48;
		ThrowClientHead(self, damage);
		self->takedamage = DAMAGE_NO;
	}
}

void
CopyToBodyQue(edict_t *ent)
{
	edict_t *body;

	if (!ent)
	{
		return;
	}

	/* grab a body que and cycle to the next one */
	body = &g_edicts[(int)maxclients->value + level.body_que + 1];
	level.body_que = (level.body_que + 1) % BODY_QUEUE_SIZE;

	gi.unlinkentity(ent);
	gi.unlinkentity(body);
	body->s = ent->s;
	body->s.number = body - g_edicts;

	body->svflags = ent->svflags;
	VectorCopy(ent->mins, body->mins);
	VectorCopy(ent->maxs, body->maxs);
	VectorCopy(ent->absmin, body->absmin);
	VectorCopy(ent->absmax, body->absmax);
	VectorCopy(ent->size, body->size);
	body->solid = ent->solid;
	body->clipmask = ent->clipmask;
	body->owner = ent->owner;
	body->movetype = ent->movetype;

	body->die = body_die;
	body->takedamage = DAMAGE_YES;

	gi.linkentity(body);
}

void
respawn(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (self->client->oldplayer)
	{
			G_FreeEdict(self->client->oldplayer);
	}
	self->client->oldplayer = NULL;

	if (self->client->chasecam)
	{
			G_FreeEdict(self->client->chasecam);
	}
	self->client->chasecam = NULL;

	if (deathmatch->value || coop->value)
	{
//JABot[start]
		if (self->ai && self->ai->is_bot){
			BOT_Respawn (self);
			return;
		}
//JABot[end]

		/* spectator's don't leave bodies */
		if (self->movetype != MOVETYPE_NOCLIP)
		{
			CopyToBodyQue(self);
		}

		self->svflags &= ~SVF_NOCLIENT;
		PutClientInServer(self);

		/* add a teleportation effect */
		self->s.event = EV_PLAYER_TELEPORT;

		/* hold in place briefly */
		self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		self->client->ps.pmove.pm_time = 14;

		self->client->respawn_time = level.time;

		return;
	}

	/* restart the entire server */
	gi.AddCommandString("menu_loadgame\n");
}

/*
 * only called when pers.spectator changes
 * note that resp.spectator should be the
 * opposite of pers.spectator here
 */
void
spectator_respawn(edict_t *ent)
{
	int i, numspec;

	if (!ent)
	{
		return;
	}

	/* if the user wants to become a spectator,
	   make sure he doesn't exceed max_spectators */
	if (ent->client->pers.spectator)
	{
		char *value = Info_ValueForKey(ent->client->pers.userinfo, "spectator");

		if (*spectator_password->string &&
			strcmp(spectator_password->string, "none") &&
			strcmp(spectator_password->string, value))
		{
			gi.cprintf(ent, PRINT_HIGH, "Spectator password incorrect.\n");
			ent->client->pers.spectator = false;
			gi.WriteByte(svc_stufftext);
			gi.WriteString("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}

		/* count spectators */
		for (i = 1, numspec = 0; i <= maxclients->value; i++)
		{
			if (g_edicts[i].inuse && g_edicts[i].client->pers.spectator)
			{
				numspec++;
			}
		}

		if (numspec >= maxspectators->value)
		{
			gi.cprintf(ent, PRINT_HIGH, "Server spectator limit is full.");
			ent->client->pers.spectator = false;

			/* reset his spectator var */
			gi.WriteByte(svc_stufftext);
			gi.WriteString("spectator 0\n");
			gi.unicast(ent, true);
			return;
		}

		/* Third person view */
		if (ent->client->chasetoggle)
		{
			ChasecamRemove(ent);
			ent->client->pers.chasetoggle = 1;
		}
		else
		{
			ent->client->pers.chasetoggle = 0;
		}
	}
	else
	{
		/* he was a spectator and wants to join the
		   game he must have the right password */
		char *value = Info_ValueForKey(ent->client->pers.userinfo, "password");

		if (*password->string && strcmp(password->string, "none") &&
			strcmp(password->string, value))
		{
			gi.cprintf(ent, PRINT_HIGH, "Password incorrect.\n");
			ent->client->pers.spectator = true;
			gi.WriteByte(svc_stufftext);
			gi.WriteString("spectator 1\n");
			gi.unicast(ent, true);
			return;
		}
	}

	/* clear client on respawn */
	ent->client->resp.score = ent->client->pers.score = 0;

	ent->svflags &= ~SVF_NOCLIENT;
	PutClientInServer(ent);

	/* add a teleportation effect */
	if (!ent->client->pers.spectator)
	{
		/* send effect */
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_LOGIN);
		gi.multicast(ent->s.origin, MULTICAST_PVS);

		/* hold in place briefly */
		ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		ent->client->ps.pmove.pm_time = 14;
	}

	ent->client->respawn_time = level.time;

	if (ent->client->pers.spectator)
	{
		gi.bprintf(PRINT_HIGH, "%s has moved to the sidelines\n",
				ent->client->pers.netname);
	}
	else
	{
		gi.bprintf(PRINT_HIGH, "%s joined the game\n",
				ent->client->pers.netname);
	}
}

// [Paril-KEX] force the fog transition on the given player,
// optionally instantaneously (ignore any transition time)
void ForceFogTransition(edict_t *ent, qboolean instant)
{
	const height_fog_t *wanted_hf;
	svc_fog_data_t fog = {0};
	unsigned bits = 0;
	height_fog_t *hf;

	hf = &ent->client->heightfog;
	wanted_hf = &ent->client->pers.wanted_heightfog;

	// sanity check; if we're not changing the values, don't bother
	if (!memcmp(ent->client->fog, ent->client->pers.wanted_fog, sizeof(ent->client->pers.wanted_fog)) &&
		!memcmp(hf, wanted_hf, sizeof(*wanted_hf)) &&
		!instant)
	{
		return;
	}

	// check regular fog
	if (ent->client->pers.wanted_fog[0] != ent->client->fog[0] ||
		ent->client->pers.wanted_fog[4] != ent->client->fog[4])
	{
		bits |= FOGBIT_DENSITY;
		fog.density = ent->client->pers.wanted_fog[0];
		fog.skyfactor = ent->client->pers.wanted_fog[4] * 255.f;
	}

	if (ent->client->pers.wanted_fog[1] != ent->client->fog[1])
	{
		bits |= FOGBIT_R;
		fog.red = ent->client->pers.wanted_fog[1] * 255.f;
	}

	if (ent->client->pers.wanted_fog[2] != ent->client->fog[2])
	{
		bits |= FOGBIT_G;
		fog.green = ent->client->pers.wanted_fog[2] * 255.f;
	}

	if (ent->client->pers.wanted_fog[3] != ent->client->fog[3])
	{
		bits |= FOGBIT_B;
		fog.blue = ent->client->pers.wanted_fog[3] * 255.f;
	}

	if (!instant && ent->client->pers.fog_transition_time)
	{
		bits |= FOGBIT_TIME;
		fog.time = Q_clamp(ent->client->pers.fog_transition_time * 1000,
			0, 65535);
	}

	/* check heightfog stuff */
	if (hf->falloff != wanted_hf->falloff)
	{
		bits |= FOGBIT_HEIGHTFOG_FALLOFF;
		if (!wanted_hf->falloff)
		{
			fog.hf_falloff = 0;
		}
		else
		{
			fog.hf_falloff = wanted_hf->falloff;
		}
	}

	if (hf->density != wanted_hf->density)
	{
		bits |= FOGBIT_HEIGHTFOG_DENSITY;

		if (!wanted_hf->density)
		{
			fog.hf_density = 0;
		}
		else
		{
			fog.hf_density = wanted_hf->density;
		}
	}

	if (hf->start[0] != wanted_hf->start[0])
	{
		bits |= FOGBIT_HEIGHTFOG_START_R;
		fog.hf_start_r = wanted_hf->start[0] * 255.f;
	}

	if (hf->start[1] != wanted_hf->start[1])
	{
		bits |= FOGBIT_HEIGHTFOG_START_G;
		fog.hf_start_g = wanted_hf->start[1] * 255.f;
	}

	if (hf->start[2] != wanted_hf->start[2])
	{
		bits |= FOGBIT_HEIGHTFOG_START_B;
		fog.hf_start_b = wanted_hf->start[2] * 255.f;
	}

	if (hf->start[3] != wanted_hf->start[3])
	{
		bits |= FOGBIT_HEIGHTFOG_START_DIST;
		fog.hf_start_dist = wanted_hf->start[3];
	}

	if (hf->end[0] != wanted_hf->end[0])
	{
		bits |= FOGBIT_HEIGHTFOG_END_R;
		fog.hf_end_r = wanted_hf->end[0] * 255.f;
	}

	if (hf->end[1] != wanted_hf->end[1])
	{
		bits |= FOGBIT_HEIGHTFOG_END_G;
		fog.hf_end_g = wanted_hf->end[1] * 255.f;
	}

	if (hf->end[2] != wanted_hf->end[2])
	{
		bits |= FOGBIT_HEIGHTFOG_END_B;
		fog.hf_end_b = wanted_hf->end[2] * 255.f;
	}

	if (hf->end[3] != wanted_hf->end[3])
	{
		bits |= FOGBIT_HEIGHTFOG_END_DIST;
		fog.hf_end_dist = wanted_hf->end[3];
	}

	if (bits & 0xFF00)
	{
		bits |= FOGBIT_MORE_BITS;
	}

	gi.WriteByte(svc_fog);

	if (bits & FOGBIT_MORE_BITS)
	{
		gi.WriteShort(bits);
	}
	else
	{
		gi.WriteByte(bits);
	}

	if (bits & FOGBIT_DENSITY)
	{
		gi.WriteFloat(fog.density);
		gi.WriteByte(fog.skyfactor);
	}

	if (bits & FOGBIT_R)
	{
		gi.WriteByte(fog.red);
	}

	if (bits & FOGBIT_G)
	{
		gi.WriteByte(fog.green);
	}

	if (bits & FOGBIT_B)
	{
		gi.WriteByte(fog.blue);
	}

	if (bits & FOGBIT_TIME)
	{
		gi.WriteShort(fog.time);
	}

	if (bits & FOGBIT_HEIGHTFOG_FALLOFF)
	{
		gi.WriteFloat(fog.hf_falloff);
	}

	if (bits & FOGBIT_HEIGHTFOG_DENSITY)
	{
		gi.WriteFloat(fog.hf_density);
	}

	if (bits & FOGBIT_HEIGHTFOG_START_R)
	{
		gi.WriteByte(fog.hf_start_r);
	}

	if (bits & FOGBIT_HEIGHTFOG_START_G)
	{
		gi.WriteByte(fog.hf_start_g);
	}

	if (bits & FOGBIT_HEIGHTFOG_START_B)
	{
		gi.WriteByte(fog.hf_start_b);
	}

	if (bits & FOGBIT_HEIGHTFOG_START_DIST)
	{
		gi.WriteLong(fog.hf_start_dist);
	}

	if (bits & FOGBIT_HEIGHTFOG_END_R)
	{
		gi.WriteByte(fog.hf_end_r);
	}

	if (bits & FOGBIT_HEIGHTFOG_END_G)
	{
		gi.WriteByte(fog.hf_end_g);
	}

	if (bits & FOGBIT_HEIGHTFOG_END_B)
	{
		gi.WriteByte(fog.hf_end_b);
	}

	if (bits & FOGBIT_HEIGHTFOG_END_DIST)
	{
		gi.WriteLong(fog.hf_end_dist);
	}

	gi.unicast(ent, true);

	memcpy(ent->client->fog, ent->client->pers.wanted_fog,
		sizeof(ent->client->fog));
	memcpy(hf, wanted_hf, sizeof(*wanted_hf));
}

/* ============================================================== */

/*
 * Called when a player connects to
 * a server or respawns in a deathmatch.
 */
void
PutClientInServer(edict_t *ent)
{
	vec3_t mins = {-16, -16, -24};
	vec3_t maxs = {16, 16, 32};
	int index;
	vec3_t spawn_origin, spawn_angles;
	gclient_t *client;
	int i, chasetoggle;
	client_persistant_t saved;
	client_respawn_t resp;

	if (!ent)
	{
		return;
	}

	/* find a spawn point do it before setting
	   health back up, so farthest ranging
	   doesn't count this client */
	if (gamerules && gamerules->value && DMGame.SelectSpawnPoint)
	{
		DMGame.SelectSpawnPoint(ent, spawn_origin, spawn_angles);
	}
	else
	{
		SelectSpawnPoint(ent, spawn_origin, spawn_angles);
	}

	index = ent - g_edicts - 1;
	client = ent->client;
	chasetoggle = client->pers.chasetoggle;

	/* deathmatch wipes most client data every spawn */
	if (deathmatch->value)
	{
		char userinfo[MAX_INFO_STRING];

		resp = client->resp;
		memcpy(userinfo, client->pers.userinfo, sizeof(userinfo));
		InitClientPersistant(ent);
		ClientUserinfoChanged(ent, userinfo);
	}
	else if (coop->value)
	{
		int n;
		char userinfo[MAX_INFO_STRING];

		resp = client->resp;
		memcpy(userinfo, client->pers.userinfo, sizeof(userinfo));
		resp.coop_respawn.game_helpchanged = client->pers.game_helpchanged;
		resp.coop_respawn.helpchanged = client->pers.helpchanged;

		/* this is kind of ugly, but it's how we want to handle keys in coop */
		for (n = 0; n < MAX_ITEMS; n++)
		{
			if (itemlist[n].flags & IT_KEY)
			{
				resp.coop_respawn.inventory[n] = client->pers.inventory[n];
			}
		}

		client->pers = resp.coop_respawn;
		ClientUserinfoChanged(ent, userinfo);

		if (resp.score > client->pers.score)
		{
			client->pers.score = resp.score;
		}
	}
	else
	{
		char userinfo[MAX_INFO_STRING];

		memset(&resp, 0, sizeof(resp));
		memcpy(userinfo, client->pers.userinfo, sizeof(userinfo));
		ClientUserinfoChanged (ent, userinfo);
	}

	/* clear everything but the persistant data */
	saved = client->pers;
	memset(client, 0, sizeof(*client));
	client->pers = saved;

	if (client->pers.health <= 0)
	{
		InitClientPersistant(ent);
	}

	client->resp = resp;
	client->pers.chasetoggle = chasetoggle;

	/* copy some data from the client to the entity */
	FetchClientEntData(ent);

	/* clear entity values */
	ent->groundentity = NULL;
	ent->client = &game.clients[index];
	ent->takedamage = DAMAGE_AIM;
	ent->movetype = MOVETYPE_WALK;
	ent->viewheight = 22;
	ent->inuse = true;
	ent->classname = "player";
	ent->mass = 200;
	ent->solid = SOLID_BBOX;
	ent->deadflag = DEAD_NO;
	ent->air_finished = level.time + 12;
	ent->clipmask = MASK_PLAYERSOLID;
	ent->model = "players/male/tris.md2";
	ent->pain = player_pain;
	ent->die = player_die;
	ent->waterlevel = 0;
	ent->watertype = 0;
	ent->flags &= ~FL_NO_KNOCKBACK;
	ent->svflags &= ~SVF_DEADMONSTER;
	/* Third person view */
	ent->svflags &= ~SVF_NOCLIENT;
	/* Turn off prediction */
	ent->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;

	VectorCopy(mins, ent->mins);
	VectorCopy(maxs, ent->maxs);
	VectorClear(ent->velocity);

	/* clear playerstate values */
	memset(&ent->client->ps, 0, sizeof(client->ps));

	/*
	 * set ps.pmove.origin is not required as server uses ent.origin instead
	 */
	client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;

	if (deathmatch->value && ((int)dmflags->value & DF_FIXED_FOV))
	{
		client->ps.fov = 90;
	}
	else
	{
		client->ps.fov = (int)strtol(Info_ValueForKey(client->pers.userinfo, "fov"), (char **)NULL, 10);

		if (client->ps.fov < 1)
		{
			client->ps.fov = 90;
		}
		else if (client->ps.fov > 160)
		{
			client->ps.fov = 160;
		}
	}

	if (client->pers.weapon)
	{
		client->ps.gunindex = gi.modelindex(client->pers.weapon->view_model);
	}
	else
	{
		client->ps.gunindex = 0;
	}

	/* clear entity state values */
	ent->s.effects = 0;
	ent->s.skinnum = ent - g_edicts - 1;
	ent->s.modelindex = CUSTOM_PLAYER_MODEL; /* will use the skin specified model */
	ent->s.modelindex2 = CUSTOM_PLAYER_MODEL; /* custom gun model */

	/* sknum is player num and weapon number
	   weapon number will be added in changeweapon */
	ent->s.skinnum = ent - g_edicts - 1;

	ent->s.frame = 0;
	VectorCopy(spawn_origin, ent->s.origin);
	ent->s.origin[2] += 1;  /* make sure off ground */
	VectorCopy(ent->s.origin, ent->s.old_origin);

	/* set the delta angle */
	for (i = 0; i < 3; i++)
	{
		client->ps.pmove.delta_angles[i] = ANGLE2SHORT(
				spawn_angles[i] - client->resp.cmd_angles[i]);
	}

	ent->s.angles[PITCH] = 0;
	ent->s.angles[YAW] = spawn_angles[YAW];
	ent->s.angles[ROLL] = 0;
	VectorCopy(ent->s.angles, client->ps.viewangles);
	VectorCopy(ent->s.angles, client->v_angle);

	//JABot[start]
	if (ent->ai && ent->ai->is_bot)
	{
		return;
	}
	//JABot[end]

	// [Paril-KEX] set up world fog & send it instantly
	memset(ent->client->pers.wanted_fog, 0, sizeof(ent->client->pers.wanted_fog));
	if (world->fog.density)
	{
		ent->client->pers.wanted_fog[0] = world->fog.density;
		ent->client->pers.wanted_fog[1] = world->fog.color[0];
		ent->client->pers.wanted_fog[2] = world->fog.color[1];
		ent->client->pers.wanted_fog[3] = world->fog.color[2];
	}
	else if (world->fog.altdensity)
	{
		ent->client->pers.wanted_fog[0] = world->fog.altdensity;
		ent->client->pers.wanted_fog[1] = world->fog.altcolor[0];
		ent->client->pers.wanted_fog[2] = world->fog.altcolor[1];
		ent->client->pers.wanted_fog[3] = world->fog.altcolor[2];
	}
	else if (world->fog.afog)
	{
		int res;

		/* Anachronox: Fog value */
		res = sscanf(world->fog.afog, "%f %f %f %f",
			&ent->client->pers.wanted_fog[0],
			&ent->client->pers.wanted_fog[1],
			&ent->client->pers.wanted_fog[2],
			&ent->client->pers.wanted_fog[3]);
		ent->client->pers.wanted_fog[0] *= 200;
		if (res != 4)
		{
			gi.dprintf("%s: Failed to load fog\n", __func__);
			memset(ent->client->pers.wanted_fog, 0,
				sizeof(ent->client->pers.wanted_fog));
		}
	}

	ent->client->pers.wanted_fog[4] = world->fog.sky_factor;

	VectorCopy(world->heightfog.start_color, ent->client->pers.wanted_heightfog.start);
	ent->client->pers.wanted_heightfog.start[3] = world->heightfog.start_dist;
	VectorCopy(world->heightfog.end_color, ent->client->pers.wanted_heightfog.end);
	ent->client->pers.wanted_heightfog.end[3] = world->heightfog.end_dist;
	ent->client->pers.wanted_heightfog.falloff = world->heightfog.falloff;
	ent->client->pers.wanted_heightfog.density = world->heightfog.density;

	ForceFogTransition(ent, true);

	if (CTFStartClient(ent))
	{
		return;
	}

	/* spawn a spectator */
	if (client->pers.spectator)
	{
		client->chase_target = NULL;

		client->resp.spectator = true;

		ent->movetype = MOVETYPE_NOCLIP;
		ent->solid = SOLID_NOT;
		ent->svflags |= SVF_NOCLIENT;
		ent->client->ps.gunindex = 0;
		gi.linkentity(ent);
		return;
	}
	else
	{
		client->resp.spectator = false;
	}

	if (!KillBox(ent))
	{
		/* could't spawn in? */
	}

	gi.linkentity(ent);

	ent->client->chasetoggle = 0;
	/* If chasetoggle set then turn on (delayed start of 5 frames - 0.5s) */
	if (ent->client->pers.chasetoggle && !ent->client->chasetoggle)
	{
		ent->client->delayedstart = 5;
	}

	/* my tribute to cash's level-specific hacks. I hope
	 *   live up to his trailblazing cheese. */
	if (Q_stricmp(level.mapname, "rboss") == 0)
	{
		/* if you get on to rboss in single player or coop, ensure
		   the player has the nuke key. (not in DM) */
		if (!(deathmatch->value))
		{
			gitem_t *item;

			item = FindItem("Antimatter Bomb");
			client->pers.selected_item = ITEM_INDEX(item);
			client->pers.inventory[client->pers.selected_item] = 1;
		}
	}

	/* force the current weapon up */
	client->newweapon = client->pers.weapon;
	ChangeWeapon(ent);
}

/*
 * A client has just connected to the server in
 * deathmatch mode, so clear everything out before
 * starting them.
 */
void
ClientBeginDeathmatch(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	G_InitEdict(ent);
	InitClientResp(ent->client);

	if (gamerules && gamerules->value && DMGame.ClientBegin)
	{
		DMGame.ClientBegin(ent);
	}

	/* locate ent at a spawn point */
	PutClientInServer(ent);

	if (level.intermissiontime)
	{
		MoveClientToIntermission(ent);
	}
	else
	{
		/* send effect */
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(ent - g_edicts);
		gi.WriteByte(MZ_LOGIN);
		gi.multicast(ent->s.origin, MULTICAST_PVS);
	}

	//JABot[start]
	AI_EnemyAdded(ent);
	//[end]

	gi.bprintf(PRINT_HIGH, "%s entered the game\n", ent->client->pers.netname);

	/* make sure all view stuff is valid */
	ClientEndServerFrame(ent);
}

/*
 * called when a client has finished connecting, and is ready
 * to be placed into the game.  This will happen every level load.
 */
void
ClientBegin(edict_t *ent)
{
	int i;

	if (!ent)
	{
		return;
	}

	ent->client = game.clients + (ent - g_edicts - 1);

	if (deathmatch->value)
	{
		ClientBeginDeathmatch(ent);
		return;
	}

	/* if there is already a body waiting for us (a loadgame),
	   just take it, otherwise spawn one from scratch */
	if (ent->inuse == true)
	{
		/* the client has cleared the client side viewangles upon
		   connecting to the server, which is different than the
		   state when the game is saved, so we need to compensate
		   with deltaangles */
		for (i = 0; i < 3; i++)
		{
			ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(
					ent->client->ps.viewangles[i]);
		}
	}
	else
	{
		/* a spawn point will completely reinitialize the entity
		   except for the persistant data that was initialized at
		   ClientConnect() time */
		G_InitEdict(ent);
		ent->classname = "player";
		InitClientResp(ent->client);
		PutClientInServer(ent);
	}

	if (level.intermissiontime)
	{
		MoveClientToIntermission(ent);
	}
	else
	{
		/* send effect if in a multiplayer game */
		if (game.maxclients > 1)
		{
			gi.WriteByte(svc_muzzleflash);
			gi.WriteShort(ent - g_edicts);
			gi.WriteByte(MZ_LOGIN);
			gi.multicast(ent->s.origin, MULTICAST_PVS);

			gi.bprintf(PRINT_HIGH, "%s entered the game\n",
					ent->client->pers.netname);
		}
	}

	/* make sure all view stuff is valid */
	ClientEndServerFrame(ent);
}

/*
 * Called whenever the player updates a userinfo variable.
 * The game can override any of the settings in place
 * (forcing skins or names, etc) before copying it off.
 */
void
ClientUserinfoChanged(edict_t *ent, char *userinfo)
{
	char *s;
	int playernum;

	if (!ent || !userinfo)
	{
		return;
	}

	/* check for malformed or illegal info strings */
	if (!Info_Validate(userinfo))
	{
		strcpy(userinfo, "\\name\\badinfo\\skin\\male/grunt");
	}

	/* set name */
	s = Info_ValueForKey(userinfo, "name");
	Q_strlcpy(ent->client->pers.netname, s, sizeof(ent->client->pers.netname));

	/* set spectator */
	s = Info_ValueForKey(userinfo, "spectator");

	/* spectators are only supported in deathmatch */
	if (deathmatch->value && *s && strcmp(s, "0"))
	{
		ent->client->pers.spectator = true;
	}
	else
	{
		ent->client->pers.spectator = false;
	}

	/* set skin */
	s = Info_ValueForKey(userinfo, "skin");

	playernum = ent - g_edicts - 1;

	/* combine name and skin into a configstring */
	if (ctf->value)
	{
		CTFAssignSkin(ent, s);
	}
	else
	{
		gi.configstring(CS_PLAYERSKINS + playernum,
				va("%s\\%s", ent->client->pers.netname, s));
	}

	/* set player name field (used in id_state view) */
	gi.configstring(CS_GENERAL + playernum, ent->client->pers.netname);

	/* fov */
	if (deathmatch->value && ((int)dmflags->value & DF_FIXED_FOV))
	{
		ent->client->ps.fov = 90;
	}
	else
	{
		ent->client->ps.fov = (int)strtol(Info_ValueForKey(userinfo, "fov"), (char **)NULL, 10);

		if (ent->client->ps.fov < 1)
		{
			ent->client->ps.fov = 90;
		}
		else if (ent->client->ps.fov > 160)
		{
			ent->client->ps.fov = 160;
		}
	}

	/* handedness */
	s = Info_ValueForKey(userinfo, "hand");

	if (strlen(s))
	{
		ent->client->pers.hand = (int)strtol(s, (char **)NULL, 10);
	}

	/* save off the userinfo in case we want to check something later */
	Q_strlcpy(ent->client->pers.userinfo, userinfo, sizeof(ent->client->pers.userinfo));
}

/*
 * Called when a player begins connecting to the server.
 * The game can refuse entrance to a client by returning false.
 * If the client is allowed, the connection process will continue
 * and eventually get to ClientBegin(). Changing levels will NOT
 * cause this to be called again, but loadgames will.
 */
qboolean
ClientConnect(edict_t *ent, char *userinfo)
{
	char *value;

	if (!ent || !userinfo)
	{
		return false;
	}

	/* check to see if they are on the banned IP list */
	value = Info_ValueForKey(userinfo, "ip");

	if (SV_FilterPacket(value))
	{
		Info_SetValueForKey(userinfo, "rejmsg", "Banned.");
		return false;
	}

	/* check for a spectator */
	value = Info_ValueForKey(userinfo, "spectator");

	if (deathmatch->value && *value && strcmp(value, "0"))
	{
		int i, numspec;

		if (*spectator_password->string &&
			strcmp(spectator_password->string, "none") &&
			strcmp(spectator_password->string, value))
		{
			Info_SetValueForKey(userinfo, "rejmsg",
					"Spectator password required or incorrect.");
			return false;
		}

		/* count spectators */
		for (i = numspec = 0; i < maxclients->value; i++)
		{
			if (g_edicts[i + 1].inuse && g_edicts[i + 1].client->pers.spectator)
			{
				numspec++;
			}
		}

		if (numspec >= maxspectators->value)
		{
			Info_SetValueForKey(userinfo, "rejmsg",
					"Server spectator limit is full.");
			return false;
		}
	}
	else
	{
		/* check for a password */
		value = Info_ValueForKey(userinfo, "password");

		if (*password->string && strcmp(password->string, "none") &&
			strcmp(password->string, value))
		{
			Info_SetValueForKey(userinfo, "rejmsg",
					"Password required or incorrect.");
			return false;
		}
	}

	/* they can connect */
	ent->client = game.clients + (ent - g_edicts - 1);

	/* if there is already a body waiting for us (a loadgame),
	   just take it, otherwise spawn one from scratch */
	if (ent->inuse == false)
	{
		/* clear the respawning variables */
		ent->client->resp.ctf_team = -1;
		ent->client->resp.id_state = true;

		InitClientResp(ent->client);

		if (!game.autosaved || !ent->client->pers.weapon)
		{
			InitClientPersistant(ent);
		}
	}

	ClientUserinfoChanged(ent, userinfo);

	if (game.maxclients > 1)
	{
		gi.dprintf("%s connected\n", ent->client->pers.netname);
	}

	ent->svflags = 0; /* make sure we start with known default */
	ent->client->pers.connected = true;
	return true;
}

/*
 * Called when a player drops from the server.
 * Will not be called between levels.
 */
void
ClientDisconnect(edict_t *ent)
{
	int playernum;

	if (!ent)
	{
		return;
	}

	if (!ent->client)
	{
		return;
	}

	if (ent->client->chasetoggle)
	{
		ChasecamRemove(ent);
	}

	gi.bprintf(PRINT_HIGH, "%s disconnected\n", ent->client->pers.netname);

	if (ctf->value)
	{
		CTFDeadDropFlag(ent);
		CTFDeadDropTech(ent);
	}

	/* make sure no trackers are still hurting us. */
	if (ent->client->tracker_pain_framenum)
	{
		RemoveAttackingPainDaemons(ent);
	}

	if (ent->client->owned_sphere)
	{
		if (ent->client->owned_sphere->inuse)
		{
			G_FreeEdict(ent->client->owned_sphere);
		}

		ent->client->owned_sphere = NULL;
	}

	if (gamerules && gamerules->value)
	{
		if (DMGame.PlayerDisconnect)
		{
			DMGame.PlayerDisconnect(ent);
		}
	}

	/* send effect */
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(ent - g_edicts);
	gi.WriteByte(MZ_LOGOUT);
	gi.multicast(ent->s.origin, MULTICAST_PVS);

	gi.unlinkentity(ent);
	ent->s.modelindex = 0;
	ent->solid = SOLID_NOT;
	ent->inuse = false;
	ent->classname = "disconnected";
	ent->client->pers.connected = false;

	playernum = ent - g_edicts - 1;
	gi.configstring(CS_PLAYERSKINS + playernum, "");

	//JABot[start]
	AI_EnemyRemoved (ent);
	//[end]
}

/* ============================================================== */

/*
 * pmove doesn't need to know
 * about passent and contentmask
 */
static trace_t
PM_trace(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end)
{
	if (pm_passent->health > 0)
	{
		return gi.trace(start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
	}
	else
	{
		return gi.trace(start, mins, maxs, end, pm_passent, MASK_DEADSOLID);
	}
}

/*
 * This will be called once for each client frame, which will
 * usually be a couple times for each server frame.
 */
void
ClientThink(edict_t *ent, usercmd_t *ucmd)
{
	gclient_t *client;
	edict_t *other;
	int i, j;

	if (!ent || !ucmd)
	{
		return;
	}

	level.current_entity = ent;
	client = ent->client;

	if (level.intermissiontime)
	{
		if (client->chasetoggle)
		{
			ChasecamRemove(ent);
		}

		client->ps.pmove.pm_type = PM_FREEZE;

		/* can exit intermission after five seconds */
		if ((level.time > level.intermissiontime + 5.0) &&
			(ucmd->buttons & BUTTON_ANY))
		{
			level.exitintermission = true;
		}

		return;
	}

	if (client->chasetoggle)
	{
		ent->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;
	}
	else
	{
		ent->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
	}

	/* +use now does the cool look around stuff but only in SP games */
	if ((ucmd->buttons & BUTTON_USE) && (!deathmatch->value))
	{
		client->use = 1;
		if ((ucmd->forwardmove < 0) && (client->zoom < 60))
		{
			client->zoom++;
		}
		else if ((ucmd->forwardmove > 0) && (client->zoom > -40))
		{
			client->zoom--;
		}
		ucmd->forwardmove = 0;
		ucmd->sidemove = 0;
	}
	else if (client->use)
	{
		if (client->oldplayer)
		{
			// set angles
			for (i=0 ; i<3 ; i++)
			{
				ent->client->ps.pmove.delta_angles[i] = ANGLE2SHORT(
					ent->client->oldplayer->s.angles[i] - ent->client->resp.cmd_angles[i]);
			}
		}
		client->use = 0;
	}

	pm_passent = ent;

	if (ent->client->chase_target)
	{
		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);
	}
	else
	{
		pmove_t pm = {0};
		int origin[3];

		/* set up for pmove */
		if (ent->movetype == MOVETYPE_NOCLIP)
		{
			client->ps.pmove.pm_type = PM_SPECTATOR;
		}
		else if (ent->s.modelindex != CUSTOM_PLAYER_MODEL)
		{
			client->ps.pmove.pm_type = PM_GIB;
		}
		else if (ent->deadflag)
		{
			client->ps.pmove.pm_type = PM_DEAD;
		}
		else
		{
			client->ps.pmove.pm_type = PM_NORMAL;
		}

		client->ps.pmove.gravity = sv_gravity->value * ent->gravity;
		pm.s = client->ps.pmove;

		for (i = 0; i < 3; i++)
		{
			origin[i] = ent->s.origin[i] * 8;
			/* save to an int first, in case the short overflows
			 * so we get defined behavior (at least with -fwrapv) */
			int tmpVel = ent->velocity[i] * 8;
			pm.s.velocity[i] = tmpVel;
		}

		if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
		{
			pm.snapinitial = true;
		}

		pm.cmd = *ucmd;

		pm.trace = PM_trace; /* adds default parms */
		pm.pointcontents = gi.pointcontents;

		/* perform a pmove */
		gi.PmoveEx(&pm, origin);

		/* save results of pmove */
		client->ps.pmove = pm.s;
		client->old_pmove = pm.s;

		for (i = 0; i < 3; i++)
		{
			ent->s.origin[i] = origin[i] * 0.125;
			ent->velocity[i] = pm.s.velocity[i] * 0.125;
		}

		VectorCopy(pm.mins, ent->mins);
		VectorCopy(pm.maxs, ent->maxs);

		client->resp.cmd_angles[0] = SHORT2ANGLE(ucmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(ucmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(ucmd->angles[2]);

		if (ent->groundentity && !pm.groundentity && (pm.cmd.upmove >= 10) &&
			(pm.waterlevel == 0))
		{
			gi.sound(ent, CHAN_VOICE, gi.soundindex(
							"*jump1.wav"), 1, ATTN_NORM, 0);
			PlayerNoise(ent, ent->s.origin, PNOISE_SELF);
		}

		if (ent->flags & FL_SAM_RAIMI)
		{
			ent->viewheight = 8;
		}
		else
		{
			ent->viewheight = pm.viewheight;
		}

		ent->waterlevel = pm.waterlevel;
		ent->watertype = pm.watertype;
		ent->groundentity = pm.groundentity;

		if (pm.groundentity)
		{
			ent->groundentity_linkcount = pm.groundentity->linkcount;
		}

		if (ent->deadflag)
		{
			client->ps.viewangles[ROLL] = 40;
			client->ps.viewangles[PITCH] = -15;
			client->ps.viewangles[YAW] = client->killer_yaw;
		}
		else
		{
			VectorCopy(pm.viewangles, client->v_angle);
			VectorCopy(pm.viewangles, client->ps.viewangles);
		}

		if (client->ctf_grapple)
		{
			CTFGrapplePull(client->ctf_grapple);
		}

		gi.linkentity(ent);

		ent->gravity = 1.0;

		if (ent->movetype != MOVETYPE_NOCLIP)
		{
			G_TouchTriggers(ent);
		}

		/* touch other objects */
		for (i = 0; i < pm.numtouch; i++)
		{
			other = pm.touchents[i];

			for (j = 0; j < i; j++)
			{
				if (pm.touchents[j] == other)
				{
					break;
				}
			}

			if (j != i)
			{
				continue; /* duplicated */
			}

			if (!other->touch)
			{
				continue;
			}

			other->touch(other, ent, NULL, NULL);
		}
	}

	client->oldbuttons = client->buttons;
	client->buttons = ucmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	/* save light level the player is standing
	   on for monster sighting AI */
	ent->light_level = ucmd->lightlevel;

	/* fire weapon from final position if needed */
	if (client->latched_buttons & BUTTON_ATTACK
		&& (ent->movetype != MOVETYPE_NOCLIP))
	{
		if (client->resp.spectator)
		{
			client->latched_buttons = 0;

			if (client->chase_target)
			{
				client->chase_target = NULL;
				client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
			}
			else
			{
				GetChaseTarget(ent);
			}
		}
		else if (!client->weapon_thunk)
		{
			client->weapon_thunk = true;
			Think_Weapon(ent);
		}
	}

	if (ctf->value)
	{
		/* regen tech */
		CTFApplyRegeneration(ent);
	}

	if (client->resp.spectator)
	{
		if (ucmd->upmove >= 10)
		{
			if (!(client->ps.pmove.pm_flags & PMF_JUMP_HELD))
			{
				client->ps.pmove.pm_flags |= PMF_JUMP_HELD;

				if (client->chase_target)
				{
					ChaseNext(ent);
				}
				else
				{
					GetChaseTarget(ent);
				}
			}
		}
		else
		{
			client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
		}
	}

	/* update chase cam if being followed */
	for (i = 1; i <= maxclients->value; i++)
	{
		other = g_edicts + i;

		if (other->inuse && (other->client->chase_target == ent))
		{
			UpdateChaseCam(other);
		}
	}

	//JABot[start]
	AITools_DropNodes(ent);
	//JABot[end]

	if (ctf->value && client->menudirty && (client->menutime <= level.time))
	{
		PMenu_Do_Update(ent);
		gi.unicast(ent, true);
		client->menutime = level.time;
		client->menudirty = false;
	}
}

/*
 * This will be called once for each server
 * frame, before running any other entities
 * in the world.
 */
void
ClientBeginServerFrame(edict_t *ent)
{
	gclient_t *client;
	int buttonMask;

	if (!ent)
	{
		return;
	}

	if (level.intermissiontime)
	{
		return;
	}

	client = ent->client;

	if (client->delayedstart > 0)
	{
		client->delayedstart--;
	}

	if (client->delayedstart == 1)
	{
		ChasecamStart(ent);
	}

	if (deathmatch->value &&
		(client->pers.spectator != client->resp.spectator) &&
		((level.time - client->respawn_time) >= 5))
	{
		spectator_respawn(ent);
		return;
	}

	/* run weapon animations if it hasn't been done by a ucmd_t */
	if (!client->weapon_thunk && !client->resp.spectator
		&& (ent->movetype != MOVETYPE_NOCLIP))
	{
		Think_Weapon(ent);
	}
	else
	{
		client->weapon_thunk = false;
	}

	if (ent->deadflag)
	{
		/* wait for any button just going down */
		if (level.time > client->respawn_time)
		{
			/* in deathmatch, only wait for attack button */
			if (deathmatch->value)
			{
				buttonMask = BUTTON_ATTACK;
			}
			else
			{
				buttonMask = -1;
			}

			if ((client->latched_buttons & buttonMask) ||
				(deathmatch->value &&
				 ((int)dmflags->value & DF_FORCE_RESPAWN)) ||
				CTFMatchOn())
			{
				respawn(ent);
				client->latched_buttons = 0;
			}
		}

		return;
	}

	/* add player trail so monsters can follow */
	if (!deathmatch->value)
	{
		if (!visible(ent, PlayerTrail_LastSpot()))
		{
			PlayerTrail_Add(ent->s.old_origin);
		}
	}

	client->latched_buttons = 0;
}

/*
 * This is called to clean up the pain daemons that
 * the disruptor attaches to clients to damage them.
 */
void
RemoveAttackingPainDaemons(edict_t *self)
{
	edict_t *tracker;

	if (!self)
	{
		return;
	}

	tracker = G_Find(NULL, FOFS(classname), "pain daemon");

	while (tracker)
	{
		if (tracker->enemy == self)
		{
			G_FreeEdict(tracker);
		}

		tracker = G_Find(tracker, FOFS(classname), "pain daemon");
	}

	if (self->client)
	{
		self->client->tracker_pain_framenum = 0;
	}
}
