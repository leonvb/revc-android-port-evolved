#ifndef REVC_GPS_ROUTE_H
#define REVC_GPS_ROUTE_H

#ifdef MODERN_FEATURES
class CGpsRoute {
public:
	static void Reset();
	static void Update();
	static void Draw(bool menuMap);
	static void DrawDistance();
};
#endif
#endif
