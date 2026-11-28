#ifndef REVC_MODERN_VEHICLE_CAMERA_H
#define REVC_MODERN_VEHICLE_CAMERA_H

#include <math.h>
#include <cmath>

// Small, engine-independent state machine for one follow-camera instance.
// Durations are in game seconds and speed is in the game's handling units.
struct ModernVehicleCameraState
{
	float manualHold;
	float fov;
	float pitch;
	float reverseWiden;
	bool reversing;

	// The yaw controller owns Beta only during a reverse swing or its return.
	// It is independent of the FOV's shorter reverse hysteresis.
	enum YawPhase { YAW_IDLE, YAW_REVERSE, YAW_RETURN };
	YawPhase yawPhase;
	float yawVelocity; // radians per game second
	int turnSign; // stable tie-breaker at the 180-degree ambiguity

	void Reset(float baseFov)
	{
		manualHold = 0.0f;
		fov = baseFov;
		pitch = 0.0f;
		reverseWiden = 0.0f;
		reversing = false;
		ResetYaw();
	}

	void ResetYaw()
	{
		yawPhase = YAW_IDLE;
		yawVelocity = 0.0f;
		turnSign = 0;
	}

	static float AngleDifference(float target, float current)
	{
		const float pi = 3.14159265358979323846f;
		const float twoPi = 2.0f * pi;
		float delta = fmodf(target - current, twoPi);
		if (delta >= pi) delta -= twoPi;
		if (delta < -pi) delta += twoPi;
		return delta;
	}

	// The caller supplies the actual current orbit and car heading each frame;
	// no entity or camera pointers are retained. False hands control to the
	// original follow/manual camera. Changes use a shortest-angle difference,
	// so wrapping cannot introduce a full turn at +/-pi.
	bool UpdateYaw(float dt, float speed, float forwardHeading, float currentYaw,
	               bool allowed, float &result)
	{
		result = currentYaw;
		if (!allowed || HoldingManualOrbit() || !std::isfinite(speed) ||
		    !std::isfinite(forwardHeading) || !std::isfinite(currentYaw)) {
			ResetYaw();
			return false;
		}
		const float pi = 3.14159265358979323846f;
		if (yawPhase == YAW_IDLE && speed < -0.05f) {
			yawPhase = YAW_REVERSE;
			turnSign = AngleDifference(forwardHeading + pi, currentYaw) < 0.0f ? -1 : 1;
		} else if (yawPhase == YAW_REVERSE && speed > 0.05f) {
			yawPhase = YAW_RETURN;
			turnSign = -turnSign; // unwind rather than make a full revolution
		} else if (yawPhase == YAW_RETURN && speed < -0.05f) {
			yawPhase = YAW_REVERSE;
			turnSign = -turnSign;
		}
		if (yawPhase == YAW_IDLE) return false;

		if (!(dt >= 0.0f)) dt = 0.0f;
		if (dt > 0.1f) dt = 0.1f;
		const float target = forwardHeading + (yawPhase == YAW_REVERSE ? pi : 0.0f);
		const float maxSpeed = yawPhase == YAW_REVERSE ? 7.0f : 10.0f;
		const float acceleration = yawPhase == YAW_REVERSE ? 30.0f : 50.0f;
		const float gain = yawPhase == YAW_REVERSE ? 10.0f : 14.0f;
		// Fixed small substeps keep the bounded acceleration and stopping
		// distance stable at different frame rates and after a stalled frame.
		while (dt > 0.0f) {
			float step = fminf(dt, 1.0f / 120.0f);
			float error = AngleDifference(target, result);
			if (fabsf(error) > pi - 0.05f)
				error = turnSign * fabsf(error);
			float remaining = fmaxf(0.0f, fabsf(error) - fabsf(yawVelocity) * step);
			float desired = fminf(maxSpeed, fminf(sqrtf(2.0f * acceleration * remaining), gain * fabsf(error)));
			if (error < 0.0f) desired = -desired;
			float dv = fmaxf(-acceleration * step, fminf(acceleration * step, desired - yawVelocity));
			float nextVelocity = yawVelocity + dv;
			float change = (yawVelocity + nextVelocity) * 0.5f * step;
			// A moving target or repeated direction change may carry the orbit
			// past its target. Allow the small overshoot instead of snapping its
			// velocity to zero and violating the acceleration limit.
			result += change;
			yawVelocity = nextVelocity;
			dt -= step;
		}
		if (yawPhase == YAW_RETURN && fabsf(AngleDifference(target, result)) < 0.01f && fabsf(yawVelocity) < 0.15f)
			ResetYaw();
		return true;
	}

	void Update(float dt, bool manualInput, float delay, float speed,
	            float maxSpeed, float baseFov, float forwardZ = 0.0f)
	{
		// A stalled frame should not jump the camera or skip a hold period.
		if (!(dt >= 0.0f)) dt = 0.0f;
		if (dt > 0.1f) dt = 0.1f;
		if (!(delay >= 0.0f)) delay = 0.0f;
		if (manualInput)
			manualHold = delay;
		else
			manualHold = fmaxf(0.0f, manualHold - dt);

		float speedRatio = maxSpeed > 0.001f ? fabsf(speed) / maxSpeed : 0.0f;
		if (!(speedRatio >= 0.0f)) speedRatio = 0.0f;
		speedRatio = fminf(speedRatio, 1.0f);
		// Hysteresis avoids flickering between reverse and neutral on a slope.
		if (speed < -0.05f) reversing = true;
		else if (speed > -0.025f) reversing = false;
		const float blend = 1.0f - expf(-3.0f * dt);
		reverseWiden += ((reversing ? 1.0f : 0.0f) - reverseWiden) * blend;
		const float targetFov = baseFov + fminf(10.0f, 10.0f * speedRatio + 4.0f * reverseWiden);
		fov += (targetFov - fov) * blend;

		// Look slightly above/below the vehicle when climbing/descending. This
		// is a transient view direction offset, not an accumulating camera angle.
		if (!(forwardZ >= -1.0f && forwardZ <= 1.0f)) forwardZ = 0.0f;
		float slope = fmaxf(-1.0f, fminf(1.0f, forwardZ * 3.0f));
		float targetPitch = HoldingManualOrbit() ? 0.0f : slope * speedRatio * 0.06981317f;
		pitch += (targetPitch - pitch) * (1.0f - expf(-2.5f * dt));
	}

	bool HoldingManualOrbit() const { return manualHold > 0.0f; }
};

#endif
