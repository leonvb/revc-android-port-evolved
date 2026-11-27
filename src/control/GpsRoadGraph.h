#ifndef REVC_GPS_ROAD_GRAPH_H
#define REVC_GPS_ROAD_GRAPH_H

#include "GpsPathSearch.h"

// Adapts CPathFind's car graph without taking ownership or modifying it. Kept
// generic so the same adapter can be tested with small synthetic road graphs.
template <typename Paths>
class GpsRoadGraph {
	const Paths &m_paths;
public:
	explicit GpsRoadGraph(const Paths &paths) : m_paths(paths) {}
	unsigned Count() const { return m_paths.m_numCarPathNodes > 0 ? unsigned(m_paths.m_numCarPathNodes) : 0; }
	GpsPathSearch::Position Position(unsigned node) const {
		const auto &n = m_paths.m_pathNodes[node];
		return { n.x / 8.0f, n.y / 8.0f, n.z / 8.0f };
	}
	bool Enabled(unsigned node) const {
		const auto &n = m_paths.m_pathNodes[node];
		return !n.bDisabled && !n.bWaterPath && n.numLinks > 0;
	}
	int Group(unsigned node) const { return m_paths.m_pathNodes[node].group; }
	unsigned LinkCount(unsigned node) const { return m_paths.m_pathNodes[node].numLinks; }
	bool Link(unsigned node, unsigned offset, unsigned *next, bool *againstOneWay) const {
		const auto &n = m_paths.m_pathNodes[node];
		if (offset >= n.numLinks) return false;
		int connection = n.firstLink + int(offset);
		if (connection < 0 || connection >= m_paths.m_numConnections) return false;
		unsigned other = m_paths.m_connections[connection] & 0x3FFF;
		int road = m_paths.m_carPathConnections[connection];
		if (other >= Count() || road < 0 || road >= m_paths.m_numCarPathLinks) return false;
		const auto &link = m_paths.m_carPathLinks[road];
		*next = other;
		// Matches CCarCtrl::PickNextNodeAccordingStrategy's direction check.
		*againstOneWay = link.pathNodeIndex == int(node) ? link.numRightLanes == 0 : link.numLeftLanes == 0;
		return true;
	}

	// Height participates in the distance so a flyover and the road underneath
	// do not collapse into the same start/end node.
	int FindNearest(const GpsPathSearch::Position &pos, int group = -1000) const {
		int nearest = -1;
		float best = 1.0e30f;
		for (unsigned i = 0; i < Count(); ++i) {
			if (!Enabled(i) || (group != -1000 && Group(i) != group)) continue;
			GpsPathSearch::Position p = Position(i);
			float x = p.x - pos.x, y = p.y - pos.y, z = p.z - pos.z;
			// Weight vertical separation like the engine's closest-road lookup.
			float d = x*x + y*y + 9.0f*z*z;
			if (d < best) { best = d; nearest = int(i); }
		}
		return nearest;
	}
};
#endif
