#include "../src/core/ModernVisualPolicy.h"
#include <cassert>
#include <cmath>
#include <limits>

int main()
{
	using namespace ModernVisualPolicy;
	assert(LodMultiplier(false, 4.0f) == 1.0f);
	assert(LodMultiplier(true, 0.0f) == 1.0f);
	assert(LodMultiplier(true, 8.0f) == 4.0f);
	assert(LodMultiplier(true, std::numeric_limits<float>::quiet_NaN()) == 1.0f);
	assert(LodMultiplier(true, 2.2f) == 2.2f);
	assert(GenerationMultiplier(1.0f) == 1.0f);
	assert(GenerationMultiplier(4.0f) == 1.375f);
	assert(FarClip(500.0f, 1.0f) == 500.0f);
	assert(FarClip(500.0f, 2.0f) == 1000.0f);
	assert(FarClip(700.0f, 4.0f) == 2000.0f);
	assert(FarClip(2200.0f, 1.0f) == 2200.0f);
	const size_t MB = 1024u * 1024u;
	assert(StreamingBudget(0) == 0);
	assert(StreamingBudget(64u * MB) == 65u * MB);
	assert(StreamingBudget(260u * MB) == 65u * MB);
	assert(StreamingBudget(512u * MB) == 128u * MB);
	assert(StreamingBudget(1024u * MB) == 256u * MB);
	assert(StreamingBudget(size_t(4) * 1024u * MB) == 512u * MB);
}
