#ifndef REVC_GPS_ROUTE_SEARCH_H
#define REVC_GPS_ROUTE_SEARCH_H

#ifdef MODERN_FEATURES
// Road-graph search only. The caller owns the output buffer; a zero result
// means that there is no route or that the buffer is too small.
class CGpsRouteSearch {
public:
	static int FindNearest(const CVector &position, int group = -1000);
	static unsigned Find(unsigned start, unsigned goal, uint16 *nodes, unsigned capacity);
};
#endif
#endif
