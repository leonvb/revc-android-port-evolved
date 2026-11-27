#ifndef REVC_GPS_PATH_SEARCH_H
#define REVC_GPS_PATH_SEARCH_H

#include <stdint.h>
#include <math.h>
#include <string.h>

// A self-contained A* workspace. It deliberately never writes to the game's
// path nodes: traffic AI uses those nodes' own distance and search lists.
// Graph must provide Count(), Position(node), Enabled(node), Group(node),
// LinkCount(node), and Link(node, offset, &next, &againstOneWay).
namespace GpsPathSearch {
struct Position {
	float x, y, z;
};

inline float Distance(const Position &a, const Position &b)
{
	float x = a.x - b.x, y = a.y - b.y, z = a.z - b.z;
	return sqrtf(x*x + y*y + z*z);
}

// The cost is stored in whole metres; rounding each edge up keeps the
// straight-line heuristic admissible. Costs of 65535 m or more cannot be
// represented and are rejected rather than wrapping. Parent indices use the
// upper bit for the closed flag, so the graph must contain < 32767 nodes.
template <unsigned Capacity>
class Workspace {
	static_assert(Capacity < 32767, "GPS parent index does not fit");
	static const uint16_t UNSEEN = 0xFFFF;
	static const uint16_t CLOSED = 0x8000;
	uint16_t m_cost[Capacity];
	uint16_t m_parent[Capacity];
	uint8_t m_open[(Capacity + 7) / 8];

	void SetOpen(unsigned i) { m_open[i / 8] |= uint8_t(1u << (i % 8)); }
	void ClearOpen(unsigned i) { m_open[i / 8] &= uint8_t(~(1u << (i % 8))); }
public:
	// Returns the number of nodes written, in start-to-goal order, or zero
	// if no path exists or output is too small. Output is never truncated.
	template <typename Graph>
	unsigned Find(const Graph &graph, unsigned start, unsigned goal, uint16_t *out, unsigned outCapacity)
	{
		const unsigned count = graph.Count();
		if (!out || count > Capacity || start >= count || goal >= count ||
		    !graph.Enabled(start) || !graph.Enabled(goal) || graph.Group(start) != graph.Group(goal))
			return 0;
		memset(m_parent, 0xFF, sizeof(m_parent));
		memset(m_open, 0, sizeof(m_open));
		m_cost[start] = 0;
		m_parent[start] = uint16_t(start);
		SetOpen(start);
		const Position target = graph.Position(goal);

		for (;;) {
			unsigned best = count;
			float bestScore = 1.0e30f;
			// Only examine non-empty frontier bytes; no heap or per-node AI
			// metadata is needed, keeping all scratch below 40 KiB at 9650 nodes.
			for (unsigned byte = 0; byte < (count + 7) / 8; ++byte) {
				uint8_t bits = m_open[byte];
				if (!bits) continue;
				for (unsigned bit = 0; bit < 8; ++bit) {
					if (!(bits & (1u << bit))) continue;
					unsigned node = byte * 8 + bit;
					float score = m_cost[node] + Distance(graph.Position(node), target);
					if (score < bestScore) { best = node; bestScore = score; }
				}
			}
			if (best == count) return 0;
			if (best == goal) {
				unsigned length = 1, node = goal;
				while (node != start) {
					node = m_parent[node] & ~CLOSED;
					if (++length > count) return 0;
				}
				if (length > outCapacity) return 0;
				node = goal;
				for (unsigned i = length; i > 0; --i) {
					out[i - 1] = uint16_t(node);
					if (node != start) node = m_parent[node] & ~CLOSED;
				}
				return length;
			}
			ClearOpen(best);
			m_parent[best] |= CLOSED;
			for (unsigned i = 0; i < graph.LinkCount(best); ++i) {
				unsigned next;
				bool againstOneWay;
				if (!graph.Link(best, i, &next, &againstOneWay) || next >= count ||
				    !graph.Enabled(next) || graph.Group(next) != graph.Group(start) ||
				    (m_parent[next] != UNSEEN && (m_parent[next] & CLOSED))) continue;
				float distance = Distance(graph.Position(best), graph.Position(next));
				float weight = ceilf(distance * (againstOneWay ? 5.0f : 1.0f));
				// Comparisons also reject NaN and infinity before integer conversion.
				if (!(weight >= 0.0f && weight < 65535.0f)) continue;
				unsigned cost = unsigned(m_cost[best]) + unsigned(weight);
				if (cost >= 65535) continue;
				if (m_parent[next] == UNSEEN || cost < m_cost[next]) {
					m_cost[next] = uint16_t(cost);
					m_parent[next] = uint16_t(best);
					SetOpen(next);
				}
			}
		}
	}
};
}
#endif
