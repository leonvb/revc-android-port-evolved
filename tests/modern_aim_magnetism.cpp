#include "../src/core/ModernAimMagnetism.h"
#include <cassert>
#include <cmath>

using namespace ModernAimMagnetism;
static bool near(float a, float b, float tolerance = 0.0001f) { return std::fabs(a - b) <= tolerance; }

static float simulate(int fps, int strength)
{
	Tuning tuning = GetTuning(strength, false);
	float yaw = 0.0f;
	for (int i = 0; i < fps; ++i)
		yaw += Attraction(0.05f - yaw, 1.0f / fps, 1.0f, tuning);
	return yaw;
}

int main()
{
	Tuning off = GetTuning(0, false), low = GetTuning(1, false);
	Tuning med = GetTuning(2, false), high = GetTuning(3, false);
	Tuning touch = GetTuning(2, true);
	assert(off.friction == 0.0f && Attraction(1.0f, 1.0f, 1.0f, off) == 0.0f);
	assert(GetTuning(-1, false).rate == 0.0f && GetTuning(4, false).rate == 0.0f);
	assert(low.friction < med.friction && med.friction < high.friction);
	assert(simulate(60, 1) < simulate(60, 2) && simulate(60, 2) < simulate(60, 3));
	assert(touch.rate < med.rate && touch.friction < med.friction && touch.breakInput < med.breakInput);
	assert(ForAimMode(med, false).rate < ForAimMode(med, true).rate);
	assert(ForAimMode(off, false).rate == 0.0f);
	assert(near(simulate(30, 2), simulate(60, 2), 0.0002f));
	assert(near(simulate(60, 2), simulate(120, 2), 0.0002f));
	assert(std::fabs(Attraction(5.0f, 1.0f, 1.0f, high)) <= high.maxRate * 0.1f + 0.00001f);
	assert(Attraction(1.0f, -1.0f, 1.0f, high) == 0.0f);
	assert(Attraction(1.0f, 0.1f, 0.0f, high) == 0.0f);
	assert(ConeWeight(0.9902681f) == 0.0f && ConeWeight(1.0f) == 1.0f);
	const float pi = 3.14159265358979323846f;
	assert(near(ShortestAngle((-pi + 0.01f) - (pi - 0.01f)), 0.02f));
	assert(near(BodyHeight(10.6f, 10.0f, false), 10.6f)); // keep a head-level aim
	assert(near(BodyHeight(12.0f, 10.0f, false), 10.7f));
	assert(near(BodyHeight(12.0f, 10.0f, true), 10.3f));
	assert(InConeAndRange(0.998f, 0.996f, 100.0f, 10.0f));
	assert(!InConeAndRange(0.995f, 0.996f, 100.0f, 10.0f));
	assert(!InConeAndRange(0.998f, 0.996f, 101.0f, 10.0f));
	// Scoring chooses only the highest dot among geometrically eligible and
	// visible candidates. The real scan supplies its current best as coneDot.
	float best = 0.9961947f;
	assert(InConeAndRange(0.998f, best, 25.0f, 20.0f)); best = 0.998f;
	assert(!InConeAndRange(0.997f, best, 25.0f, 20.0f));
	assert(InConeAndRange(0.999f, best, 25.0f, 20.0f));

	State s; s.Reset();
	assert(!s.BeginFrame(false, false, 0.016f) && s.CanAcquire());
	assert(s.BeginFrame(true, false, 0.016f) && s.CanAcquire());
	s.Scanned();
	assert(!s.CanAcquire());
	assert(s.BeginFrame(true, false, 0.1f) && !s.CanAcquire());
	assert(s.BeginFrame(true, false, 0.03f) && s.CanAcquire());
	s.Acquire(0x105);
	assert(s.Visible(true, true, 0.016f) && !s.CanAcquire());
	assert(!s.Visible(true, false, 0.1f) && s.target == 0x105); // grace; no attraction
	assert(s.Visible(true, true, 0.016f) && s.occluded == 0.0f);
	assert(!s.Visible(true, false, 0.1f));
	assert(!s.Visible(true, false, 0.1f) && s.target == -1 && !s.CanAcquire());
	// Never hop to a new target without a deliberate input/re-aim.
	assert(s.BeginFrame(true, false, 0.016f) && !s.CanAcquire());
	assert(!s.BeginFrame(true, true, 0.016f) && s.target == -1);
	assert(!s.BeginFrame(true, false, 0.1f));
	assert(!s.BeginFrame(true, false, 0.1f));
	assert(s.BeginFrame(true, false, 0.1f) && s.CanAcquire());
	s.Acquire(0x106);
	assert(!s.Visible(false, true, 0.016f) && !s.CanAcquire()); // deleted / invalid
	assert(!s.BeginFrame(false, false, 0.016f) && s.CanAcquire()); // stop aim/reset
	s.Acquire(0x107);
	assert(!s.BeginFrame(true, true, 0.016f)); // manual input overrides immediately
	assert(s.target == -1 && s.overrideCooldown > 0.0f);
	s.Reset();
	assert(s.target == -1 && !s.lockedOut && s.overrideCooldown == 0.0f);
}
