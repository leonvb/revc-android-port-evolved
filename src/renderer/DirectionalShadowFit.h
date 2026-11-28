#ifndef REVC_DIRECTIONAL_SHADOW_FIT_H
#define REVC_DIRECTIONAL_SHADOW_FIT_H

#include <cmath>

// Backend-independent orthographic shadow-frustum fitting. This only computes
// a transform; it does not allocate a depth texture or render any shadows.
namespace DirectionalShadowFit {
struct Vec3 { double x, y, z; };
struct Result {
	Vec3 right, up, forward;
	double centerX, centerY, halfExtent, nearDepth, farDepth, texelSize;
	// Row-major world-to-clip matrix. Clip coordinates are in [-1, 1].
	double worldToClip[16];
};

inline Vec3 Sub(Vec3 a, Vec3 b) { return {a.x-b.x, a.y-b.y, a.z-b.z}; }
inline double Dot(Vec3 a, Vec3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline Vec3 Cross(Vec3 a, Vec3 b)
{
	return {a.y*b.z-a.z*b.y, a.z*b.x-a.x*b.z, a.x*b.y-a.y*b.x};
}
inline bool Finite(Vec3 v) { return std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z); }

// corners: world-space corners of one camera frustum slice; forward: direction
// light travels (from light toward scene). casterDepth extends toward the light
// to include off-camera occluders. padding is in world units. A bounding sphere
// keeps the XY scale independent of camera rotation. Snapping its center to
// shadow texels reduces shimmering from small translations. Resolution must be
// at least 2: one texel of slack is needed to contain the snapped frustum.
// Pass the previous successful right axis to reduce basis rotation while the
// light moves; reset it on discontinuous light changes. The reference-axis
// fallback can still jump if that axis becomes parallel to the new direction.
inline bool Build(const Vec3 corners[8], Vec3 forward, int resolution,
                  double padding, double casterDepth, Result *out,
                  const Vec3 *previousRight = nullptr)
{
	if(!corners || !out || resolution < 2 || !Finite(forward) ||
	   !std::isfinite(padding) || padding < 0.0 ||
	   !std::isfinite(casterDepth) || casterDepth < 0.0)
		return false;
	double length2 = Dot(forward, forward);
	if(!std::isfinite(length2) || length2 < 1.0e-20) return false;
	double inv = 1.0 / std::sqrt(length2);
	forward = {forward.x*inv, forward.y*inv, forward.z*inv};
	// Transport the previous horizontal axis onto the new light plane. This
	// avoids a sudden 90-degree rotation at the reference-axis threshold.
	Vec3 right = {0.0, 0.0, 0.0};
	if(previousRight && Finite(*previousRight)){
		double along = Dot(*previousRight, forward);
		right = {previousRight->x - along*forward.x,
		         previousRight->y - along*forward.y,
		         previousRight->z - along*forward.z};
	}
	if(!std::isfinite(Dot(right, right)) || Dot(right, right) < 1.0e-12){
		Vec3 reference = std::fabs(forward.z) < 0.9 ? Vec3{0.0, 0.0, 1.0} : Vec3{0.0, 1.0, 0.0};
		right = Cross(reference, forward);
	}
	inv = 1.0 / std::sqrt(Dot(right, right));
	right = {right.x*inv, right.y*inv, right.z*inv};
	Vec3 up = Cross(forward, right);

	Vec3 center = {0.0, 0.0, 0.0};
	for(int i = 0; i < 8; ++i){
		if(!Finite(corners[i])) return false;
		// Divide before summing to reduce overflow for large finite inputs.
		center.x += corners[i].x / 8.0;
		center.y += corners[i].y / 8.0;
		center.z += corners[i].z / 8.0;
	}
	if(!Finite(center)) return false;
	double radius2 = 0.0;
	double minDepth = Dot(corners[0], forward), maxDepth = minDepth;
	for(int i = 0; i < 8; ++i){
		Vec3 delta = Sub(corners[i], center);
		double d2 = Dot(delta, delta), depth = Dot(corners[i], forward);
		if(!std::isfinite(d2) || !std::isfinite(depth)) return false;
		if(d2 > radius2) radius2 = d2;
		if(depth < minDepth) minDepth = depth;
		if(depth > maxDepth) maxDepth = depth;
	}
	if(!(radius2 > 1.0e-20)) return false;
	double extent = (std::sqrt(radius2) + padding) / (1.0 - 1.0 / resolution);
	double texel = 2.0 * extent / resolution;
	double centerX = std::floor(Dot(center, right) / texel + 0.5) * texel;
	double centerY = std::floor(Dot(center, up) / texel + 0.5) * texel;
	minDepth -= casterDepth + padding;
	maxDepth += padding;
	double depthRange = maxDepth - minDepth;
	if(!std::isfinite(extent) || !std::isfinite(texel) || !std::isfinite(centerX) ||
	   !std::isfinite(centerY) || !std::isfinite(minDepth) || !std::isfinite(maxDepth) ||
	   !std::isfinite(depthRange) || !(depthRange > 1.0e-10)) return false;

	Result result = {};
	result.right = right; result.up = up; result.forward = forward;
	result.centerX = centerX; result.centerY = centerY; result.halfExtent = extent;
	result.nearDepth = minDepth; result.farDepth = maxDepth; result.texelSize = texel;
	result.worldToClip[0] = right.x / extent; result.worldToClip[1] = right.y / extent;
	result.worldToClip[2] = right.z / extent; result.worldToClip[3] = -centerX / extent;
	result.worldToClip[4] = up.x / extent; result.worldToClip[5] = up.y / extent;
	result.worldToClip[6] = up.z / extent; result.worldToClip[7] = -centerY / extent;
	result.worldToClip[8] = 2.0*forward.x / depthRange; result.worldToClip[9] = 2.0*forward.y / depthRange;
	result.worldToClip[10] = 2.0*forward.z / depthRange; result.worldToClip[11] = -1.0 - 2.0*minDepth / depthRange;
	result.worldToClip[15] = 1.0;
	for(int i = 0; i < 16; ++i)
		if(!std::isfinite(result.worldToClip[i])) return false;
	*out = result;
	return true;
}
}
#endif
