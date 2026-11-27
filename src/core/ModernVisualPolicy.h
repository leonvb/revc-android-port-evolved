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
inline float GenerationMultiplier(float lod)
{
	// Keep spawn rings within the fixed-size ped/vehicle pools. Applying the
	// full visual multiplier would starve the player's immediate surroundings.
	return 1.0f + (LodMultiplier(true, lod) - 1.0f) * 0.125f;
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
}
#endif
