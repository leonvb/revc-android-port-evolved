#ifndef __GTA_MODERN_VEHICLE_PHYSICS_H__
#define __GTA_MODERN_VEHICLE_PHYSICS_H__

#include "common.h"

#ifdef MODERN_FEATURES

class CVehicle;
class CAutomobile;
class CBike;

struct tModernVehicleRuntimeState
{
	CVehicle *pVehicle;
	uint8 nPoolGenId;
	bool bInitialized;

	// Velocity & weight transfer tracking (world / chassis space)
	CVector vecPrevMoveSpeed;
	float fSmoothedLongAccel;
	float fSmoothedLatAccel;
	float fFrontLoadTransfer;  // [-0.30, +0.30] normalized axle load delta (+ = nose-down under braking)
	float fRightLoadTransfer;  // [-0.30, +0.30] normalized lateral load delta (+ = outside right wheels loaded)

	// Airborne timer for M1.2 Mid-Air Vehicle Control
	float fAirborneTimeSec;

	// Fractional accumulators for frame-rate-independent VFX / sleep counters (M1.1)
	float fParticleAccum;
	float fFireParticleAccum;
	float fBoatSplashAccum;
	float fBoatDropAccum;
	float fEngineDamageAccum;

	void Reset(CVehicle *veh = nil, uint8 genId = 0);
};

class CModernVehiclePhysics
{
	static tModernVehicleRuntimeState ms_aStates[NUMVEHICLES];

public:
	static void ResetAll();
	static void ResetVehicle(CVehicle *pVehicle);
	static tModernVehicleRuntimeState *GetState(CVehicle *pVehicle);

	// Called once per ProcessControl tick before wheel processing to update load transfer & airborne timers
	static void UpdateCarPreWheel(CAutomobile *pCar, int32 nWheelsOnGround);
	static void UpdateBikePreWheel(CBike *pBike, int32 nWheelsOnGround);

	// Dynamic normal-load multiplier for individual wheels in Modern Driving mode
	static float GetCarWheelLoadScale(CAutomobile *pCar, int8 wheelId);
	static float GetBikeWheelLoadScale(CBike *pBike, int8 wheelId);

	// Modern Pacejka-lite progressive lateral tire grip helper
	static float ComputeModernLateralForce(
		CVehicle *pVehicle,
		float rawRight,
		float contactSpeedFwd,
		float contactSpeedRight,
		float adhesion,
		bool bAlreadySkidding,
		bool bIsRearWheel);

	// M1.2 Optional Mid-Air Vehicle Control (>150 ms airborne pitch/roll torque)
	static void ProcessCarMidAirControl(CAutomobile *pCar);
	static void ProcessBikeMidAirControl(CBike *pBike);

	// M1.1 Frame-rate-independent 30 Hz emission gates
	static int32 ConsumeWheelParticleTicks(CVehicle *pVehicle);
	static int32 ConsumeFireParticleTicks(CVehicle *pVehicle);
	static int32 ConsumeBoatSplashTicks(CVehicle *pVehicle);
	static int32 ConsumeBoatWaterDropTicks(CVehicle *pVehicle);
	static float ConsumeEngineDamageStep(CVehicle *pVehicle, float per30FpsDamage);
};

#endif // MODERN_FEATURES

#endif // __GTA_MODERN_VEHICLE_PHYSICS_H__
