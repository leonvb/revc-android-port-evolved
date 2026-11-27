#include "../src/control/GpsPathSearch.h"
#include "../src/control/GpsRoadGraph.h"
#include "../src/control/GpsGeometry.h"
#include <cassert>
#include <vector>
#include <cstdlib>
#include <algorithm>

struct Edge { unsigned to; bool against; };
struct Node {
	GpsPathSearch::Position pos;
	bool enabled;
	int group;
	std::vector<Edge> edges;
};
struct Graph {
	std::vector<Node> nodes;
	unsigned Count() const { return nodes.size(); }
	GpsPathSearch::Position Position(unsigned i) const { return nodes[i].pos; }
	bool Enabled(unsigned i) const { return nodes[i].enabled; }
	int Group(unsigned i) const { return nodes[i].group; }
	unsigned LinkCount(unsigned i) const { return nodes[i].edges.size(); }
	bool Link(unsigned i, unsigned j, unsigned *next, bool *against) const {
		*next = nodes[i].edges[j].to; *against = nodes[i].edges[j].against; return true;
	}
};

struct RoadNode {
	int16_t x, y, z, firstLink;
	int8_t group;
	unsigned numLinks;
	bool bDisabled, bWaterPath;
};
struct RoadLink { int pathNodeIndex, numLeftLanes, numRightLanes; };
struct Roads {
	int m_numCarPathNodes, m_numConnections, m_numCarPathLinks;
	RoadNode m_pathNodes[4];
	uint16_t m_connections[4];
	int16_t m_carPathConnections[4];
	RoadLink m_carPathLinks[2];
};

static void TestRoadAdapter()
{
	Roads paths = { 3, 2, 1,
		{{0,0,0,0,1,1,false,false}, {80,0,0,1,1,1,false,false},
		 {0,0,160,0,1,1,false,false}},
		{1, 0}, {0, 0}, {{0, 1, 0}} };
	GpsRoadGraph<Roads> graph(paths);
	unsigned next; bool against;
	assert(graph.Link(0, 0, &next, &against) && next == 1 && against);
	assert(graph.Link(1, 0, &next, &against) && next == 0 && !against);
	assert(graph.FindNearest({0, 0, 19}) == 2); // flyover, not road below
	paths.m_pathNodes[2].bDisabled = true;
	assert(graph.FindNearest({0, 0, 19}) == 0);
	paths.m_pathNodes[1].group = -2;
	assert(graph.FindNearest({0, 0, 0}, -2) == 1);
	paths.m_pathNodes[0].bWaterPath = true;
	assert(!graph.Enabled(0));
	paths.m_carPathConnections[1] = -1;
	assert(!graph.Link(1, 0, &next, &against));
}

// Compare against a simple Dijkstra implementation over directed, weighted
// synthetic roads. Includes cycles, disconnected nodes and disabled bridges.
static void TestAgainstDijkstra()
{
	std::srand(42);
	for (int trial = 0; trial < 200; ++trial) {
		Graph graph;
		for (int i = 0; i < 24; ++i)
			graph.nodes.push_back({{float(std::rand()%150), float(std::rand()%150), float(std::rand()%20)},
			                       (std::rand()%8) != 0, 0, {}});
		graph.nodes[0].enabled = graph.nodes[23].enabled = true;
		for (unsigned a = 0; a < 24; ++a)
			for (unsigned b = 0; b < 24; ++b)
				if (a != b && std::rand()%12 == 0)
					graph.nodes[a].edges.push_back({b, std::rand()%4 == 0});
		unsigned dist[24]; bool done[24] = {};
		std::fill(dist, dist + 24, 65535u); dist[0] = 0;
		for (unsigned iter = 0; iter < 24; ++iter) {
			unsigned current = 24;
			for (unsigned i = 0; i < 24; ++i)
				if (!done[i] && dist[i] < 65535 && (current == 24 || dist[i] < dist[current])) current = i;
			if (current == 24) break;
			done[current] = true;
			for (const Edge &e : graph.nodes[current].edges) {
				if (!graph.nodes[e.to].enabled) continue;
				unsigned weight = unsigned(ceilf(GpsPathSearch::Distance(graph.Position(current), graph.Position(e.to)) * (e.against ? 5.0f : 1.0f)));
				dist[e.to] = std::min(dist[e.to], dist[current] + weight);
			}
		}
		GpsPathSearch::Workspace<24> search;
		uint16_t path[24];
		unsigned n = search.Find(graph, 0, 23, path, 24);
		if (dist[23] == 65535) { assert(n == 0); continue; }
		assert(n >= 2 && path[0] == 0 && path[n-1] == 23);
		unsigned cost = 0;
		for (unsigned i = 1; i < n; ++i) {
			bool found = false;
			for (const Edge &e : graph.nodes[path[i-1]].edges)
				if (e.to == path[i]) {
					cost += unsigned(ceilf(GpsPathSearch::Distance(graph.Position(path[i-1]), graph.Position(path[i])) * (e.against ? 5.0f : 1.0f)));
					found = true; break;
				}
			assert(found);
		}
		assert(cost == dist[23]);
	}
}

struct Point { float x, y; };
static void TestClipping()
{
	Point a = {-2, 0}, b = {2, 0};
	assert(GpsGeometry::ClipCircle(a, b, 0.94f));
	assert(fabsf(a.x + 0.94f) < 0.0001f && fabsf(b.x - 0.94f) < 0.0001f);
	a = {0, 0}; b = {0.5f, 0};
	assert(GpsGeometry::ClipCircle(a, b, 1));
	assert(a.x == 0 && b.x == 0.5f);
	a = {2, 2}; b = {3, 3};
	assert(!GpsGeometry::ClipCircle(a, b, 1));
	a = {-2, 1}; b = {2, 1};
	assert(!GpsGeometry::ClipCircle(a, b, 1)); // tangent has no visible length
	a = {0, 0}; b = {0, 0};
	assert(!GpsGeometry::ClipCircle(a, b, 1));
}

int main()
{
	TestClipping();
	TestRoadAdapter();
	TestAgainstDijkstra();
	static_assert(sizeof(GpsPathSearch::Workspace<9650>) < 40000, "GPS scratch budget");
	Graph graph;
	graph.nodes = {
		{{0,0,0}, true, 1, {{1,false}, {2,false}}},
		{{10,0,0}, true, 1, {{3,true}}},
		{{10,10,0}, true, 1, {{3,false}}},
		{{20,0,0}, true, 1, {}},
		{{10,0,20}, true, 2, {}}
	};
	GpsPathSearch::Workspace<16> search;
	uint16_t path[16];
	unsigned n = search.Find(graph, 0, 3, path, 16);
	assert(n == 3 && path[0] == 0 && path[1] == 2 && path[2] == 3);
	graph.nodes[2].enabled = false;
	n = search.Find(graph, 0, 3, path, 16);
	assert(n == 3 && path[1] == 1); // wrong-way is possible, but penalised
	graph.nodes[1].enabled = false;
	assert(search.Find(graph, 0, 3, path, 16) == 0);
	assert(search.Find(graph, 0, 4, path, 16) == 0); // disconnected group
	assert(search.Find(graph, 0, 0, path, 16) == 1 && path[0] == 0);
	graph.nodes[1].enabled = true;
	path[0] = 777;
	assert(search.Find(graph, 0, 3, path, 2) == 0 && path[0] == 777);
	assert(search.Find(graph, 0, 100, path, 16) == 0);
	graph.nodes[1].pos.x = 70000;
	assert(search.Find(graph, 0, 3, path, 16) == 0); // no cost wraparound
}
