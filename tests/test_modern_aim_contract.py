#!/usr/bin/env python3
from pathlib import Path

code = (Path(__file__).resolve().parents[1] / 'src/weapons/Weapon.cpp').read_text()
helper = code.split('PrepareModernMuzzleRay(', 1)[1].split('\nCWeaponInfo *', 1)[0]
for guard in ('m_bMasterEnable', 'MODERN_AIM_CLASSIC', 'FindPlayerPed()', 'm_pPointGunAt',
              'bInVehicle', 'GetTarget()', 'ArePlayerControlsDisabled()',
              'GetLookBehindForPed()', 'CAMCONTROL_GAME', 'Using3rdPersonMouseCam()',
              'IsCutsceneProcessing()', 'CReplay::IsPlayingBack()'):
    assert guard in helper, f'missing aim safety guard: {guard}'
assert helper.count('CWorld::ProcessLineOfSight(') == 2
assert 'CWorld::pIgnoreEntity = oldIgnoredEntity' in helper
assert 'ModernMuzzleAimDirection' in helper
shot = code.split('CWeapon::FireInstantHit(', 1)[1].split('CWeapon::FireShotgun(', 1)[0]
assert shot.index('PrepareModernMuzzleRay(') < shot.index('ProcessLineOfSight(source, target, point, victim', shot.index('PrepareModernMuzzleRay('))
assert 'CWorld::pIgnoreEntity = oldIgnoredEntity' in shot
shotgun = code.split('CWeapon::FireShotgun(', 1)[1].split('CWeapon::FireProjectile(', 1)[0]
assert shotgun.count('PrepareModernMuzzleRay(') == 1
assert 'if (modernShot) CWorld::pIgnoreEntity = shooter' in shotgun
assert 'CWorld::pIgnoreEntity = oldIgnoredEntity' in shotgun
print('modern muzzle ray is gated and followed by a normal world collision ray')
