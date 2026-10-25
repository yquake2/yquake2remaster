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
 * This file implements the entity and network protocol parsing
 *
 * =======================================================================
 */

#include "header/client.h"
#include "input/header/input.h"
#include "../common/unzip/miniz/miniz.h"

static int bitcounts[64]; /* just for protocol profiling */

static const char *svc_strings[] = {
	"svc_bad",

	"svc_muzzleflash",
	"svc_muzzlflash2",
	"svc_temp_entity",
	"svc_layout",
	"svc_inventory",

	"svc_nop",
	"svc_disconnect",
	"svc_reconnect",
	"svc_sound",
	"svc_print",
	"svc_stufftext",
	"svc_serverdata",
	"svc_configstring",
	"svc_spawnbaseline",
	"svc_centerprint",
	"svc_download",
	"svc_playerinfo",
	"svc_packetentities",
	"svc_deltapacketentities",
	"svc_frame",

	"svc_splitclient",
	"svc_configblast",
	"svc_spawnbaselineblast",
	"svc_level_restart",
	"svc_damage",
	"svc_locprint",
	"svc_fog",
	"svc_waitingforplayers",
	"svc_bot_chat",
	"svc_poi",
	"svc_help_path",
	"svc_muzzleflash3",
	"svc_achievement",
};

void
CL_RegisterSounds(void)
{
	int i;

	S_BeginRegistration();
	CL_RegisterTEntSounds();

	for (i = 1; i < MAX_SOUNDS; i++)
	{
		if (!cl.configstrings[CS_SOUNDS + i][0])
		{
			break;
		}

		cl.sound_precache[i] = S_RegisterSound(cl.configstrings[CS_SOUNDS + i]);
		IN_Update();
	}

	S_EndRegistration();
}

/*
 * Returns the entity number and the header bits
 */
static unsigned
CL_ParseEntityBits(uint64_t *bits)
{
	int i, b, number;
	uint64_t total;

	b = MSG_ReadByte(&net_message);
	if (b < 0)
	{
		Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
		return 0;
	}

	total = (uint64_t)b;

	if (total & U_MOREBITS1)
	{
		b = MSG_ReadByte(&net_message);
		if (b < 0)
		{
			Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
			return 0;
		}

		total |= (uint64_t)b << 8;
	}

	if (total & U_MOREBITS2)
	{
		b = MSG_ReadByte(&net_message);
		if (b < 0)
		{
			Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
			return 0;
		}

		total |= (uint64_t)b << 16;
	}

	if (total & U_MOREBITS3)
	{
		b = MSG_ReadByte(&net_message);
		if (b < 0)
		{
			Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
			return 0;
		}

		total |= (uint64_t)b << 24;
	}

	if (total & U_MOREBITS4)
	{
		b = MSG_ReadByte(&net_message);
		if (b < 0)
		{
			Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
			return 0;
		}

		total |= (uint64_t)b << 32;
	}

	/* count the bits for net profiling */
	for (i = 0; i < ARRLEN(bitcounts); i++)
	{
		if (total & (1ULL << i))
		{
			bitcounts[i]++;
		}
	}

	if (total & U_NUMBER16)
	{
		number = MSG_ReadShort(&net_message);
	}

	else
	{
		number = MSG_ReadByte(&net_message);
	}

	if (number < 0)
	{
		Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
		return 0;
	}

	*bits = total;

	return number;
}

/*
 * Can go from either a baseline or a previous packet_entity
 */
static void
CL_ParseDelta(const entity_xstate_t *from, entity_xstate_t *to, int number, uint64_t bits)
{
	static const entity_xstate_t es_nullstate = {0};
	entity_xstate_t dummy;

	if (!from)
	{
		from = &es_nullstate;
	}

	if (!to)
	{
		to = &dummy;
	}

	/* set everything to the state we are delta'ing from */
	*to = *from;

	VectorCopy(from->origin, to->old_origin);
	to->number = number;

	if (cls.serverProtocol != PROTOCOL_VERSION &&
		!IS_QIIRR_PROTOCOL(cls.serverProtocol))
	{
		int i;

		/* Always set scale to 1.0f for old clients */
		for (i = 0; i < 3; i++)
		{
			to->scale[i] = 1.0f;
		}
	}

	if (IS_QII97_PROTOCOL(cls.serverProtocol) ||
		(IS_QIIRR_PROTOCOL(cls.serverProtocol) && !(bits & U_MODEL16)))
	{
		if (bits & U_MODEL)
		{
			to->modelindex = MSG_ReadByte(&net_message);
			if (to->modelindex == QII97_PLAYER_MODEL)
			{
				to->modelindex = CUSTOM_PLAYER_MODEL;
			}
		}

		if (bits & U_MODEL2)
		{
			to->modelindex2 = MSG_ReadByte(&net_message);
			if (to->modelindex2 == QII97_PLAYER_MODEL)
			{
				to->modelindex2 = CUSTOM_PLAYER_MODEL;
			}
		}

		if (bits & U_MODEL3)
		{
			to->modelindex3 = MSG_ReadByte(&net_message);
		}

		if (bits & U_MODEL4)
		{
			to->modelindex4 = MSG_ReadByte(&net_message);
		}
	}
	else
	{
		if (bits & U_MODEL)
		{
			to->modelindex = MSG_ReadShort(&net_message);
		}

		if (bits & U_MODEL2)
		{
			to->modelindex2 = MSG_ReadShort(&net_message);
		}

		if (bits & U_MODEL3)
		{
			to->modelindex3 = MSG_ReadShort(&net_message);
		}

		if (bits & U_MODEL4)
		{
			to->modelindex4 = MSG_ReadShort(&net_message);
		}
	}

	/* Validate message end error in receive models indexes */
	if (bits & U_MODEL)
	{
		if (to->modelindex < 0)
		{
			Com_Error(ERR_DROP, "%s: unexpected message end, bad modelindex",
				__func__);
		}
	}

	if (bits & U_MODEL2)
	{
		if (to->modelindex2 < 0)
		{
			Com_Error(ERR_DROP, "%s: unexpected message end, bad modelindex2",
				__func__);
		}
	}

	if (bits & U_MODEL3)
	{
		if (to->modelindex3 < 0)
		{
			Com_Error(ERR_DROP, "%s: unexpected message end, bad modelindex3",
				__func__);
		}
	}

	if (bits & U_MODEL4)
	{
		if (to->modelindex4 < 0)
		{
			Com_Error(ERR_DROP, "%s: unexpected message end, bad modelindex4",
				__func__);
		}
	}

	if (bits & U_FRAME8)
	{
		to->frame = MSG_ReadByte(&net_message);
	}

	if (bits & U_FRAME16)
	{
		to->frame = MSG_ReadShort(&net_message);
	}

	/* used for laser colors */
	if ((bits & U_SKIN8) && (bits & U_SKIN16))
	{
		to->skinnum = MSG_ReadLong(&net_message);
	}
	else if (bits & U_SKIN8)
	{
		to->skinnum = MSG_ReadByte(&net_message);
	}
	else if (bits & U_SKIN16)
	{
		to->skinnum = MSG_ReadShort(&net_message);
	}

	if ((bits & (U_EFFECTS8 | U_EFFECTS16)) == (U_EFFECTS8 | U_EFFECTS16) ||
		(bits & U_EFFECTS64))
	{
		to->effects = MSG_ReadLong(&net_message);
	}
	else if (bits & U_EFFECTS8)
	{
		to->effects = MSG_ReadByte(&net_message);
	}
	else if (bits & U_EFFECTS16)
	{
		to->effects = MSG_ReadShort(&net_message);
	}

	/* ReRelease effects */
	if (cls.serverProtocol != PROTOCOL_VERSION)
	{
		if (bits & U_EFFECTS64)
		{
			/* RR22 */
			if ((bits & (U_EFFECTS8 | U_EFFECTS16)) == (U_EFFECTS8 | U_EFFECTS16))
			{
				to->rr_effects = MSG_ReadLong(&net_message);
			}
			else if (bits & U_EFFECTS8)
			{
				to->rr_effects = MSG_ReadByte(&net_message);
			}
			else if (bits & U_EFFECTS16)
			{
				to->rr_effects = MSG_ReadShort(&net_message);
			}
		}
		else
		{
			to->rr_effects = 0;
		}

		to->rr_mesh = 0;
	}
	else
	{
		if ((bits & (U_EFFECTS8 | U_EFFECTS16)) == (U_EFFECTS8 | U_EFFECTS16))
		{
			to->rr_effects = MSG_ReadLong(&net_message);
			to->rr_mesh = MSG_ReadLong(&net_message);
		}
		else if (bits & U_EFFECTS8)
		{
			to->rr_effects = MSG_ReadByte(&net_message);
			to->rr_mesh = MSG_ReadByte(&net_message);
		}
		else if (bits & U_EFFECTS16)
		{
			to->rr_effects = MSG_ReadShort(&net_message);
			to->rr_mesh = MSG_ReadShort(&net_message);
		}
	}

	if ((bits & (U_RENDERFX8 | U_RENDERFX16)) == (U_RENDERFX8 | U_RENDERFX16))
	{
		to->renderfx = MSG_ReadLong(&net_message);
	}
	else if (bits & U_RENDERFX8)
	{
		to->renderfx = MSG_ReadByte(&net_message);
	}
	else if (bits & U_RENDERFX16)
	{
		to->renderfx = MSG_ReadShort(&net_message);
	}

	if (IS_QIIRR_PROTOCOL(cls.serverProtocol) && (bits & U_SOLID))
	{
		to->solid = MSG_ReadLong(&net_message);
	}

	if (bits & U_ORIGIN1)
	{
		if ((cls.serverProtocol == PROTOCOL_RR22_VERSION) && to->solid)
		{
			/* Read to float hack */
			to->origin[0] = MSG_ReadCoord(&net_message, PROTOCOL_RR23_VERSION);
		}
		else
		{
			to->origin[0] = MSG_ReadCoord(&net_message, cls.serverProtocol);
		}
	}

	if (bits & U_ORIGIN2)
	{
		if ((cls.serverProtocol == PROTOCOL_RR22_VERSION) && to->solid)
		{
			/* Read to float hack */
			to->origin[1] = MSG_ReadCoord(&net_message, PROTOCOL_RR23_VERSION);
		}
		else
		{
			to->origin[1] = MSG_ReadCoord(&net_message, cls.serverProtocol);
		}
	}

	if (bits & U_ORIGIN3)
	{
		if ((cls.serverProtocol == PROTOCOL_RR22_VERSION) && to->solid)
		{
			/* Read to float hack */
			to->origin[2] = MSG_ReadCoord(&net_message, PROTOCOL_RR23_VERSION);
		}
		else
		{
			to->origin[2] = MSG_ReadCoord(&net_message, cls.serverProtocol);
		}
	}

	if (bits & U_ANGLE1)
	{
		to->angles[0] = MSG_ReadAngle(&net_message, cls.serverProtocol);
	}

	if (bits & U_ANGLE2)
	{
		to->angles[1] = MSG_ReadAngle(&net_message, cls.serverProtocol);
	}

	if (bits & U_ANGLE3)
	{
		to->angles[2] = MSG_ReadAngle(&net_message, cls.serverProtocol);
	}

	if (bits & U_OLDORIGIN)
	{
		if (cls.serverProtocol == PROTOCOL_RR22_VERSION && to->solid)
		{
			/* read to float */
			MSG_ReadPos(&net_message, to->old_origin, PROTOCOL_VERSION);
		}
		else
		{
			MSG_ReadPos(&net_message, to->old_origin, cls.serverProtocol);
		}
	}

	if (bits & U_SOUND)
	{
		if (IS_QIIRR_PROTOCOL(cls.serverProtocol))
		{
			int sound_word = MSG_ReadShort(&net_message);

			to->sound = sound_word & 0x3fff;

			if (sound_word & SOUND_RR22_ATTENUATION)
			{
				int attenuation;

				attenuation = MSG_ReadByte(&net_message);
				Com_DPrintf("%s: ReRelease attenuation %d",
					__func__, attenuation);
			}

			if (sound_word & SOUND_RR22_VOLUME)
			{
				int volume;

				volume = MSG_ReadByte(&net_message);
				Com_DPrintf("%s: ReRelease volume %d",
					__func__, volume);
			}
		}
		else if (IS_QII97_PROTOCOL(cls.serverProtocol) ||
			(cls.serverProtocol == PROTOCOL_Q2TEST_VERSION))
		{
			to->sound = MSG_ReadByte(&net_message);
		}
		else if (cls.serverProtocol == PROTOCOL_VERSION)
		{
			to->sound = MSG_ReadShort(&net_message);
		}
		else
		{
			Com_Error(ERR_DROP, "%s: unexpected protocol %d",
				__func__, cls.serverProtocol);
		}
	}

	if (bits & U_EVENT)
	{
		to->event = MSG_ReadByte(&net_message);
	}
	else
	{
		to->event = 0;
	}

	if (bits & U_ALPHA)
	{
		if (cls.serverProtocol == PROTOCOL_VERSION)
		{
			to->rr_alpha = MSG_ReadFloat(&net_message);
		}
		else
		{
			to->rr_alpha = MSG_ReadByte(&net_message) / 255.0f;
		}
	}

	if (bits & U_SCALE)
	{
		if (cls.serverProtocol == PROTOCOL_VERSION)
		{
			int i;

			for (i = 0; i < 3; i++)
			{
				to->scale[i] = MSG_ReadFloat(&net_message);
			}
		}
		else
		{
			float scale;

			scale = MSG_ReadByte(&net_message) / 255.0f;
			VectorSet(to->scale, scale, scale, scale);
		}
	}

	if (bits & U_INSTANCE)
	{
		int instance;

		instance = MSG_ReadByte(&net_message);
		Com_DPrintf("%s: ReRelease instance %d", __func__, instance);
	}

	if (bits & U_OWNER)
	{
		int owner;

		owner = MSG_ReadShort(&net_message);
		Com_DPrintf("%s: ReRelease owner %d", __func__, owner);
	}

	if (bits & U_OLDFRAME)
	{
		int oldframe;

		oldframe = MSG_ReadShort(&net_message);
		Com_DPrintf("%s: ReRelease oldframe %d", __func__, oldframe);
	}

	if ((bits & U_SOLID) && !IS_QIIRR_PROTOCOL(cls.serverProtocol))
	{
		to->solid = MSG_ReadShort(&net_message);
	}

	/* Revalidate model indexes limits */
	if (bits & U_MODEL)
	{
		if (to->modelindex >= MAX_MODELS)
		{
			Com_Error(ERR_DROP, "%s: bad modelindex %d", __func__, to->modelindex);
		}
	}

	if ((bits & U_MODEL2) && !(to->renderfx & RF_FLARE))
	{
		if (to->modelindex2 >= MAX_MODELS)
		{
			Com_Error(ERR_DROP, "%s: bad modelindex2 %d", __func__, to->modelindex2);
		}
	}

	if ((bits & U_MODEL3) && !(to->renderfx & RF_FLARE))
	{
		if (to->modelindex3 >= MAX_MODELS)
		{
			Com_Error(ERR_DROP, "%s: bad modelindex3 %d", __func__, to->modelindex3);
		}
	}

	if (bits & U_MODEL4)
	{
		if (to->modelindex4 >= MAX_MODELS)
		{
			Com_Error(ERR_DROP, "%s: bad modelindex4 %d", __func__, to->modelindex4);
		}
	}
}

/*
 * Parses deltas from the given base and adds the resulting entity to
 * the current frame
 */
static void
CL_DeltaEntity(frame_t *frame, int newnum, const entity_xstate_t *old, uint64_t bits)
{
	centity_t dummy, *ent;
	entity_xstate_t *state;

	ent = CL_AllocEntity(newnum);
	if (!ent)
	{
		memset(&dummy, 0, sizeof(dummy));
		ent = &dummy;
	}

	state = &cl_parse_entities[cl.parse_entities & (MAX_PARSE_ENTITIES - 1)];
	cl.parse_entities++;
	frame->num_entities++;

	CL_ParseDelta(old, state, newnum, bits);

	/* some data changes will force no lerping */
	if ((state->modelindex != ent->current.modelindex) ||
		(state->modelindex2 != ent->current.modelindex2) ||
		(state->modelindex3 != ent->current.modelindex3) ||
		(state->modelindex4 != ent->current.modelindex4) ||
		(state->event == EV_PLAYER_TELEPORT) ||
		(state->event == EV_OTHER_TELEPORT) ||
		(abs((int)(state->origin[0] - ent->current.origin[0])) > 512) ||
		(abs((int)(state->origin[1] - ent->current.origin[1])) > 512) ||
		(abs((int)(state->origin[2] - ent->current.origin[2])) > 512)
		)
	{
		ent->serverframe = -99;
	}

	/* wasn't in last update, so initialize some things */
	if (ent->serverframe != cl.frame.serverframe - 1)
	{
		ent->trailcount = 1024; /* for diminishing rocket / grenade trails */

		/* duplicate the current state so
		   lerping doesn't hurt anything */
		ent->prev = *state;

		ent->serverframe_created = cl.frame.serverframe;

		if (state->event == EV_OTHER_TELEPORT)
		{
			VectorCopy(state->origin, ent->prev.origin);
			VectorCopy(state->origin, ent->lerp_origin);
		}
		else
		{
			VectorCopy(state->old_origin, ent->prev.origin);
			VectorCopy(state->old_origin, ent->lerp_origin);
		}
	}
	else
	{
		/* shuffle the last state to previous */
		ent->prev = ent->current;
	}

	ent->serverframe = cl.frame.serverframe;
	ent->current = *state;
}

/*
 * An svc_packetentities has just been
 * parsed, deal with the rest of the
 * data stream.
 */
static void
CL_ParsePacketEntities(const frame_t *oldframe, frame_t *newframe)
{
	centity_t *ent;
	const entity_xstate_t *oldstate = NULL;
	int oldindex, oldnum;

	newframe->parse_entities = cl.parse_entities;
	newframe->num_entities = 0;

	/* delta from the entities present in oldframe */
	oldindex = 0;

	if (!oldframe)
	{
		oldnum = 99999;
	}

	else
	{
		if (oldindex >= oldframe->num_entities)
		{
			oldnum = 99999;
		}

		else
		{
			oldstate = &cl_parse_entities[(oldframe->parse_entities +
									oldindex) & (MAX_PARSE_ENTITIES - 1)];
			oldnum = oldstate->number;
		}
	}

	while (1)
	{
		unsigned int newnum;
		uint64_t bits;

		newnum = CL_ParseEntityBits(&bits);

		if (newnum > MAX_CL_ENTNUM)
		{
			Com_Error(ERR_DROP, "%s: bad entity %d > %d\n",
				__func__, newnum, MAX_CL_ENTNUM);
			return;
		}

		if (net_message.readcount > net_message.cursize)
		{
			Com_Error(ERR_DROP, "%s: end of message", __func__);
			return;
		}

		if (!newnum)
		{
			break;
		}

		while (oldnum < newnum)
		{
			/* one or more entities from the old packet are unchanged */
			if (cl_shownet->value == 3)
			{
				Com_Printf("   unchanged: %i\n", oldnum);
			}

			CL_DeltaEntity(newframe, oldnum, oldstate, 0);

			oldindex++;

			if (oldindex >= oldframe->num_entities)
			{
				oldnum = 99999;
			}

			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities +
										oldindex) & (MAX_PARSE_ENTITIES - 1)];
				oldnum = oldstate->number;
			}
		}

		if (bits & U_REMOVE)
		{
			/* the entity present in oldframe is not in the current frame */
			if (cl_shownet->value == 3)
			{
				Com_Printf("   remove: %i\n", newnum);
			}

			if (oldnum != newnum)
			{
				Com_Printf("U_REMOVE: oldnum != newnum: %d %d\n", oldnum, newnum);
			}

			oldindex++;

			if (!oldframe)
			{
				continue;
			}

			if (oldindex >= oldframe->num_entities)
			{
				oldnum = 99999;
			}

			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities +
										oldindex) & (MAX_PARSE_ENTITIES - 1)];
				oldnum = oldstate->number;
			}

			continue;
		}

		if (oldnum == newnum)
		{
			/* delta from previous state */
			if (cl_shownet->value == 3)
			{
				Com_Printf("   delta: %i\n", newnum);
			}

			CL_DeltaEntity(newframe, newnum, oldstate, bits);

			oldindex++;

			if (oldindex >= oldframe->num_entities)
			{
				oldnum = 99999;
			}

			else
			{
				oldstate = &cl_parse_entities[(oldframe->parse_entities +
										oldindex) & (MAX_PARSE_ENTITIES - 1)];
				oldnum = oldstate->number;
			}

			continue;
		}

		if (oldnum > newnum)
		{
			/* delta from baseline */
			if (cl_shownet->value == 3)
			{
				Com_Printf("   baseline: %i\n", newnum);
			}

			ent = CL_AllocEntity(newnum);

			CL_DeltaEntity(newframe, newnum,
					ent ? &ent->baseline : NULL,
					bits);

			continue;
		}
	}

	/* any remaining entities in the old frame are copied over */
	while (oldnum != 99999)
	{
		/* one or more entities from the old packet are unchanged */
		if (cl_shownet->value == 3)
		{
			Com_Printf("   unchanged: %i\n", oldnum);
		}

		CL_DeltaEntity(newframe, oldnum, oldstate, 0);

		oldindex++;

		if (oldindex >= oldframe->num_entities)
		{
			oldnum = 99999;
		}

		else
		{
			oldstate = &cl_parse_entities[(oldframe->parse_entities +
									oldindex) & (MAX_PARSE_ENTITIES - 1)];
			oldnum = oldstate->number;
		}
	}
}

static void
CL_ParsePlayerstate(frame_t *oldframe, frame_t *newframe, int protocol)
{
	unsigned flags, group, stats_size;
	player_xstate_t *state;
	int flag_short;

	state = &newframe->playerstate;

	/* clear to old value before delta parsing */
	if (oldframe)
	{
		*state = oldframe->playerstate;
		VectorCopy(oldframe->origin, newframe->origin);
	}

	else
	{
		memset(state, 0, sizeof(*state));
		memset(newframe->origin, 0, sizeof(newframe->origin));
	}

	flag_short = MSG_ReadShort(&net_message);
	if (flag_short == -1)
	{
		Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
	}

	flags = (unsigned short)flag_short;
	if (flags & PS_MOREBITS)
	{
		flag_short = MSG_ReadShort(&net_message);
		if (flag_short == -1)
		{
			Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
		}

		flags |= (unsigned short)flag_short << 16;
	}

	/* parse the pmove_state_t */
	if (flags & PS_M_TYPE)
	{
		state->pmove.pm_type = MSG_ReadByte(&net_message);
	}

	if (flags & PS_M_ORIGIN)
	{
		MSG_ReadOrigin(&net_message, newframe->origin, protocol);
	}

	if (flags & PS_M_VELOCITY)
	{
		MSG_ReadVel(&net_message, state->pmove.velocity, protocol);
	}

	if (flags & PS_M_TIME)
	{
		if (IS_QIIRR_PROTOCOL(protocol))
		{
			int pm_time;

			/* KEX sends milliseconds, pmove_state_t uses 8 ms units */
			pm_time = MSG_ReadShort(&net_message) / 8;
			state->pmove.pm_time = pm_time > 255 ? 255 : pm_time;
		}
		else
		{
			state->pmove.pm_time = MSG_ReadByte(&net_message);
		}
	}

	if (flags & PS_M_FLAGS)
	{
		if (IS_QIIRR_PROTOCOL(protocol))
		{
			/* KEX flags above PMF_NO_PREDICTION have no vanilla equivalent */
			state->pmove.pm_flags = MSG_ReadShort(&net_message) & 0x7f;
		}
		else
		{
			state->pmove.pm_flags = MSG_ReadByte(&net_message);
		}
	}

	if (flags & PS_M_GRAVITY)
	{
		state->pmove.gravity = MSG_ReadShort(&net_message);
	}

	if (flags & PS_M_DELTA_ANGLES)
	{
		MSG_ReadDeltaAngles(&net_message, state->pmove.delta_angles, protocol);
	}

	if (cl.attractloop)
	{
		state->pmove.pm_type = PM_FREEZE; /* demo playback */
	}

	/* parse the rest of the player_xstate_t */
	if (flags & PS_VIEWOFFSET)
	{
		MSG_ReadViewOffset(&net_message, state->viewoffset, protocol);
	}

	if (flags & PS_VIEWANGLES)
	{
		MSG_ReadViewAngles(&net_message, state->viewangles, protocol);
	}

	if (flags & PS_KICKANGLES)
	{
		MSG_ReadKickAngles(&net_message, state->kick_angles, protocol);
	}

	if (flags & PS_WEAPONINDEX)
	{
		if (IS_QII97_PROTOCOL(protocol))
		{
			state->gunindex = MSG_ReadByte(&net_message);
		}
		else
		{
			state->gunindex = MSG_ReadShort(&net_message);
			if (IS_QIIRR_PROTOCOL(protocol))
			{
				state->gunindex &= 0x1fff;
			}

			if (state->gunindex < 0 || state->gunindex >= MAX_MODELS)
			{
				Com_Error(ERR_DROP, "%s: bad gunindex %d", __func__, state->gunindex);
			}
		}
	}

	if (flags & PS_WEAPONFRAME)
	{
		MSG_ReadWeaponFrameAngles(&net_message, state->gunoffset,
			state->gunangles, &state->gunframe, protocol);
	}

	if (flags & PS_BLEND)
	{
		state->blend[0] = MSG_ReadByte(&net_message) / 255.0f;
		state->blend[1] = MSG_ReadByte(&net_message) / 255.0f;
		state->blend[2] = MSG_ReadByte(&net_message) / 255.0f;
		state->blend[3] = MSG_ReadByte(&net_message) / 255.0f;
	}

	if (flags & PS_FOV)
	{
		state->fov = (float)MSG_ReadByte(&net_message);
	}

	if (flags & PS_RDFLAGS)
	{
		state->rdflags = MSG_ReadByte(&net_message);
	}

	/* parse stats */
	if (IS_QII97_PROTOCOL(protocol) ||
		IS_QIIRR_PROTOCOL(protocol))
	{
		stats_size = P_GetCountOfStats(protocol);
	}
	else
	{
		int b;

		b = MSG_ReadByte(&net_message);
		if (b < 0)
		{
			Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
		}
		stats_size = b;
	}

	/* Read stats bits */
	for (group = 0; group < (int)((stats_size + 31) / 32); group++)
	{
		int statbits, i;

		statbits = MSG_ReadLong(&net_message);

		for (i = group * 32; i < (group + 1) * 32; i++)
		{
			if (statbits & (1u << (i % 32)))
			{
				if (i < MAX_XSTATS)
				{
					state->stats[i] = MSG_ReadShort(&net_message);

					if (i == STAT_PICKUP_STRING)
					{
						state->stats[i] = P_ConvertConfigStringFrom(state->stats[i],
							protocol);
					}
				}
				else
				{
					Com_DPrintf("%s: unknown stats %d: %d\n",
						__func__, i, MSG_ReadShort(&net_message));
				}
			}
		}
	}

	if (flags & PS_DAMAGE_BLEND)
	{
		byte color[4];
		int i;

		/* damage blend color */
		for (i = 0; i < 4; i++)
		{
			color[i] = MSG_ReadByte(&net_message);
		}

		Com_DPrintf("%s: damage blend color 0x%2x%2x%2x%2x\n",
			__func__, color[0], color[1], color[2], color[3]);
	}

	if (flags & PS_TEAM_ID)
	{
		int team_id;

		team_id = MSG_ReadByte(&net_message);
		Com_DPrintf("%s: team id %d\n", __func__, team_id);
	}
}

static void
CL_FireEntityEvents(const frame_t *frame)
{
	int pnum;

	for (pnum = 0; pnum < frame->num_entities; pnum++)
	{
		entity_xstate_t *s1;
		int num;

		num = (frame->parse_entities + pnum) & (MAX_PARSE_ENTITIES - 1);
		s1 = &cl_parse_entities[num];

		if (s1->event)
		{
			CL_EntityEvent(s1);
		}

		if (s1->effects & EF_TELEPORTER)
		{
			CL_TeleporterParticles(s1);
		}

		if (s1->rr_effects & EF_TELEPORTER2)
		{
			CL_TeleporterParticles2(s1);
		}
	}
}

static void
SHOWNET(const char *s)
{
	if (cl_shownet->value >= 2)
	{
		Com_Printf("%3i:%s\n", net_message.readcount - 1, s);
	}
}

static void
CL_ShowNetCmd(int cmd)
{
	if (cmd < 0)
	{
		Com_Error(ERR_DROP, "%3i: unexpected message end",
			net_message.readcount - 1);
		return;
	}

	if (cl_shownet->value >= 2)
	{
		if (cmd >= ARRLEN(svc_strings))
		{
			Com_Printf("%3i:BAD CMD %i\n", net_message.readcount - 1, cmd);
			return;
		}

		SHOWNET(svc_strings[cmd]);
	}
}

static void
CL_ParseFrame(void)
{
	int cmd;
	int len;
	frame_t *old;

	memset(&cl.frame, 0, sizeof(cl.frame));

	cl.frame.serverframe = MSG_ReadLong(&net_message);
	cl.frame.deltaframe = MSG_ReadLong(&net_message);
	cl.frame.servertime = cl.frame.serverframe * cl.frame_msec;

	/* BIG HACK to let old demos continue to work */
	if (cls.serverProtocol != PROTOCOL_RELEASE_VERSION)
	{
		cl.surpressCount = MSG_ReadByte(&net_message);
	}

	if (cl_shownet->value == 3)
	{
		Com_Printf("   frame:%i  delta:%i\n", cl.frame.serverframe,
				cl.frame.deltaframe);
	}

	/* If the frame is delta compressed from data that we
	   no longer have available, we must suck up the rest of
	   the frame, but not use it, then ask for a non-compressed
	   message */
	if (cl.frame.deltaframe <= 0)
	{
		cl.frame.valid = true; /* uncompressed frame */
		old = NULL;
		cls.demowaiting = false; /* we can start recording now */
	}
	else
	{
		old = &cl.frames[cl.frame.deltaframe & UPDATE_MASK];

		if (!old->valid)
		{
			/* should never happen */
			Com_Printf("Delta from invalid frame (not supposed to happen!).\n");
		}

		if (old->serverframe != cl.frame.deltaframe)
		{
			/* The frame that the server did the delta from
			   is too old, so we can't reconstruct it properly. */
			Com_Printf("Delta frame too old.\n");
		}
		else if (cl.parse_entities - old->parse_entities > MAX_PARSE_ENTITIES -
				 128)
		{
			Com_Printf("Delta parse_entities too old.\n");
		}
		else
		{
			cl.frame.valid = true; /* valid delta parse */
		}
	}

	/* clamp time */
	if (cl.time > cl.frame.servertime)
	{
		cl.time = cl.frame.servertime;
	}

	else if (cl.time < cl.frame.servertime - 100)
	{
		cl.time = cl.frame.servertime - 100;
	}

	/* read areabits */
	len = MSG_ReadByte(&net_message);

	if (len == -1 || (byte)len > sizeof(cl.frame.areabits))
	{
		Com_Error(ERR_DROP, "%s: areabits overflow (%d > " YQ2_COM_PRIdS ")",
				__func__, len, sizeof(cl.frame.areabits));
		return;
	}

	MSG_ReadData(&net_message, &cl.frame.areabits, len);

	/* read playerinfo */
	cmd = P_CmdConvert(MSG_ReadByte(&net_message), cls.serverProtocol);
	CL_ShowNetCmd(cmd);

	if (cmd != svc_playerinfo)
	{
		Com_Error(ERR_DROP, "%s: 0x%X not playerinfo",
				__func__, cmd);
		return;
	}

	CL_ParsePlayerstate(old, &cl.frame, cls.serverProtocol);

	/* read packet entities */
	cmd = P_CmdConvert(MSG_ReadByte(&net_message), cls.serverProtocol);
	CL_ShowNetCmd(cmd);

	if (cmd != svc_packetentities)
	{
		Com_Error(ERR_DROP, "%s: 0x%X not packetentities",
			__func__, cmd);
		return;
	}

	CL_ParsePacketEntities(old, &cl.frame);

	/* save the frame off in the backup array for later delta comparisons */
	cl.frames[cl.frame.serverframe & UPDATE_MASK] = cl.frame;

	if (cl.frame.valid)
	{
		/* getting a valid frame message ends the connection process */
		if (cls.state != ca_active)
		{
			cls.state = ca_active;
			cl.force_refdef = true;
			cl.predicted_origin[0] = cl.frame.origin[0] * 0.125f;
			cl.predicted_origin[1] = cl.frame.origin[1] * 0.125f;
			cl.predicted_origin[2] = cl.frame.origin[2] * 0.125f;
			VectorCopy(cl.frame.playerstate.viewangles, cl.predicted_angles);

			if ((cls.disable_servercount != cl.servercount) && cl.refresh_prepped)
			{
				SCR_EndLoadingPlaque();  /* get rid of loading plaque */
			}

			cl.sound_prepped = true;

			if (paused_at_load)
			{
				if (cl_loadpaused->value == 1)
				{
					Cvar_Set("paused", "0");
				}

				paused_at_load = false;
			}
		}

		/* fire entity events */
		CL_FireEntityEvents(&cl.frame);

		if (!(!cl_predict->value ||
			  (cl.frame.playerstate.pmove.pm_flags &
			   PMF_NO_PREDICTION)))
		{
			CL_CheckPredictionError();
		}
	}
}

static const char *
CL_GetProtocolName(int protocol)
{
	switch (protocol)
	{
		case PROTOCOL_Q2TEST_VERSION:
			return "Quake 2 Test Demo";
		case PROTOCOL_RELEASE_VERSION:
			return "Quake 2 Demo";
		case PROTOCOL_XATRIX_VERSION:
			return "Quake 2 Xatrix Demo";
		case PROTOCOL_DEMO_VERSION:
			return "Quake 2 Release Demo";
		/* Network protocol */
		case PROTOCOL_R97_VERSION:
			return "Quake 2";
		/* Heretic 2 demo */
		case PROTOCOL_H2DEMO_VERSION:
			return "Heretic II Demo";
		/* ReRelease Demo */
		case PROTOCOL_RR22_VERSION:
			return "ReRelease Quake 2 Demo";
		/* ReRelease network protocol */
		case PROTOCOL_RR23_VERSION:
			return "ReRelease Quake 2";
		/* Our new protocol */
		case PROTOCOL_VERSION:
			return "ReRelease Quake 2 Custom version";
		default:
			return "Unknown protocol version";
	};
}

static void
CL_ParseServerData(void)
{
	extern cvar_t *fs_gamedirvar;
	char *str;
	int i;

	/* Clear all key states */
	In_FlushQueue();

	Com_DPrintf("Serverdata packet received.\n");

	/* wipe the client_state_t struct */
	CL_ClearState();
	cls.state = ca_connected;

	/* parse protocol version number */
	i = MSG_ReadLong(&net_message);
	cls.serverProtocol = i;

	/* another demo hack */
	if (Com_ServerState() && (
		IS_QII97_PROTOCOL(i) ||
		IS_QIIRR_PROTOCOL(i) ||
		(i == PROTOCOL_Q2TEST_VERSION) ||
		(i == PROTOCOL_H2DEMO_VERSION) ||
		(i == PROTOCOL_VERSION)))
	{
		Com_Printf("Network protocol: %s\n", CL_GetProtocolName(i));
	}
	else if (i != PROTOCOL_VERSION)
	{
		Com_Error(ERR_DROP, "Server returned version %i, not %i",
				i, PROTOCOL_VERSION);
		return;
	}

	cl.servercount = MSG_ReadLong(&net_message);
	cl.attractloop = MSG_ReadByte(&net_message);
	cl.frame_msec = 100;

	if (IS_QIIRR_PROTOCOL(cls.serverProtocol))
	{
		int fps;

		fps = MSG_ReadByte(&net_message);

		if (fps > 0)
		{
			cl.frame_msec = 1000 / fps;
		}
	}

	Com_DPrintf("Used %d ms frame time\n", cl.frame_msec);

	if ((cl_shownet->value == 0) &&
		((i == PROTOCOL_H2DEMO_VERSION) ||
		 (i == PROTOCOL_Q2TEST_VERSION)))
	{
		Com_Error(ERR_DROP, "Network protocol '%s' is currently unsupported\n",
			CL_GetProtocolName(i));
		return;
	}

	/* game directory */
	str = MSG_ReadString(&net_message);
	Q_strlcpy(cl.gamedir, str, sizeof(cl.gamedir));

	/* set gamedir */
	if ((*str && (!fs_gamedirvar->string || !*fs_gamedirvar->string ||
		  strcmp(fs_gamedirvar->string, str))) ||
		(!*str && (fs_gamedirvar->string && !*fs_gamedirvar->string)))
	{
		Cvar_Set("game", str);
	}

	/* parse player entity number */
	cl.playernum = MSG_ReadShort(&net_message);

	/* get the full level name */
	str = MSG_ReadString(&net_message);

	if (cl.playernum == -1)
	{
		/* playing a cinematic or showing a pic, not a level */
		SCR_PlayCinematic(str);
	}
	else
	{
		/* seperate the printfs so the server
		 * message can have a color */
		Com_Printf("\n\n\35\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\36\37\n\n");
		Com_Printf("%c%s\n", 2, str);

		/* need to prep refresh at next oportunity */
		cl.refresh_prepped = false;
	}
}

static void
CL_ParseBaseline(void)
{
	uint64_t bits;
	int newnum;
	centity_t *ent;

	newnum = CL_ParseEntityBits(&bits);
	ent = CL_AllocEntity(newnum);

	CL_ParseDelta(NULL, ent ? &ent->baseline : NULL, newnum, bits);
}

void
CL_LoadClientinfo(clientinfo_t *ci, char *s)
{
	char *t;
	char model_name[MAX_QPATH];
	char model_filename[MAX_QPATH];
	char skin_filename[MAX_QPATH];
	char weapon_filename[MAX_QPATH];

	Q_strlcpy(ci->cinfo, s, sizeof(ci->cinfo));
	s = ci->cinfo;

	/* isolate the player's name */
	Q_strlcpy(ci->name, s, sizeof(ci->name));
	t = strstr(s, "\\");

	if (t)
	{
		ci->name[t - s] = 0;
		s = t + 1;
	}

	if (cl_noskins->value || (*s == 0))
	{
		strcpy(model_filename, "players/male/tris.md2");
		strcpy(weapon_filename, "players/male/weapon.md2");
		strcpy(skin_filename, "players/male/grunt.pcx");
		strcpy(ci->iconname, "/players/male/grunt_i.pcx");
		ci->model = R_RegisterModel(model_filename);
		memset(ci->weaponmodel, 0, sizeof(ci->weaponmodel));
		ci->weaponmodel[0] = R_RegisterModel(weapon_filename);
		ci->skin = R_RegisterSkin(skin_filename);
		ci->icon = Draw_FindPic(ci->iconname);
	}
	else
	{
		char skin_name[MAX_QPATH];
		int i;

		/* isolate the model name */
		Q_strlcpy(model_name, s, sizeof(model_name));
		t = strstr(model_name, "/");

		if (!t)
		{
			t = strstr(model_name, "\\");
		}

		if (!t)
		{
			t = model_name;
		}

		*t = 0;

		/* isolate the skin name */
		 Q_strlcpy(skin_name, s + strlen(model_name) + 1, sizeof(skin_name));

		/* model file */
		Com_sprintf(model_filename, sizeof(model_filename),
				"players/%s/tris.md2", model_name);
		ci->model = R_RegisterModel(model_filename);

		if (!ci->model)
		{
			strcpy(model_name, "male");
			Com_sprintf(model_filename, sizeof(model_filename),
					"players/male/tris.md2");
			ci->model = R_RegisterModel(model_filename);
		}

		/* skin file */
		Com_sprintf(skin_filename, sizeof(skin_filename),
				"players/%s/%s.pcx", model_name, skin_name);
		ci->skin = R_RegisterSkin(skin_filename);

		/* if we don't have the skin and the model wasn't male,
		 * see if the male has it (this is for CTF's skins) */
		if (!ci->skin && Q_stricmp(model_name, "male"))
		{
			/* change model to male */
			strcpy(model_name, "male");
			Com_sprintf(model_filename, sizeof(model_filename),
					"players/male/tris.md2");
			ci->model = R_RegisterModel(model_filename);

			/* see if the skin exists for the male model */
			Com_sprintf(skin_filename, sizeof(skin_filename),
					"players/%s/%s.pcx", model_name, skin_name);
			ci->skin = R_RegisterSkin(skin_filename);
		}

		/* if we still don't have a skin, it means that the male model didn't have
		 * it, so default to grunt */
		if (!ci->skin)
		{
			/* see if the skin exists for the male model */
			Com_sprintf(skin_filename, sizeof(skin_filename),
					"players/%s/grunt.pcx", model_name);
			ci->skin = R_RegisterSkin(skin_filename);
		}

		/* weapon file */
		for (i = 0; i < num_cl_weaponmodels; i++)
		{
			Com_sprintf(weapon_filename, sizeof(weapon_filename),
					"players/%s/%s", model_name, cl_weaponmodels[i]);
			ci->weaponmodel[i] = R_RegisterModel(weapon_filename);

			if (!ci->weaponmodel[i] && (strcmp(model_name, "cyborg") == 0))
			{
				/* try male */
				Com_sprintf(weapon_filename, sizeof(weapon_filename),
						"players/male/%s", cl_weaponmodels[i]);
				ci->weaponmodel[i] = R_RegisterModel(weapon_filename);
			}

			if (!cl_vwep->value)
			{
				break; /* only one when vwep is off */
			}
		}

		/* icon file */
		Com_sprintf(ci->iconname, sizeof(ci->iconname),
				"/players/%s/%s_i.pcx", model_name, skin_name);
		ci->icon = Draw_FindPic(ci->iconname);
		if (!ci->icon)
		{
			char shortskin_name[MAX_QPATH];

			strcpy(shortskin_name, skin_name);

			/* Search skin without posible suffix */
			while(!ci->icon)
			{
				int len;

				len = strlen(shortskin_name);
				if (!len)
				{
					break;
				}

				shortskin_name[len - 1] = 0;
				Com_sprintf(ci->iconname, sizeof(ci->iconname),
						"/players/%s/%s_i.pcx", model_name, shortskin_name);
				ci->icon = Draw_FindPic(ci->iconname);
			}
		}
	}

	/* must have loaded all data types to be valid */
	if (!ci->skin || !ci->icon || !ci->model)
	{
		ci->skin = NULL;
		ci->icon = NULL;
		ci->model = NULL;
		ci->weaponmodel[0] = NULL;
		return;
	}

	VectorClear(ci->maxs);
	VectorClear(ci->mins);

	if (strlen(ci->iconname) > 4)
	{
		char suff[MAX_QPATH];
		char *pos;

		Q_strlcpy(suff, ci->iconname + 1, sizeof(suff));
		pos = strrchr(suff, '/');

		if (pos)
		{
			const dmdxframegroup_t *frames;
			int num, i;

			memcpy(pos + 1, "tris.md2", sizeof("tris.md2"));

			frames = Mod_GetModelInfo(suff, &num, NULL, NULL);

			if (frames)
			{
				for (i = 0; i < num; i++)
				{
					if (!strcmp(frames[i].name, "stand") ||
					    !strcmp(frames[i].name, "idle"))
					{
						VectorCopy(frames[i].maxs, ci->maxs);
						VectorCopy(frames[i].mins, ci->mins);
						break;
					}
				}
			}
		}
	}
}

/*
 * Load the skin, icon, and model for a client
 */
void
CL_ParseClientinfo(int player)
{
	char *s;
	clientinfo_t *ci;

	s = cl.configstrings[player + CS_PLAYERSKINS];

	ci = &cl.clientinfo[player];

	CL_LoadClientinfo(ci, s);
}

/*
 * Parses a shadow light configstring and stores it in cl.shadowdefs
 * Format: "num;iscone;radius;resolution;intensity;fade_start;fade_end;lightstyle;coneangle;conedirx;conediry;conedirz"
 */
void
CL_LoadShadowLight(int idx, const char *s)
{
	char buf[MAX_CONFIGSTRING] = {0};
	cl_shadowdef_t *shadow;
	size_t semis = 0, i;
	qboolean is_cone;
	char *p;

	/* copy and convert semicolons to spaces so COM_Parse can be used */
	for (i = 0; i < sizeof(buf) - 1; i++, s++)
	{
		if (!*s)
		{
			buf[i] = '\0';
			break;
		}
		else if (*s == ';')
		{
			buf[i] = ' ';
			semis++;
		}
		else
		{
			buf[i] = *s;
		}
	}

	/* validate expected number of fields */
	if (semis != 11 || idx < 0 || idx >= MAX_SHADOW_LIGHTS)
	{
		Com_DPrintf("%s: wrong shadow %d light %s\n",
			__func__, idx, buf);
		return;
	}

	p = buf;
	shadow = cl.shadowdefs + idx;
	shadow->number = atoi(COM_Parse(&p));

	if (shadow->number < 0 || shadow->number > MAX_EDICTS)
	{
		Com_DPrintf("%s: wrong shadow %d number %d\n",
			__func__, idx, shadow->number);
		return;
	}

	is_cone = !!atoi(COM_Parse(&p));
	shadow->light.radius = atof(COM_Parse(&p));
	shadow->light.resolution = atoi(COM_Parse(&p));
	shadow->light.intensity = atof(COM_Parse(&p));
	shadow->light.fade_start = atof(COM_Parse(&p));
	shadow->light.fade_end = atof(COM_Parse(&p));
	shadow->light.lightstyle = atoi(COM_Parse(&p));
	shadow->light.coneangle = atof(COM_Parse(&p));
	shadow->light.conedirection[0] = atof(COM_Parse(&p));
	shadow->light.conedirection[1] = atof(COM_Parse(&p));
	shadow->light.conedirection[2] = atof(COM_Parse(&p));

	if (!is_cone)
	{
		shadow->light.coneangle = 0.0f;
	}
}

static void
CL_ParseConfigStringValue(int orig_i, char *s)
{
	int i, cs_changed;
	size_t length, space;
	char *cs;

	i = P_ConvertConfigStringFrom(orig_i, cls.serverProtocol);

	if ((i < 0) || (i >= MAX_CONFIGSTRINGS))
	{
		Com_Printf("%s: bad index: %i\n", __func__, i);
		return;
	}

	if (i == CS_SKIP)
	{
		Com_DPrintf("%s: unknown config string %d: %s, protocol %s\n",
			__func__, orig_i, s, CL_GetProtocolName(cls.serverProtocol));
		return;
	}

	cs = cl.configstrings[i];
	cs_changed = strcmp(s, cs) != 0;

	length = strlen(s);

	/* statusbar code covers several configstring indices */
	if ((i >= CS_STATUSBAR) && (i < CS_STATUSBAR_END))
	{
		space = CS_STATUSBAR_SPACE(i);

		if (length >= space)
		{
			Com_Printf("%s: %i: statusbar code too long: " YQ2_COM_PRIdS " > " YQ2_COM_PRIdS "\n",
				__func__, i, length, space - 1);

			return;
		}

		memcpy(cs, s, length + 1);
	}
	else
	{
		space = sizeof(cl.configstrings[i]);

		if (length >= space)
		{
			Com_Printf("%s: %i: string too long: " YQ2_COM_PRIdS " > " YQ2_COM_PRIdS "\n",
				__func__, i, length, space - 1);

			return;
		}

		strcpy(cs, s);
	}

	if ((i >= CS_LIGHTS) && (i < CS_LIGHTS + MAX_LIGHTSTYLES))
	{
		CL_SetLightstyle(i - CS_LIGHTS);
	}
	else if (i == CS_CDTRACK)
	{
		if (cl.refresh_prepped)
		{
			OGG_PlayTrack(cl.configstrings[CS_CDTRACK], true, true);
		}
	}
	else if (i == CS_SKYROTATE || i == CS_SKYAXIS || i == CS_SKY)
	{
		if (cl.refresh_prepped)
		{
			CL_SetSky();
		}
	}
	else if ((i >= CS_MODELS) && (i < CS_MODELS + MAX_MODELS))
	{
		if (cl.refresh_prepped)
		{
			cl.model_draw[i - CS_MODELS] = R_RegisterModel(cl.configstrings[i]);

			if (cl.configstrings[i][0] == '*')
			{
				cl.model_clip[i - CS_MODELS] = CM_InlineModel(cl.configstrings[i]);
			}

			else
			{
				cl.model_clip[i - CS_MODELS] = NULL;
			}
		}
	}
	else if ((i >= CS_SOUNDS) && (i < CS_SOUNDS + MAX_SOUNDS))
	{
		if (cl.refresh_prepped)
		{
			cl.sound_precache[i - CS_SOUNDS] =
				S_RegisterSound(cl.configstrings[i]);
		}
	}
	else if ((i >= CS_IMAGES) && (i < CS_IMAGES + MAX_IMAGES))
	{
		if (cl.refresh_prepped)
		{
			cl.image_precache[i - CS_IMAGES] = Draw_FindPic(cl.configstrings[i]);
		}
	}
	else if ((i >= CS_PLAYERSKINS) && (i < CS_PLAYERSKINS + MAX_CLIENTS))
	{
		if (cl.refresh_prepped && cs_changed)
		{
			CL_ParseClientinfo(i - CS_PLAYERSKINS);
		}
	}
	else if ((i >= CS_SHADOWLIGHTS) && (i < CS_SHADOWLIGHTS + MAX_SHADOW_LIGHTS))
	{
		CL_LoadShadowLight(i - CS_SHADOWLIGHTS, s);
	}
}

static void
CL_ParseConfigString(sizebuf_t *message)
{
	int orig_i;
	char *s;

	orig_i = MSG_ReadShort(message) & 0xFFFF;
	s = MSG_ReadString(message);
	CL_ParseConfigStringValue(orig_i, s);
}

static void
CL_ParseLevelRestart(void)
{
	while (1)
	{
		int index;
		char *value;

		if (net_message.readcount > net_message.cursize ||
			net_message.cursize - net_message.readcount < 2)
		{
			Com_Error(ERR_DROP, "%s: truncated configstring list", __func__);
			return;
		}

		index = MSG_ReadShort(&net_message);
		if (index == -1)
		{
			break;
		}

		value = MSG_ReadString(&net_message);
		if (net_message.readcount > net_message.cursize)
		{
			Com_Error(ERR_DROP, "%s: truncated configstring value", __func__);
			return;
		}

		CL_ParseConfigStringValue(index, value);
	}
}

static void
CL_InflateBlast(byte **data, int *data_len)
{
	int compressed_len, uncompressed_len;
	mz_ulong actual_len;
	byte *compressed_data, *uncompressed_data;
	int result;

	if (net_message.readcount > net_message.cursize ||
		net_message.cursize - net_message.readcount < 4)
	{
		Com_Error(ERR_DROP, "%s: truncated blast header", __func__);
		return;
	}

	compressed_len = MSG_ReadShort(&net_message) & 0xFFFF;
	uncompressed_len = MSG_ReadShort(&net_message) & 0xFFFF;

	if (!compressed_len || !uncompressed_len ||
		compressed_len > net_message.cursize - net_message.readcount)
	{
		Com_Error(ERR_DROP, "%s: invalid blast sizes %d/%d",
			__func__, compressed_len, uncompressed_len);
		return;
	}

	compressed_data = malloc(compressed_len);
	uncompressed_data = malloc(uncompressed_len);

	if (!compressed_data || !uncompressed_data)
	{
		free(compressed_data);
		free(uncompressed_data);
		Com_Error(ERR_DROP, "%s: couldn't allocate blast buffers", __func__);
		return;
	}

	MSG_ReadData(&net_message, compressed_data, compressed_len);
	actual_len = uncompressed_len;
	result = mz_uncompress(uncompressed_data, &actual_len,
		compressed_data, compressed_len);
	free(compressed_data);

	if (result != MZ_OK || actual_len != (mz_ulong)uncompressed_len)
	{
		free(uncompressed_data);
		Com_Error(ERR_DROP, "%s: couldn't inflate blast (miniz %d, size %lu/%d)",
			__func__, result, (unsigned long)actual_len, uncompressed_len);
		return;
	}

	*data = uncompressed_data;
	*data_len = uncompressed_len;
}

static void
CL_ParseConfigBlast(void)
{
	byte *uncompressed_data = NULL;
	int uncompressed_len;
	sizebuf_t inflated_message;

	CL_InflateBlast(&uncompressed_data, &uncompressed_len);
	if (!uncompressed_data)
	{
		return;
	}

	SZ_Init(&inflated_message, uncompressed_data, uncompressed_len);

	while (inflated_message.readcount < inflated_message.cursize)
	{
		CL_ParseConfigString(&inflated_message);

		if (inflated_message.readcount > inflated_message.cursize)
		{
			free(uncompressed_data);
			Com_Error(ERR_DROP, "%s: malformed inflated config strings", __func__);
			return;
		}
	}

	free(uncompressed_data);
}

static void
CL_ParseBaselineBlast(void)
{
	byte *uncompressed_data = NULL;
	int uncompressed_len;
	sizebuf_t inflated_message, original_message;
	qboolean malformed = false;

	CL_InflateBlast(&uncompressed_data, &uncompressed_len);
	if (!uncompressed_data)
	{
		return;
	}

	SZ_Init(&inflated_message, uncompressed_data, uncompressed_len);
	original_message = net_message;
	net_message = inflated_message;

	while (net_message.readcount < net_message.cursize)
	{
		CL_ParseBaseline();

		if (net_message.readcount > net_message.cursize)
		{
			malformed = true;
			break;
		}
	}

	net_message = original_message;
	free(uncompressed_data);

	if (malformed)
	{
		Com_Error(ERR_DROP, "%s: malformed inflated baselines", __func__);
	}
}

static void
CL_ParseStartSoundPacket(void)
{
	vec3_t pos_v;
	float *pos;
	int channel, ent;
	int sound_num;
	float volume;
	float attenuation;
	int flags;
	float ofs;

	flags = MSG_ReadByte(&net_message);
	if (flags < 0)
	{
		Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
		return;
	}

	if (IS_QII97_PROTOCOL(cls.serverProtocol))
	{
		sound_num = MSG_ReadByte(&net_message);
	}
	else
	{
		sound_num = MSG_ReadShort(&net_message);
	}

	if (sound_num < 0)
	{
		Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
		return;
	}

	if (flags & SND_VOLUME)
	{
		volume = MSG_ReadByte(&net_message);
		if (volume < 0)
		{
			Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
			return;
		}

		volume /= 255.0f;
	}

	else
	{
		volume = DEFAULT_SOUND_PACKET_VOLUME;
	}

	if (flags & SND_ATTENUATION)
	{
		attenuation = MSG_ReadByte(&net_message);
		if (attenuation < 0)
		{
			Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
			return;
		}

		attenuation /= 64.0f;
	}

	else
	{
		attenuation = DEFAULT_SOUND_PACKET_ATTENUATION;
	}

	if (flags & SND_OFFSET)
	{
		ofs = MSG_ReadByte(&net_message);
		if (ofs < 0)
		{
			Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
			return;
		}

		ofs /= 1000.0f;
	}

	else
	{
		ofs = 0;
	}

	if (flags & SND_ENT)
	{
		/* entity relative */
		if (flags & SND_LARGE_ENT)
		{
			channel = MSG_ReadLong(&net_message);
		}
		else
		{
			channel = MSG_ReadShort(&net_message);
		}
		ent = channel >> 3;

		if (ent < 0)
		{
			Com_Printf("%s: incorrect channel %d\n",
					__func__, channel);
			return;
		}

		channel &= 7;
	}
	else
	{
		ent = 0;
		channel = 0;
	}

	if (flags & SND_POS)
	{
		/* positioned in space */
		MSG_ReadPos(&net_message, pos_v, cls.serverProtocol);

		pos = pos_v;
	}
	else
	{
		/* use entity number */
		pos = NULL;
	}

	if (sound_num >= MAX_SOUNDS)
	{
		Com_Printf("%s: incorrect sound id %d > MAX_SOUNDS\n",
			__func__, sound_num);
		return;
	}

	if (!cl.sound_precache[sound_num])
	{
		return;
	}

	S_StartSound(pos, ent, channel, cl.sound_precache[sound_num],
			volume, attenuation, ofs);
}

static void
CL_ParsePOI(void)
{
	int key, time, image, color, flags;
	vec3_t pos;

	key = MSG_ReadShort(&net_message);
	time = MSG_ReadShort(&net_message);
	pos[0] = MSG_ReadFloat(&net_message);
	pos[1] = MSG_ReadFloat(&net_message);
	pos[2] = MSG_ReadFloat(&net_message);
	image = MSG_ReadShort(&net_message);
	color = MSG_ReadByte(&net_message);
	flags = MSG_ReadByte(&net_message);

	Com_DPrintf("%s: key %d, time %d, image %d, color %d, flags %d at %.2fx%.2fx%.2f\n",
		__func__, key, time, image, color, flags, pos[0], pos[1], pos[2]);
}

static void
CL_ParseHelpPath(void)
{
	int start;
	vec3_t pos, dir;

	start = MSG_ReadByte(&net_message);
	pos[0] = MSG_ReadFloat(&net_message);
	pos[1] = MSG_ReadFloat(&net_message);
	pos[2] = MSG_ReadFloat(&net_message);
	MSG_ReadDir(&net_message, dir);

	Com_DPrintf("%s: start %d at %.2fx%.2fx%.2f, direction %.2fx%.2fx%.2f\n",
		__func__, start, pos[0], pos[1], pos[2], dir[0], dir[1], dir[2]);
}

static void
CL_ParseWaitingForPlayers(void)
{
	int count;

	count = MSG_ReadByte(&net_message);
	if (count < 0)
	{
		Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
		return;
	}

	Com_DPrintf("%s: %d players waiting\n", __func__, count);
}

static void
CL_ParseBotChat(void)
{
	char bot_name[2048], message[2048];
	int client;

	Q_strlcpy(bot_name, MSG_ReadString(&net_message), sizeof(bot_name));
	client = MSG_ReadShort(&net_message);
	Q_strlcpy(message, MSG_ReadString(&net_message), sizeof(message));

	Com_DPrintf("%s: %s to client %d: %s\n",
		__func__, bot_name, client, message);
}

static void
CL_ParseDamage(void)
{
	int count, i;

	count = MSG_ReadByte(&net_message);
	if (count < 0)
	{
		Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
		return;
	}

	for (i = 0; i < count; i++)
	{
		vec3_t direction;
		int damage_type;

		damage_type = MSG_ReadByte(&net_message);
		if (damage_type < 0)
		{
			Com_Error(ERR_DROP, "%s: unexpected message end", __func__);
			return;
		}

		MSG_ReadDir(&net_message, direction);

		Com_DPrintf("%s: %d: damage %d, health %d, armor %d, shield %d from %.2fx%.2fx%.2f\n",
			__func__, i,
			(damage_type & 0x1f) * 3,
			(damage_type & 0x20) >> 5,
			(damage_type & 0x40) >> 6,
			(damage_type & 0x80) >> 7,
			direction[0], direction[1], direction[2]);
	}
}

static void
CL_ParseLocPrint(void)
{
	int num_args, seq_arg = 0, flags, i, formatted_len;
	char **args, *dest, *message, *formatted;
	const char *format, *p, *end;

	flags = MSG_ReadByte(&net_message);	/* flags */
	message = strdup(MSG_ReadString(&net_message));
	num_args = MSG_ReadByte(&net_message);

	if (num_args < 0)
	{
		free(message);
		Com_Error(ERR_DROP, "%s: Invalid localized print argument count %d\n",
			__func__, num_args);
		return;
	}

	args = calloc(num_args, sizeof(*args));
	if (!args)
	{
		free(message);
		Com_Error(ERR_DROP, "%s: can't allocate args\n", __func__);
		return;
	}

	/* allocate little bit bigger if arg has used several times */
	formatted_len = 128;

	for (i = 0; i < num_args; i++)
	{
		args[i] = strdup(SV_LocalizationMessage(
			MSG_ReadString(&net_message), NULL));
		formatted_len += strlen(args[i]);
	}

	format = SV_LocalizationMessage(message, NULL);
	formatted_len += strlen(format);
	formatted = malloc(formatted_len + 1);
	if (!formatted)
	{
		for (i = 0; i < num_args; i++)
		{
			free(args[i]);
		}

		free(args);
		free(message);
		Com_Error(ERR_DROP, "%s: can't alloc formated message\n", __func__);
		return;
	}

	p = format;
	dest = formatted;
	end = formatted + formatted_len;

	while (*p && dest < end)
	{
		if (*p == '{')
		{
			const char *brace_start = p;
			p++;
			if (*p == '}')
			{
				p++;
				if (seq_arg < num_args)
				{
					const char *arg = args[seq_arg++];
					while (*arg && dest < end)
					{
						*dest++ = *arg++;
					}
				}
			}
			else if (*p >= '0' && *p <= '9')
			{
				int index = 0;
				while (*p >= '0' && *p <= '9')
				{
					index = index * 10 + (*p - '0');
					p++;
				}
				if (*p == '}')
				{
					p++;
					if (index >= 0 && index < num_args)
					{
						const char *arg = args[index];
						while (*arg && dest < end)
						{
							*dest++ = *arg++;
						}
					}
				}
				else
				{
					const char *src = brace_start;
					while (src < p && dest < end)
					{
						*dest++ = *src++;
					}
				}
			}
			else
			{
				const char *src = brace_start;
				while (src <= p && dest < end)
				{
					*dest++ = *src++;
				}
			}
		}
		else
		{
			*dest++ = *p++;
		}
	}
	*dest = '\0';

	if (flags == PRINT_CHAT)
	{
		S_StartLocalSound("misc/talk.wav");
		con.ormask = 128;
	}

	Com_Printf("%s", formatted);
	con.ormask = 0;

	/* clean message */
	free(formatted);
	free(message);

	/* clean arguments */
	for (i = 0; i < num_args; i++)
	{
		free(args[i]);
	}
	free(args);
}

void
CL_ParseServerMessage(void)
{
	char *s;
	int i;

	/* if recording demos, copy the message out */
	if (cl_shownet->value == 1)
	{
		Com_Printf("%i ", net_message.cursize);
	}

	else if (cl_shownet->value >= 2)
	{
		Com_Printf("------------------\n");
	}

	/* parse the message */
	while (1)
	{
		int cmd;

		if (net_message.readcount > net_message.cursize)
		{
			Com_Error(ERR_DROP, "%s: Bad server message", __func__);
			return;
		}

		cmd = MSG_ReadByte(&net_message);

		if (cmd == -1)
		{
			SHOWNET("END OF MESSAGE");
			break;
		}

		if (!cls.serverProtocol)
		{
			if (cmd == 0x07)
			{
				cls.serverProtocol = PROTOCOL_Q2TEST_VERSION;
			}
			else if (cmd == 0x0b)
			{
				cls.serverProtocol = PROTOCOL_H2DEMO_VERSION;
			}
		}

		cmd = P_CmdConvert(cmd, cls.serverProtocol);

		CL_ShowNetCmd(cmd);

		/* other commands */
		switch (cmd)
		{
			default:
				Com_Error(ERR_DROP, "%s: Illegible server message 0x%02x\n",
					__func__, cmd);
				return;

			case svc_nop:
				break;

			case svc_splitclient:
				/* next message for such split screen*/
				Com_DPrintf("%s: package for split screen %d\n",
					__func__, MSG_ReadByte(&net_message));
				break;

			case svc_disconnect:
				Com_Error(ERR_DISCONNECT, "Server disconnected\n");
				return;

			case svc_reconnect:
				Com_Printf("Server disconnected, reconnecting\n");

				if (cls.download)
				{
					/* close download */
					fclose(cls.download);
					cls.download = NULL;
				}

				cls.state = ca_connecting;
				cls.serverProtocol = 0;
				cls.connect_time = -99999; /* CL_CheckForResend() will fire immediately */
				break;

			case svc_print:
				i = MSG_ReadByte(&net_message);
				if (i < 0)
				{
					SHOWNET("END OF MESSAGE");
					break;
				}

				if (i == PRINT_CHAT)
				{
					S_StartLocalSound("misc/talk.wav");
					con.ormask = 128;
				}

				Com_Printf("%s", MSG_ReadString(&net_message));
				con.ormask = 0;
				break;

			case svc_locprint:
				CL_ParseLocPrint();
				break;

			case svc_centerprint:
				SCR_CenterPrint(MSG_ReadString(&net_message));
				break;

			case svc_stufftext:
				s = MSG_ReadString(&net_message);
				Com_DPrintf("stufftext: %s\n", s);
				Cbuf_AddText(s);
				break;

			case svc_serverdata:
				Cbuf_Execute();  /* make sure any stuffed commands are done */
				CL_ParseServerData();
				break;

			case svc_configstring:
				CL_ParseConfigString(&net_message);
				break;

			case svc_configblast:
				CL_ParseConfigBlast();
				break;

			case svc_level_restart:
				CL_ParseLevelRestart();
				break;

			case svc_sound:
				CL_ParseStartSoundPacket();
				break;

			case svc_damage:
				CL_ParseDamage();
				break;

			case svc_spawnbaseline:
				CL_ParseBaseline();
				break;

			case svc_spawnbaselineblast:
				CL_ParseBaselineBlast();
				break;

			case svc_temp_entity:
				CL_ParseTEnt();
				break;

			case svc_muzzleflash:
				CL_AddMuzzleFlash();
				break;

			case svc_muzzleflash2:
				CL_AddMuzzleFlash2();
				break;

			case svc_muzzleflash3:
				CL_AddMuzzleFlash3();
				break;

			case svc_download:
				CL_ParseDownload();
				break;

			case svc_frame:
				CL_ParseFrame();
				break;

			case svc_inventory:
				CL_ParseInventory();
				break;

			case svc_layout:
				s = MSG_ReadString(&net_message);
				Q_strlcpy(cl.layout, s, sizeof(cl.layout));
				break;

			case svc_fog:
				CL_AddFog(&cl.refdef.fog);
				break;

			case svc_waitingforplayers:
				CL_ParseWaitingForPlayers();
				break;

			case svc_bot_chat:
				CL_ParseBotChat();
				break;

			case svc_poi:
				CL_ParsePOI();
				break;

			case svc_help_path:
				CL_ParseHelpPath();
				break;

			case svc_achievement:
				Com_DPrintf("%s: achievement %s\n",
					__func__, MSG_ReadString(&net_message));
				break;

			case svc_playerinfo:
			case svc_packetentities:
			case svc_deltapacketentities:
				Com_Error(ERR_DROP, "Out of place frame data");
				return;
		}
	}

	CL_AddNetgraph();

	/* we don't know if it is ok to save a demo message
	   until after we have parsed the frame */
	if (cls.demorecording && !cls.demowaiting)
	{
		CL_WriteDemoMessage();
	}
}
