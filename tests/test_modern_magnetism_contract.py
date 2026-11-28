#!/usr/bin/env python3
"""Integration guard checks; math/state behavior is exercised by the host test."""
from pathlib import Path
root = Path(__file__).resolve().parents[1]
cam = (root / 'src/core/Cam.cpp').read_text()
config = (root / 'src/core/ModernConfig.cpp').read_text()
ini = (root / 'src/core/re3.cpp').read_text()
menu = (root / 'src/core/MenuScreensCustom.cpp').read_text()
text = (root / 'src/text/Text.cpp').read_text()
weapon = (root / 'src/weapons/Weapon.cpp').read_text()
ped = (root / 'src/peds/Ped.cpp').read_text()
cop = (root / 'src/peds/CopPed.cpp').read_text()
# Engine facts behind the regression: random cops inherit the default friend bit.
assert 'CharCreatedBy = RANDOM_CHAR;' in ped and 'bIsPlayerFriend = true;' in ped
assert 'SetObjective(OBJECTIVE_KILL_CHAR_ON_FOOT, FindPlayerPed())' in cop
candidate = cam.split('ModernMagnetismCandidate(', 1)[1].split('// A single scan', 1)[0]
assert 'ModernAimMagnetism::IsEligibleRelationship(ped->CharCreatedBy == MISSION_CHAR,' in candidate
assert 'ped->bIsPlayerFriend, ped->m_leader == player, hostile' in candidate
assert 'ped->bIsPlayerFriend ||' not in candidate  # default bit is NOT an unconditional friend check
assert 'ped->m_pedInObjective == player' in candidate
assert 'ped->m_pPointGunAt == player' in candidate
assert 'm_nAimMagnetism = MODERN_MAGNETISM_OFF' in config
assert 'm_nAimMagnetism = MODERN_MAGNETISM_MEDIUM' in config
assert 'm_nAimMagnetism > MODERN_MAGNETISM_HIGH' in config
assert 'ReadIniIfExists("Modern", "AimMagnetism"' in ini
assert 'StoreIni("Modern", "AimMagnetism"' in ini
assert 'MODERN_PRESET_MODERN ?' in ini
assert '"FEM_MAG"' in menu and 'modernMagnetismOpts' in menu
assert all(f'"{key}"' in text for key in ('FEM_MAG', 'FAM_LOW', 'FAM_MED', 'FAM_HIG'))
for language in ('american', 'french', 'german', 'italian', 'spanish'):
    data = (root / 'utils/gxt' / (language + '.txt')).read_text(encoding='utf-8')
    for key in ('FEM_MAG', 'FAM_LOW', 'FAM_MED', 'FAM_HIG'):
        assert f'[{key}]' in data, (language, key)
for item in ('MODERN_MAGNETISM_OFF', 'controllerInput', '!UseMouse', 'aimDiscontinuity',
             'TheCamera.ActiveCam', '!FrontEndMenuManager.GetIsMenuActive()',
             '!TheCamera.Using1stPersonWeaponMode()', '!CModernWheels::IsOpen()',
             '!TheCamera.m_bUseTransitionBeta', 'WEAPONTYPE_COLT45', 'WEAPONTYPE_RUGER',
             'WEAPON_FIRE_INSTANT_HIT', 'AndroidPhysicalControllerConnected()',
             'inputAmount >= tuning.breakInput', 'ModernAimMagnetism::InputAmount',
             'ControlsManager.m_rStickSensX', 'ModernAimMagnetism::ManualScale',
             'ped->m_nPedState == PED_ATTACK && ped->m_pSeekTarget == player',
             'ModernAimMagnetism::ACQUIRE_CONE_DOT', 'ModernAimMagnetism::RETAIN_CONE_DOT',
             'magnet.BeginFrame', 'magnet.Visible',
             'aimPool->GetAt(magnet.target)', 'CPools::GetPedRef(ped)',
             'ModernAimMagnetism::IsEligibleRelationship', 'ped->CharCreatedBy == MISSION_CHAR',
             'ped->bIsPlayerFriend', 'ped->m_leader == player', 'ped->DyingOrDead()', 'ped->bNeverEverTargetThisPed',
             'ModernMagnetismLineClear(chest, muzzle', 'ModernMagnetismLineClear(Source, magnetPoint',
             'ModernMagnetismLineClear(muzzle, magnetPoint', 'ModernAimMagnetism::BodyHeight',
             'ModernAimMagnetism::ShortestAngle', 'ModernAimMagnetism::Attraction'):
    assert item in cam, item
assert cam.count('FindModernAimThreat(aimPlayer,') == 1
assert 'snapTime[aimCamIndex]' not in cam and 'LookLeftRight *= 0.65f' not in cam
assert 'PrepareModernMuzzleRay' in weapon
print('Modern aim magnetism config, source, visibility and lifecycle contract passed')
