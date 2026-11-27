#include "common.h"
#include "ModernConfig.h"

#ifdef MODERN_FEATURES

#include "Frontend.h"
#include "skeleton.h"

#if defined ANDROID
#include "JavaWrapper.h"
#endif

int8 CModernConfig::m_nPreset = MODERN_PRESET_CLASSIC;
int8 CModernConfig::m_bMasterEnable = 0;

int8 CModernConfig::m_nTargetFPS = MODERN_FPS_30;

int8 CModernConfig::m_bHighFpsPhysicsFix = 0;
int8 CModernConfig::m_nDrivingStyle = MODERN_DRIVING_CLASSIC;
int8 CModernConfig::m_bMidAirControl = 0;

int8 CModernConfig::m_nGpsRouteMode = MODERN_GPS_OFF;
int8 CModernConfig::m_bAutosave = 0;
int8 CModernConfig::m_bFastInteriorFades = 0;

int8 CModernConfig::m_bModernVehicleCam = 0;
int8 CModernConfig::m_nAimMode = MODERN_AIM_CLASSIC;
int8 CModernConfig::m_bRightShoulder = 1;
int8 CModernConfig::m_bWeaponRadioWheels = 0;
float CModernConfig::m_fVehCamAutoCenterDelay = 1.75f;

int8 CModernConfig::m_nHapticsIntensity = MODERN_HAPTICS_OFF;
int8 CModernConfig::m_nGyroAimMode = MODERN_GYRO_OFF;
float CModernConfig::m_fGyroSensitivity = 1.0f;

int8 CModernConfig::m_bHoldToSprint = 0;
int8 CModernConfig::m_bSwimming = 0;
int8 CModernConfig::m_bLedgeMantling = 0;
float CModernConfig::m_fMaxMantleHeight = 2.15f;

float CModernConfig::m_fLodDistMultiplier = 1.0f;
int8 CModernConfig::m_nNeonBloom = 0;
int8 CModernConfig::m_bFxaa = 0;
int8 CModernConfig::m_bDynamicPointLights = 0;
int8 CModernConfig::m_bWetRoadReflections = 0;
int8 CModernConfig::m_nRealtimeShadows = 0;

void
CModernConfig::ResetToClassicDefaults()
{
	m_nPreset = MODERN_PRESET_CLASSIC;
	m_bMasterEnable = 0;

	m_nTargetFPS = MODERN_FPS_30;

	m_bHighFpsPhysicsFix = 0;
	m_nDrivingStyle = MODERN_DRIVING_CLASSIC;
	m_bMidAirControl = 0;

	m_nGpsRouteMode = MODERN_GPS_OFF;
	m_bAutosave = 0;
	m_bFastInteriorFades = 0;

	m_bModernVehicleCam = 0;
	m_nAimMode = MODERN_AIM_CLASSIC;
	m_bRightShoulder = 1;
	m_bWeaponRadioWheels = 0;
	m_fVehCamAutoCenterDelay = 1.75f;

	m_nHapticsIntensity = MODERN_HAPTICS_OFF;
	m_nGyroAimMode = MODERN_GYRO_OFF;
	m_fGyroSensitivity = 1.0f;

	m_bHoldToSprint = 0;
	m_bSwimming = 0;
	m_bLedgeMantling = 0;
	m_fMaxMantleHeight = 2.15f;

	m_fLodDistMultiplier = 1.0f;
	m_nNeonBloom = 0;
	m_bFxaa = 0;
	m_bDynamicPointLights = 0;
	m_bWetRoadReflections = 0;
	m_nRealtimeShadows = 0;

	ApplyFrameRateSettings();
}

void
CModernConfig::ApplyPreset(int8 preset)
{
	if (preset == MODERN_PRESET_CLASSIC) {
		ResetToClassicDefaults();
		return;
	}

	if (preset == MODERN_PRESET_MODERN) {
		m_nPreset = MODERN_PRESET_MODERN;
		m_bMasterEnable = 1;

		m_nTargetFPS = MODERN_FPS_60;

		m_bHighFpsPhysicsFix = 1;
		m_nDrivingStyle = MODERN_DRIVING_MODERN;
		m_bMidAirControl = 1;

		m_nGpsRouteMode = MODERN_GPS_IN_VEHICLE;
		m_bAutosave = 1;
		m_bFastInteriorFades = 1;

		m_bModernVehicleCam = 1;
#if defined(ANDROID) || defined(GTA_HANDHELD)
		m_nAimMode = MODERN_AIM_ASSISTED;
#else
		m_nAimMode = MODERN_AIM_FREE;
#endif
		m_bRightShoulder = 1;
		m_bWeaponRadioWheels = 1;
		m_fVehCamAutoCenterDelay = 1.75f;

		m_nHapticsIntensity = MODERN_HAPTICS_FULL;
		m_nGyroAimMode = MODERN_GYRO_ADS_ONLY;
		m_fGyroSensitivity = 1.0f;

		m_bHoldToSprint = 1;
		m_bSwimming = 1;
		m_bLedgeMantling = 1;
		m_fMaxMantleHeight = 2.15f;

		m_fLodDistMultiplier = 2.2f;
		m_nNeonBloom = 2;
		m_bFxaa = 1;
		m_bDynamicPointLights = 1;
		m_bWetRoadReflections = 1;
		m_nRealtimeShadows = 1;

		ApplyFrameRateSettings();
	} else {
		m_nPreset = MODERN_PRESET_CUSTOM;
		ApplyFrameRateSettings();
	}
}

void
CModernConfig::MarkCustomPresetIfNeeded()
{
	m_nPreset = MODERN_PRESET_CUSTOM;
}

void
CModernConfig::ClampValues()
{
	if (m_nPreset < MODERN_PRESET_CLASSIC || m_nPreset > MODERN_PRESET_CUSTOM)
		m_nPreset = MODERN_PRESET_CLASSIC;
	m_bMasterEnable = m_bMasterEnable ? 1 : 0;

	if (m_nTargetFPS < MODERN_FPS_30 || m_nTargetFPS >= MODERN_FPS_COUNT)
		m_nTargetFPS = MODERN_FPS_30;

	m_bHighFpsPhysicsFix = m_bHighFpsPhysicsFix ? 1 : 0;
	if (m_nDrivingStyle < MODERN_DRIVING_CLASSIC || m_nDrivingStyle > MODERN_DRIVING_MODERN)
		m_nDrivingStyle = MODERN_DRIVING_CLASSIC;
	m_bMidAirControl = m_bMidAirControl ? 1 : 0;

	if (m_nGpsRouteMode < MODERN_GPS_OFF || m_nGpsRouteMode > MODERN_GPS_ALWAYS)
		m_nGpsRouteMode = MODERN_GPS_OFF;
	m_bAutosave = m_bAutosave ? 1 : 0;
	m_bFastInteriorFades = m_bFastInteriorFades ? 1 : 0;

	m_bModernVehicleCam = m_bModernVehicleCam ? 1 : 0;
	if (m_nAimMode < MODERN_AIM_CLASSIC || m_nAimMode > MODERN_AIM_ASSISTED)
		m_nAimMode = MODERN_AIM_CLASSIC;
	m_bRightShoulder = m_bRightShoulder ? 1 : 0;
	m_bWeaponRadioWheels = m_bWeaponRadioWheels ? 1 : 0;
	if (m_fVehCamAutoCenterDelay < 0.25f) m_fVehCamAutoCenterDelay = 0.25f;
	if (m_fVehCamAutoCenterDelay > 10.0f) m_fVehCamAutoCenterDelay = 10.0f;

	if (m_nHapticsIntensity < MODERN_HAPTICS_OFF || m_nHapticsIntensity > MODERN_HAPTICS_FULL)
		m_nHapticsIntensity = MODERN_HAPTICS_OFF;
	if (m_nGyroAimMode < MODERN_GYRO_OFF || m_nGyroAimMode > MODERN_GYRO_ALWAYS)
		m_nGyroAimMode = MODERN_GYRO_OFF;
	if (m_fGyroSensitivity < 0.1f) m_fGyroSensitivity = 0.1f;
	if (m_fGyroSensitivity > 5.0f) m_fGyroSensitivity = 5.0f;

	m_bHoldToSprint = m_bHoldToSprint ? 1 : 0;
	m_bSwimming = m_bSwimming ? 1 : 0;
	m_bLedgeMantling = m_bLedgeMantling ? 1 : 0;
	if (m_fMaxMantleHeight < 0.6f) m_fMaxMantleHeight = 0.6f;
	if (m_fMaxMantleHeight > 4.0f) m_fMaxMantleHeight = 4.0f;

	if (m_fLodDistMultiplier < 1.0f) m_fLodDistMultiplier = 1.0f;
	if (m_fLodDistMultiplier > 4.0f) m_fLodDistMultiplier = 4.0f;
	if (m_nNeonBloom < 0 || m_nNeonBloom > 2) m_nNeonBloom = 0;
	m_bFxaa = m_bFxaa ? 1 : 0;
	m_bDynamicPointLights = m_bDynamicPointLights ? 1 : 0;
	m_bWetRoadReflections = m_bWetRoadReflections ? 1 : 0;
	if (m_nRealtimeShadows < 0 || m_nRealtimeShadows > 2) m_nRealtimeShadows = 0;
}

int32
CModernConfig::GetTargetFpsValue()
{
	switch (m_nTargetFPS) {
	case MODERN_FPS_30:
		return 30;
	case MODERN_FPS_60:
		return 60;
	case MODERN_FPS_90:
		return 90;
	case MODERN_FPS_120:
		return 120;
	case MODERN_FPS_NATIVE:
	default:
		return 0; // 0 = Native display refresh / uncapped by software timer
	}
}

bool
CModernConfig::ShouldFrameLimitGameplay()
{
	if (!FrontEndMenuManager.m_PrefsFrameLimiter)
		return false;
	return GetTargetFpsValue() > 0;
}

bool
CModernConfig::ShouldWaitVSyncInGameplay()
{
	if (!FrontEndMenuManager.m_PrefsFrameLimiter) {
#ifdef LEGACY_MENU_OPTIONS
		return FrontEndMenuManager.m_PrefsVsync != 0;
#else
		return false;
#endif
	}
#ifdef LEGACY_MENU_OPTIONS
	return FrontEndMenuManager.m_PrefsVsync != 0;
#else
	return true;
#endif
}

void
CModernConfig::ApplyFrameRateSettings()
{
	ClampValues();

	int32 targetFps = GetTargetFpsValue();
	RsGlobal.maxFPS = (targetFps > 0) ? targetFps : 240;

#if defined ANDROID
	// When Frame Limiter is disabled in Graphics Setup, request native display refresh (0).
	int32 displayHz = FrontEndMenuManager.m_PrefsFrameLimiter ? targetFps : 0;
	CJavaWrapper::SetTargetRefreshRate(displayHz);
#endif
}

#endif // MODERN_FEATURES
