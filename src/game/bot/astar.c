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

#include <limits.h>
#include "../header/local.h"
#include "ai_local.h"

//==========================================
//
//
//==========================================

static int alist[MAX_NODES];	//list contains all studied nodes, Open and Closed together
static int alist_nodesnum;

typedef enum {
	NOLIST,
	OPENLIST,
	CLOSEDLIST
} astarnodelist_e;

typedef struct
{
	int parent;
	int g;
	int h;

	astarnodelist_e list;
} astarnode_t;

static astarnode_t	astar_nodes[MAX_NODES];

//==========================================
//
//
//==========================================
static int originNode;
static int goalNode;
static int currentNode;

static int ValidLinksMask;
#define DEFAULT_MOVETYPES_MASK ( \
	LINK_MOVE | \
	LINK_STAIRS | \
	LINK_FALL | \
	LINK_WATER | \
	LINK_WATERJUMP | \
	LINK_JUMPPAD | \
	LINK_PLATFORM | \
	LINK_TELEPORT)
//==========================================
//
//
//
//==========================================

/*
 * Check if a node is within valid bounds.
 */
static inline qboolean
AStar_IsValidNode(int node)
{
	return (node >= 0 && node < MAX_NODES);
}

/*
 * Check if a node is in the Closed list.
 */
static inline qboolean
AStar_nodeIsInClosed(int node)
{
	return (AStar_IsValidNode(node) && astar_nodes[node].list == CLOSEDLIST);
}

/*
 * Check if a node is in the Open list.
 */
static inline qboolean
AStar_nodeIsInOpen(int node)
{
	return (AStar_IsValidNode(node) && astar_nodes[node].list == OPENLIST);
}

static void
AStar_InitLists(void)
{
	size_t i;

	for (i = 0; i < MAX_NODES; i++)
	{
		astar_nodes[i].g = 0;
		astar_nodes[i].h = 0;
		astar_nodes[i].parent = 0;
		astar_nodes[i].list = NOLIST;
	}

	alist_nodesnum = 0;
	memset(alist, -1, sizeof(alist));
}

static int
AStar_PlinkDistance(int n1, int n2)
{
	size_t i;

	if (!AStar_IsValidNode(n1) || !AStar_IsValidNode(n2))
	{
		if (bot_debugmonster->value)
		{
			Com_Printf("WARNING: %s - incorrect distance nodes %d, %d\n",
				__func__, n1, n2);
		}

		return -1;
	}

	for (i = 0; i < pLinks[n1].numLinks; i++)
	{
		if (pLinks[n1].nodes[i] == n2)
		{
			return (int)pLinks[n1].dist[i];
		}
	}

	return -1;
}

static int
Astar_HDist_ManhatanGuess(int node)
{
	vec3_t DistVec;
	size_t i;

	if (!AStar_IsValidNode(node))
	{
		if (bot_debugmonster->value)
		{
			Com_Printf("WARNING: %s - incorrect %d\n",
				__func__, node);
		}

		return -1;
	}

	//teleporters are exceptional
	if (nodes[node].flags & NODEFLAGS_TELEPORTER_IN)
	{
		node++; //it's tele out is stored in the next node in the array
	}

	for (i = 0 ; i < 3 ; i++)
	{
		DistVec[i] = fabs(nodes[goalNode].origin[i] - nodes[node].origin[i]);
	}

	return (DistVec[0] + DistVec[1] + DistVec[2]);
}

static void
AStar_PutInClosed(int node)
{
	if (!AStar_IsValidNode(node))
	{
		if (bot_debugmonster->value)
		{
			Com_Printf("WARNING: %s - incorrect %d\n",
				__func__, node);
		}

		return;
	}

	if (!astar_nodes[node].list)
	{
		alist[alist_nodesnum] = node;
		alist_nodesnum++;
	}

	astar_nodes[node].list = CLOSEDLIST;
}

static void
AStar_PutAdjacentsInOpen(int node)
{
	size_t i;

	if (!AStar_IsValidNode(node))
	{
		if (bot_debugmonster->value)
		{
			Com_Printf("WARNING: %s - incorrect %d\n",
				__func__, node);
		}

		return;
	}

	for (i = 0; i < pLinks[node].numLinks; i++)
	{
		int addnode;

		// ignore invalid links
		if (!(ValidLinksMask & pLinks[node].moveType[i]))
		{
			continue;
		}

		addnode = pLinks[node].nodes[i];

		// ignore self
		if (addnode == node)
		{
			continue;
		}

		// ignore if it's already in closed list
		if (AStar_nodeIsInClosed(addnode))
		{
			continue;
		}

		// if it's already inside open list
		if (AStar_nodeIsInOpen(addnode))
		{
			int plink_dist;

			plink_dist = AStar_PlinkDistance(node, addnode);
			if (plink_dist < 0 && bot_debugmonster->value)
			{
				Com_Printf("WARNING: %s - Couldn't find distance between nodes\n",
					__func__);
			}
			// compare G distances and choose best parent
			else if (astar_nodes[addnode].g > (astar_nodes[node].g + plink_dist))
			{
				astar_nodes[addnode].parent = node;
				astar_nodes[addnode].g = astar_nodes[node].g + plink_dist;
			}
		}
		else
		{
			// just put it in
			int plink_dist;

			plink_dist = AStar_PlinkDistance( node, addnode );
			if (plink_dist < 0)
			{
				plink_dist = AStar_PlinkDistance(addnode, node);
				if (plink_dist < 0)
				{
					plink_dist = INT_MAX;
				}

				if (bot_debugmonster->value)
				{
					Com_Printf("WARNING: %s - Couldn't find distance between nodes\n",
						__func__);
				}
			}

			// put in global list
			if (!astar_nodes[addnode].list)
			{
				alist[alist_nodesnum] = addnode;
				alist_nodesnum++;
			}

			astar_nodes[addnode].parent = node;
			astar_nodes[addnode].g = astar_nodes[node].g + plink_dist;
			astar_nodes[addnode].h = Astar_HDist_ManhatanGuess( addnode );
			astar_nodes[addnode].list = OPENLIST;
		}
	}
}

static int
AStar_FindInOpen_BestF(void)
{
	size_t i;
	int	bestF = -1;
	int best = -1;

	for (i = 0; i < alist_nodesnum; i++)
	{
		int node = alist[i];

		if (!AStar_IsValidNode(node) || astar_nodes[node].list != OPENLIST)
		{
			continue;
		}

		if (bestF < 0 || bestF > (astar_nodes[node].g + astar_nodes[node].h))
		{
			bestF = astar_nodes[node].g + astar_nodes[node].h;
			best = node;
		}
	}

	if (bot_debugmonster->value)
	{
		Com_Printf("BEST:%i\n", best);
	}

	return best;
}

static void
AStar_ListsToPath(struct astarpath_s *astar_path)
{
	int count = 0;
	int cur = goalNode;
	int *pnode;

	if (!astar_path)
	{
		return;
	}

	astar_path->numNodes = 0;
	pnode = astar_path->nodes;
	while (cur != originNode && cur >= 0)
	{
		*pnode = cur;
		pnode++;
		cur = astar_nodes[cur].parent;
		count++;
	}

	astar_path->numNodes = count - 1;
}

static qboolean
AStar_FillLists(void)
{
	// put current node inside closed list
	AStar_PutInClosed(currentNode);

	// put adjacent nodes inside open list
	AStar_PutAdjacentsInOpen(currentNode);

	// find best adjacent and make it our current
	currentNode = AStar_FindInOpen_BestF();

	return (currentNode != -1);	//if -1 path is bloqued
}

static qboolean
AStar_ResolvePath(int origin, int goal, int movetypes, struct astarpath_s *path)
{
	if (origin < 0 || goal < 0)
	{
		return false;
	}

	ValidLinksMask = movetypes;
	if (!ValidLinksMask)
	{
		ValidLinksMask = DEFAULT_MOVETYPES_MASK;
	}

	AStar_InitLists();

	currentNode = originNode = origin;
	goalNode = goal;

	while (!AStar_nodeIsInOpen(goalNode))
	{
		if(!AStar_FillLists())
		{
			return false;	//failed
		}
	}

	AStar_ListsToPath(path);

	return true;
}

qboolean
AStar_GetPath(int origin, int goal, int movetypes, struct astarpath_s *path)
{
	if (!AStar_ResolvePath(origin, goal, movetypes, path))
	{
		return false;
	}

	path->originNode = origin;
	path->goalNode = goal;
	return true;
}
