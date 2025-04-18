/*
 * Copyright (C) 1997-2001 Id Software, Inc.
 * Copyright (C) 2001 Steve Yeager
 * Copyright (C) 2001-2004 Pat AfterMoon
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
 * --------------------------------------------------------------
 * The ACE Bot is a product of Steve Yeager, and is available from
 * the ACE Bot homepage, at http://www.axionfx.com/ace.
 *
 * This program is a modification of the ACE Bot, and is therefore
 * in NO WAY supported by Steve Yeager.
 */

#include "../header/local.h"
#include "ai_local.h"


//===============================================================
//
//				BOT SPAWN
//
//===============================================================



///////////////////////////////////////////////////////////////////////
// Respawn the bot
///////////////////////////////////////////////////////////////////////
void
BOT_Respawn(edict_t *self)
{
	CopyToBodyQue(self);

	PutClientInServer(self);

	// add a teleportation effect
	self->s.event = EV_PLAYER_TELEPORT;

		// hold in place briefly
	self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	self->client->ps.pmove.pm_time = 14;

	self->client->respawn_time = level.time;

	AI_ResetWeights(self);
	AI_ResetNavigation(self);
}


///////////////////////////////////////////////////////////////////////
// Find a free client spot - //jabot092(2)
///////////////////////////////////////////////////////////////////////
static edict_t *
BOT_FindFreeClient(void)
{
	edict_t *bot;
	edict_t	*ent;
	int	i;
	int max_count=0;

	bot = NULL;
	for( i = 0, ent = g_edicts + 1; i < game.maxclients; i++, ent++ )
	{
		if( !ent->inuse && bot == NULL )
			bot = ent;

		//count bots for bot names
		if( ent->count > max_count )
			max_count = ent->count;
	}

	if (bot == NULL || (max_count + 2) >= game.maxclients ) //always leave room for 1 player
	{
		return NULL;
	}

	bot->count = max_count + 1; // Will become bot name...
	return bot;
}

static const char *bot_skin_table[] = {
	"female/athena",
	"female/brianna",
	"female/cobalt",
	"female/ensign",
	"female/jezebel",
	"female/jungle",
	"female/lotus",
	"female/stiletto",
	"female/venus",
	"female/voodoo",
	"male/cipher",
	"male/flak",
	"male/grunt",
	"male/howitzer",
	"male/major",
	"male/nightops",
	"male/pointman",
	"male/psycho",
	"male/razor",
	"male/sniper"
};

///////////////////////////////////////////////////////////////////////
// Set the name of the bot and update the userinfo
///////////////////////////////////////////////////////////////////////
static void
BOT_SetName(edict_t *bot, char *name, char *skin, char *team)
{
	char userinfo[MAX_INFO_STRING];
	char bot_skin[MAX_INFO_STRING];
	char bot_name[MAX_INFO_STRING];

	// Set the name for the bot.
	// name
	if(strlen(name) == 0)
	{
		sprintf(bot_name, "Bot%d", bot->count);
	}
	else
	{
		strcpy(bot_name, name);
	}

	// skin
	if(strlen(skin) == 0)
	{
		int rnd;

		/* randomly choose skin */
		rnd = rand() % (sizeof(bot_skin_table) / sizeof(*bot_skin_table));
		sprintf(bot_skin, bot_skin_table[rnd]);
	}
	else
	{
		strcpy(bot_skin, skin);
	}

	// initialise userinfo
	memset(userinfo, 0, sizeof(userinfo));

	// add bot's name/skin/hand to userinfo
	Info_SetValueForKey(userinfo, "name", bot_name);
	Info_SetValueForKey(userinfo, "skin", bot_skin);
	Info_SetValueForKey(userinfo, "hand", "2"); // bot is center handed for now!

	ClientConnect(bot, userinfo);
}

//==========================================
// BOT_NextCTFTeam
// Get the emptier CTF team
//==========================================
static int
BOT_NextCTFTeam()
{
	int	i;
	int	onteam1 = 0;
	int	onteam2 = 0;
	edict_t		*self;

	// Only use in CTF games
	if (!ctf->value)
		return 0;

	for (i = 0; i < game.maxclients + 1; i++)
	{
		self = g_edicts +i + 1;
		if (self->inuse && self->client)
		{
			if (self->client->resp.ctf_team == CTF_TEAM1)
				onteam1++;
			else if (self->client->resp.ctf_team == CTF_TEAM2)
				onteam2++;
		}
	}

	if (onteam1 > onteam2)
		return (2);
	else if (onteam2 >= onteam1)
		return (1);

	return (1);
}

//==========================================
// BOT_JoinCTFTeam
// Assign a team for the bot
//==========================================
static qboolean
BOT_JoinCTFTeam(edict_t *ent, char *team_name)
{
	char	*s;
	int		team;
//	edict_t	*event;


	if (ent->client->resp.ctf_team != CTF_NOTEAM)
		return false;

	// find what ctf team
	if ((team_name !=NULL) && (strcmp(team_name, "blue") == 0))
		team = CTF_TEAM2;
	else if ((team_name !=NULL) && (strcmp(team_name, "red") == 0))
		team = CTF_TEAM1;
	else
		team = BOT_NextCTFTeam();

	if (team == CTF_NOTEAM)
		return false;

	//join ctf team
	ent->svflags &= ~SVF_NOCLIENT;
	ent->client->resp.ctf_state = 1;//0?
	ent->client->resp.ctf_team = team;
	s = Info_ValueForKey (ent->client->pers.userinfo, "skin");
	CTFAssignSkin(ent, s);

	PutClientInServer(ent);

	// hold in place briefly
	ent->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
	ent->client->ps.pmove.pm_time = 14;

	Com_Printf ( "%s joined the %s team.\n",
		ent->client->pers.netname, CTFTeamName(ent->client->resp.ctf_team));

	return true;
}


//==========================================
// BOT_DMClass_JoinGame
// put the bot into the game.
//==========================================
void BOT_DMClass_JoinGame (edict_t *ent, char *team_name)
{
	if ( !BOT_JoinCTFTeam(ent, team_name) )
		Com_Printf ( "%s joined the game.\n",
		ent->client->pers.netname);

	ent->think = AI_Think;
	ent->nextthink = level.time + FRAMETIME;

	//join game
	ent->movetype = MOVETYPE_WALK;
	ent->solid = SOLID_BBOX;
	ent->svflags &= ~SVF_NOCLIENT;
	ent->client->ps.gunindex = 0;

	if (!KillBox (ent))
	{	// could't spawn in?
	}
	gi.linkentity (ent);
}

//==========================================
// BOT_StartAsSpectator
//==========================================
void BOT_StartAsSpectator (edict_t *ent)
{
	// start as 'observer'
	ent->movetype = MOVETYPE_NOCLIP;
	ent->solid = SOLID_NOT;
	ent->svflags |= SVF_NOCLIENT;
	ent->client->resp.ctf_team = CTF_NOTEAM;
	ent->client->ps.gunindex = 0;
	gi.linkentity (ent);
}


//==========================================
// BOT_JoinGame
// 3 for teams and such
//==========================================
static void
BOT_JoinBlue(edict_t *ent)
{
	BOT_DMClass_JoinGame(ent, "blue");
}

static void
BOT_JoinRed(edict_t *ent)
{
	BOT_DMClass_JoinGame(ent, "red");
}

static void
BOT_JoinGame(edict_t *ent)
{
	BOT_DMClass_JoinGame(ent, NULL);
}

///////////////////////////////////////////////////////////////////////
// Spawn the bot
///////////////////////////////////////////////////////////////////////
void BOT_SpawnBot (char *team, char *name, char *skin, char *userinfo)
{
	edict_t *bot;

	if(!nav.loaded)
	{
		Com_Printf("Can't spawn bots without a valid navigation file\n");
		return;
	}

	bot = BOT_FindFreeClient();

	if (!bot)
	{
		gi.bprintf(PRINT_MEDIUM, "Server is full, increase Maxclients.\n");
		return;
	}

	/* init the bot */
	bot->inuse = true;
	bot->yaw_speed = 100;

	/* To allow bots to respawn */
	if(userinfo == NULL)
	{
		BOT_SetName(bot, name, skin, team);
	}
	else
	{
		ClientConnect(bot, userinfo);
	}

	G_InitEdict(bot);
	G_SpawnAI(bot);
	bot->ai->is_bot = true;
	InitClientResp(bot->client);

	PutClientInServer(bot);
	BOT_StartAsSpectator(bot);

	/* skill */
	bot->ai->pers.skillLevel = (int)(random() * MAX_BOT_SKILL);
	if (bot->ai->pers.skillLevel > MAX_BOT_SKILL)
	{
		/* fix if off-limits */
		bot->ai->pers.skillLevel =  MAX_BOT_SKILL;
	}
	else if (bot->ai->pers.skillLevel < 0)
	{
		bot->ai->pers.skillLevel =  0;
	}

	BOT_DMclass_InitPersistant(bot);
	AI_ResetWeights(bot);
	AI_ResetNavigation(bot);

	bot->think = BOT_JoinGame;
	bot->nextthink = level.time + (int)(random() * 6.0);
	if(ctf->value && team != NULL)
	{
		if (!Q_stricmp(team, "blue"))
		{
			bot->think = BOT_JoinBlue;
		}
		else if (!Q_stricmp( team, "red"))
		{
			bot->think = BOT_JoinRed;
		}
	}

	AI_EnemyAdded(bot); // let the ai know we added another
}


///////////////////////////////////////////////////////////////////////
// Remove a bot by name or all bots
///////////////////////////////////////////////////////////////////////
void BOT_RemoveBot(char *name)
{
	int i;
	edict_t *bot;

	for (i = 0; i < maxclients->value; i++)
	{
		bot = g_edicts + i + 1;

		if (!bot->inuse || !bot->ai)
		{
			continue;
		}

		if (bot->ai->is_bot &&
			(!strcmp(bot->client->pers.netname,name) || !strcmp(name,"all")))
		{
			bot->health = 0;
			player_die (bot, bot, bot, 100000, vec3_origin);

			/* don't even bother waiting for death frames */
			bot->deadflag = DEAD_DEAD;
			bot->inuse = false;
			AI_EnemyRemoved(bot);
			G_FreeAI(bot);
			gi.bprintf(PRINT_MEDIUM, "%s removed\n", bot->client->pers.netname);
		}
	}
}
