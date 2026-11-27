#ifndef REVC_MODERN_BLOOM_PLAN_H
#define REVC_MODERN_BLOOM_PLAN_H

#include <stdint.h>

// CPU-side plan for a three-level bloom chain.
// The byte count is a *minimum* for tightly packed RGBA8 color targets, not a
// guarantee of driver allocation size, support for NPOT targets, or available
// GPU memory. Allocation/format/FBO failure must still bypass the effect.
namespace ModernBloomPlan {
struct Size { int width, height; };
struct Plan { Size levels[3]; uint64_t minimumBytes; };
inline bool Build(int width, int height, int maxTargetSize, uint64_t byteBudget, Plan *out)
{
	if(!out || width < 8 || height < 8 || maxTargetSize <= 0)
		return false;
	Plan plan = {};
	uint64_t bytes = 0;
	for(int i = 0; i < 3; ++i){
		// ceil(n/2), without overflowing at INT_MAX.
		width = width / 2 + width % 2;
		height = height / 2 + height % 2;
		if(width > maxTargetSize || height > maxTargetSize)
			return false;
		plan.levels[i].width = width;
		plan.levels[i].height = height;
		const uint64_t levelBytes = uint64_t(width) * uint64_t(height) * 4;
		if(levelBytes > byteBudget - bytes)
			return false;
		bytes += levelBytes;
	}
	plan.minimumBytes = bytes;
	*out = plan;
	return true;
}

// Bounds in normalized coordinates, inclusive of first/last texel centers.
// x/y specify the physical origin of the valid rectangle in texture space;
// GL's upper-aligned scene copy and D3D's upper-left copy have different y.
inline bool Bounds(int textureWidth, int textureHeight, int x, int y,
                   int width, int height, float out[4])
{
	if(!out || textureWidth <= 0 || textureHeight <= 0 || x < 0 || y < 0 ||
	   width <= 0 || height <= 0 || x > textureWidth || y > textureHeight ||
	   width > textureWidth - x || height > textureHeight - y)
		return false;
	out[0] = (float(x) + 0.5f) / float(textureWidth);
	out[1] = (float(y) + 0.5f) / float(textureHeight);
	out[2] = (float(x + width) - 0.5f) / float(textureWidth);
	out[3] = (float(y + height) - 0.5f) / float(textureHeight);
	return true;
}

// Daytime highlights are deliberately subdued; transitions span two hours.
inline float NightFactor(int hour, int minute)
{
	if(hour < 0 || hour > 23 || minute < 0 || minute > 59)
		return 0.35f;
	float t = float(hour) + float(minute) / 60.0f;
	float night = t < 5.0f || t >= 20.0f ? 1.0f :
	              t < 7.0f ? (7.0f - t) / 2.0f :
	              t < 18.0f ? 0.0f : (t - 18.0f) / 2.0f;
	return 0.35f + 0.65f * night;
}

// -1 denotes the captured scene; levels 0, 1, 2 are distinct textures.
// Composite separately onto the scene without sampling the destination.
struct Pass { int source, destination, kind; };
inline Pass PassAt(int i)
{
	static const Pass passes[5] = {{-1, 0, 0}, {0, 1, 1}, {1, 2, 1},
	                               {2, 1, 2}, {1, 0, 2}};
	return i >= 0 && i < 5 ? passes[i] : Pass{-2, -2, -1};
}
}
#endif
