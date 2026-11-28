#ifndef REVC_DIRECTIONAL_SHADOW_PLAN_H
#define REVC_DIRECTIONAL_SHADOW_PLAN_H

#include "DirectionalShadowFit.h"
#include <cstddef>

// Shared CPU-side building blocks for a future directional shadow pass. They
// neither touch graphics state nor replace the game's classic shadows.
namespace DirectionalShadowPlan {
using DirectionalShadowFit::Vec3;
using DirectionalShadowFit::Result;
using DirectionalShadowFit::Dot;
using DirectionalShadowFit::Finite;
using DirectionalShadowFit::Sub;

inline Vec3 Add(Vec3 a, Vec3 b) { return {a.x+b.x, a.y+b.y, a.z+b.z}; }
inline Vec3 Scale(Vec3 a, double k) { return {a.x*k, a.y*k, a.z*k}; }

// Construct perspective frustum corners using a world-space orthonormal camera
// basis and half-width/height at unit depth. This handles centered perspective
// cameras only, not an offset or parallel projection. near/far are distances
// along the forward axis, not spherical distances. On failure, output is unchanged.
inline bool PerspectiveCorners(Vec3 eye, Vec3 right, Vec3 up, Vec3 forward,
                               double viewX, double viewY, double nearDistance,
                               double farDistance, Vec3 out[8])
{
	if(!out || !Finite(eye) || !Finite(right) || !Finite(up) || !Finite(forward) ||
	   !std::isfinite(viewX) || !std::isfinite(viewY) || !std::isfinite(nearDistance) ||
	   !std::isfinite(farDistance) || viewX <= 0 || viewY <= 0 ||
	   nearDistance <= 0 || farDistance <= nearDistance) return false;
	// Reject malformed camera frames instead of producing a subtly wrong fit.
	if(std::fabs(Dot(right,right)-1.0) > 0.001 || std::fabs(Dot(up,up)-1.0) > 0.001 ||
	   std::fabs(Dot(forward,forward)-1.0) > 0.001 || std::fabs(Dot(right,up)) > 0.001 ||
	   std::fabs(Dot(right,forward)) > 0.001 || std::fabs(Dot(up,forward)) > 0.001) return false;
	Vec3 corners[8];
	for(int i = 0; i < 8; ++i){
		double distance = (i & 4) ? farDistance : nearDistance;
		corners[i] = Add(Add(Add(eye, Scale(forward, distance)),
		                     Scale(right, ((i & 1) ? viewX : -viewX)*distance)),
		                     Scale(up, ((i & 2) ? viewY : -viewY)*distance));
		if(!Finite(corners[i])) return false;
	}
	for(int i = 0; i < 8; ++i) out[i] = corners[i];
	return true;
}

struct CelestialLight {
	Vec3 direction; // direction from the light toward the scene
	double strength; // 0..1, before weather and material response
	bool moon;
};

// Timecycle supplies a direction TO the sun, not its rays. A provisional moon
// is opposite that vector; it is deliberately much dimmer. Fade both to zero
// at the horizon to avoid a popping direction flip at dawn/dusk. The caller
// still needs to apply weather/interior/cutscene policy and invalidate old maps.
inline bool SelectCelestialLight(Vec3 toSun, CelestialLight *out)
{
	if(!out || !Finite(toSun)) return false;
	double length2 = Dot(toSun, toSun);
	if(!std::isfinite(length2) || length2 < 1.0e-20) return false;
	toSun = Scale(toSun, 1.0/std::sqrt(length2));
	bool moon = toSun.z < 0.0;
	Vec3 toLight = moon ? Scale(toSun, -1.0) : toSun;
	double t = (toLight.z - 0.02) / 0.10;
	if(t < 0.0) t = 0.0;
	if(t > 1.0) t = 1.0;
	t = t*t*(3.0 - 2.0*t);
	CelestialLight light = {Scale(toLight, -1.0), t * (moon ? 0.2 : 1.0), moon};
	*out = light;
	return true;
}

// Conservative sphere-vs-oriented-box test. The box is the fit extended
// toward the light, so offscreen casters aren't incorrectly rejected merely
// because they're outside the main view. May accept corner false positives.
inline bool Overlaps(const Result &fit, Vec3 center, double radius)
{
	if(!Finite(center) || !std::isfinite(radius) || radius < 0.0 ||
	   !Finite(fit.right) || !Finite(fit.up) || !Finite(fit.forward) ||
	   !std::isfinite(fit.centerX) || !std::isfinite(fit.centerY) ||
	   !std::isfinite(fit.halfExtent) || fit.halfExtent <= 0.0 ||
	   !std::isfinite(fit.nearDepth) || !std::isfinite(fit.farDepth) ||
	   fit.nearDepth >= fit.farDepth) return false;
	double x = Dot(center, fit.right), y = Dot(center, fit.up), z = Dot(center, fit.forward);
	return std::isfinite(x) && std::isfinite(y) && std::isfinite(z) &&
	       std::fabs(x-fit.centerX) <= fit.halfExtent+radius &&
	       std::fabs(y-fit.centerY) <= fit.halfExtent+radius &&
	       z >= fit.nearDepth-radius && z <= fit.farDepth+radius;
}

// Bounded nearest-first collector. IDs are caller-owned and only used as a
// stable tie breaker. Once full, farther spheres are dropped, so capacity must
// be chosen and profiled by the eventual renderer; this is not exhaustive.
template <std::size_t Capacity>
struct CasterCollector {
	struct Entry { std::size_t id; double distanceSquared; };
	Entry entries[Capacity ? Capacity : 1];
	std::size_t count = 0;

	bool AddCandidate(const Result &fit, Vec3 eye, Vec3 center, double radius, std::size_t id)
	{
		if(!Finite(eye) || !Overlaps(fit, center, radius)) return false;
		Vec3 delta = Sub(center, eye);
		double dist = Dot(delta, delta);
		if(!std::isfinite(dist)) return false;
		std::size_t at = 0;
		while(at < count && (entries[at].distanceSquared < dist ||
		      (entries[at].distanceSquared == dist && entries[at].id <= id))) ++at;
		if(at >= Capacity) return false;
		std::size_t end = count < Capacity ? count++ : Capacity - 1;
		while(end > at){ entries[end] = entries[end-1]; --end; }
		entries[at] = {id, dist};
		return true;
	}
};
}
#endif
