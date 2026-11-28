#include "../src/renderer/DirectionalShadowPlan.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <random>
#include <algorithm>
#include <vector>

using namespace DirectionalShadowPlan;
using DirectionalShadowFit::Build;

int main()
{
	Vec3 corners[8];
	Vec3 eye = {10, 20, 30}, right = {1, 0, 0}, up = {0, 0, 1}, forward = {0, 1, 0};
	assert(PerspectiveCorners(eye, right, up, forward, 1.0, 0.5, 1.0, 10.0, corners));
	assert(corners[0].x == 9 && corners[0].y == 21 && corners[0].z == 29.5);
	assert(corners[7].x == 20 && corners[7].y == 30 && corners[7].z == 35);
	Vec3 saved = corners[0];
	assert(!PerspectiveCorners(eye, right, up, forward, 1, 1, 10, 10, corners));
	assert(!PerspectiveCorners(eye, {2,0,0}, up, forward, 1, 1, 1, 10, corners));
	assert(!PerspectiveCorners(eye, right, up, forward, 1, 1, 1, 10, nullptr));
	assert(corners[0].x == saved.x && corners[0].y == saved.y && corners[0].z == saved.z);

	CelestialLight light;
	assert(SelectCelestialLight({0,0,1}, &light));
	assert(!light.moon && light.strength == 1 && light.direction.z == -1);
	assert(SelectCelestialLight({0,0,-1}, &light));
	assert(light.moon && light.strength == 0.2 && light.direction.z == -1);
	assert(SelectCelestialLight({1,0,0}, &light) && light.strength == 0);
	assert(SelectCelestialLight({1,0,-0.001}, &light) && light.moon && light.strength == 0);
	assert(!SelectCelestialLight({0,0,0}, &light));
	assert(!SelectCelestialLight({0,0,1}, nullptr));
	assert(!SelectCelestialLight({0,0,std::numeric_limits<double>::infinity()}, &light));

	Result fit;
	assert(Build(corners, {0,0,-1}, 1024, 1, 20, &fit));
	for(Vec3 p : corners){
		assert(Overlaps(fit, p, 0));
		assert(Overlaps(fit, Add(p, Scale(fit.forward, -20)), 0));
	}
	assert(!Overlaps(fit, {1e9, 1e9, 1e9}, 1));
	assert(!Overlaps(fit, eye, -1));
	assert(!Overlaps(fit, {NAN,0,0}, 1));

	// Insertion order must not affect nearest-first truncation or equal-distance ties.
	std::vector<std::size_t> order;
	for(std::size_t i = 0; i < 100; ++i) order.push_back(i);
	std::mt19937 gen(17);
	for(int run = 0; run < 100; ++run){
		std::shuffle(order.begin(), order.end(), gen);
		CasterCollector<5> collector;
		for(std::size_t id : order){
			// Identical sphere centers force tie handling to use the ID.
			collector.AddCandidate(fit, eye, eye, 100, id);
		}
		assert(collector.count == 5);
		for(std::size_t i = 0; i < collector.count; ++i) assert(collector.entries[i].id == i);
	}
	CasterCollector<2> nearest;
	assert(nearest.AddCandidate(fit, eye, Add(eye,{4,0,0}), 100, 4));
	assert(nearest.AddCandidate(fit, eye, Add(eye,{3,0,0}), 100, 3));
	assert(nearest.AddCandidate(fit, eye, Add(eye,{1,0,0}), 100, 1));
	assert(nearest.count == 2 && nearest.entries[0].id == 1 && nearest.entries[1].id == 3);
	assert(!nearest.AddCandidate(fit, eye, Add(eye,{5,0,0}), 100, 5));
	CasterCollector<0> empty;
	assert(!empty.AddCandidate(fit, eye, eye, 0, 1) && empty.count == 0);

	// Follow the changing light through the former reference-axis boundary.
	Result previous;
	assert(Build(corners, {0.44, 0, -0.897}, 1024, 0, 10, &previous));
	for(int i = 0; i <= 100; ++i){
		double z = -0.897 - i*0.00006;
		Result current;
		assert(Build(corners, {std::sqrt(1-z*z), 0, z}, 1024, 0, 10, &current, &previous.right));
		assert(Dot(previous.right, current.right) > 0.9999);
		previous = current;
	}
	// A bad prior basis uses the safe reference-axis fallback.
	Vec3 bad = {NAN,0,0};
	assert(Build(corners, {0,0,-1}, 1024, 0, 10, &fit, &bad));
}
