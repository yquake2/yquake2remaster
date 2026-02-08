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
 * Targets.
 *
 * =======================================================================
 */

#include "header/local.h"

#define TARGET_HELP_PRIMARY 1
#define TARGET_HELP_THINK_DELAY 0.3f

#define LASER_ON 0x0001
#define LASER_RED 0x0002
#define LASER_GREEN 0x0004
#define LASER_BLUE 0x0008
#define LASER_YELLOW 0x0010
#define LASER_ORANGE 0x0020
#define LASER_FAT 0x0040
#define LASER_STOPWINDOW 0x0080

/*
 * QUAKED target_temp_entity (1 0 0) (-8 -8 -8) (8 8 8)
 *
 * Fire an origin based temp entity event to the clients.
 * "style"		type byte
 */
void
Use_Target_Tent(edict_t *ent, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!ent)
	{
		return;
	}

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(ent->style);
	gi.WritePosition(ent->s.origin);
	gi.multicast(ent->s.origin, MULTICAST_PVS);
}

void
SP_target_temp_entity(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->use = Use_Target_Tent;
}

/* ========================================================== */

/*
 * QUAKED target_speaker (1 0 0) (-8 -8 -8) (8 8 8) looped-on looped-off reliable
 *
 * "noise" wav file to play
 *
 * "attenuation"
 *   -1 = none, send to whole level
 *    1 = normal fighting sounds
 *    2 = idle sound level
 *    3 = ambient sound level
 *
 * "volume"	0.0 to 1.0
 *
 * Normal sounds play each time the target is used.
 * The reliable flag can be set for crucial voiceovers.
 *
 * Looped sounds are always atten 3 / vol 1, and the use function toggles it on/off.
 * Multiple identical looping sounds will just increase volume without any speed cost.
 */
void
Use_Target_Speaker(edict_t *ent, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	int chan;

	if (!ent)
	{
		return;
	}

	if (ent->spawnflags & 3)
	{
		/* looping sound toggles */
		if (ent->s.sound)
		{
			ent->s.sound = 0; /* turn it off */
		}
		else
		{
			ent->s.sound = ent->noise_index; /* start it */
		}
	}
	else
	{
		/* normal sound */
		if (ent->spawnflags & 4)
		{
			chan = CHAN_VOICE | CHAN_RELIABLE;
		}
		else
		{
			chan = CHAN_VOICE;
		}

		/* use a positioned_sound, because this entity won't
		   normally be sent to any clients because it is invisible */
		gi.positioned_sound(ent->s.origin, ent, chan, ent->noise_index,
				ent->volume, ent->attenuation, 0);
	}
}

void
SP_target_speaker(edict_t *ent)
{
	char buffer[MAX_QPATH];

	if (!ent)
	{
		return;
	}

	if (!st.noise)
	{
		gi.dprintf("target_speaker with no noise set at %s\n",
				vtos(ent->s.origin));
		return;
	}

	if (!strstr(st.noise, ".wav"))
	{
		Com_sprintf(buffer, sizeof(buffer), "%s.wav", st.noise);
	}
	else
	{
		Q_strlcpy(buffer, st.noise, sizeof(buffer));
	}

	ent->noise_index = gi.soundindex(buffer);

	if (!ent->volume)
	{
		ent->volume = 1.0;
	}

	if (!ent->attenuation)
	{
		ent->attenuation = 1.0;
	}
	else if (ent->attenuation == -1) /* use -1 so 0 defaults to 1 */
	{
		ent->attenuation = 0;
	}

	/* check for prestarted looping sound */
	if (ent->spawnflags & 1)
	{
		ent->s.sound = ent->noise_index;
	}

	ent->use = Use_Target_Speaker;

	/* must link the entity so we get areas and clusters so
	   the server can determine who to send updates to */
	gi.linkentity(ent);
}

/* ========================================================== */

static void
Target_Help_Apply(const char *msg, int is_primary)
{
	char *curr;
	size_t sz;

	if (!msg)
	{
		return;
	}

	if (is_primary)
	{
		curr = game.helpmessage1;
		sz = sizeof (game.helpmessage1);
	}
	else
	{
		curr = game.helpmessage2;
		sz = sizeof (game.helpmessage2);
	}

	if (strcmp(curr, msg) == 0)
	{
		return;
	}

	Q_strlcpy(curr, msg, sz - 1);

	game.helpchanged++;
}

void
Target_Help_Think (edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	Target_Help_Apply(ent->message, ent->spawnflags & TARGET_HELP_PRIMARY);
	ent->think = NULL;
}

void
Use_Target_Help(edict_t *ent, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!ent)
	{
		return;
	}

	if (level.time > TARGET_HELP_THINK_DELAY)
	{
		Target_Help_Apply(ent->message, ent->spawnflags & TARGET_HELP_PRIMARY);
	}
	else
	{
		/* The game is still pre-loading so delay the help message a bit,
		   otherwise its changes to game structure will leak past save loads
		*/
		ent->think = Target_Help_Think;
		ent->nextthink = TARGET_HELP_THINK_DELAY;
	}
}

/*
 * QUAKED target_help (1 0 1) (-16 -16 -24) (16 16 24) help1
 *
 * When fired, the "message" key becomes the current personal computer string,
 * and the message light will be set on all clients status bars.
 */
void
SP_target_help(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (deathmatch->value)
	{
		/* auto-remove for deathmatch */
		G_FreeEdict(ent);
		return;
	}

	if (!ent->message)
	{
		gi.dprintf("%s with no message at %s\n", ent->classname,
				vtos(ent->s.origin));
		G_FreeEdict(ent);
		return;
	}

	ent->use = Use_Target_Help;
}

/* ========================================================== */

/*
 * QUAKED target_secret (1 0 1) (-8 -8 -8) (8 8 8)
 * Counts a secret found. These are single use targets.
 */
void
use_target_secret(edict_t *ent, edict_t *other /* unused */, edict_t *activator /* acticator */)
{
	if (!ent)
	{
		return;
	}

	gi.sound(ent, CHAN_VOICE, ent->noise_index, 1, ATTN_NORM, 0);

	level.found_secrets++;

	G_UseTargets(ent, activator);
	G_FreeEdict(ent);
}

void
SP_target_secret(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (deathmatch->value)
	{
		/* auto-remove for deathmatch */
		G_FreeEdict(ent);
		return;
	}

	ent->use = use_target_secret;

	if (!st.noise)
	{
		st.noise = "misc/secret.wav";
	}

	ent->noise_index = gi.soundindex(st.noise);
	ent->svflags = SVF_NOCLIENT;
	level.total_secrets++;

	/* Map quirk for mine3 */
	if (!Q_stricmp(level.mapname, "mine3") && (ent->s.origin[0] == 280) &&
		(ent->s.origin[1] == -2048) &&
		(ent->s.origin[2] == -624))
	{
		ent->message = "You have found a secret area.";
	}
}

/* ========================================================== */

/*
 * QUAKED target_goal (1 0 1) (-8 -8 -8) (8 8 8)
 * Counts a goal completed. These are single use targets.
 */
void
use_target_goal(edict_t *ent, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!ent)
	{
		return;
	}

	gi.sound(ent, CHAN_VOICE, ent->noise_index, 1, ATTN_NORM, 0);

	level.found_goals++;

	if (level.found_goals == level.total_goals)
	{
		gi.configstring(CS_CDTRACK, "0");
	}

	G_UseTargets(ent, activator);
	G_FreeEdict(ent);
}

void
SP_target_goal(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (deathmatch->value)
	{
		/* auto-remove for deathmatch */
		G_FreeEdict(ent);
		return;
	}

	ent->use = use_target_goal;

	if (!st.noise)
	{
		st.noise = "misc/secret.wav";
	}

	ent->noise_index = gi.soundindex(st.noise);
	ent->svflags = SVF_NOCLIENT;
	level.total_goals++;
}

/* ========================================================== */

/*
 * QUAKED target_explosion (1 0 0) (-8 -8 -8) (8 8 8)
 * Spawns an explosion temporary entity when used.
 *
 * "delay"		wait this long before going off
 * "dmg"		how much radius damage should be done, defaults to 0
 */
void
target_explosion_explode(edict_t *self)
{
	float save;

	if (!self)
	{
		return;
	}

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_EXPLOSION1);
	gi.WritePosition(self->s.origin);
	gi.multicast(self->s.origin, MULTICAST_PHS);

	T_RadiusDamage(self, self->activator, self->dmg, NULL,
			self->dmg + 40, MOD_EXPLOSIVE);

	save = self->delay;
	self->delay = 0;
	G_UseTargets(self, self->activator);
	self->delay = save;
}

void
use_target_explosion(edict_t *self, edict_t *other /* unused */, edict_t *activator)
{
	if (!self)
	{
		return;
	}
	self->activator = activator;

	if (!activator)
	{
		return;
	}

	if (!self->delay)
	{
		target_explosion_explode(self);
		return;
	}

	self->think = target_explosion_explode;
	self->nextthink = level.time + self->delay;
}

void
SP_target_explosion(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	ent->use = use_target_explosion;
	ent->svflags = SVF_NOCLIENT;
}

/* ========================================================== */

/*
 * QUAKED target_changelevel (1 0 0) (-8 -8 -8) (8 8 8)
 *
 * Changes level to "map" when fired
 */
void
use_target_changelevel(edict_t *self, edict_t *other, edict_t *activator)
{
	if (!self || !other  || !activator)
	{
		return;
	}

	if (level.intermissiontime)
	{
		return; /* already activated */
	}

	if (!deathmatch->value && !coop->value)
	{
		if (g_edicts[1].health <= 0)
		{
			return;
		}
	}

	/* if noexit, do a ton of damage to other */
	if (deathmatch->value && !((int)dmflags->value & DF_ALLOW_EXIT) &&
		(other != world))
	{
		T_Damage(other, self, self, vec3_origin, other->s.origin,
				vec3_origin, 10 * other->max_health, 1000,
				0, MOD_EXIT);
		return;
	}

	/* if multiplayer, let everyone know who hit the exit */
	if (deathmatch->value)
	{
		if (activator && activator->client)
		{
			gi.bprintf(PRINT_HIGH, "%s exited the level.\n",
					activator->client->pers.netname);
		}
	}

	/* if going to a new unit, clear cross triggers */
	if (strstr(self->map, "*"))
	{
		game.serverflags &= ~(SFL_CROSS_TRIGGER_MASK);
	}

	BeginIntermission(self);
}

void
SP_target_changelevel(edict_t *ent)
{
	if (!ent)
	{
		return;
	}

	if (!ent->map)
	{
		gi.dprintf("target_changelevel with no map at %s\n", vtos(ent->s.origin));
		G_FreeEdict(ent);
		return;
	}

	/* Mapquirk for secret exists in fact1 and fact3 */
	if ((Q_stricmp(level.mapname, "fact1") == 0) &&
		(Q_stricmp(ent->map, "fact3") == 0))
	{
		ent->map = "fact3$secret1";
	}

	ent->use = use_target_changelevel;
	ent->svflags = SVF_NOCLIENT;
}

/* ========================================================== */

/*
 * QUAKED target_splash (1 0 0) (-8 -8 -8) (8 8 8)
 * Creates a particle splash effect when used.
 *
 * Set "sounds" to one of the following:
 * 1) sparks
 * 2) blue water
 * 3) brown water
 * 4) slime
 * 5) lava
 * 6) blood
 *
 * "count"	how many pixels in the splash
 * "dmg"	if set, does a radius damage at this location when it splashes
 *          useful for lava/sparks
 */
void
use_target_splash(edict_t *self, edict_t *other /* unused */, edict_t *activator)
{
	if (!self || !activator)
	{
		return;
	}

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_SPLASH);
	gi.WriteByte(self->count);
	gi.WritePosition(self->s.origin);
	gi.WriteDir(self->movedir);
	gi.WriteByte(self->sounds);
	gi.multicast(self->s.origin, MULTICAST_PVS);

	if (self->dmg)
	{
		T_RadiusDamage(self, activator, self->dmg, NULL,
				self->dmg + 40, MOD_SPLASH);
	}
}

void
SP_target_splash(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->use = use_target_splash;
	G_SetMovedir(self->s.angles, self->movedir);

	if (!self->count)
	{
		self->count = 32;
	}

	self->svflags = SVF_NOCLIENT;
}

/* ========================================================== */

/*
 * QUAKED target_spawner (1 0 0) (-8 -8 -8) (8 8 8)
 * Set target to the type of entity you want spawned.
 * Useful for spawning monsters and gibs in the factory levels.
 *
 * For monsters:
 *  Set direction to the facing you want it to have.
 *
 * For gibs:
 *  Set direction if you want it moving and
 *  speed how fast it should be moving otherwise it
 *  will just be dropped
 */

void
use_target_spawner(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	edict_t *ent;

	if (!self)
	{
		return;
	}

	ent = G_Spawn();
	ent->classname = self->target;
	VectorCopy(self->s.origin, ent->s.origin);
	VectorCopy(self->s.angles, ent->s.angles);
	ED_CallSpawn(ent);
	gi.unlinkentity(ent);
	KillBox(ent);
	gi.linkentity(ent);

	if (self->speed)
	{
		VectorCopy(self->movedir, ent->velocity);
	}

	ent->s.renderfx |= RF_IR_VISIBLE; /* PGM */
}

void
SP_target_spawner(edict_t *self)
{
	vec3_t	forward;
	vec3_t	fact2spawnpoint1 = {-1504, 512, 72};

	if (!self)
	{
		return;
	}

	self->use = use_target_spawner;
	self->svflags = SVF_NOCLIENT;

	/* Maphack for the insane spawner in Mobs-Egerlings
	   beloved fact2. Found in KMQuake2 */
	if (!Q_stricmp(level.mapname, "fact2")
		&& VectorCompare(self->s.origin, fact2spawnpoint1) )
	{
		VectorSet(forward, 0, 0, 1);
		VectorMA (self->s.origin, -8, forward, self->s.origin);
	}

	if (self->speed)
	{
		G_SetMovedir(self->s.angles, self->movedir);
		VectorScale(self->movedir, self->speed, self->movedir);
	}
}

/* ========================================================== */

/*
 * QUAKED target_blaster (1 0 0) (-8 -8 -8) (8 8 8) NOTRAIL NOEFFECTS
 * Fires a blaster bolt in the set direction when triggered.
 *
 * dmg		default is 15
 * speed	default is 1000
 */

void
use_target_blaster(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!self)
	{
		return;
	}

	fire_blaster(self, self->s.origin, self->movedir, self->dmg,
			self->speed, EF_BLASTER, MOD_TARGET_BLASTER);
	gi.sound(self, CHAN_VOICE, self->noise_index, 1, ATTN_NORM, 0);
}

void
SP_target_blaster(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->use = use_target_blaster;
	G_SetMovedir(self->s.angles, self->movedir);
	self->noise_index = gi.soundindex("weapons/laser2.wav");

	if (!self->dmg)
	{
		self->dmg = 15;
	}

	if (!self->speed)
	{
		self->speed = 1000;
	}

	self->svflags = SVF_NOCLIENT;
}

/* ========================================================== */

/*
 * QUAKED target_crosslevel_trigger (.5 .5 .5) (-8 -8 -8) (8 8 8) trigger1 trigger2 trigger3 trigger4 trigger5 trigger6 trigger7 trigger8
 *
 * Once this trigger is touched/used, any trigger_crosslevel_target
 * with the same trigger number is automatically used when a level
 * is started within the same unit. It is OK to check multiple triggers.
 * Message, delay, target, and killtarget also work.
 */
void
trigger_crosslevel_trigger_use(edict_t *self, edict_t *other /* unused */,
		edict_t *activator)
{
	if (!self || !activator)
	{
		return;
	}

	game.serverflags |= self->spawnflags;
	G_UseTargets (self, activator);
	G_FreeEdict(self);
}

void
SP_target_crosslevel_trigger(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->svflags = SVF_NOCLIENT;
	self->use = trigger_crosslevel_trigger_use;
}

/*
 * QUAKED target_crosslevel_target (.5 .5 .5) (-8 -8 -8) (8 8 8) trigger1 trigger2 trigger3 trigger4 trigger5 trigger6 trigger7 trigger8
 *
 * Triggered by a trigger_crosslevel elsewhere within a unit.
 * If multiple triggers are checked, all must be true. Delay,
 * target and killtarget also work.
 *
 * "delay" delay before using targets if the trigger has been
 *         activated (default 1)
 */
void
target_crosslevel_target_think(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (self->spawnflags ==
		(game.serverflags & SFL_CROSS_TRIGGER_MASK & self->spawnflags))
	{
		G_UseTargets(self, self);
		G_FreeEdict(self);
	}
}

void
SP_target_crosslevel_target(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!self->delay)
	{
		self->delay = 1;
	}

	self->svflags = SVF_NOCLIENT;

	self->think = target_crosslevel_target_think;
	self->nextthink = level.time + self->delay;
}

/* ========================================================== */

/*
 * QUAKED target_laser (0 .5 .8) (-8 -8 -8) (8 8 8) START_ON RED GREEN BLUE YELLOW ORANGE FAT WINDOWSTOP
 * When triggered, fires a laser.  You can either set a target
 * or a direction.
 *
 * WINDOWSTOP - stops at CONTENTS_WINDOW
 */
void
target_laser_think(edict_t *self)
{
	edict_t *ignore;
	vec3_t start;
	vec3_t end;
	trace_t tr;
	vec3_t point;
	vec3_t last_movedir;
	int count;

	if (!self)
	{
		return;
	}

	if (self->spawnflags & 0x80000000)
	{
		count = 8;
	}
	else
	{
		count = 4;
	}

	if (self->enemy)
	{
		VectorCopy(self->movedir, last_movedir);
		VectorMA(self->enemy->absmin, 0.5, self->enemy->size, point);
		VectorSubtract(point, self->s.origin, self->movedir);
		VectorNormalize(self->movedir);

		if (!VectorCompare(self->movedir, last_movedir))
		{
			self->spawnflags |= 0x80000000;
		}
	}

	ignore = self;
	VectorCopy(self->s.origin, start);
	VectorMA(start, 2048, self->movedir, end);

	while (1)
	{
		if (self->spawnflags & LASER_STOPWINDOW)
		{
			tr = gi.trace(start, NULL, NULL, end, ignore, MASK_SHOT);
		}
		else
		{
			tr = gi.trace(start, NULL, NULL, end, ignore,
					CONTENTS_SOLID | CONTENTS_MONSTER | CONTENTS_DEADMONSTER);
		}

		if (!tr.ent)
		{
			break;
		}

		/* hurt it if we can */
		if ((tr.ent->takedamage) && !(tr.ent->flags & FL_IMMUNE_LASER))
		{
			T_Damage(tr.ent,
					self,
					self->activator,
					self->movedir,
					tr.endpos,
					vec3_origin,
					self->dmg,
					1,
					DAMAGE_ENERGY,
					MOD_TARGET_LASER);
		}

		/* if we hit something that's not a monster
		   or player or is immune to lasers, we're done */
		if (!(tr.ent->svflags & SVF_MONSTER) && (!tr.ent->client) &&
			!(tr.ent->svflags & SVF_DAMAGEABLE))
		{
			if (self->spawnflags & 0x80000000)
			{
				self->spawnflags &= ~0x80000000;
				gi.WriteByte(svc_temp_entity);
				gi.WriteByte(TE_LASER_SPARKS);
				gi.WriteByte(count);
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

	self->nextthink = level.time + FRAMETIME;
}

void
target_laser_on(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!self->activator)
	{
		self->activator = self;
	}

	self->spawnflags |= 0x80000001;
	self->svflags &= ~SVF_NOCLIENT;
	target_laser_think(self);
}

void
target_laser_off(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->spawnflags &= ~1;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0;
}

void
target_laser_use(edict_t *self, edict_t *other /* unused */, edict_t *activator)
{
	if (!self || !activator)
	{
		return;
	}

	self->activator = activator;

	if (self->spawnflags & 1)
	{
		target_laser_off(self);
	}
	else
	{
		target_laser_on(self);
	}
}

void
target_laser_start(edict_t *self)
{
	edict_t *ent;

	if (!self)
	{
		return;
	}

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->s.renderfx |= RF_BEAM | RF_TRANSLUCENT;
	self->s.modelindex = 1; /* must be non-zero */

	/* set the beam diameter */
	if (self->spawnflags & 64)
	{
		self->s.frame = 16;
	}
	else
	{
		self->s.frame = 4;
	}

	/* set the color */
	if (self->spawnflags & 2)
	{
		self->s.skinnum = 0xf2f2f0f0;
	}
	else if (self->spawnflags & 4)
	{
		self->s.skinnum = 0xd0d1d2d3;
	}
	else if (self->spawnflags & 8)
	{
		self->s.skinnum = 0xf3f3f1f1;
	}
	else if (self->spawnflags & 16)
	{
		self->s.skinnum = 0xdcdddedf;
	}
	else if (self->spawnflags & 32)
	{
		self->s.skinnum = 0xe0e1e2e3;
	}

	if (!self->enemy)
	{
		if (self->target)
		{
			ent = G_Find(NULL, FOFS(targetname), self->target);

			if (!ent)
			{
				gi.dprintf("%s at %s: %s is a bad target\n",
						self->classname, vtos(self->s.origin),
						self->target);
			}

			self->enemy = ent;
		}
		else
		{
			G_SetMovedir(self->s.angles, self->movedir);
		}
	}

	self->use = target_laser_use;
	self->think = target_laser_think;

	if (!self->dmg)
	{
		self->dmg = 1;
	}

	VectorSet(self->mins, -8, -8, -8);
	VectorSet(self->maxs, 8, 8, 8);
	gi.linkentity(self);

	if (self->spawnflags & 1)
	{
		target_laser_on(self);
	}
	else
	{
		target_laser_off(self);
	}
}

void
SP_target_laser(edict_t *self)
{
	if (!self)
	{
		return;
	}

	/* let everything else get spawned before we start firing */
	self->think = target_laser_start;
	self->nextthink = level.time + 1;
}

/* QUAKED target_mal_laser (1 0 0) (-4 -4 -4) (4 4 4) START_ON RED GREEN BLUE YELLOW ORANGE FAT
 * Mal's laser
 */
void
target_mal_laser_on(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!self->activator)
	{
		self->activator = self;
	}

	self->spawnflags |= 0x80000001;
	self->svflags &= ~SVF_NOCLIENT;
	self->nextthink = level.time + self->wait + self->delay;
}

void
target_mal_laser_off(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->spawnflags &= ~1;
	self->svflags |= SVF_NOCLIENT;
	self->nextthink = 0;
}

void
target_mal_laser_use(edict_t *self, edict_t *other /* unused */, edict_t *activator)
{
	if (!self || !activator)
	{
		return;
	}

	self->activator = activator;

	if (self->spawnflags & 1)
	{
		target_mal_laser_off(self);
	}
	else
	{
		target_mal_laser_on(self);
	}
}

void
mal_laser_think(edict_t *self)
{
	if (!self)
	{
		return;
	}

	target_laser_think(self);
	self->nextthink = level.time + self->wait + 0.1;
	self->spawnflags |= 0x80000000;
}

void
SP_target_mal_laser(edict_t *self)
{
	if (!self)
	{
		return;
	}

	self->movetype = MOVETYPE_NONE;
	self->solid = SOLID_NOT;
	self->s.renderfx |= RF_BEAM | RF_TRANSLUCENT;
	self->s.modelindex = 1; /* must be non-zero */

	/* set the beam diameter */
	if (self->spawnflags & 64)
	{
		self->s.frame = 16;
	}
	else
	{
		self->s.frame = 4;
	}

	/* set the color */
	if (self->spawnflags & 2)
	{
		self->s.skinnum = 0xf2f2f0f0;
	}
	else if (self->spawnflags & 4)
	{
		self->s.skinnum = 0xd0d1d2d3;
	}
	else if (self->spawnflags & 8)
	{
		self->s.skinnum = 0xf3f3f1f1;
	}
	else if (self->spawnflags & 16)
	{
		self->s.skinnum = 0xdcdddedf;
	}
	else if (self->spawnflags & 32)
	{
		self->s.skinnum = 0xe0e1e2e3;
	}

	G_SetMovedir(self->s.angles, self->movedir);

	if (!self->delay)
	{
		self->delay = 0.1;
	}

	if (!self->wait)
	{
		self->wait = 0.1;
	}

	if (!self->dmg)
	{
		self->dmg = 5;
	}

	VectorSet(self->mins, -8, -8, -8);
	VectorSet(self->maxs, 8, 8, 8);

	self->nextthink = level.time + self->delay;
	self->think = mal_laser_think;

	self->use = target_mal_laser_use;

	gi.linkentity(self);

	if (self->spawnflags & 1)
	{
		target_mal_laser_on(self);
	}
	else
	{
		target_mal_laser_off(self);
	}
}

/* ========================================================== */

/*
 * QUAKED target_lightramp (0 .5 .8) (-8 -8 -8) (8 8 8) TOGGLE
 *
 * speed		How many seconds the ramping will take
 * message		two letters; starting lightlevel and ending lightlevel
 */

void
target_lightramp_think(edict_t *self)
{
	char style[2];

	if (!self)
	{
		return;
	}

	style[0] = 'a' + self->movedir[0] +
			   (level.time - self->timestamp) / FRAMETIME * self->movedir[2];
	style[1] = 0;
	gi.configstring(CS_LIGHTS + self->enemy->style, style);

	if ((level.time - self->timestamp) < self->speed)
	{
		self->nextthink = level.time + FRAMETIME;
	}
	else if (self->spawnflags & 1)
	{
		char temp;

		temp = self->movedir[0];
		self->movedir[0] = self->movedir[1];
		self->movedir[1] = temp;
		self->movedir[2] *= -1;
	}
}

void
target_lightramp_use(edict_t *self, edict_t *other /* unused */, edict_t *activator /* unused */)
{
	if (!self)
	{
		return;
	}

	if (!self->enemy)
	{
		edict_t *e;

		/* check all the targets */
		e = NULL;

		while (1)
		{
			e = G_Find(e, FOFS(targetname), self->target);

			if (!e)
			{
				break;
			}

			if (strcmp(e->classname, "light") != 0)
			{
				gi.dprintf("%s at %s ", self->classname, vtos(self->s.origin));
				gi.dprintf("target %s (%s at %s) is not a light\n",
						self->target, e->classname, vtos(e->s.origin));
			}
			else
			{
				self->enemy = e;
			}
		}

		if (!self->enemy)
		{
			gi.dprintf("%s target %s not found at %s\n",
					self->classname, self->target,
					vtos(self->s.origin));
			G_FreeEdict(self);
			return;
		}
	}

	self->timestamp = level.time;
	target_lightramp_think(self);
}

void
SP_target_lightramp(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!self->message || (strlen(self->message) != 2) ||
		(self->message[0] < 'a') || (self->message[0] > 'z') ||
		(self->message[1] < 'a') || (self->message[1] > 'z') ||
		(self->message[0] == self->message[1]))
	{
		gi.dprintf("target_lightramp has bad ramp (%s) at %s\n",
				self->message, vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}

	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	if (!self->target)
	{
		gi.dprintf("%s with no target at %s\n", self->classname,
				vtos(self->s.origin));
		G_FreeEdict(self);
		return;
	}

	self->svflags |= SVF_NOCLIENT;
	self->use = target_lightramp_use;
	self->think = target_lightramp_think;

	self->movedir[0] = self->message[0] - 'a';
	self->movedir[1] = self->message[1] - 'a';
	self->movedir[2] =
		(self->movedir[1] - self->movedir[0]) / (self->speed / FRAMETIME);
}

/* ========================================================== */

/*
 * QUAKED target_earthquake (1 0 0) (-8 -8 -8) (8 8 8) SILENT
 *
 * When triggered, this initiates a level-wide earthquake.
 * All players and monsters are affected.
 * "speed"		severity of the quake (default:200)
 * "count"		duration of the quake (default:5)
 */
void
target_earthquake_think(edict_t *self)
{
	int i;
	edict_t *e;

	if (!self)
	{
		return;
	}

	if (!(self->spawnflags & 1))
	{
		if (self->last_move_time < level.time)
		{
			gi.positioned_sound(self->s.origin,
					self,
					CHAN_AUTO,
					self->noise_index,
					1.0,
					ATTN_NONE,
					0);
			self->last_move_time = level.time + 0.5;
		}
	}

	for (i = 1, e = g_edicts + i; i < globals.num_edicts; i++, e++)
	{
		if (!e->inuse)
		{
			continue;
		}

		if (!e->client)
		{
			continue;
		}

		if (!e->groundentity)
		{
			continue;
		}

		e->groundentity = NULL;
		e->velocity[0] += crandom() * 150;
		e->velocity[1] += crandom() * 150;
		e->velocity[2] = self->speed * (100.0 / e->mass);
	}

	if (level.time < self->timestamp)
	{
		self->nextthink = level.time + FRAMETIME;
	}
}

void
target_earthquake_use(edict_t *self, edict_t *other /* unused */, edict_t *activator)
{
	if (!self || !activator)
	{
		return;
	}

	self->timestamp = level.time + self->count;
	self->nextthink = level.time + FRAMETIME;
	self->activator = activator;
	self->last_move_time = 0;
}

void
SP_target_earthquake(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (!self->targetname)
	{
		gi.dprintf("untargeted %s at %s\n", self->classname,
				vtos(self->s.origin));
	}

	if (!self->count)
	{
		self->count = 5;
	}

	if (!self->speed)
	{
		self->speed = 200;
	}

	self->svflags |= SVF_NOCLIENT;
	self->think = target_earthquake_think;
	self->use = target_earthquake_use;

	if (!(self->spawnflags & 1))
	{
		self->noise_index = gi.soundindex("world/quake.wav");
	}
}

/*
 * QUAKED target_camera (1 0 0) (-8 -8 -8) (8 8 8)
 *
 * ReRelease: Creates a camera path as seen in the N64 version.
 */
#define HACKFLAG_TELEPORT_OUT 2
#define HACKFLAG_SKIPPABLE 64
#define HACKFLAG_END_OF_UNIT 128

static void
camera_lookat_pathtarget(edict_t* self, vec3_t origin, vec3_t* dest)
{
	if (self->pathtarget)
	{
		edict_t* pt = NULL;

		pt = G_Find(pt, FOFS(targetname), self->pathtarget);
		if (pt)
		{
			float yaw, pitch, d;
			vec3_t delta;

			VectorSubtract(pt->s.origin, origin, delta);

			d = delta[0] * delta[0] + delta[1] * delta[1];
			if (d == 0.0f)
			{
				yaw = 0.0f;
				pitch = (delta[2] > 0.0f) ? 90.0f : -90.0f;
			}
			else
			{
				yaw = atan2(delta[1], delta[0]) * (180.0f / M_PI);
				pitch = atan2(delta[2], sqrt(d)) * (180.0f / M_PI);
			}

			(*dest)[YAW] = yaw;
			(*dest)[PITCH] = -pitch;
			(*dest)[ROLL] = 0;
		}
	}
}

void
update_target_camera_think(edict_t *self)
{
	qboolean do_skip = false;

#if ORIGIN_CODE
	/* only allow skipping after 2 seconds */
	if ((self->hackflags & HACKFLAG_SKIPPABLE) && level.time > 2)
	{
		int i;

		for (i = 0; i < game.maxclients; i++)
		{
			edict_t *client = g_edicts + 1 + i;

			if (!client->inuse || !client->client->pers.connected)
			{
				continue;
			}

			if (client->client->buttons & BUTTON_ANY)
			{
				do_skip = true;
				break;
			}
		}
	}
#endif // ORIGIN_CODE

	if (!do_skip && self->movetarget)
	{
		self->moveinfo.remaining_distance -= (self->moveinfo.move_speed * FRAMETIME) * 0.8f;

		if (self->moveinfo.remaining_distance <= 0)
		{
#if ORIGIN_CODE
			if (self->movetarget->hackflags & HACKFLAG_TELEPORT_OUT)
			{
				if (self->enemy)
				{
					self->enemy->s.event = EV_PLAYER_TELEPORT;
					self->enemy->hackflags = HACKFLAG_TELEPORT_OUT;
					self->enemy->pain_debounce_time = self->enemy->timestamp = self->movetarget->wait;
				}
			}
#endif // ORIGIN_CODE

			VectorCopy(self->movetarget->s.origin, self->s.origin);
			self->nextthink = level.time + self->movetarget->wait;

			if (self->movetarget->target)
			{
				self->movetarget = G_PickTarget(self->movetarget->target);

				if (self->movetarget)
				{
					vec3_t diff;

					self->moveinfo.move_speed = self->movetarget->speed ? self->movetarget->speed : 55;
					VectorSubtract(self->movetarget->s.origin, self->s.origin, diff);
					self->moveinfo.remaining_distance = VectorNormalize(diff);
					self->moveinfo.distance = self->moveinfo.remaining_distance;
				}
			}
			else
			{
				self->movetarget = NULL;
			}

			return;
		}
		else
		{
			vec3_t delta, newpos;
			float frac;
			int i;

			frac = 1.0f - (self->moveinfo.remaining_distance / self->moveinfo.distance);

#if ORIGIN_CODE
			if (self->enemy && (self->enemy->hackflags & HACKFLAG_TELEPORT_OUT))
			{
				self->enemy->s.alpha = Q_max(1.f / 255.f, frac);
			}
#endif // ORIGIN_CODE

			VectorSubtract(self->movetarget->s.origin, self->s.origin, delta);
			VectorScale(delta, frac, delta);

			VectorAdd(self->s.origin, delta, newpos);

			camera_lookat_pathtarget(self, newpos, &level.intermission_angle);
			VectorCopy(newpos, level.intermission_origin);

			/* move all clients to the intermission point */
			for (i = 0; i < game.maxclients; i++)
			{
				edict_t *client = g_edicts + 1 + i;

				if (!client->inuse)
				{
					continue;
				}

				MoveClientToIntermission(client);
			}
		}
	}
	else
	{
		if (self->killtarget)
		{
			edict_t *t = NULL;

			/* destroy dummy player */
			if (self->enemy)
			{
				G_FreeEdict(self->enemy);
			}

			level.intermissiontime = 0;

#if ORIGIN_CODE
			level.level_intermission_set = true;
#endif //ORIGIN_CODE

			while ((t = G_Find(t, FOFS(targetname), self->killtarget)))
			{
				t->use(t, self, self->activator);
			}

			level.intermissiontime = level.time;
#if ORIGIN_CODE
			level.intermission_server_frame = gi.ServerFrame();
#endif //ORIGIN_CODE

			/* end of unit requires a wait */
			if (level.changemap && !strchr(level.changemap, '*'))
			{
				level.exitintermission = true;
			}
		}

		self->think = NULL;
		return;
	}

	self->nextthink = level.time + FRAMETIME;
}

void
target_camera_dummy_think(edict_t *self)
{
	/*
	 * bit of a hack, but this will let the dummy
	 * move like a player
	 */
	self->client = self->owner->client;
	G_SetClientFrame(self, sqrtf(
		self->velocity[0] * self->velocity[0] +
		self->velocity[1] * self->velocity[1]));
	self->client = NULL;

#if ORIGIN_CODE
	/* alpha fade out for voops */
	if (self->hackflags & HACKFLAG_TELEPORT_OUT)
	{
		self->timestamp = Q_max(0, self->timestamp - FRAMETIME);
		self->s.alpha = Q_max(1.f / 255.f, self->timestamp  / self->pain_debounce_time);
	}
#endif //ORIGIN_CODE

	self->nextthink = level.time + FRAMETIME;
}

void
use_target_camera(edict_t *self, edict_t *other, edict_t *activator)
{
	vec3_t diff;
	int i;

	if (!self)
	{
		return;
	}

	if (self->sounds)
	{
		gi.configstring(CS_CDTRACK, va("%i", self->sounds));
	}

	if (!self->target)
	{
		return;
	}

	self->movetarget = G_PickTarget(self->target);

	if (!self->movetarget)
	{
		return;
	}

	level.intermissiontime = level.time;
#if ORIGIN_CODE
	level.intermission_server_frame = gi.ServerFrame();
#endif //ORIGIN_CODE
	level.exitintermission = 0;

	/* spawn fake player dummy where we were */
	if (activator->client)
	{
		edict_t *dummy;

		dummy = self->enemy = G_Spawn();
		dummy->owner = activator;
		dummy->clipmask = activator->clipmask;
		VectorCopy(activator->s.origin, dummy->s.origin);
		VectorCopy(activator->s.angles, dummy->s.angles);
		dummy->groundentity = activator->groundentity;
		dummy->groundentity_linkcount = dummy->groundentity ? dummy->groundentity->linkcount : 0;
		dummy->think = target_camera_dummy_think;
		dummy->nextthink = level.time + FRAMETIME;
		dummy->solid = SOLID_BBOX;
		dummy->movetype = MOVETYPE_STEP;
		VectorCopy(activator->mins, dummy->mins);
		VectorCopy(activator->maxs, dummy->maxs);
		dummy->s.modelindex = dummy->s.modelindex2 = CUSTOM_PLAYER_MODEL;
		dummy->s.skinnum = activator->s.skinnum;
		VectorCopy(activator->velocity, dummy->velocity);
		dummy->s.renderfx = RF_MINLIGHT;
		dummy->s.frame = activator->s.frame;
		gi.linkentity(dummy);
	}

	camera_lookat_pathtarget(self, self->s.origin, &level.intermission_angle);
	VectorCopy(self->s.origin, level.intermission_origin);

	/* move all clients to the intermission point */
	for (i = 0; i < game.maxclients; i++)
	{
		edict_t* client = g_edicts + 1 + i;
		if (!client->inuse)
		{
			continue;
		}

		/* respawn any dead clients */
		if (client->health <= 0)
		{
#if ORIGIN_CODE
			/*
			 * give us our max health back since it will reset
			 * to pers.health; in instanced items we'd lose the items
			 * we touched so we always want to respawn with our max.
			 */
			if (P_UseCoopInstancedItems())
			{
				client->client->pers.health = client->client->pers.max_health = client->max_health;
			}
#endif //ORIGIN_CODE

			respawn(client);
		}

		MoveClientToIntermission(client);
	}

	self->activator = activator;
	self->think = update_target_camera_think;
	self->nextthink = level.time + self->wait;
	self->moveinfo.move_speed = self->speed;

	VectorSubtract(self->movetarget->s.origin, self->s.origin,  diff);

	self->moveinfo.remaining_distance = VectorNormalize(diff);
	self->moveinfo.distance = self->moveinfo.remaining_distance;

#if ORIGIN_CODE
	if (self->hackflags & HACKFLAG_END_OF_UNIT)
	{
		G_EndOfUnitMessage();
	}
#endif //ORIGIN_CODE
}

void
SP_target_camera(edict_t* self)
{
	if (deathmatch->value)
	{
		/* auto-remove for deathmatch */
		G_FreeEdict(self);
		return;
	}

	self->use = use_target_camera;
	self->svflags = SVF_NOCLIENT;
}

/*
 * QUAKED target_gravity (1 0 0) (-8 -8 -8) (8 8 8) NOTRAIL NOEFFECTS
 *
 * ReRelease: Changes gravity, as seen in the N64 version
 */
void
use_target_gravity(edict_t *self, edict_t *other, edict_t *activator)
{
	gi.cvar_set("sv_gravity", va("%f", self->gravity));
}

void
SP_target_gravity(edict_t* self)
{
	self->use = use_target_gravity;
	self->gravity = atof(st.gravity);
}

/*
 * QUAKED target_soundfx (1 0 0) (-8 -8 -8) (8 8 8) NOTRAIL NOEFFECTS
 *
 * ReRelease: Plays a sound fx, as seen in the N64 version
 */
void
update_target_soundfx(edict_t *self)
{
	gi.positioned_sound(self->s.origin, self, CHAN_VOICE, self->noise_index,
		self->volume, self->attenuation, 0);
}

void
use_target_soundfx(edict_t *self, edict_t *other, edict_t *activator)
{
	self->think = update_target_soundfx;
	self->nextthink = level.time + self->delay;
}

void
SP_target_soundfx(edict_t* self)
{
	if (!self->volume)
	{
		self->volume = 1.0;
	}

	if (!self->attenuation)
	{
		self->attenuation = 1.0;
	}
	else if (self->attenuation == -1)
	{
		/* use -1 so 0 defaults to 1 */
		self->attenuation = 0;
	}

	self->noise_index = atoi(st.noise);

	switch (self->noise_index)
	{
	case 1:
		self->noise_index = gi.soundindex("world/x_alarm.wav");
		break;
	case 2:
		self->noise_index = gi.soundindex("world/flyby1.wav");
		break;
	case 4:
		self->noise_index = gi.soundindex("world/amb12.wav");
		break;
	case 5:
		self->noise_index = gi.soundindex("world/amb17.wav");
		break;
	case 7:
		self->noise_index = gi.soundindex("world/bigpump2.wav");
		break;
	default:
		gi.dprintf("%s: unknown noise %d\n", self->classname, self->noise_index);
		return;
	}

	self->use = use_target_soundfx;
}

/*QUAKED target_light (1 0 0) (-8 -8 -8) (8 8 8) START_ON NO_LERP FLICKER
[Paril-KEX] dynamic light entity that follows a lightstyle.

*/

#define SPAWNFLAG_TARGET_LIGHT_START_ON 1
#define SPAWNFLAG_TARGET_LIGHT_NO_LERP 2 // not used in N64, but I'll use it for this
#define SPAWNFLAG_TARGET_LIGHT_FLICKER 4

void
target_light_flicker_think(edict_t *self)
{
	if ((random() < 0.5))
	{
		self->svflags ^= SVF_NOCLIENT;
	}

	self->nextthink = level.time + FRAMETIME;
}

/* think function handles interpolation from start to finish. */
void
target_light_think(edict_t *self)
{
	int index, my_rgb, target_rgb, my_b, my_g, my_r,
		target_b, target_g, target_r,
		r, g, b;
	float current_lerp, lerp, backlerp;
	const char *style;
	char style_value;

	if (self->spawnflags & SPAWNFLAG_TARGET_LIGHT_FLICKER)
	{
		target_light_flicker_think(self);
	}

	style = gi.GetConfigString(CS_LIGHTS + self->style);
	self->delay += self->speed;

	index = ((int) self->delay) % strlen(style);
	style_value = style[index];
	current_lerp = (float) (style_value - 'a') / (float) ('z' - 'a');

	if (!(self->spawnflags & SPAWNFLAG_TARGET_LIGHT_NO_LERP))
	{
		float next_lerp, mod_lerp;
		char next_style_value;
		int next_index;

		next_index = (index + 1) % strlen(style);
		next_style_value = style[next_index];

		next_lerp = (float) (next_style_value - 'a') / (float) ('z' - 'a');

		mod_lerp = fmod(self->delay, 1.0f);
		lerp = (next_lerp * mod_lerp) + (current_lerp * (1.f - mod_lerp));
	}
	else
	{
		lerp = current_lerp;
	}

	my_rgb = self->count;
	target_rgb = self->chain->s.skinnum;

	my_b = ((my_rgb >> 8 ) & 0xff);
	my_g = ((my_rgb >> 16) & 0xff);
	my_r = ((my_rgb >> 24) & 0xff);

	target_b = ((target_rgb >> 8 ) & 0xff);
	target_g = ((target_rgb >> 16) & 0xff);
	target_r = ((target_rgb >> 24) & 0xff);

	backlerp = 1.0f - lerp;

	b = (target_b * lerp) + (my_b * backlerp);
	g = (target_g * lerp) + (my_g * backlerp);
	r = (target_r * lerp) + (my_r * backlerp);

	self->s.skinnum = (b << 8) | (g << 16) | (r << 24);

	self->nextthink = level.time + FRAMETIME;
}

void
target_light_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (!self)
	{
		return;
	}

	self->health = !self->health;

	if (self->health)
	{
		self->svflags &= ~SVF_NOCLIENT;
	}
	else
	{
		self->svflags |= SVF_NOCLIENT;
	}

	if (!self->health)
	{
		self->think = NULL;
		self->nextthink = 0;
		return;
	}

	/* has dynamic light "target" */
	if (self->chain)
	{
		self->think = target_light_think;
		self->nextthink = level.time + FRAMETIME;
	}
	else if (self->spawnflags & SPAWNFLAG_TARGET_LIGHT_FLICKER)
	{
		self->think = target_light_flicker_think;
		self->nextthink = level.time + FRAMETIME;
	}
}

void
SP_target_light(edict_t *self)
{
	self->s.modelindex = 1;
	self->s.renderfx = RF_CUSTOM_LIGHT;
	self->s.frame = st.radius ? st.radius : 150;
	self->count = self->s.skinnum;
	self->svflags |= SVF_NOCLIENT;
	self->health = 0;

	if (self->target)
	{
		self->chain = G_PickTarget(self->target);
	}

	if (self->spawnflags & SPAWNFLAG_TARGET_LIGHT_START_ON)
	{
		target_light_use(self, self, self);
	}

	if (!self->speed)
	{
		self->speed = 1.0f;
	}
	else
	{
		self->speed = 0.1f / self->speed;
	}

	self->use = target_light_use;

	gi.linkentity(self);
}

/*
 * QUAKED target_music (1 0 0) (-8 -8 -8) (8 8 8)
 * Change music when used
 */
void
target_music_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (!self)
	{
		return;
	}

	gi.configstring(CS_CDTRACK, va("%i", self->sounds));
}

void
SP_target_music(edict_t* self)
{
	if (!self)
	{
		return;
	}

	self->use = target_music_use;
}

/*
 * QUAKED target_autosave (0 1 0) (-8 -8 -8) (8 8 8)
 *
 * Auto save on command.
 */
void
use_target_autosave(edict_t *ent, edict_t *other, edict_t *activator)
{
	float save_time = gi.cvar("g_athena_auto_save_min_time", "60", CVAR_NOSET)->value;

	if (level.time - level.next_auto_save > save_time)
	{
		gi.AddCommandString("save quick\n");
		level.next_auto_save = level.time;
	}
}

void
SP_target_autosave(edict_t *self)
{
	if (!self)
	{
		return;
	}

	if (deathmatch->value)
	{
		G_FreeEdict(self);
		return;
	}

	self->use = use_target_autosave;
}

/*
 * QUAKED target_sky (1 0 0) (-8 -8 -8) (8 8 8)
 *
 * Change sky parameters
 *  - sky: Environment map name
 *  - skyaxis: Vector axis for rotating sky
 *  - skyrotate: Speed of rotation (degrees/second)
 *  - skyautorotate: Disable to set sky rotation manually
 */
void
target_sky_use(edict_t *self, edict_t *other, edict_t *activator)
{
	if (!self)
	{
		return;
	}

	if (self->map && self->map[0])
	{
		gi.configstring(CS_SKY, self->map);
	}

	if (self->count & 3)
	{
		float rotate;
		int autorotate;

		sscanf(gi.GetConfigString(CS_SKYROTATE), "%f %i", &rotate, &autorotate);

		if (self->count & 1)
		{
			rotate = self->accel;
		}

		if (self->count & 2)
		{
			autorotate = self->style;
		}

		gi.configstring(CS_SKYROTATE, va("%f %d", rotate, autorotate));
	}

	if (self->count & 4)
	{
		gi.configstring(CS_SKYAXIS, va("%f %f %f",
			self->movedir[0], self->movedir[1], self->movedir[2]));
	}
}

static qboolean
spawn_ent_has_key(const edict_t *self, const char *key)
{
	if (!self || !self->ent_str)
	{
		return false;
	}

	if (strstr(self->ent_str, key))
	{
		return true;
	}

	return false;
}

void
SP_target_sky(edict_t* self)
{
	if (!self)
	{
		return;
	}

	self->use = target_sky_use;

	if (spawn_ent_has_key(self, "\"sky\"") && st.sky && st.sky[0])
	{
		self->map = st.sky;
	}

	if (spawn_ent_has_key(self, "\"skyaxis\""))
	{
		self->count |= 4;
		VectorCopy(st.skyaxis, self->movedir);
	}

	if (spawn_ent_has_key(self, "\"skyrotate\""))
	{
		self->count |= 1;
		self->accel = st.skyrotate;
	}

	if (spawn_ent_has_key(self, "\"skyautorotate\""))
	{
		self->count |= 2;
		self->style = st.skyautorotate;
	}
}
