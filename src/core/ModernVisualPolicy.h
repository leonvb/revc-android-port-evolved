#ifndef REVC_MODERN_VISUAL_POLICY_H
#define REVC_MODERN_VISUAL_POLICY_H

#include <stddef.h>

// Pure, bounded policy shared by the runtime and standalone tests. The
// streaming budget is an upper limit, not an allocation or a RAM reservation.
namespace ModernVisualPolicy {
inline float LodMultiplier(bool enabled, float value)
{
	if (!enabled || !(value >= 1.0f)) return 1.0f; // also catches NaN
	return value > 4.0f ? 4.0f : value;
}
inline float SceneLodMultiplier(bool eligible, float configuredLod)
{
	return eligible ? LodMultiplier(true, configuredLod) : 1.0f;
}
inline bool ShouldPrepareFarClip(bool extended, bool wasExtended, float currentFar, float lastExtendedFar)
{
	// Do not replace a clip deliberately set by a scripted or top-down camera.
	return extended || (wasExtended && currentFar == lastExtendedFar);
}
// Keep the previously rendered LOD across a narrow threshold band. This is
// per-instance (the caller identifies the geometry), not a frame counter.
inline bool KeepPreviousLod(int previous, int selected, float distance, float boundary)
{
	if(previous < 0 || selected < 0 || (previous + 1 != selected && selected + 1 != previous) ||
	   !(boundary > 0.0f) || !(distance >= 0.0f))
		return false;
	const float band = boundary < 100.0f ? 1.0f : 2.0f;
	return distance >= boundary - band && distance <= boundary + band;
}
// The narrow priority frustum protects nearby streaming requests. Extending it
// along with the visual horizon lets many distant requests monopolize the
// limited queue; retain the original camera/FOV and Display scaling instead.
inline float PriorityLodDistance(float extendedDistance, float modernLod)
{
	return extendedDistance / LodMultiplier(true, modernLod);
}
inline float GenerationMultiplier(float lod)
{
	// Keep spawn rings within the fixed-size ped/vehicle pools. Applying the
	// full visual multiplier would starve the player's immediate surroundings.
	return 1.0f + (LodMultiplier(true, lod) - 1.0f) * 0.125f;
}
// Alternate batches without consuming game RNG or moving all spawns away from
// the player. Use bit 2 since car selection already uses the low two bits.
inline float SpawnHorizon(float lod, unsigned int frame, bool initialBatch = false)
{
	return !initialBatch && (frame & 4u) ? GenerationMultiplier(lod) : 1.0f;
}
inline float FarClip(float original, float lod)
{
	float limit = original * LodMultiplier(true, lod);
	// Never shorten the original weather/cutscene clip if it is already larger.
	if (limit > 2000.0f && original < 2000.0f) return 2000.0f;
	return original > 2000.0f ? original : limit;
}
inline size_t StreamingBudget(size_t freeBytes)
{
	const size_t MB = 1024u * 1024u;
	// Use at most a quarter of reported free RAM except for the existing
	// engine minimum of 65 MB. Zero means memory information is missing.
	if (!freeBytes) return 0;
	size_t budget = freeBytes / 4;
	if (budget < 65u * MB) budget = 65u * MB;
	return budget > 512u * MB ? 512u * MB : budget;
}
// Never shrink the established cache when enabling longer draw distances.
// A lower limit makes the streamer evict models that the larger view still needs.
inline size_t EffectiveStreamingBudget(size_t classicBudget, size_t freeBytes)
{
	const size_t modernBudget = StreamingBudget(freeBytes);
	return modernBudget > classicBudget ? modernBudget : classicBudget;
}
}
#endif
