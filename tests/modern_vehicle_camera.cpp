#include "../src/core/ModernVehicleCamera.h"
#include <cassert>
#include <cmath>

static bool near(float a, float b, float tolerance = 0.01f)
{
	return std::fabs(a - b) < tolerance;
}

static void testRate(int fps)
{
	ModernVehicleCameraState state;
	state.Reset(70.0f);
	state.Update(1.0f / fps, true, 1.75f, 0.0f, 1.0f, 70.0f);
	for (int i = 0; i < fps; ++i)
		state.Update(1.0f / fps, false, 1.75f, 1.0f, 1.0f, 70.0f);
	assert(state.HoldingManualOrbit());
	assert(near(state.manualHold, 0.75f, 0.002f));
	assert(near(state.fov, 70.0f + 10.0f * (1.0f - std::exp(-3.0f)), 0.02f));
	for (int i = 0; i < fps; ++i)
		state.Update(1.0f / fps, false, 1.75f, 1.0f, 1.0f, 70.0f);
	assert(!state.HoldingManualOrbit());
	assert(state.fov < 80.0f && state.fov > 79.9f);
	state.Update(1.0f / fps, true, 1.75f, -2.0f, 1.0f, 70.0f);
	assert(near(state.manualHold, 1.75f));
}

static void testReverseAndPitch(int fps)
{
	ModernVehicleCameraState state;
	state.Reset(70.0f);
	for (int i = 0; i < fps * 3; ++i)
		state.Update(1.0f / fps, false, 1.75f, -0.06f, 1.0f, 70.0f, 0.25f);
	assert(state.reversing && state.reverseWiden > 0.99f);
	assert(state.fov > 74.5f && state.fov < 75.0f);
	assert(state.pitch > 0.002f && state.pitch < 0.069814f);
	state.Update(1.0f / fps, false, 1.75f, -0.04f, 1.0f, 70.0f);
	assert(state.reversing); // deadband
	state.Update(1.0f / fps, false, 1.75f, -0.01f, 1.0f, 70.0f);
	assert(!state.reversing && state.reverseWiden > 0.8f); // no abrupt zoom
	for (int i = 0; i < fps * 3; ++i)
		state.Update(1.0f / fps, false, 1.75f, 1.0f, 1.0f, 70.0f, -0.5f);
	assert(state.reverseWiden < 0.001f);
	assert(state.pitch < -0.06f && state.pitch >= -0.069814f);
	state.Update(1.0f / fps, true, 1.75f, 1.0f, 1.0f, 70.0f, 1.0f);
	assert(state.HoldingManualOrbit());
	assert(state.pitch < 0.0f); // eases back rather than snapping
}

int main()
{
	testRate(30); testRate(60); testRate(120);
	testReverseAndPitch(30); testReverseAndPitch(60); testReverseAndPitch(120);
	ModernVehicleCameraState state;
	state.Reset(70.0f);
	state.Update(1.0f, true, 2.0f, 0.0f, 0.0f, 70.0f);
	assert(near(state.manualHold, 2.0f));
	assert(near(state.fov, 70.0f));
	state.Update(-1.0f, false, 2.0f, 0.0f, 0.0f, 70.0f);
	assert(near(state.manualHold, 2.0f));
	state.Update(1.0f, false, 2.0f, 0.0f, 0.0f, 70.0f);
	assert(near(state.manualHold, 1.9f));
	state.Reset(70.0f);
	assert(!state.HoldingManualOrbit() && near(state.fov, 70.0f));
}
