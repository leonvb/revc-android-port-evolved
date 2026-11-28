#!/usr/bin/env python3
from pathlib import Path
root = Path(__file__).resolve().parents[1]
config = (root / 'src/core/ModernConfig.cpp').read_text()
cam = (root / 'src/core/Cam.cpp').read_text()
ped = (root / 'src/peds/PlayerPed.cpp').read_text()
hud = (root / 'src/renderer/Hud.cpp').read_text()
weapon = (root / 'src/weapons/Weapon.cpp').read_text()
policy = config.split('CModernConfig::IsModernAimActive()', 1)[1].split('\nint8 CModernConfig::m_nPreset', 1)[0]
for item in ('MODERN_AIM_CLASSIC', 'CReplay::IsPlayingBack()', 'CRecordDataForGame::IsPlayingBack()',
             'IsCutsceneProcessing()', 'CAMCONTROL_GAME', 'MODE_FOLLOWPED', 'WEAPONTYPE_MINIGUN', 'WEAPONTYPE_FLAMETHROWER'):
    assert item in policy, item
assert 'ModernAimBlendOutAllowed' in cam and '0.14f' in cam and '52.0f' in cam
assert 'FindModernAimThreat' in cam and '0.9781476f' in cam and 'snapTime[aimCamIndex] = 0.1f' in cam
assert 'flickSide[aimCamIndex]' in cam and 'LookLeftRight *= 0.65f' in cam
assert 'GetDPadRightJustDown()' in ped and 'm_bRightShoulder' in ped
assert 'CModernConfig::IsModernAimActive()' in hud and 'WEAPONSTATE_FIRING' in hud
area = weapon.split('CWeapon::FireAreaEffect(', 1)[1].split('CWeapon::LaserScopeDot(', 1)[0]
assert 'PrepareModernMuzzleRay' in area
print('OTS, target assist, shoulder swap and heavy weapon routing checks passed')
