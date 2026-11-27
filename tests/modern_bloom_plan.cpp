#include "../src/extras/ModernBloomPlan.h"
#include <cassert>
#include <climits>
#include <cstdint>

int main()
{
	using namespace ModernBloomPlan;
	Plan p = {};
	assert(!Build(1920, 1080, 4096, UINT64_MAX, 0));
	assert(!Build(0, 1080, 4096, UINT64_MAX, &p));
	assert(!Build(1920, 7, 4096, UINT64_MAX, &p));
	assert(!Build(1920, 1080, 512, UINT64_MAX, &p));
	assert(!Build(INT_MAX, INT_MAX, INT_MAX, 1000000, &p));
	assert(Build(1920, 1080, 4096, UINT64_MAX, &p));
	assert(p.levels[0].width == 960 && p.levels[0].height == 540);
	assert(p.levels[1].width == 480 && p.levels[1].height == 270);
	assert(p.levels[2].width == 240 && p.levels[2].height == 135);
	assert(p.minimumBytes == uint64_t(960*540 + 480*270 + 240*135) * 4);
	uint64_t minimum = p.minimumBytes;
	assert(!Build(1920, 1080, 4096, minimum - 1, &p));
	assert(Build(1920, 1080, 4096, minimum, &p));
	assert(Build(17, 9, 100, UINT64_MAX, &p));
	assert(p.levels[0].width == 9 && p.levels[0].height == 5);
	assert(p.levels[1].width == 5 && p.levels[1].height == 3);
	assert(p.levels[2].width == 3 && p.levels[2].height == 2);
	float b[4];
	assert(!Bounds(1024, 1024, INT_MAX, 0, 1, 1, b));
	assert(!Bounds(1024, 1024, 0, 0, 0, 1, b));
	assert(!Bounds(1024, 1024, 0, 0, 1, 1, 0));
	assert(Bounds(1024, 1024, 0, 304, 640, 720, b));
	assert(b[0] == 0.5f/1024 && b[1] == 304.5f/1024);
	assert(b[2] == 639.5f/1024 && b[3] == 1023.5f/1024);
	assert(Bounds(1, 1, 0, 0, 1, 1, b));
	assert(b[0] == b[2] && b[1] == b[3]);
	assert(NightFactor(12, 0) == 0.35f);
	assert(NightFactor(23, 0) == 1.0f);
	assert(NightFactor(6, 0) > NightFactor(7, 0));
	assert(NightFactor(19, 0) > NightFactor(18, 0));
	assert(NightFactor(-1, 0) == 0.35f);
	for(int i = 0; i < 5; ++i){
		Pass pass = PassAt(i);
		assert(pass.source != pass.destination);
		assert(pass.destination >= 0 && pass.destination < 3);
		assert(pass.source >= -1 && pass.source < 3);
	}
	assert(PassAt(0).source == -1 && PassAt(0).kind == 0);
	assert(PassAt(4).destination == 0 && PassAt(4).kind == 2);
	assert(PassAt(-1).kind == -1 && PassAt(5).kind == -1);
}
