#ifndef REVC_MODERN_TRAVERSAL_MATH_H
#define REVC_MODERN_TRAVERSAL_MATH_H

#include <math.h>

// Pure locomotion helpers shared with host-side tests.
namespace ModernTraversalMath {
inline float Bound(float value, float lo, float hi) { return fmaxf(lo, fminf(hi, value)); }
struct Pivot {
	float lastX, lastY, remaining;
	bool hasDirection;
	void Reset() { lastX = lastY = remaining = 0.0f; hasDirection = false; }
	float Update(float x, float y, float speed, float dt)
	{
		float length = sqrtf(x*x + y*y);
		if (length < 0.2f) { hasDirection = false; remaining = 0.0f; return 1.0f; }
		x /= length; y /= length;
		if (hasDirection && speed > 0.6f && x*lastX + y*lastY < -0.5f && remaining <= 0.0f)
			remaining = 0.12f;
		lastX = x; lastY = y; hasDirection = true;
		if (remaining <= 0.0f) return 1.0f;
		float factor = 0.35f + 0.65f * (1.0f - remaining / 0.12f);
		remaining = fmaxf(0.0f, remaining - Bound(dt, 0.0f, 0.1f));
		return factor;
	}
};
}
#endif
