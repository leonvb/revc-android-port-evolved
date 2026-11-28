#ifndef REVC_MODERN_AIM_MAGNETISM_H
#define REVC_MODERN_AIM_MAGNETISM_H

#include <math.h>

// Engine-independent policy/math. Handles are opaque generation-checked ped
// pool references, never pointers. The engine performs all visibility checks.
namespace ModernAimMagnetism {
struct State {
	int target;
	float occluded;
	float overrideCooldown;
	float scanCooldown;
	bool lockedOut;

	void Reset() { target = -1; occluded = overrideCooldown = scanCooldown = 0.0f; lockedOut = false; }

	bool BeginFrame(bool eligible, bool strongInput, float dt)
	{
		if (!eligible) { Reset(); return false; }
		if (!(dt >= 0.0f)) dt = 0.0f;
		if (dt > 0.1f) dt = 0.1f;
		if (strongInput) {
			target = -1; occluded = 0.0f; lockedOut = false;
			overrideCooldown = 0.25f;
			return false;
		}
		overrideCooldown = fmaxf(0.0f, overrideCooldown - dt);
		scanCooldown = fmaxf(0.0f, scanCooldown - dt);
		return overrideCooldown <= 0.0f;
	}

	bool CanAcquire() const { return target == -1 && !lockedOut && overrideCooldown <= 0.0f && scanCooldown <= 0.0f; }
	void Scanned() { scanCooldown = 0.12f; }
	void Acquire(int handle) { target = handle; occluded = 0.0f; }
	void Lose() { target = -1; occluded = 0.0f; lockedOut = true; }

	// A temporarily blocked target is retained but never assisted through a wall.
	// Once lost, wait for re-aim or an intentional strong input before acquiring
	// another; two nearby enemies cannot silently trade ownership.
	bool Visible(bool stillEligible, bool visible, float dt)
	{
		if (target == -1) return false;
		if (!stillEligible) { Lose(); return false; }
		if (visible) { occluded = 0.0f; return true; }
		if (!(dt >= 0.0f)) dt = 0.0f;
		if (dt > 0.1f) dt = 0.1f;
		occluded += dt;
		if (occluded > 0.18f) Lose();
		return false;
	}
};

struct Tuning { float friction, rate, maxRate, breakInput; };
inline Tuning GetTuning(int strength, bool touch)
{
	// Full-stick deflection is an escape gesture, not ordinary camera movement.
	// Keep an appreciable response throughout the normal range of stick travel.
	static const Tuning levels[] = {
		{ 0.0f, 0.0f, 0.0f, 0.97f },
		{ 0.16f, 1.0f, 0.22f, 0.97f },
		{ 0.32f, 1.9f, 0.38f, 0.97f },
		{ 0.52f, 3.2f, 0.60f, 0.97f },
	};
	if (strength < 0 || strength > 3) strength = 0;
	Tuning t = levels[strength];
	if (touch) { t.friction *= 0.9f; t.rate *= 0.9f; t.maxRate *= 0.9f; t.breakInput = 0.94f; }
	return t;
}

// Input arrives after per-axis controller sensitivity scaling. Undo it before
// detecting a deliberate full-stick override; a high sensitivity setting must
// not make every moderate movement look like a full deflection. Max-axis
// rather than radial magnitude also keeps ordinary diagonals from breaking aim.
inline float InputAmount(float x, float y, float sensitivityX, float sensitivityY)
{
	float sx = fabsf(sensitivityX), sy = fabsf(sensitivityY);
	if (sx < 0.01f) sx = 1.0f;
	if (sy < 0.01f) sy = 1.0f;
	return fminf(1.0f, fmaxf(fabsf(x) / (128.0f * sx), fabsf(y) / (128.0f * sy)));
}
inline float ManualScale(float input, const Tuning &t)
{
	return fmaxf(0.0f, fminf(1.0f, (t.breakInput - input) / 0.20f));
}

static const float ACQUIRE_CONE_DOT = 0.9925462f; // 7 degrees
static const float RETAIN_CONE_DOT = 0.9848078f; // 10 degrees

inline Tuning ForAimMode(Tuning t, bool assisted)
{
	// Free aim offers a lighter version; Assisted uses the selected strength.
	if (!assisted) { t.friction *= 0.65f; t.rate *= 0.65f; t.maxRate *= 0.65f; }
	return t;
}
inline bool InConeAndRange(float dot, float coneDot, float distanceSquared, float range)
{
	return range > 0.0f && distanceSquared >= 0.0f && distanceSquared <= range * range && dot >= coneDot;
}
inline float BodyHeight(float rayHeight, float pedHeight, bool ducking)
{
	return fmaxf(pedHeight - 0.25f, fminf(pedHeight + (ducking ? 0.3f : 0.7f), rayHeight));
}
inline float ConeWeight(float dot)
{
	// Fade smoothly to zero at the 10-degree retention boundary.
	const float edge = RETAIN_CONE_DOT;
	float weight = (dot - edge) / (1.0f - edge);
	return fmaxf(0.0f, fminf(1.0f, weight));
}
inline float ShortestAngle(float delta)
{
	const float pi = 3.14159265358979323846f;
	delta = fmodf(delta, 2.0f * pi);
	if (delta >= pi) delta -= 2.0f * pi;
	if (delta < -pi) delta += 2.0f * pi;
	return delta;
}
inline float Attraction(float error, float dt, float weight, const Tuning &t)
{
	if (!(dt > 0.0f) || !(weight > 0.0f) || !(t.rate > 0.0f)) return 0.0f;
	if (dt > 0.1f) dt = 0.1f;
	weight = fminf(weight, 1.0f);
	float amount = error * (1.0f - expf(-t.rate * weight * dt));
	float limit = t.maxRate * weight * dt;
	return fmaxf(-limit, fminf(limit, amount));
}
}
#endif
