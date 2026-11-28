// Exercise the actual preset and effective distance getter without a game.
#include "../src/core/ModernConfig.cpp"
#include <cassert>
#include <limits>
#ifdef main
#undef main
#endif

RsGlobalType RsGlobal = {};

int main()
{
	CModernConfig::m_bMasterEnable = 1;
	const float saved[] = {1.0f, 1.01f, 2.2f, 4.0f, std::numeric_limits<float>::quiet_NaN()};
	for(float value : saved){
		CModernConfig::m_fLodDistMultiplier = value;
		CModernConfig::ClampValues();
		assert(CModernConfig::GetLodDistanceMultiplier() == 1.0f);
	}
	CModernConfig::ApplyPreset(MODERN_PRESET_MODERN);
	assert(CModernConfig::m_bMasterEnable == 1);
	assert(CModernConfig::m_fLodDistMultiplier == 1.0f);
	assert(CModernConfig::m_nNeonBloom == 2);
	assert(CModernConfig::GetLodDistanceMultiplier() == 1.0f);
	CModernConfig::m_fLodDistMultiplier = 4.0f;
	CModernConfig::m_bMasterEnable = 0;
	assert(CModernConfig::GetLodDistanceMultiplier() == 1.0f);
	CModernConfig::ApplyPreset(MODERN_PRESET_CLASSIC);
	assert(CModernConfig::m_bMasterEnable == 0);
	assert(CModernConfig::GetLodDistanceMultiplier() == 1.0f);
}
