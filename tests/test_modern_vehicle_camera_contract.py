#!/usr/bin/env python3
"""Source-level safety checks complement the independent camera-state test."""
from pathlib import Path

cam = (Path(__file__).resolve().parents[1] / 'src/core/Cam.cpp').read_text()
policy = cam.split('UseModernVehicleFollowCam(CEntity *entity)', 1)[1].split('\n#endif', 1)[0]
for guard in ('m_bMasterEnable', 'm_bModernVehicleCam', 'FindPlayerVehicle()',
              'STATUS_PLAYER', 'CAMCONTROL_GAME', 'IsCutsceneProcessing()',
              'CReplay::IsPlayingBack()', 'VEHICLE_APPEARANCE_CAR',
              'VEHICLE_APPEARANCE_BIKE', 'MI_RCBANDIT', 'MI_RHINO', 'MI_FIRETRUCK',
              'CVehicle::bCheat5'):
    assert guard in policy, f'missing camera eligibility guard: {guard}'

mode = cam.split('case MODE_CAM_ON_A_STRING:', 1)[1].split('case MODE_BEHINDBOAT:', 1)[0]
assert 'UseModernVehicleFollowCam(CamTargetEntity)' in mode
assert 'Process_FollowCar_SA' in mode and 'Process_Cam_On_A_String' in mode
follow = cam.split('CCam::Process_FollowCar_SA(', 1)[1]
assert 'UseModernVehicleFollowCam(car)' in follow
assert 'modernVehicleCamera && nextDirectionIsForward' in follow
print('modern vehicle camera eligibility and classic fallback checks passed')
