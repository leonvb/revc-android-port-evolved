#include "common.h"

#ifdef MODERN_FEATURES

#include "ModernVehiclePhysics.h"
#include "ModernConfig.h"
#include "Timer.h"
#include "Pools.h"
#include "Pad.h"
#include "Camera.h"
#include "Vehicle.h"
#include "Automobile.h"
#include "Bike.h"

tModernVehicleRuntimeState CModernVehiclePhysics::ms_aStates[NUMVEHICLES];

void
tModernVehicleRuntimeState::Reset(CVehicle *veh, uint8 genId)
{
	pVehicle = veh;
	nPoolGenId = genId;
	bInitialized = false;
	vecPrevMoveSpeed = CVector(0.0f, 0.0f, 0.0f);
	fSmoothedLongAccel = 0.0f;
	fSmoothedLatAccel = 0.0f;
	fFrontLoadTransfer = 0.0f;
	fRightLoadTransfer = 0.0f;
	fAirborneTimeSec = 0.0f;
	fParticleAccum = 0.0f;
	fFireParticleAccum = 0.0f;
	fBoatSplashAccum = 0.0f;
	fBoatDropAccum = 0.0f;
	fEngineDamageAccum = 0.0f;
}

void
CModernVehiclePhysics::ResetAll()
{
	for (int32 i = 0; i < NUMVEHICLES; i++)
		ms_aStates[i].Reset(nil, 0);
}

void
CModernVehiclePhysics::ResetVehicle(CVehicle *pVehicle)
{
	if (!pVehicle || !CPools::GetVehiclePool())
		return;
	int32 idx = CPools::GetVehiclePool()->GetJustIndex_NoFreeAssert(pVehicle);
	if (idx < 0 || idx >= NUMVEHICLES)
		return;
	uint8 genId = (uint8)CPools::GetVehiclePool()->GetId(idx);
	ms_aStates[idx].Reset(pVehicle, genId);
}

tModernVehicleRuntimeState *
CModernVehiclePhysics::GetState(CVehicle *pVehicle)
{
	if (!pVehicle || !CPools::GetVehiclePool())
		return nil;
	int32 idx = CPools::GetVehiclePool()->GetJustIndex_NoFreeAssert(pVehicle);
	if (idx < 0 || idx >= NUMVEHICLES)
		return nil;
	if (CPools::GetVehiclePool()->GetIsFree(idx))
		return nil;

	uint8 genId = (uint8)CPools::GetVehiclePool()->GetId(idx);
	tModernVehicleRuntimeState &st = ms_aStates[idx];
	if (st.pVehicle != pVehicle || st.nPoolGenId != genId) {
		st.Reset(pVehicle, genId);
	}
	return &st;
}

static void
UpdateVehicleWeightAndAirborne(CVehicle *pVehicle, tModernVehicleRuntimeState *st, bool bGrounded)
{
	if (!st)
		return;

	const float dtTicks = Max(CTimer::GetTimeStep(), 0.001f);
	const float dtSec = CTimer::GetTimeStepInSeconds();

	if (!st->bInitialized) {
		st->vecPrevMoveSpeed = pVehicle->GetMoveSpeed();
		st->bInitialized = true;
	}

	// Acceleration in GTA world-units per (1/50s)^2; GRAVITY is 0.008f (= 1g)
	CVector deltaV = pVehicle->GetMoveSpeed() - st->vecPrevMoveSpeed;
	st->vecPrevMoveSpeed = pVehicle->GetMoveSpeed();

	float rawLongG = DotProduct(deltaV, pVehicle->GetForward()) / (dtTicks * GRAVITY);
	float rawLatG  = DotProduct(deltaV, pVehicle->GetRight()) / (dtTicks * GRAVITY);
	rawLongG = Clamp(rawLongG, -2.5f, 2.5f);
	rawLatG  = Clamp(rawLatG,  -2.5f, 2.5f);

	st->fSmoothedLongAccel = CTimer::ExpSmooth(st->fSmoothedLongAccel, rawLongG, 0.25f);
	st->fSmoothedLatAccel  = CTimer::ExpSmooth(st->fSmoothedLatAccel,  rawLatG,  0.25f);

	if (bGrounded) {
		st->fAirborneTimeSec = 0.0f;

		const CVector &dim = pVehicle->pHandling->Dimension;
		float wheelbase = Max(dim.y * 0.65f, 1.4f);
		float trackWidth = Max(dim.x * 0.80f, 0.9f);
		float cgHeight = Clamp(dim.z * 0.35f + Max(pVehicle->pHandling->CentreOfMass.z, -0.25f) * 0.35f, 0.30f, 1.15f);

		// Braking (rawLongG < 0) shifts weight forward (+fFrontLoadTransfer);
		// Accelerating (rawLongG > 0) shifts weight rearward (-fFrontLoadTransfer).
		float targetFront = Clamp(-st->fSmoothedLongAccel * (cgHeight / wheelbase) * 0.85f, -0.28f, 0.28f);
		// Left turn (centripetal accel along -Right, rawLatG < 0) loads outside right wheels (+fRightLoadTransfer).
		float targetRight = Clamp(-st->fSmoothedLatAccel  * (cgHeight / trackWidth) * 0.65f, -0.26f, 0.26f);

		st->fFrontLoadTransfer = CTimer::ExpSmooth(st->fFrontLoadTransfer, targetFront, 0.30f);
		st->fRightLoadTransfer = CTimer::ExpSmooth(st->fRightLoadTransfer, targetRight, 0.30f);
	} else {
		st->fAirborneTimeSec += dtSec;
		st->fFrontLoadTransfer = CTimer::ExpSmooth(st->fFrontLoadTransfer, 0.0f, 0.20f);
		st->fRightLoadTransfer = CTimer::ExpSmooth(st->fRightLoadTransfer, 0.0f, 0.20f);
	}
}

void
CModernVehiclePhysics::UpdateCarPreWheel(CAutomobile *pCar, int32 nWheelsOnGround)
{
	tModernVehicleRuntimeState *st = GetState(pCar);
	if (!st)
		return;
	bool bGrounded = (nWheelsOnGround > 0) || pCar->bIsInWater || pCar->bTouchingWater;
	UpdateVehicleWeightAndAirborne(pCar, st, bGrounded);
}

void
CModernVehiclePhysics::UpdateBikePreWheel(CBike *pBike, int32 nWheelsOnGround)
{
	tModernVehicleRuntimeState *st = GetState(pBike);
	if (!st)
		return;
	bool bGrounded = (nWheelsOnGround > 0) || pBike->bIsInWater || pBike->bTouchingWater;
	UpdateVehicleWeightAndAirborne(pBike, st, bGrounded);
}

float
CModernVehiclePhysics::GetCarWheelLoadScale(CAutomobile *pCar, int8 wheelId)
{
	if (!CModernConfig::IsModernDrivingEnabled() || pCar->GetStatus() != STATUS_PLAYER)
		return 1.0f;

	tModernVehicleRuntimeState *st = GetState(pCar);
	if (!st)
		return 1.0f;

	const bool bFront = (wheelId == CARWHEEL_FRONT_LEFT || wheelId == CARWHEEL_FRONT_RIGHT);
	const bool bRight = (wheelId == CARWHEEL_FRONT_RIGHT || wheelId == CARWHEEL_REAR_RIGHT);

	float longMult = bFront ? (1.0f + st->fFrontLoadTransfer) : (1.0f - st->fFrontLoadTransfer);
	float latMult  = bRight ? (1.0f + st->fRightLoadTransfer) : (1.0f - st->fRightLoadTransfer);

	// Sub-linear tire load sensitivity (real pneumatic tires gain grip with load, with gentle saturation)
	float combined = Clamp(longMult * latMult, 0.55f, 1.45f);
	return 1.0f + (combined - 1.0f) * 0.75f;
}

float
CModernVehiclePhysics::GetBikeWheelLoadScale(CBike *pBike, int8 wheelId)
{
	if (!CModernConfig::IsModernDrivingEnabled() || pBike->GetStatus() != STATUS_PLAYER)
		return 1.0f;

	tModernVehicleRuntimeState *st = GetState(pBike);
	if (!st)
		return 1.0f;

	float longMult = (wheelId == BIKEWHEEL_FRONT)
		? (1.0f + st->fFrontLoadTransfer * 0.85f)
		: (1.0f - st->fFrontLoadTransfer * 0.85f);
	return Clamp(longMult, 0.65f, 1.35f);
}

float
CModernVehiclePhysics::ComputeModernLateralForce(
	CVehicle *pVehicle,
	float rawRight,
	float contactSpeedFwd,
	float contactSpeedRight,
	float adhesion,
	bool bAlreadySkidding,
	bool bIsRearWheel)
{
	if (!CModernConfig::IsModernDrivingEnabled() ||
	    !pVehicle ||
	    pVehicle->GetStatus() != STATUS_PLAYER ||
	    adhesion <= 0.00001f ||
	    rawRight == 0.0f) {
		return rawRight;
	}

	const float absFwd = Abs(contactSpeedFwd);
	// At parking / low speeds, keep crisp rigid kinematic response so tight turns and 3-point turns stay sharp
	if (absFwd < 0.08f)
		return rawRight;

	// Slip angle alpha in radians
	float slipAngle = Atan2(Abs(contactSpeedRight), Max(absFwd, 0.05f));

	// Pacejka-lite magic formula: sin(C * atan(B * alpha - E * (B * alpha - atan(B * alpha))))
	// Peak lateral grip around ~10-12 degrees, progressive post-peak plateau (~86% of peak) instead of abrupt binary snap
	const float B = bIsRearWheel ? 6.8f : 7.4f;
	const float C = 1.35f;
	const float E = 0.35f;
	float bx = B * slipAngle;
	float pacejka = Sin(C * Atan(bx - E * (bx - Atan(bx))));
	pacejka = Clamp(pacejka, 0.0f, 1.0f);

	// Blend between Classic linear response at small slip and Pacejka envelope at medium/high slip
	float targetMag = adhesion * pacejka * 1.06f;
	if (bAlreadySkidding) {
		// Smoothly soften classic binary fTractionLoss cliff so counter-steering recovery is progressive
		float recoveryBoost = lerp(0.35f, 1.0f, 1.0f / Max(pVehicle->pHandling->fTractionLoss, 0.55f));
		targetMag *= recoveryBoost;
	}

	float rawMag = Abs(rawRight);
	float shapedMag = Min(rawMag, Max(targetMag, rawMag * 0.68f));
	return (rawRight > 0.0f) ? shapedMag : -shapedMag;
}

static void
ReadPlayerAirControlInputs(float &outPitch, float &outRoll)
{
	CPad *pad = CPad::GetPad(0);
	if (!pad || pad->ArePlayerControlsDisabled()) {
		outPitch = 0.0f;
		outRoll = 0.0f;
		return;
	}

	outRoll = Clamp(pad->GetSteeringLeftRight() / 128.0f, -1.0f, 1.0f);

	float ud = pad->GetSteeringUpDown() / 128.0f;
#ifdef FREE_CAM
	if (!CCamera::bFreeCam || (CCamera::bFreeCam && !CPad::IsAffectedByController))
#endif
	{
		if (Abs(pad->GetCarGunUpDown()) > 1.0f && Abs(ud) < 0.05f)
			ud = -pad->GetCarGunUpDown() / 128.0f;
	}
	outPitch = Clamp(ud, -1.0f, 1.0f);

	// Apply small deadzone so neutral sticks never drift vehicle attitude in mid-air
	if (Abs(outRoll) < 0.08f)
		outRoll = 0.0f;
	if (Abs(outPitch) < 0.08f)
		outPitch = 0.0f;
}

void
CModernVehiclePhysics::ProcessCarMidAirControl(CAutomobile *pCar)
{
	if (!CModernConfig::IsMidAirControlEnabled() || !pCar)
		return;
	if (pCar->GetStatus() != STATUS_PLAYER)
		return;
	if (pCar->IsRealHeli() || pCar->IsRealPlane() || pCar->GetModelIndex() == MI_DODO || pCar->GetModelIndex() == MI_RCBARON || CVehicle::bAllDodosCheat)
		return;
	if (pCar->m_nWheelsOnGround > 0 || pCar->bIsInWater || pCar->bTouchingWater || pCar->bHasContacted || pCar->bHasHitWall)
		return;

	tModernVehicleRuntimeState *st = GetState(pCar);
	if (!st || st->fAirborneTimeSec < 0.15f)
		return;

	float pitchInput = 0.0f, rollInput = 0.0f;
	ReadPlayerAirControlInputs(pitchInput, rollInput);
	if (pitchInput == 0.0f && rollInput == 0.0f)
		return;

	const float step = CTimer::GetClampedTimeStep(3.0f);
	CVector localTurn = Multiply3x3(pCar->GetTurnSpeed(), pCar->GetMatrix());
	const float maxPitchRate = 0.055f;
	const float maxRollRate  = 0.065f;

	CVector worldCOM = Multiply3x3(pCar->GetMatrix(), pCar->m_vecCentreOfMass);

	// Pitch around local X (GetRight): ApplyTurnForce(+Up, COM + Forward) produces +X angular impulse (nose up)
	// Standard GTA convention: SteeringUpDown < 0 is push-up (nose down), > 0 is pull-back (nose up)
	if (pitchInput != 0.0f && (pitchInput * localTurn.x) < maxPitchRate) {
		float pitchAuthority = 0.0011f * pCar->m_fTurnMass * step * pitchInput;
		pCar->ApplyTurnForce(pitchAuthority * pCar->GetUp(), worldCOM + pCar->GetForward());
	}

	// Roll around local Y (GetForward): steering right (rollInput > 0) rolls right side down
	if (rollInput != 0.0f && (rollInput * localTurn.y) < maxRollRate) {
		float rollAuthority = 0.0013f * pCar->m_fTurnMass * step * rollInput;
		pCar->ApplyTurnForce(-rollAuthority * pCar->GetUp(), worldCOM + pCar->GetRight());
	}
}

void
CModernVehiclePhysics::ProcessBikeMidAirControl(CBike *pBike)
{
	if (!CModernConfig::IsMidAirControlEnabled() || !pBike)
		return;
	if (pBike->GetStatus() != STATUS_PLAYER)
		return;
	if (pBike->m_nWheelsOnGround > 0 || pBike->bIsInWater || pBike->bTouchingWater || pBike->bHasContacted || pBike->bHasHitWall)
		return;

	tModernVehicleRuntimeState *st = GetState(pBike);
	if (!st || st->fAirborneTimeSec < 0.15f)
		return;

	float pitchInput = 0.0f, rollInput = 0.0f;
	ReadPlayerAirControlInputs(pitchInput, rollInput);
	if (rollInput == 0.0f && pitchInput == 0.0f)
		return;

	const float step = CTimer::GetClampedTimeStep(3.0f);
	CVector localTurn = Multiply3x3(pBike->GetTurnSpeed(), pBike->GetMatrix());
	CVector worldCOM = Multiply3x3(pBike->GetMatrix(), pBike->m_vecCentreOfMass);

	// Bikes already have rider lean pitch torque in CBike::ProcessControl; add gentle mid-air roll & supplemental pitch
	if (rollInput != 0.0f && (rollInput * localTurn.y) < 0.060f) {
		float rollAuthority = 0.0009f * pBike->m_fTurnMass * step * rollInput;
		pBike->ApplyTurnForce(-rollAuthority * pBike->GetUp(), worldCOM + pBike->GetRight());
	}
	if (pitchInput != 0.0f && (pitchInput * localTurn.x) < 0.055f) {
		float pitchAuthority = 0.00045f * pBike->m_fTurnMass * step * pitchInput;
		pBike->ApplyTurnForce(pitchAuthority * pBike->GetUp(), worldCOM + pBike->GetForward());
	}
}

static int32
Consume30HzTicks(float &accum)
{
	accum += CTimer::GetTimeStepRatio30();
	if (accum < 1.0f)
		return 0;
	int32 ticks = (int32)accum;
	accum -= (float)ticks;
	if (ticks > 2)
		ticks = 2;
	return ticks;
}

int32
CModernVehiclePhysics::ConsumeWheelParticleTicks(CVehicle *pVehicle)
{
	if (!CModernConfig::IsHighFpsPhysicsFixEnabled())
		return 1;
	tModernVehicleRuntimeState *st = GetState(pVehicle);
	if (!st)
		return 1;
	return Consume30HzTicks(st->fParticleAccum);
}

int32
CModernVehiclePhysics::ConsumeFireParticleTicks(CVehicle *pVehicle)
{
	if (!CModernConfig::IsHighFpsPhysicsFixEnabled())
		return 1;
	tModernVehicleRuntimeState *st = GetState(pVehicle);
	if (!st)
		return 1;
	return Consume30HzTicks(st->fFireParticleAccum);
}

int32
CModernVehiclePhysics::ConsumeBoatSplashTicks(CVehicle *pVehicle)
{
	if (!CModernConfig::IsHighFpsPhysicsFixEnabled())
		return 1;
	tModernVehicleRuntimeState *st = GetState(pVehicle);
	if (!st)
		return 1;
	return Consume30HzTicks(st->fBoatSplashAccum);
}

int32
CModernVehiclePhysics::ConsumeBoatWaterDropTicks(CVehicle *pVehicle)
{
	if (!CModernConfig::IsHighFpsPhysicsFixEnabled())
		return 1;
	tModernVehicleRuntimeState *st = GetState(pVehicle);
	if (!st)
		return 1;
	return Consume30HzTicks(st->fBoatDropAccum);
}

float
CModernVehiclePhysics::ConsumeEngineDamageStep(CVehicle *pVehicle, float per30FpsDamage)
{
	if (!CModernConfig::IsHighFpsPhysicsFixEnabled())
		return per30FpsDamage;
	tModernVehicleRuntimeState *st = GetState(pVehicle);
	if (!st)
		return per30FpsDamage * CTimer::GetTimeStepRatio30();
	st->fEngineDamageAccum += per30FpsDamage * CTimer::GetTimeStepRatio30();
	float dmg = st->fEngineDamageAccum;
	st->fEngineDamageAccum = 0.0f;
	return dmg;
}

#endif // MODERN_FEATURES
