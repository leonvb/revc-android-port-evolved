#!/usr/bin/env python3
"""Integration checks; migration and pacing behavior run in the C++ test."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
menu = (root / 'src/core/MenuScreensCustom.cpp').read_text()
config = (root / 'src/core/ModernConfig.cpp').read_text()
ini = (root / 'src/core/re3.cpp').read_text()
frontend = (root / 'src/core/Frontend.cpp').read_text()
java = (root / 'android/launcher/app/src/main/java/com/revc/game/GameActivity.java').read_text()
text = (root / 'src/text/Text.cpp').read_text()
assert menu.count('new CCFOSelect(&CModernConfig::m_nFrameRate') == 1
assert menu.count('FRAME_RATE_SELECTOR\n') == 2  # alternative display/graphics pages
modern_page = menu.split('// MENUPAGE_MODERN_FEATURES', 1)[1]
assert 'm_nFrameRate' not in modern_page and 'TARGET_FPS_SELECTOR' not in menu
assert '"FFR_RAT"' in menu and '"FFR_UNC"' in menu
for lang in ('american', 'french', 'german', 'italian', 'spanish'):
    gxt = (root / ('utils/gxt/' + lang + '.txt')).read_text(encoding='utf-8')
    assert '[FFR_RAT]' in gxt and '[FFR_UNC]' in gxt
assert '{ "FFR_RAT", "FRAME RATE" }' in text
assert '{ "FFR_UNC", "UNCAPPED" }' in text
preset = config.split('CModernConfig::ResetToClassicDefaults()', 1)[1].split('CModernConfig::ClampValues()', 1)[0]
assert 'm_nFrameRate =' not in preset
fps_callback = menu.split('void ModernTargetFpsAfterChange', 1)[1].split('void ModernOptionAfterChange', 1)[0]
assert 'MarkCustomPresetIfNeeded' not in fps_callback and 'ApplyFrameRateSettings' in fps_callback
assert 'ModernFrameRate::Migrate' in ini
assert 'StoreIni("Graphics", "FrameRate", CModernConfig::m_nFrameRate)' in ini
assert 'StoreIni("Modern", "TargetFPS", CModernConfig::m_nFrameRate)' in ini
assert 'CModernConfig::GetTargetFpsValue() > 0' in ini
assert 'CModernConfig::m_nFrameRate = MODERN_FPS_30' in menu
assert 'CModernConfig::m_nFrameRate = MODERN_FPS_30' in frontend
assert 'CModernConfig::ApplyFrameRateSettings()' in frontend
assert 'std::chrono::steady_clock::now()' in config
assert 'CJavaWrapper::SetTargetRefreshRate(targetFps)' in config
vsync = config.split('CModernConfig::ShouldWaitVSyncInGameplay()', 1)[1].split('CModernConfig::ApplyFrameRateSettings()', 1)[0]
assert 'm_PrefsFrameLimiter' not in vsync and 'm_PrefsVsync' in vsync
for backend in ('sdl2/sdl2.cpp', 'glfw/glfw.cpp', 'win/win.cpp'):
    code = (root / 'src/skel' / backend).read_text()
    assert 'CModernConfig::ShouldRenderFrame()' in code, backend
    assert 'CModernConfig::ShouldRenderMenuFrame()' in code, backend
    assert 'FrontEndMenuManager.m_PrefsFrameLimiter ||' in code # Vanilla path retained
assert 'applyDisplayRefreshRate(requestedRefreshRateHz)' in java
assert 'surface.setFrameRate(surfaceFps, compatibility)' in java
print('Canonical frame rate, migration, menu, backends and refresh integration contract passed')
