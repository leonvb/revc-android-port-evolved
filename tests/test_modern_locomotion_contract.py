from pathlib import Path

source = Path('src/peds/PlayerPed.cpp').read_text()
config = Path('src/core/ModernConfig.cpp').read_text()
pad = Path('src/core/Pad.cpp').read_text()
assert '#include "ModernTraversalMath.h"' in source
assert 'player == FindPlayerPed()' in source
for guard in ('m_bMasterEnable', 'm_bHoldToSprint', 'ArePlayerControlsDisabled()',
              'CReplay::IsPlayingBack()', 'CRecordDataForGame::IsPlayingBack()',
              'CCutsceneMgr::IsCutsceneProcessing()', 'CAMCONTROL_GAME',
              'm_objective == OBJECTIVE_NONE'):
    assert guard in source
for guard in ('PED_FALL', 'bIsStanding', 'UINT32_MAX',
              'ANIM_STD_ROLLOUT_LHS', 'ANIM_STD_ROLLOUT_RHS',
              'GetProgress() >= 0.75f'):
    assert guard in source
assert 'static ModernTraversalMath::Pivot s_modernPivot;' in source
assert 's_modernPivot.Reset(); s_pivotOwner = nil;' in source
assert 'CModernConfig::m_bHoldToSprint = 0' in config
# In this port the stock Sprint input is already level-triggered; do not
# replace it or silently change classic input semantics.
sprint = pad[pad.index('bool CPad::GetSprint(void)'):pad.index('bool CPad::ShiftTargetLeftJustDown(void)')]
assert 'NewState.Cross' in sprint and 'NewState.Circle' in sprint
assert 'OldState' not in sprint
print('Modern locomotion source contract passed')
