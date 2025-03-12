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
 */

#include "../header/local.h"
#include "ai_local.h"

//ACE

//==========================================
// BOT_ServerCommand
// Special server command processor
//==========================================
qboolean BOT_ServerCommand (void)
{
	char	*cmd;

	cmd = gi.argv(1);

	if (Q_stricmp (cmd, "addbot") == 0)
	{
		if(ctf->value) // name, skin, team
		{
			BOT_SpawnBot ( gi.argv(2), gi.argv(3), gi.argv(4), NULL );
		}
		else // name, skin
		{
			BOT_SpawnBot ( NULL, gi.argv(2), gi.argv(3), NULL );
		}
	}
	else if( !Q_stricmp (cmd, "editnodes") )
	{
		AITools_InitEditnodes();
	}
	else if (!Q_stricmp (cmd, "makenodes"))
	{
		AITools_InitMakenodes();
	}
	else if (!Q_stricmp (cmd, "savenodes"))
	{
		AITools_SaveNodes();
	}
	else if (!Q_stricmp (cmd, "addbotroam"))
	{
		AITools_AddBotRoamNode();
	}
	else
	{
		return false;
	}

	return true;
}

///////////////////////////////////////////////////////////////////////
// These routines are bot safe print routines, all id code needs to be
// changed to these so the bots do not blow up on messages sent to them.
// Do a find and replace on all code that matches the below criteria.
//
// (Got the basic idea from Ridah)
//
//  change: gi.cprintf to safe_cprintf
//  change: gi.bprintf to safe_bprintf
//  change: gi.centerprintf to safe_centerprintf
//
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////
// botsafe cprintf
///////////////////////////////////////////////////////////////////////
void safe_cprintf (edict_t *ent, int printlevel, char *fmt, ...)
{
	char	bigbuffer[0x10000];
	va_list		argptr;

	if (ent && (!ent->inuse || ent->ai))
		return;

	va_start (argptr,fmt);
	vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);

	gi.cprintf(ent, printlevel, bigbuffer);

}

///////////////////////////////////////////////////////////////////////
// botsafe centerprintf
///////////////////////////////////////////////////////////////////////
void safe_centerprintf (edict_t *ent, char *fmt, ...)
{
	char	bigbuffer[0x10000];
	va_list		argptr;

	if (!ent->inuse || ent->ai)
		return;

	va_start (argptr,fmt);
	vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);

	gi.centerprintf(ent, bigbuffer);

}

///////////////////////////////////////////////////////////////////////
// botsafe bprintf
///////////////////////////////////////////////////////////////////////
void safe_bprintf (int printlevel, char *fmt, ...)
{
	int i;
	char	bigbuffer[0x10000];
	va_list		argptr;
	edict_t	*cl_ent;

	va_start (argptr,fmt);
	vsprintf (bigbuffer,fmt,argptr);
	va_end (argptr);

	if (dedicated->value)
		gi.cprintf(NULL, printlevel, bigbuffer);

	for (i=0 ; i<maxclients->value ; i++)
	{
		cl_ent = g_edicts + 1 + i;
		if (!cl_ent->inuse || cl_ent->ai)
			continue;

		gi.cprintf(cl_ent, printlevel, bigbuffer);
	}
}
