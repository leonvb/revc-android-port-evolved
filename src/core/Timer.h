#ifndef __GTA_TIMER_H__
#define __GTA_TIMER_H__

class CTimer
{

	static uint32 m_snTimeInMilliseconds;
	static uint32 m_snTimeInMillisecondsPauseMode;
	static uint32 m_snTimeInMillisecondsNonClipped;
	static uint32 m_snPreviousTimeInMilliseconds;
	static uint32 m_FrameCounter;
	static float ms_fTimeScale;
	static float ms_fTimeStep;
	static float ms_fTimeStepNonClipped;
#ifdef FIX_BUGS
	static uint32 m_LogicalFrameCounter;
	static uint32 m_LogicalFramesPassed;
#endif
public:
	static bool  m_UserPause;
	static bool  m_CodePause;

	static const float &GetTimeStep(void) { return ms_fTimeStep; }
	static void SetTimeStep(float ts) { ms_fTimeStep = ts; }
	static float GetTimeStepInSeconds() { return ms_fTimeStep / 50.0f; }
	static uint32 GetTimeStepInMilliseconds() { return ms_fTimeStep / 50.0f * 1000.0f; }
	static const float &GetTimeStepNonClipped(void) { return ms_fTimeStepNonClipped; }
	static float GetTimeStepNonClippedInSeconds(void) { return ms_fTimeStepNonClipped / 50.0f; }
	static float GetTimeStepNonClippedInMilliseconds(void) { return ms_fTimeStepNonClipped / 50.0f * 1000.0f; }
	static void SetTimeStepNonClipped(float ts) { ms_fTimeStepNonClipped = ts; }
	static const uint32 &GetFrameCounter(void) { return m_FrameCounter; }
	static void SetFrameCounter(uint32 fc) { m_FrameCounter = fc; }
	static const uint32 &GetTimeInMilliseconds(void) { return m_snTimeInMilliseconds; }
	static void SetTimeInMilliseconds(uint32 t) { m_snTimeInMilliseconds = t; }
	static uint32 GetTimeInMillisecondsNonClipped(void) { return m_snTimeInMillisecondsNonClipped; }
	static void SetTimeInMillisecondsNonClipped(uint32 t) { m_snTimeInMillisecondsNonClipped = t; }
	static uint32 GetTimeInMillisecondsPauseMode(void) { return m_snTimeInMillisecondsPauseMode; }
	static void SetTimeInMillisecondsPauseMode(uint32 t) { m_snTimeInMillisecondsPauseMode = t; }
	static uint32 GetPreviousTimeInMilliseconds(void) { return m_snPreviousTimeInMilliseconds; }
	static void SetPreviousTimeInMilliseconds(uint32 t) { m_snPreviousTimeInMilliseconds = t; }
	static const float &GetTimeScale(void) { return ms_fTimeScale; }
	static void SetTimeScale(float ts) { ms_fTimeScale = ts; }
	static uint32 GetCyclesPerFrame();

	static bool GetIsPaused() { return m_UserPause || m_CodePause; }
	static bool GetIsUserPaused() { return m_UserPause; }
	static bool GetIsCodePaused() { return m_CodePause; }
	static void SetCodePause(bool pause) { m_CodePause = pause; }
	
	static void Initialise(void);
	static void Shutdown(void);
	static void Update(void);
	static void Suspend(void);
	static void Resume(void);
	static uint32 GetCyclesPerMillisecond(void);
	static uint32 GetCurrentTimeInCycles(void);
	static bool GetIsSlowMotionActive(void);
	static void Stop(void);
	static void StartUserPause(void);
	static void EndUserPause(void);

	friend bool GenericLoad(void);
	friend bool GenericSave(int file);
	friend class CMemoryCard;

#ifdef FIX_BUGS
	static float GetDefaultTimeStep(void) { return 50.0f / 30.0f; }
	static float GetTimeStepFix(void) { return GetTimeStep() / GetDefaultTimeStep(); }
	static uint32 GetLogicalFrameCounter(void) { return m_LogicalFrameCounter; }
	static uint32 GetLogicalFramesPassed(void) { return m_LogicalFramesPassed; }
#endif

	// Frame-rate-independent math helpers normalized to Vice City's 30 FPS baseline (dt30 = 50/30 = 1.6667)
	static float GetBaseTimeStep30(void) { return 50.0f / 30.0f; }
	static float GetTimeStepRatio30(void) { return ms_fTimeStep / (50.0f / 30.0f); }
	static float GetTimeStepScale(void) { return GetTimeStepRatio30(); }
	static float GetClampedTimeStep(float maxStep = 3.0f) { return ms_fTimeStep > maxStep ? maxStep : (ms_fTimeStep < 0.0f ? 0.0f : ms_fTimeStep); }
	static float PowTimeStep(float per30FpsFactor) { return powf(per30FpsFactor, GetTimeStepRatio30()); }
	static float ExpDecayFactor(float ratePer30Fps)
	{
		if (ratePer30Fps <= 0.0f) return 0.0f;
		if (ratePer30Fps >= 1.0f) return 1.0f;
		return 1.0f - powf(1.0f - ratePer30Fps, GetTimeStepRatio30());
	}
	static float ExpSmooth(float current, float target, float ratePer30Fps)
	{
		return current + (target - current) * ExpDecayFactor(ratePer30Fps);
	}
};

#endif // __GTA_TIMER_H__
