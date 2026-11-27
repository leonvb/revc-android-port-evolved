#ifndef REVC_GPS_GEOMETRY_H
#define REVC_GPS_GEOMETRY_H
#include <math.h>

namespace GpsGeometry {
// Clips a segment to a circle in radar space. Endpoints outside the same side
// are rejected, but segments crossing the disc with both endpoints outside are
// retained. The radius may be inset to accommodate a thick outline.
template <typename Point>
bool ClipCircle(Point &a, Point &b, float radius)
{
	float dx = b.x-a.x, dy = b.y-a.y;
	float A = dx*dx+dy*dy;
	if (A < 0.0000001f || radius <= 0.0f) return false;
	float B = 2.0f*(a.x*dx+a.y*dy);
	float C = a.x*a.x+a.y*a.y-radius*radius;
	float discriminant = B*B-4.0f*A*C;
	if (discriminant < 0.0f) return false;
	float root = sqrtf(discriminant);
	float lo = (-B-root)/(2.0f*A), hi = (-B+root)/(2.0f*A);
	if (lo < 0.0f) lo = 0.0f;
	if (hi > 1.0f) hi = 1.0f;
	if (lo >= hi) return false;
	Point origin = a;
	a.x = origin.x+lo*dx; a.y = origin.y+lo*dy;
	b.x = origin.x+hi*dx; b.y = origin.y+hi*dy;
	return true;
}
}
#endif
