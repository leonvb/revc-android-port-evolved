"""Source-level integration guards; actual rendering still requires a GPU test."""
from pathlib import Path

menu = Path('src/core/MenuScreensCustom.cpp').read_text()
assert 'off_on, 2, false, ModernMasterAfterChange)' in menu
callback = menu.split('void ModernMasterAfterChange', 1)[1].split('\n}', 1)[0]
assert 'CModernConfig::OnMasterSwitchChanged(before, after);' in callback
assert 'FrontEndMenuManager.SaveSettings();' in callback

pipes = Path('src/extras/custompipes_gl.cpp').read_text()
world = pipes.split('worldRenderCB(', 1)[1].split('if(!LightmapEnable && !useModern)', 1)[0]
first = pipes.split('AtomicFirstPass(', 1)[1].split('bool setupDone', 1)[0]
assert 'Geometry::NORMALS' not in world and 'Geometry::NORMALS' not in first
assert '(atomic->geometry->flags & Geometry::NORMALS) ? 1.0f : 0.0f' in pipes
assert '(flags & Geometry::NORMALS) ? 1.0f : 0.0f' in pipes
vert = Path('src/extras/shaders/modernWorld.vert').read_text()
frag = Path('src/extras/shaders/modernWorld.frag').read_text()
assert 'if(u_modernOptions.z > 0.5)' in vert
assert 'cross(dFdx(v_worldPos), dFdy(v_worldPos))' in frag
assert '#ifndef GL2' in frag
assert 'shadowVisibility(v_worldPos)' in frag

resize = Path('src/rw/RwHelper.cpp').read_text().split('CameraSize(', 1)[1]
resize = resize.split('/* Figure out the view window */', 1)[0]
assert 'camera == Scene.camera && (CPostFX::pFrontBuffer || CPostFX::pBackBuffer)' in resize
assert 'CPostFX::Open(camera);' in resize
