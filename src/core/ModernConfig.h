#ifndef __GTA_MODERN_CONFIG_H__
#define __GTA_MODERN_CONFIG_H__

#ifdef MODERN_FEATURES

enum eModernPreset
{
	MODERN_PRESET_CLASSIC = 0,
	MODERN_PRESET_MODERN = 1,
	MODERN_PRESET_CUSTOM = 2,
};

enum eModernFpsTarget
{
	MODERN_FPS_30 = 0,
	MODERN_FPS_60 = 1,
	MODERN_FPS_90 = 2,
	MODERN_FPS_120 = 3,
	MODERN_FPS_NATIVE = 4,
	MODERN_FPS_COUNT
};

enum eModernDrivingStyle
{
	MODERN_DRIVING_CLASSIC = 0,
	MODERN_DRIVING_MODERN = 1,
};

enum eModernGpsMode
{
	MODERN_GPS_OFF = 0,
	MODERN_GPS_IN_VEHICLE = 1,
	MODERN_GPS_ALWAYS = 2,
};

enum eModernAimMode
{
	MODERN_AIM_CLASSIC = 0,
	MODERN_AIM_FREE = 1,
	MODERN_AIM_ASSISTED = 2,
};

enum eModernHapticsMode
{
	MODERN_HAPTICS_OFF = 0,
	MODERN_HAPTICS_SUBTLE = 1,
	MODERN_HAPTICS_FULL = 2,
};

enum eModernGyroMode
{
	MODERN_GYRO_OFF = 0,
	MODERN_GYRO_ADS_ONLY = 1,
	MODERN_GYRO_ALWAYS = 2,
};

class CModernConfig
{
public:
	// Master modernization preset & enable state
	static int8 m_nPreset;             // eModernPreset (default: MODERN_PRESET_CLASSIC)
	static int8 m_bMasterEnable;       // 0 = Classic behavior, 1 = Modern features active

	// Phase 0 (M0.2): Frame-rate target & display refresh pacing
	static int8 m_nTargetFPS;          // eModernFpsTarget

	// Phase 1: High-FPS physics parity & driving dynamics
	static int8 m_bHighFpsPhysicsFix;  // 0/1
	static int8 m_nDrivingStyle;       // eModernDrivingStyle
	static int8 m_bMidAirControl;      // 0/1

	// Phase 2: Navigation, Autosave & QoL
	static int8 m_nGpsRouteMode;       // eModernGpsMode
	static int8 m_bAutosave;           // 0/1
	static int8 m_bFastInteriorFades;  // 0/1

	// Phase 3: Camera, OTS Aiming & Radial Wheels
	static int8 m_bModernVehicleCam;   // 0/1
	static int8 m_nAimMode;            // eModernAimMode
	static int8 m_bRightShoulder;      // 1 = right shoulder, 0 = left shoulder
	static int8 m_bWeaponRadioWheels;  // 0/1
	static float m_fVehCamAutoCenterDelay; // seconds (default: 1.75f)

	// Phase 4: Android / Mobile UX
	static int8 m_nHapticsIntensity;   // eModernHapticsMode
	static int8 m_nGyroAimMode;        // eModernGyroMode
	static float m_fGyroSensitivity;   // multiplier (default: 1.0f)

	// Phase 5: Player Traversal (SCM-agnostic, data-configurable)
	static int8 m_bHoldToSprint;       // 0/1
	static int8 m_bSwimming;           // 0/1
	static int8 m_bLedgeMantling;      // 0/1
	static float m_fMaxMantleHeight;   // meters (default: 2.15f)

	// Phase 6: Rendering, Lighting & World Visuals
	static float m_fLodDistMultiplier; // 1.0f..4.0f (default: 1.0f in Classic)
	static int8 m_nNeonBloom;          // 0 = Off, 1 = Low, 2 = High
	static int8 m_bFxaa;               // 0/1
	static int8 m_bExperimentalEdgeAA; // opt-in preview, not FXAA 3.11
	static int8 m_bDynamicPointLights; // 0/1
	static int8 m_bWetRoadReflections; // 0/1
	static int8 m_nRealtimeShadows;    // 0 = Classic, 1 = Medium, 2 = High

	static void ResetToClassicDefaults();
	static void ApplyPreset(int8 preset);
	static void MarkCustomPresetIfNeeded();
	static void ClampValues();

	// Frame-rate target helpers (M0.2)
	static int32 GetTargetFpsValue();
	static bool ShouldFrameLimitGameplay();
	static bool ShouldWaitVSyncInGameplay();
	static void ApplyFrameRateSettings();

	// Phase 6: master-off and invalid values must always behave like Classic.
	static float GetLodDistanceMultiplier();

	// Phase 1 (M1.1 / M1.2) runtime feature predicates
	static bool IsHighFpsPhysicsFixEnabled() { return m_bMasterEnable != 0 && m_bHighFpsPhysicsFix != 0; }
	static bool IsModernDrivingEnabled() { return m_bMasterEnable != 0 && m_nDrivingStyle == MODERN_DRIVING_MODERN; }
	static bool IsMidAirControlEnabled() { return m_bMasterEnable != 0 && m_bMidAirControl != 0; }
};

#endif // MODERN_FEATURES

#endif // __GTA_MODERN_CONFIG_H__
