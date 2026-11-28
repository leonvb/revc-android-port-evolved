#ifndef REVC_MODERN_VEHICLE_CAMERA_H
#define REVC_MODERN_VEHICLE_CAMERA_H

#include <math.h>

// Small, engine-independent state machine for one follow-camera instance.
// Durations are in game seconds and speed is in the game's handling units.
struct ModernVehicleCameraState
{
	float manualHold;
	float fov;
	float pitch;
	float reverseWiden;
	bool reversing;

	void Reset(float baseFov)
	{
		manualHold = 0.0f;
		fov = baseFov;
		pitch = 0.0f;
		reverseWiden = 0.0f;
		reversing = false;
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
