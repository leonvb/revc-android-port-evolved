#include "common.h"
#include "GpsRouteSearch.h"

#ifdef MODERN_FEATURES
#include "PathFind.h"
#include "GpsRoadGraph.h"

namespace {
GpsPathSearch::Workspace<NUM_PATHNODES> search;
static_assert(sizeof(search) < 40000, "GPS search workspace exceeds 40 KB");
}

int
CGpsRouteSearch::FindNearest(const CVector &position, int group)
{
	GpsRoadGraph<CPathFind> graph(ThePaths);
	return graph.FindNearest({position.x, position.y, position.z}, group);
}

unsigned
CGpsRouteSearch::Find(unsigned start, unsigned goal, uint16 *nodes, unsigned capacity)
{
	GpsRoadGraph<CPathFind> graph(ThePaths);
	return search.Find(graph, start, goal, nodes, capacity);
}
#endif
