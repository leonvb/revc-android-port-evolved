#ifndef REVC_MODERN_LIGHTING_POLICY_H
#define REVC_MODERN_LIGHTING_POLICY_H

#include <cmath>

namespace ModernLightingPolicy {
enum { MaxLights = 8 };
struct Light {
	float x, y, z, radius;
	float r, g, b;
	float dirX, dirY, dirZ;
	bool directional;
};

// Select the strongest intersecting local lights for an object's bounding
// sphere. Stable ties prevent flicker. Reject malformed input before it can
// become a shader uniform. Returns indices into lights, not copies.
inline int Select(const Light *lights, int count, float x, float y, float z,
                  float sphereRadius, int out[MaxLights])
{
	if(!lights || !out || count <= 0 || !std::isfinite(x) || !std::isfinite(y) ||
	   !std::isfinite(z) || !std::isfinite(sphereRadius) || sphereRadius < 0.0f)
		return 0;
	float scores[MaxLights] = {};
	int used = 0;
	for(int i = 0; i < count; ++i){
		const Light &l = lights[i];
		if(!std::isfinite(l.x) || !std::isfinite(l.y) || !std::isfinite(l.z) ||
		   !std::isfinite(l.radius) || l.radius <= 0.0f ||
		   !std::isfinite(l.r) || !std::isfinite(l.g) || !std::isfinite(l.b) ||
		   !std::isfinite(l.dirX) || !std::isfinite(l.dirY) || !std::isfinite(l.dirZ))
			continue;
		float peak = std::fmax(l.r, std::fmax(l.g, l.b));
		if(peak <= 0.0f) continue;
		// double avoids overflow for very distant or corrupt coordinates.
		double dx = double(l.x)-x, dy = double(l.y)-y, dz = double(l.z)-z;
		double distance = std::sqrt(dx*dx + dy*dy + dz*dz);
		double nearest = std::fmax(0.0, distance - sphereRadius);
		if(nearest >= l.radius) continue;
		float falloff = float(1.0 - nearest / l.radius);
		float score = peak * falloff * falloff;
		int at = 0;
		while(at < used && scores[at] >= score) ++at;
		if(at >= MaxLights) continue;
		if(used < MaxLights) ++used;
		for(int j = used - 1; j > at; --j){ scores[j] = scores[j-1]; out[j] = out[j-1]; }
		scores[at] = score; out[at] = i;
	}
	return used;
}
}
#endif
