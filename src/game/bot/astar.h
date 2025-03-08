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

//==========================================
//
//
//==========================================

typedef struct astarpath_s
{
	int numNodes;
	int nodes[MAX_NODES];
	int originNode;
	int goalNode;

} astarpath_t;

//	A* PROPS
//===========================================
int	AStar_nodeIsInClosed( int node );
int	AStar_nodeIsInOpen( int node );
int	AStar_nodeIsInPath( int node );
int	AStar_ResolvePath ( int origin, int goal, int movetypes );
//===========================================
int AStar_GetPath( int origin, int goal, int movetypes, struct astarpath_s *path );
