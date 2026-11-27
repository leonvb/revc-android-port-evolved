#include "../src/core/ModernVisualPolicy.h"
#include <cassert>
#include <cmath>
#include <limits>
#include <initializer_list>

int main()
{
	using namespace ModernVisualPolicy;
	assert(LodMultiplier(false, 4.0f) == 1.0f);
	assert(LodMultiplier(true, 0.0f) == 1.0f);
	assert(LodMultiplier(true, 8.0f) == 4.0f);
	assert(LodMultiplier(true, std::numeric_limits<float>::quiet_NaN()) == 1.0f);
	assert(LodMultiplier(true, 2.2f) == 2.2f);
	assert(SceneLodMultiplier(false, 4.0f) == 1.0f);
	assert(SceneLodMultiplier(true, 2.2f) == 2.2f);
	assert(SceneLodMultiplier(true, std::numeric_limits<float>::quiet_NaN()) == 1.0f);
	assert(!ShouldPrepareFarClip(false, false, 500.0f, 1000.0f));
	assert(ShouldPrepareFarClip(true, false, 500.0f, 1000.0f));
	assert(ShouldPrepareFarClip(false, true, 1000.0f, 1000.0f));
	assert(!ShouldPrepareFarClip(false, true, 200.0f, 1000.0f));
	assert(KeepPreviousLod(0, 1, 100.5f, 100.0f));
	assert(KeepPreviousLod(1, 0, 99.5f, 100.0f));
	assert(!KeepPreviousLod(0, 1, 103.0f, 100.0f));
	assert(!KeepPreviousLod(0, 2, 100.0f, 100.0f));
	assert(!KeepPreviousLod(-1, 0, 100.0f, 100.0f));
	assert(!KeepPreviousLod(0, 1, std::numeric_limits<float>::quiet_NaN(), 100.0f));
	// Camera jitter within the band must not toggle the selected geometry,
	// regardless of how many frames occur while the player is standing still.
	int prior = 0;
	for(int frame = 0; frame < 10000; ++frame){
		float distance = frame % 2 ? 100.25f : 99.75f;
		int selected = distance < 100.0f ? 0 : 1;
		if(!KeepPreviousLod(prior, selected, distance, 100.0f)) prior = selected;
		assert(prior == 0);
	}
	// The normal horizon grows, but the priority wedge retains the camera's
	// original FOV/Display-scaled distance, including when the far clip caps.
	for(float classicDistance : {180.0f, 360.0f, 660.0f}){
		for(float modern : {1.0f, 2.2f, 4.0f}){
			float extended = classicDistance * modern;
			assert(std::fabs(PriorityLodDistance(extended, modern) - classicDistance) < 0.001f);
		}
	}
	assert(PriorityLodDistance(360.0f, 0.0f) == 360.0f);
	assert(PriorityLodDistance(360.0f, std::numeric_limits<float>::quiet_NaN()) == 360.0f);
	assert(GenerationMultiplier(1.0f) == 1.0f);
	assert(GenerationMultiplier(4.0f) == 1.375f);
	for (unsigned int frame = 0; frame != 16; ++frame) {
		assert(SpawnHorizon(1.0f, frame) == 1.0f);
		assert(SpawnHorizon(4.0f, frame, true) == 1.0f);
		assert(SpawnHorizon(4.0f, frame) == ((frame & 4u) ? 1.375f : 1.0f));
	}
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
	assert(EffectiveStreamingBudget(240u * MB, 512u * MB) == 240u * MB);
	assert(EffectiveStreamingBudget(65u * MB, 1024u * MB) == 256u * MB);
	assert(EffectiveStreamingBudget(512u * MB, 0) == 512u * MB);
	assert(EffectiveStreamingBudget(768u * MB, size_t(4) * 1024u * MB) == 768u * MB);
}
