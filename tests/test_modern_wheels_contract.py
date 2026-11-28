#!/usr/bin/env python3
from pathlib import Path

root = Path(__file__).resolve().parents[1]
wheel = (root / 'src/core/ModernWheels.cpp').read_text()
timer = (root / 'src/core/Timer.h').read_text()
game = (root / 'src/core/Game.cpp').read_text()
ped = (root / 'src/peds/PlayerPed.cpp').read_text()
music = (root / 'src/audio/MusicManager.cpp').read_text()
hud = (root / 'src/renderer/Hud.cpp').read_text()
for guard in ('m_bMasterEnable', 'm_bWeaponRadioWheels', 'GetIsPaused()',
              'CReplay::IsPlayingBack()', 'CRecordDataForGame::IsPlayingBack()',
              'IsCutsceneProcessing()', 'CAMCONTROL_GAME', 'UsesPoliceRadio', 'UsesTaxiRadio'):
    assert guard in wheel, guard
assert 'GetTimeScaleRevision() != scaleRevision' in wheel
assert 'GetTimeScaleRevision() == scaleRevision' in wheel
assert '++m_TimeScaleRevision' in timer
assert 'CTimer::SetTimeScale(previousScale)' in wheel
assert 'CTimer::GetTimeInMillisecondsPauseMode()' in wheel
process = game.split('void CGame::Process(void)', 1)[1]
assert process.index('CPad::UpdatePads()') < process.index('CModernWheels::Update()') < process.index('CTheScripts::Process()')
assert game.count('CModernWheels::Reset()') >= 2
assert 'CModernWheels::TakeWeaponTap()' in ped
assert 'CModernWheels::TakeRadioTap()' in music and 'CModernWheels::TakeRadioSelection()' in music
assert 'CModernWheels::Draw()' in hud
assert 'RADAR_SPRITE_RADIO_WILDSTYLE + i' in wheel
print('wheel input, rendering, radio, and time-scale guards wired')
