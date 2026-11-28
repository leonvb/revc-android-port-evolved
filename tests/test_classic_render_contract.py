"""Guard removal of opt-in rendering, including legacy saved settings."""
from pathlib import Path

removed_symbols = (
    'm_fLodDistMultiplier', 'm_nNeonBloom', 'm_bFxaa',
    'm_bExperimentalEdgeAA', 'm_bDynamicPointLights',
    'm_bWetRoadReflections', 'm_nRealtimeShadows',
    'ModernVisualPolicy', 'ModernRenderDiagnostics',
    'GetModernPopulationHorizon', 'GetModernLodMultiplier',
    'RenderDirectionalShadowMap', 'RenderModernBloom',
    'RenderExperimentalEdgeAA', 'modernWorldShader',
)
for base in (Path('src'), Path('vendor/librw/src')):
    for path in base.rglob('*'):
        if path.is_file() and path.suffix in ('.cpp', '.h', '.vert', '.frag', '.hlsl'):
            contents = path.read_text(errors='replace')
            for symbol in removed_symbols:
                assert symbol not in contents, f'{symbol} unexpectedly remains in {path}'

menu = Path('src/core/MenuScreensCustom.cpp').read_text()
for key in ('FEM_LDM', 'FEM_BLM', 'FEM_FXA', 'FEM_EAA', 'FEM_DPL', 'FEM_WET', 'FEM_RSH'):
    assert f'"{key}"' not in menu
for pattern in ('modern*', 'directionalShadow*', 'fxaa311.h'):
    assert not list(Path('src/extras/shaders').glob(pattern))
    assert not list(Path('src/extras/shaders/obj').glob(pattern))
