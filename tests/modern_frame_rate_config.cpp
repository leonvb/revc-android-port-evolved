// Exercise the real global setting, preset/reset logic and VSync policy without
// the full game. Unused ModernConfig functions are removed by the linker.
#include "common.h"
#include "Frontend.h"
#include "ModernConfig.h"
#include "skeleton.h"
#include <cassert>

void CSprite2d::Delete() {}
void CMenuManager::UnloadTextures() {}
CMenuManager::CMenuManager() {}
CMenuManager FrontEndMenuManager;
RsGlobalType RsGlobal;

int main()
{
	for (int i = MODERN_FPS_30; i < MODERN_FPS_COUNT; ++i) {
		CModernConfig::m_nFrameRate = i;
		CModernConfig::ApplyFrameRateSettings();
		int hz = CModernConfig::GetTargetFpsValue();
		assert(FrontEndMenuManager.m_PrefsFrameLimiter == (hz > 0));
		assert(CModernConfig::ShouldFrameLimitGameplay() == (hz > 0));
		assert(RsGlobal.maxFPS == (hz > 0 ? hz : 240));
		FrontEndMenuManager.m_PrefsVsync = 0;
		assert(!CModernConfig::ShouldWaitVSyncInGameplay());
		FrontEndMenuManager.m_PrefsVsync = 1;
		assert(CModernConfig::ShouldWaitVSyncInGameplay());
		CModernConfig::ApplyPreset(MODERN_PRESET_MODERN);
		assert(CModernConfig::m_nFrameRate == i);
		CModernConfig::ApplyPreset(MODERN_PRESET_CLASSIC);
		assert(CModernConfig::m_nFrameRate == i);
		CModernConfig::ApplyPreset(MODERN_PRESET_CUSTOM);
		assert(CModernConfig::m_nFrameRate == i);
	}
	CModernConfig::m_nFrameRate = -1;
	CModernConfig::ApplyFrameRateSettings();
	assert(CModernConfig::m_nFrameRate == MODERN_FPS_30);
}
