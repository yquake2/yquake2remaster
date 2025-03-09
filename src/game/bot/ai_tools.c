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


//==========================================
// AIDebug_ToogleBotDebug
//==========================================
void AIDebug_ToogleBotDebug(void)
{
/*	if (AIDevel.debugMode || !sv_cheats->integer )
	{
//		Com_Printf("BOT: Debug Mode Off\n");
		AIDevel.debugMode = false;
		return;
	}

	//Activate debug mode
	Com_Printf("\n======================================\n");
	Com_Printf("--==[ D E B U G ]==--\n");
	Com_Printf("======================================\n");
	Com_Printf("'addnode [nodetype]' -- Add [specified] node to players current location\n");
	Com_Printf("'movenode [node] [x y z]' -- Move [node] to [x y z] coordinates\n");
	Com_Printf("'findnode' -- Finds closest node\n");
	Com_Printf("'removelink [node1 node2]' -- Removes link between two nodes\n");
	Com_Printf("'addlink [node1 node2]' -- Adds a link between two nodes\n");
	Com_Printf("======================================\n\n");

	Com_Printf("BOT: Debug Mode On\n");

	AIDevel.debugMode = true;
*/
}


//==========================================
// AIDebug_SetChased
// Theorically, only one client
//	at the time will chase. Otherwise it will
//	be a really fucked up situation.
//==========================================
void AIDebug_SetChased(edict_t *ent)
{
/*	int i;
	AIDevel.chaseguy = NULL;
	AIDevel.debugChased = false;

	if (!AIDevel.debugMode || !sv_cheats->integer)
		return;

	//find if anyone is chasing this bot
	for(i=0;i<game.maxclients+1;i++)
	{
//		AIDevel.chaseguy = game.edicts + i + 1;
		AIDevel.chaseguy = g_edicts + i + 1;
		if(AIDevel.chaseguy->inuse && AIDevel.chaseguy->client){
			if( AIDevel.chaseguy->client->chase_target &&
				AIDevel.chaseguy->client->chase_target == ent)
				break;
		}
		AIDevel.chaseguy = NULL;
	}

	if (!AIDevel.chaseguy)
		return;

	AIDevel.debugChased = true;
*/
}



//=======================================================================
//							NODE TOOLS
//=======================================================================



//==========================================
// AITools_DrawLine
// Just so I don't hate to write the event every time
//==========================================
void AITools_DrawLine(vec3_t origin, vec3_t dest)
{
/*
	edict_t		*event;

	event = G_SpawnEvent ( EV_BFG_LASER, 0, origin );
	event->svflags = SVF_FORCEOLDORIGIN;
	VectorCopy ( dest, event->s.origin2 );
*/
}


//==========================================
// AITools_DrawPath
// Draws the current path (floods as hell also)
//==========================================
// static int	drawnpath_timeout;
void AITools_DrawPath(edict_t *self, int node_from, int node_to)
{
/*
	int			count = 0;
	int			pos = 0;

	//don't draw it every frame (flood)
	if (level.time < drawnpath_timeout)
		return;
	drawnpath_timeout = level.time + 4*FRAMETIME;

	if( self->ai.path->goalNode != node_to )
		return;

	//find position in stored path
	while( self->ai.path->nodes[pos] != node_from )
	{
		pos++;
		if( self->ai.path->goalNode == self->ai.path->nodes[pos] )
			return;	//failed
	}

	// Now set up and display the path
	while( self->ai.path->nodes[pos] != node_to && count < 32)
	{
		edict_t		*event;

		event = G_SpawnEvent ( EV_BFG_LASER, 0, nodes[self->ai.path->nodes[pos]].origin );
		event->svflags = SVF_FORCEOLDORIGIN;
		VectorCopy ( nodes[self->ai.path->nodes[pos+1]].origin, event->s.origin2 );
		pos++;
		count++;
	}
*/
}

//==========================================
// AITools_ShowPlinks
// Draws lines from the current node to it's plinks nodes
//==========================================
// static int	debugdrawplinks_timeout;
void AITools_ShowPlinks( void )
{
/*	int		current_node;
	int		plink_node;
	int		i;

	if (AIDevel.plinkguy == NULL)
		return;

	//don't draw it every frame (flood)
	if (level.time < debugdrawplinks_timeout)
		return;
	debugdrawplinks_timeout = level.time + 4*FRAMETIME;

	//do it
	current_node = AI_FindClosestReachableNode(AIDevel.plinkguy,NODE_DENSITY*3,NODE_ALL);
	if (!pLinks[current_node].numLinks)
		return;

	for (i=0; i<nav.num_items;i++){
		if (nav.items[i].node == current_node){
			if( !nav.items[i].ent->classname )
				G_CenterPrintMsg(AIDevel.plinkguy, "no classname");
			else
				G_CenterPrintMsg(AIDevel.plinkguy, "%s", nav.items[i].ent->classname);
			break;
		}
	}

	for (i=0; i<pLinks[current_node].numLinks; i++)
	{
		plink_node = pLinks[current_node].nodes[i];
		AITools_DrawLine(nodes[current_node].origin, nodes[plink_node].origin);
	}
*/
}


//=======================================================================
//=======================================================================


//==========================================
// AITools_Frame
// Gives think time to the debug tools found
// in this archive (those witch need it)
//==========================================
void AITools_Frame(void)
{
	//debug
	if( AIDevel.showPLinks )
		AITools_ShowPlinks();
}


