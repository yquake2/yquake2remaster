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

//===========================================================
//
//				EDIT NODES
//
//===========================================================

//==========================================
// AI_AddNode
// Add a node of normal navigation types.
// Not valid to add nodes from entities nor items
//==========================================
static int
AI_AddNode(edict_t *ent, vec3_t origin, int flagsmask)
{
	if (nav.num_nodes + 1 > MAX_NODES)
	{
		return INVALID;
	}

	if (flagsmask & NODEFLAGS_WATER)
	{
		flagsmask |= NODEFLAGS_FLOAT;
	}

	VectorCopy(origin, nodes[nav.num_nodes].origin);
	if (!(flagsmask & NODEFLAGS_FLOAT))
	{
		AI_DropNodeOriginToFloor(nodes[nav.num_nodes].origin, ent);
	}

	//if (!(flagsmask & NODEFLAGS_NOWORLD)) {	//don't spawn inside solids
	//	trace_t	trace;
	//	trace = gi.trace( nodes[nav.num_nodes].origin, tv(-15, -15, -8), tv(15, 15, 8), nodes[nav.num_nodes].origin, ent, MASK_NODESOLID );
	//	if (trace.startsolid)
	//		return INVALID;
	//}

	nodes[nav.num_nodes].flags = flagsmask;
	nodes[nav.num_nodes].flags |= AI_FlagsForNode( nodes[nav.num_nodes].origin, ent );

	if (bot_debugmonster->value)
	{
		Com_Printf("Dropped Node\n");
	}

	nav.num_nodes++;
	return nav.num_nodes-1; // return the node added
}

//==========================================
// AI_UpdateNodeEdge
// Add/Update node connections (paths)
//==========================================
static void
AI_UpdateNodeEdge(int from, int to)
{
	int	link;

	if (from == INVALID || to == INVALID || from == to)
	{
		return; // safety
	}

	if (AI_PlinkExists(from, to))
	{
		link = AI_PlinkMoveType(from, to);
	}
	else
	{
		link = AI_FindLinkType(from, to);
	}

	if (bot_debugmonster->value)
	{
		Com_Printf("%s: Link %d -> %d. %s\n",
			__func__, from, to, AI_LinkString(link));
	}
}

//===========================================================
//
//			PLAYER DROPPING NODES
//
//===========================================================

//==========================================
// AI_DropLadderNodes
// drop nodes all along the ladder
//==========================================
static void
AI_DropLadderNodes(edict_t *self)
{
	vec3_t	torigin;
	vec3_t	borigin;
	vec3_t	droporigin;
	int		step;
	trace_t trace;

	//find top & bottom of the ladder
	VectorCopy(self->s.origin, torigin);
	VectorCopy(self->s.origin, borigin);

	while (AI_IsLadder(torigin, self->client->ps.viewangles, self->mins, self->maxs, self))
	{
		torigin[2]++;
	}
	torigin[2] += (self->mins[2] + 8);

	//drop node on top
	AI_AddNode(self, torigin, (NODEFLAGS_LADDER|NODEFLAGS_FLOAT));

	//find bottom. Try simple first
	trace = gi.trace( borigin, tv(-15,-15,-24), tv(15,15,0), tv(borigin[0], borigin[1], borigin[2] - 2048), self, MASK_NODESOLID );
	if (!trace.startsolid &&
		trace.fraction < 1.0 &&
		AI_IsLadder(trace.endpos, self->client->ps.viewangles, self->mins, self->maxs, self))
	{
		VectorCopy(trace.endpos, borigin);
	}
	else
	{
		//it wasn't so easy
		trace = gi.trace(borigin, tv(-15,-15,-25), tv(15,15,0), borigin, self, MASK_NODESOLID);
		while (AI_IsLadder(borigin, self->client->ps.viewangles, self->mins, self->maxs, self)
			&& !trace.startsolid)
		{
			borigin[2]--;
			trace = gi.trace( borigin, tv(-15,-15,-25), tv(15,15,0), borigin, self, MASK_NODESOLID );
		}

		//if trace never reached solid, put the node on the ladder
		if (!trace.startsolid)
			borigin[2] -= self->mins[2];
	}

	//drop node on bottom
	AI_AddNode(self, borigin, (NODEFLAGS_LADDER | NODEFLAGS_FLOAT));

	if (torigin[2] - borigin[2] < NODE_DENSITY)
	{
		return;
	}

	//make subdivisions and add nodes in between
	step = NODE_DENSITY*0.8;
	VectorCopy( borigin, droporigin );
	droporigin[2] += step;
	while (droporigin[2] < torigin[2] - 32)
	{
		AI_AddNode(self, droporigin, (NODEFLAGS_LADDER|NODEFLAGS_FLOAT));
		droporigin[2] += step;
	}
}

//==========================================
// AI_CheckForLadder
// Check for adding ladder nodes
//==========================================
static qboolean
AI_CheckForLadder(edict_t *self)
{
	int			closest_node;

	// If there is a ladder and we are moving up, see if we should add a ladder node
	if (!self->client || self->velocity[2] < 5)
	{
		return false;
	}

	if (!AI_IsLadder(self->s.origin, self->client->ps.viewangles, self->mins, self->maxs, self))
	{
		return false;
	}

	// If there is already a ladder node in here we've already done this ladder
	closest_node = AI_FindClosestReachableNode( self->s.origin, self, NODE_DENSITY, NODEFLAGS_LADDER );
	if (closest_node != INVALID)
	{
		return false;
	}

	//proceed:
	AI_DropLadderNodes(self);
	return true;
}

//==========================================
// AI_TouchWater
// Capture when players touches water for mapping purposes.
//==========================================
static void
AI_WaterJumpNode(edict_t *self)
{
	int			closest_node;
	vec3_t		waterorigin;
	trace_t		trace;
	edict_t		ent;

	//don't drop if player is riding elevator or climbing a ladder
	if (self->groundentity && self->groundentity != world)
	{
		if (self->groundentity->classname)
		{
			if (!strcmp( self->groundentity->classname, "func_plat")
				|| !strcmp(self->groundentity->classname, "trigger_push")
				|| !strcmp(self->groundentity->classname, "func_train")
				|| !strcmp(self->groundentity->classname, "func_rotate")
				|| !strcmp(self->groundentity->classname, "func_bob")
				|| !strcmp(self->groundentity->classname, "func_door"))
			{
				return;
			}
		}
	}
	if (AI_IsLadder(self->s.origin, self->client->ps.viewangles,
		self->mins, self->maxs, self))
	{
		return;
	}

	VectorCopy( self->s.origin, waterorigin );

	//move the origin to water limit
	if (gi.pointcontents(waterorigin) & MASK_WATER)
	{
		//reverse
		trace = gi.trace( waterorigin,
			vec3_origin,
			vec3_origin,
			tv( waterorigin[0], waterorigin[1], waterorigin[2] + NODE_DENSITY*2 ),
			self,
			MASK_ALL );

		VectorCopy( trace.endpos, waterorigin );
		if (gi.pointcontents(waterorigin) & MASK_WATER)
			return;
	}

	//find water limit
	trace = gi.trace( waterorigin,
		vec3_origin,
		vec3_origin,
		tv( waterorigin[0], waterorigin[1], waterorigin[2] - NODE_DENSITY*2 ),
		self,
		MASK_WATER );

	if (trace.fraction == 1.0)
		return;
	else
		VectorCopy( trace.endpos, waterorigin );

	//tmp test (should just move 1 downwards)
	if (!(gi.pointcontents(waterorigin) & MASK_WATER))
	{
		while (!(gi.pointcontents(waterorigin) & MASK_WATER))
		{
			waterorigin[2]--;
		}
	}

	ent = *self;
	VectorCopy( waterorigin, ent.s.origin);

	// Look for the closest node of type water
	closest_node = AI_FindClosestReachableNode( ent.s.origin, &ent, 32, NODEFLAGS_WATER);
	if (closest_node == INVALID) // we need to drop a node
	{
		closest_node = AI_AddNode(self, waterorigin, (NODEFLAGS_WATER|NODEFLAGS_FLOAT));

		// Add an edge
		AI_UpdateNodeEdge( self->last_node, closest_node);
		self->last_node = closest_node;

	} else {

		AI_UpdateNodeEdge(self->last_node, closest_node);
		self->last_node = closest_node; // zero out so other nodes will not be linked
	}
}

//==========================================
// AI_PathMap
// This routine is called to hook in the pathing code and sets
// the current node if valid.
//==========================================
#define NODE_UPDATE_DELAY	0.10;

static void
AI_PathMap(edict_t *ent)
{
	int closest_node;

	//DROP WATER JUMP NODE (not limited by delayed updates)
	if (!ent->is_swim && ent->last_node != INVALID
		&& ent->is_swim != ent->was_swim)
	{
		AI_WaterJumpNode(ent);
		ent->last_update = level.time + NODE_UPDATE_DELAY; // slow down updates a bit
		return;
	}

	if (level.time < ent->last_update)
	{
		return;
	}

	ent->last_update = level.time + NODE_UPDATE_DELAY; // slow down updates a bit

	//don't drop nodes when riding movers
	if (ent->groundentity && ent->groundentity != world)
	{
		if (ent->groundentity->classname)
		{
			if (!strcmp( ent->groundentity->classname, "func_plat")
				|| !strcmp(ent->groundentity->classname, "trigger_push")
				|| !strcmp(ent->groundentity->classname, "func_train")
				|| !strcmp(ent->groundentity->classname, "func_rotate")
				|| !strcmp(ent->groundentity->classname, "func_bob")
				|| !strcmp(ent->groundentity->classname, "func_door") )
				return;
		}
	}

	// Special check for ladder nodes
	if (AI_CheckForLadder(ent))
	{
		return;
	}

	// Not on ground, and not in the water, so bail (deeper check by using a splitmodels function)
	if (!ent->is_step)
	{
		if (!ent->is_swim)
		{
			ent->was_falling = true;
			return;
		}
		else if (ent->is_swim)
		{
			ent->was_falling = false;
		}
	}

	//player just touched the ground
	if (ent->was_falling == true)
	{
		if (!ent->groundentity) //not until it REALLY touches ground
		{
			return;
		}

		//normal nodes

		//check for duplicates (prevent adding too many)
		closest_node = AI_FindClosestReachableNode(ent->s.origin, ent, 64, NODE_ALL);

		//otherwise, add a new node
		if (closest_node == INVALID)
		{
			closest_node = AI_AddNode(ent, ent->s.origin, 0); //no flags = normal movement node
		}

		// Now add link
		if (ent->last_node != INVALID && closest_node != INVALID)
		{
			AI_UpdateNodeEdge(ent->last_node, closest_node);
		}

		if (closest_node != INVALID)
		{
			ent->last_node = closest_node; // set visited to last
		}

		ent->was_falling = false;
		return;
	}

	//jal: I'm not sure of not having nodes in lava/slime
	//being actually a good idea. When the bots incidentally
	//fall inside it they don't know how to get out
	// Lava/Slime

	// Iterate through all nodes to make sure far enough apart
	closest_node = AI_FindClosestReachableNode(ent->s.origin, ent, NODE_DENSITY, NODE_ALL);

	// Add Nodes as needed
	if (closest_node == INVALID )
	{
		// Add nodes in the water as needed
		if (ent->is_swim)
		{
			closest_node = AI_AddNode(ent, ent->s.origin,
				(NODEFLAGS_WATER | NODEFLAGS_FLOAT));
		}
		else
		{
			closest_node = AI_AddNode(ent, ent->s.origin, 0);
		}

		// Now add link
		if (ent->last_node != INVALID)
		{
			AI_UpdateNodeEdge(ent->last_node, closest_node);
		}
	}
	else if (closest_node != ent->last_node && ent->last_node != INVALID)
	{
		AI_UpdateNodeEdge(ent->last_node, closest_node);
	}

	if (closest_node != INVALID)
	{
		ent->last_node = closest_node; // set visited to last
	}
}

//==========================================
// AI_ClientPathMap
// Clients try to create new nodes while walking the map
//==========================================
void
AITools_DropNodes(edict_t *ent)
{
	if (nav.loaded)
	{
		return;
	}

	AI_CategorizePosition(ent);
	AI_PathMap(ent);
}

//==========================================
//
//==========================================
static void
AITools_EraseNodes(void)
{
	//Init nodes arrays
	nav.num_nodes = 0;
	memset(nodes, 0, sizeof(nav_node_t) * MAX_NODES);
	memset(pLinks, 0, sizeof(nav_plink_t) * MAX_NODES);

	nav.num_ents = 0;
	memset(nav.ents, 0, sizeof(nav_ents_t) * MAX_EDICTS);

	nav.num_items = 0;
	memset(nav.items, 0, sizeof(nav_item_t) * MAX_EDICTS);

	nav.loaded = false;
}

void AITools_InitEditnodes( void )
{
	if (nav.loaded) {
		AITools_EraseNodes();
		AI_LoadPLKFile(level.mapname);
		//delete everything but nodes
		memset(pLinks, 0, sizeof(nav_plink_t) * MAX_NODES);

		nav.num_ents = 0;
		memset(nav.ents, 0, sizeof(nav_ents_t) * MAX_EDICTS);

		nav.num_items = 0;
		memset(nav.items, 0, sizeof(nav_item_t) * MAX_EDICTS);
		nav.loaded = false;
	}

	Com_Printf("EDITNODES: on\n");
}

void
AITools_InitMakenodes(void)
{
	if (nav.loaded)
	{
		AITools_EraseNodes();
	}

	Com_Printf("EDITNODES: on\n");
}

//-------------------------------------------------------------

//==========================================
// AI_SavePLKFile
// save nodes and plinks to file.
// Only navigation nodes are saved. Item nodes aren't
//==========================================
static qboolean
AI_SavePLKFile(const char *mapname)
{
	FILE		*pOut;
	char		filename[MAX_OSPATH];
	int			i;
	int			version = NAV_FILE_VERSION;

	Com_sprintf(filename, sizeof(filename), "%s/%s/%s.%s",
		gi.Gamedir(), AI_NODES_FOLDER, mapname, NAV_FILE_EXTENSION);
	gi.CreatePath(filename);
	pOut = Q_fopen(filename, "wb");
	if (!pOut)
	{
		Com_Printf("Failed to store: %s\n", filename);
		return false;
	}

	fwrite(&version, sizeof(int), 1, pOut);
	fwrite(&nav.num_nodes, sizeof(int), 1, pOut);

	// write out nodes
	for(i=0; i<nav.num_nodes;i++)
	{
		fwrite(&nodes[i], sizeof(nav_node_t), 1, pOut);
	}

	// write out plinks array
	for(i=0; i<nav.num_nodes;i++)
	{
		fwrite(&pLinks[i], sizeof(nav_plink_t), 1, pOut);
	}

	fclose(pOut);

	return true;
}

//===========================================================
//
//				EDITOR
//
//===========================================================

//==================
// AITools_SaveNodes
//==================
void AITools_SaveNodes( void )
{
	int newlinks;
	int	jumplinks;

	if (!nav.num_nodes)
	{
		Com_Printf("CGame AITools: No nodes to save\n");
		return;
	}

	//find links
	newlinks = AI_LinkCloseNodes();
	Com_Printf ("Added %i new links\n", newlinks);

	//find jump links
	jumplinks = AI_LinkCloseNodes_JumpPass(0);
	Com_Printf ("Added %i new jump links\n", jumplinks);

	if (!AI_SavePLKFile(level.mapname))
	{
		Com_Printf ("Failed: Couldn't create the nodes file\n");
	}
	else
	{
		Com_Printf ("Nodes files saved\n");
	}

	//restart navigation
	AITools_EraseNodes();
	AI_InitNavigationData();
}
