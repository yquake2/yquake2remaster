
#include "g_local.h"
#include "ai_local.h"

//==========================================
// 
// 
//==========================================

static int	alist[MAX_NODES];	//list contains all studied nodes, Open and Closed together
static int	alist_numNodes;

enum {
	NOLIST,
	OPENLIST,
	CLOSEDLIST
};

typedef struct
{
	int		parent;
	int		G;
	int		H;

	int		list;

} astarnode_t;

astarnode_t	astarnodes[MAX_NODES];

static int Apath[MAX_NODES];
static int Apath_numNodes;
//==========================================
// 
// 
//==========================================
static int originNode;
static int goalNode;
static int currentNode;

int ValidLinksMask;
#define DEFAULT_MOVETYPES_MASK (LINK_MOVE|LINK_STAIRS|LINK_FALL|LINK_WATER|LINK_WATERJUMP|LINK_JUMPPAD|LINK_PLATFORM|LINK_TELEPORT);
//==========================================
// 
// 
// 
//==========================================

int	AStar_nodeIsInPath( int node )
{
	int	i;

	if( !Apath_numNodes )
		return 0;

	for (i=0; i<Apath_numNodes; i++) 
	{
		if(node == Apath[i])
			return 1;
	}

	return 0;
}

int	AStar_nodeIsInClosed( int node )
{
	if( astarnodes[node].list == CLOSEDLIST )
		return 1;

	return 0;
}

int	AStar_nodeIsInOpen( int node )
{
	if( astarnodes[node].list == OPENLIST )
		return 1;

	return 0;
}

static void AStar_InitLists (void)
{
	int i;

	for ( i=0; i<MAX_NODES; i++ )
	{
		Apath[i] = 0;

		astarnodes[i].G = 0;
		astarnodes[i].H = 0;
		astarnodes[i].parent = 0;
		astarnodes[i].list = NOLIST;
	}
	Apath_numNodes = 0;

	alist_numNodes = 0;
	for( i=0; i<MAX_NODES; i++ )
		alist[i] = -1;
}

static int AStar_PLinkDistance( n1, n2 )
{
	int	i;
	int	found = 0;
	int dist;

	for ( i=0; i<pLinks[n1].numLinks; i++)
	{
		if( pLinks[n1].nodes[i] == n2 ) {
			found = 1;
			dist = (int)pLinks[n1].dist[i];
		}
	}

	if(!found)
		return -1;

	return dist;
}

static int	Astar_HDist_ManhatanGuess( int node )
{
	vec3_t	DistVec;
	int		i;
	int		HDist;

	//teleporters are exceptional
	if( nodes[node].flags & NODEFLAGS_TELEPORTER_IN )
		node++; //it's tele out is stored in the next node in the array

	for (i=0 ; i<3 ; i++)
	{
		DistVec[i] = nodes[goalNode].origin[i] - nodes[node].origin[i];
		if( DistVec[i] < 0.0f )
			DistVec[i] = -DistVec[i];	//use only positive values. We don't care about direction.
	}

	HDist = (int)(DistVec[0] + DistVec[1] + DistVec[2]);
	return HDist;
}

static void AStar_PutInClosed( int node )
{
	if( !astarnodes[node].list ) {
		alist[alist_numNodes] = node;
		alist_numNodes++;
	}

	astarnodes[node].list = CLOSEDLIST;
}

static void AStar_PutAdjacentsInOpen( node )
{
	int	i;

	for ( i=0; i<pLinks[node].numLinks; i++)
	{
		int	addnode;

		//ignore invalid links
		if( !(ValidLinksMask & pLinks[node].moveType[i]) )
			continue;

		addnode = pLinks[node].nodes[i];

		//ignore self
		if( addnode == node )
			continue;

		//ignore if it's already in closed list
		if( AStar_nodeIsInClosed( addnode ) )
			continue;

		//if it's already inside open list
		if( AStar_nodeIsInOpen( addnode ) )
		{
			int plinkDist;
			
			plinkDist = AStar_PLinkDistance( node, addnode );
			if( plinkDist == -1)
				printf("WARNING: AStar_PutAdjacentsInOpen - Couldn't find distance between nodes\n");
			
			//compare G distances and choose best parent
			else if( astarnodes[addnode].G > (astarnodes[node].G + plinkDist) )
			{
				astarnodes[addnode].parent = node;
				astarnodes[addnode].G = astarnodes[node].G + plinkDist;
			}
			
		} else {	//just put it in

			int plinkDist;

			plinkDist = AStar_PLinkDistance( node, addnode );
			if( plinkDist == -1)
			{
				plinkDist = AStar_PLinkDistance( addnode, node );
				if( plinkDist == -1) 
					plinkDist = 999;//jalFIXME

				//ERROR
				printf("WARNING: AStar_PutAdjacentsInOpen - Couldn't find distance between nodes\n");
			}

			//put in global list
			if( !astarnodes[addnode].list ) {
				alist[alist_numNodes] = addnode;
				alist_numNodes++;
			}

			astarnodes[addnode].parent = node;
			astarnodes[addnode].G = astarnodes[node].G + plinkDist;
			astarnodes[addnode].H = Astar_HDist_ManhatanGuess( addnode );
			astarnodes[addnode].list = OPENLIST;
		}
	}
}

static int AStar_FindInOpen_BestF ( void )
{
	int	i;
	int	bestF = -1;
	int best = -1;

	for ( i=0; i<alist_numNodes; i++ )
	{
		int node = alist[i];

		if( astarnodes[node].list != OPENLIST )
			continue;

		if ( bestF == -1 || bestF > (astarnodes[node].G + astarnodes[node].H) ) {
			bestF = astarnodes[node].G + astarnodes[node].H;
			best = node;
		}
	}
	//printf("BEST:%i\n", best);
	return best;
}

static void AStar_ListsToPath ( void )
{
	int count = 0;
	int cur = goalNode;

	while ( cur != originNode ) 
	{
		cur = astarnodes[cur].parent;
		count++;
	}
	cur = goalNode;
	
	while ( count >= 0 ) 
	{
		Apath[count] = cur;
		Apath_numNodes++;
		count--;
		cur = astarnodes[cur].parent;
	}
}

static int	AStar_FillLists ( void )
{
	//put current node inside closed list
	AStar_PutInClosed( currentNode );
	
	//put adjacent nodes inside open list
	AStar_PutAdjacentsInOpen( currentNode );
	
	//find best adjacent and make it our current
	currentNode = AStar_FindInOpen_BestF();

	return (currentNode != -1);	//if -1 path is bloqued
}

int	AStar_ResolvePath ( int n1, int n2, int movetypes )
{
	ValidLinksMask = movetypes;
	if ( !ValidLinksMask )
		ValidLinksMask = DEFAULT_MOVETYPES_MASK;

	AStar_InitLists();

	originNode = n1;
	goalNode = n2;
	currentNode = originNode;

	while ( !AStar_nodeIsInOpen(goalNode) )
	{
		if( !AStar_FillLists() )
			return 0;	//failed
	}

	AStar_ListsToPath();

	printf("RESULT:\n Origin:%i\n Goal:%i\n numNodes:%i\n FirstInPath:%i\n LastInPath:%i\n", originNode, goalNode, Apath_numNodes, Apath[0], Apath[Apath_numNodes-1]);

	return 1;
}

int AStar_GetPath( int origin, int goal, int movetypes, struct astarpath_s *path )
{
	int i;

	if( !AStar_ResolvePath ( origin, goal, movetypes ) )
		return 0;

	path->numNodes = Apath_numNodes;
	path->originNode = origin;
	path->goalNode = goal;
	for(i=0; i<path->numNodes; i++)
		path->nodes[i] = Apath[i];

	return 1;
}

