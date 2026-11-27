#include "../src/extras/ModernLightingPolicy.h"
#include <cassert>
#include <limits>
int main()
{
	using namespace ModernLightingPolicy;
	Light a[12] = {};
	for(int i = 0; i < 12; ++i){
		a[i].x = float(i); a[i].radius = 10.0f;
		a[i].r = a[i].g = a[i].b = 1.0f;
	}
	int selected[MaxLights];
	assert(Select(0, 0, 0, 0, 0, 0, selected) == 0);
	assert(Select(a, 12, 0, 0, 0, 0, selected) == MaxLights);
	for(int i = 0; i < MaxLights; ++i) assert(selected[i] == i);
	a[0].radius = 0;
	a[1].r = std::numeric_limits<float>::quiet_NaN();
	assert(Select(a, 3, 0, 0, 0, 0, selected) == 1 && selected[0] == 2);
	assert(Select(a, 3, 0, 0, 0, -1, selected) == 0);
	assert(Select(a, 3, 0, 0, 0, 0, 0) == 0);
	Light tie[2] = {};
	for(int i = 0; i < 2; ++i){ tie[i].radius = 1; tie[i].r = 1; tie[i].x = 2; }
	assert(Select(tie, 2, 0, 0, 0, 0, selected) == 0);
	assert(Select(tie, 2, 0, 0, 0, 2, selected) == 2);
	assert(selected[0] == 0 && selected[1] == 1);
}
