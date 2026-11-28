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
	// Touch is slightly less aggressive because virtual stick travel is shorter.
	static const Tuning levels[] = {
		{ 0.0f, 0.0f, 0.0f, 0.70f },
		{ 0.12f, 0.8f, 0.20f, 0.70f },
		{ 0.24f, 1.4f, 0.35f, 0.70f },
		{ 0.36f, 2.2f, 0.50f, 0.70f },
	};
	if (strength < 0 || strength > 3) strength = 0;
	Tuning t = levels[strength];
	if (touch) { t.friction *= 0.8f; t.rate *= 0.8f; t.maxRate *= 0.8f; t.breakInput = 0.65f; }
	return t;
}

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
	// Fade smoothly to zero at the 8-degree retention boundary.
	const float edge = 0.9902681f;
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
