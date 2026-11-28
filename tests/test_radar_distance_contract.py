"""Source-level guards for bottom-left radar and GPS placement."""
from pathlib import Path

radar = Path('src/core/Radar.cpp').read_text()
transform = radar.split('void CRadar::TransformRadarPointToScreenSpace', 1)[1].split('\n}', 1)[0]
assert 'if (FrontEndMenuManager.m_bMenuMapActive)' in transform
assert 'SCREEN_SCALE_FROM_BOTTOM(RADAR_BOTTOM + RADAR_HEIGHT)' in transform
assert 'RADAR_TOP' not in transform
hud = Path('src/renderer/Hud.cpp').read_text()
assert 'rect.Translate(SCREEN_SCALE_X_FIX(RADAR_LEFT), SCREEN_SCALE_FROM_BOTTOM(RADAR_BOTTOM + RADAR_HEIGHT));' in hud
assert 'RADAR_TOP' not in hud
gps = Path('src/control/GpsRoute.cpp').read_text()
distance = gps.split('void CGpsRoute::DrawDistance()', 1)[1]
assert 'SCREEN_SCALE_FROM_BOTTOM(RADAR_BOTTOM - 6.0f)' in distance
assert 'RADAR_TOP' not in gps
