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

// Exercise the same yaw controller used by CCam, not just the FOV state.
static void testYawTrajectory(int fps)
{
	const float pi = 3.14159265358979323846f;
	const float dt = 1.0f / fps;
	ModernVehicleCameraState state;
	state.Reset(70.0f);
	float yaw = 0.0f, out = 0.0f, previousRate = 0.0f;
	assert(!state.UpdateYaw(dt, 0.0f, 0.0f, yaw, true, out));
	float entry = 0.0f, exit = 0.0f;
	for (int i = 0; i < fps * 2; ++i) {
		assert(state.UpdateYaw(dt, -0.1f, 0.0f, yaw, true, out));
		float rate = (out - yaw) / dt;
		assert(std::fabs(rate) <= 7.01f);
		assert(std::fabs(rate - previousRate) <= 30.0f * dt + 0.03f);
		assert(rate <= 0.001f); // deterministic 180-degree tie
		previousRate = rate; yaw = out;
		if (!entry && std::fabs(ModernVehicleCameraState::AngleDifference(pi, yaw)) < 0.1f)
			entry = (i + 1.0f) * dt;
	}
	assert(entry >= 0.5f && entry <= 0.8f);
	assert(state.yawPhase == ModernVehicleCameraState::YAW_REVERSE);
	// Neutral/hill oscillation does not start forward return.
	for (int i = 0; i < fps; ++i) {
		assert(state.UpdateYaw(dt, i % 2 ? 0.04f : -0.04f, 0.0f, yaw, true, out));
		yaw = out;
		assert(state.yawPhase == ModernVehicleCameraState::YAW_REVERSE);
	}
	previousRate = 0.0f;
	for (int i = 0; i < fps * 2; ++i) {
		bool owned = state.UpdateYaw(dt, 0.1f, 0.0f, yaw, true, out);
		float rate = (out - yaw) / dt;
		assert(std::fabs(rate) <= 10.01f);
		assert(std::fabs(rate - previousRate) <= 50.0f * dt + 0.06f);
		assert(rate >= -0.001f); // unwind, not another 180 degrees same way
		previousRate = rate; yaw = out;
		if (!exit && std::fabs(ModernVehicleCameraState::AngleDifference(0.0f, yaw)) < 0.1f)
			exit = (i + 1.0f) * dt;
		if (!owned) break;
	}
	assert(exit >= 0.35f && exit <= 0.6f);
	assert(state.yawPhase == ModernVehicleCameraState::YAW_IDLE);
	assert(std::fabs(ModernVehicleCameraState::AngleDifference(0.0f, yaw)) < 0.02f);
}

static void testYawInterruptions()
{
	const float pi = 3.14159265358979323846f;
	ModernVehicleCameraState state;
	state.Reset(70.0f);
	float yaw = pi - 0.01f, out;
	// Wrap at +/-pi takes the short 0.02 rad path, not a 2pi turn.
	assert(std::fabs(ModernVehicleCameraState::AngleDifference(-pi + 0.01f, yaw) - 0.02f) < 0.0001f);
	assert(state.UpdateYaw(1.0f / 60, -0.1f, yaw, yaw, true, out));
	float before = out;
	// Repeated direction changes decelerate rather than teleporting.
	assert(state.UpdateYaw(1.0f / 60, 0.1f, yaw, before, true, out));
	assert(std::fabs(out - before) < 0.2f);
	before = out;
	assert(state.UpdateYaw(1.0f / 60, -0.1f, yaw, before, true, out));
	assert(std::fabs(out - before) < 0.2f);
	// Manual orbit immediately cancels the controller and its old velocity.
	state.Update(1.0f / 60, true, 0.5f, -0.1f, 1.0f, 70.0f);
	assert(!state.UpdateYaw(1.0f / 60, -0.1f, yaw, out, true, before));
	assert(before == out && state.yawVelocity == 0.0f);
	assert(state.yawPhase == ModernVehicleCameraState::YAW_IDLE);
	for (int i = 0; i < 31; ++i)
		state.Update(1.0f / 60, false, 0.5f, -0.1f, 1.0f, 70.0f);
	assert(state.UpdateYaw(1.0f / 60, -0.1f, yaw, out, true, before));
	assert(!state.UpdateYaw(1.0f / 60, -0.1f, yaw, before, false, out));
	assert(state.yawPhase == ModernVehicleCameraState::YAW_IDLE);
	// A long frame is capped; negative and NaN dt do not advance yaw.
	assert(state.UpdateYaw(1.0f, -0.1f, 0.0f, 0.0f, true, out));
	assert(std::fabs(out) <= 0.151f);
	before = out;
	assert(state.UpdateYaw(-1.0f, -0.1f, 0.0f, before, true, out));
	assert(out == before);
	assert(state.UpdateYaw(std::nanf(""), -0.1f, 0.0f, before, true, out));
	assert(out == before);
	assert(!state.UpdateYaw(0.01f, std::nanf(""), 0.0f, before, true, out));
	state.Reset(70.0f);
	assert(state.yawPhase == ModernVehicleCameraState::YAW_IDLE && state.yawVelocity == 0.0f);
}

static void testYawWrapAndSwitches(int fps)
{
	const float pi = 3.14159265358979323846f;
	const float dt = 1.0f / fps;
	ModernVehicleCameraState state, other;
	state.Reset(70.0f); other.Reset(70.0f);
	float yaw = pi - 0.01f, out = yaw, lastRate = 0.0f;
	for (int i = 0; i < fps * 3; ++i) {
		float heading = i % 2 ? pi - 0.01f : -pi + 0.01f;
		// Toggle direction before the previous swing has finished.
		float speed = (i / (fps / 2)) % 2 ? 0.1f : -0.1f;
		assert(state.UpdateYaw(dt, speed, heading, yaw, true, out));
		float rate = ModernVehicleCameraState::AngleDifference(out, yaw) / dt;
		assert(std::fabs(rate) <= 10.01f);
		assert(std::fabs(rate - lastRate) <= 50.0f * dt + 0.08f);
		lastRate = rate;
		// Simulate CCam's normalization each frame.
		yaw = ModernVehicleCameraState::AngleDifference(out, 0.0f);
		assert(other.yawPhase == ModernVehicleCameraState::YAW_IDLE && other.yawVelocity == 0.0f);
	}
}

static void testMovingTargetAcceleration()
{
	const float pi = 3.14159265358979323846f, dt = 1.0f / 120.0f;
	ModernVehicleCameraState state;
	state.Reset(70.0f);
	float yaw = 0.0f, out;
	for (int i = 0; i < 35; ++i) {
		assert(state.UpdateYaw(dt, -0.1f, 0.0f, yaw, true, out));
		yaw = out;
	}
	assert(std::fabs(state.yawVelocity) > 4.0f);
	// A sudden vehicle turn places the new target immediately across the
	// camera's path. Angular momentum must not be snapped to zero on crossing.
	float heading = yaw - pi - 0.01f;
	for (int i = 0; i < 120; ++i) {
		float oldVelocity = state.yawVelocity;
		assert(state.UpdateYaw(dt, -0.1f, heading, yaw, true, out));
		assert(std::fabs(state.yawVelocity - oldVelocity) <= 30.0f * dt + 0.0001f);
		yaw = out;
	}
}

static float sampleYaw(int fps, float seconds)
{
	ModernVehicleCameraState state;
	state.Reset(70.0f);
	float yaw = 0.0f, out = yaw;
	for (int i = 0; i < int(fps * seconds + 0.5f); ++i) {
		state.UpdateYaw(1.0f / fps, -0.1f, 0.0f, yaw, true, out);
		yaw = out;
	}
	return yaw;
}

int main()
{
	testRate(30); testRate(60); testRate(120);
	testYawTrajectory(30); testYawTrajectory(60); testYawTrajectory(120);
	testYawInterruptions();
	testMovingTargetAcceleration();
	testYawWrapAndSwitches(30); testYawWrapAndSwitches(60); testYawWrapAndSwitches(120);
	assert(near(sampleYaw(30, 0.4f), sampleYaw(60, 0.4f), 0.02f));
	assert(near(sampleYaw(60, 0.4f), sampleYaw(120, 0.4f), 0.02f));
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
