#include "../src/renderer/DirectionalShadowFit.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <random>

using namespace DirectionalShadowFit;

static double Clip(const Result &r, int row, Vec3 p)
{
	const double *m = &r.worldToClip[row*4];
	return m[0]*p.x + m[1]*p.y + m[2]*p.z + m[3];
}

static void AssertInside(const Result &r, Vec3 p)
{
	for(int axis = 0; axis != 3; ++axis)
		assert(Clip(r, axis, p) >= -1.000000001 && Clip(r, axis, p) <= 1.000000001);
}

int main()
{
	Vec3 cube[8];
	for(int i = 0; i < 8; ++i)
		cube[i] = {(i & 1) ? 10.0 : -10.0, (i & 2) ? 15.0 : -15.0, (i & 4) ? 20.0 : -20.0};
	Result base;
	assert(Build(cube, {0.0, 0.0, -1.0}, 1024, 1.0, 30.0, &base));
	assert(std::fabs(Dot(base.right, base.up)) < 1e-12);
	assert(std::fabs(Dot(base.right, base.forward)) < 1e-12);
	assert(std::fabs(Dot(base.up, base.forward)) < 1e-12);
	for(Vec3 p : cube){
		AssertInside(base, p);
		// Include the same receiver translated toward the light by casterDepth.
		Vec3 caster = {p.x - 30.0*base.forward.x, p.y - 30.0*base.forward.y, p.z - 30.0*base.forward.z};
		AssertInside(base, caster);
	}
	// A sub-texel movement from a snapped center does not move the projection.
	Vec3 shifted[8];
	for(int i = 0; i < 8; ++i)
		shifted[i] = {cube[i].x + base.right.x*base.texelSize*0.25,
		              cube[i].y + base.right.y*base.texelSize*0.25,
		              cube[i].z + base.right.z*base.texelSize*0.25};
	Result moved;
	assert(Build(shifted, {0.0, 0.0, -1.0}, 1024, 1.0, 30.0, &moved));
	assert(std::fabs(base.centerX-moved.centerX) < 1e-12);
	assert(std::fabs(base.centerY-moved.centerY) < 1e-12);
	assert(std::fabs(base.halfExtent-moved.halfExtent) < 1e-12);
	// Rotating the camera slice must not change its spherical XY footprint.
	for(int i = 0; i < 8; ++i)
		shifted[i] = {-cube[i].y, cube[i].x, cube[i].z};
	assert(Build(shifted, {0.0, 0.0, -1.0}, 1024, 1.0, 30.0, &moved));
	assert(std::fabs(base.halfExtent-moved.halfExtent) < 1e-12);
	for(Vec3 p : shifted) AssertInside(moved, p);
	// Even the minimum supported resolution retains coverage after snapping.
	assert(Build(cube, {0.1, 0.2, 0.9}, 2, 0.0, 0.0, &moved));
	for(Vec3 p : cube) AssertInside(moved, p);

	Result untouched = base;
	assert(!Build(cube, {0,0,0}, 1024, 1.0, 30.0, &untouched));
	assert(untouched.halfExtent == base.halfExtent);
	assert(!Build(cube, {0,0,1}, 1, 1.0, 30.0, &untouched));
	assert(!Build(cube, {0,0,1}, 1024, -1.0, 30.0, &untouched));
	assert(!Build(cube, {0,0,1}, 1024, 1.0, -1.0, &untouched));
	assert(!Build(cube, {0,0,1}, 1024, 1.0, 30.0, nullptr));
	assert(!Build(nullptr, {0,0,1}, 1024, 1.0, 30.0, &untouched));
	Vec3 invalid[8];
	for(int i = 0; i < 8; ++i) invalid[i] = cube[i];
	invalid[3].x = std::numeric_limits<double>::quiet_NaN();
	assert(!Build(invalid, {0,0,1}, 1024, 1.0, 30.0, &untouched));
	for(Vec3 &p : invalid) p = {1.0, 2.0, 3.0};
	assert(!Build(invalid, {0,0,1}, 1024, 0.0, 0.0, &untouched));

	// Randomized asymmetric frustum corners, light directions, and shifts.
	std::mt19937 random(40231);
	std::uniform_real_distribution<double> location(-2000.0, 2000.0), offset(-100.0, 100.0), dir(-1.0, 1.0);
	for(int trial = 0; trial < 10000; ++trial){
		Vec3 center = {location(random), location(random), location(random)};
		Vec3 points[8];
		for(Vec3 &p : points) p = {center.x + offset(random), center.y + offset(random), center.z + offset(random)};
		Vec3 light = {dir(random), dir(random), dir(random)};
		Result r;
		assert(Build(points, light, 2048, 0.25, 40.0, &r));
		for(Vec3 p : points){
			AssertInside(r, p);
			Vec3 caster = {p.x - 40.0*r.forward.x, p.y - 40.0*r.forward.y, p.z - 40.0*r.forward.z};
			AssertInside(r, caster);
		}
	}
}
