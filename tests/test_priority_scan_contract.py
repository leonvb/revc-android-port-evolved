"""Check that the renderer uses the separate, bounded priority scan wedge.

This is a source-level integration guard; it does not simulate streaming or
render a scene. The policy math is exercised in modern_visual_policy.cpp.
"""
from pathlib import Path

source = Path('src/renderer/Renderer.cpp').read_text()
scan = source.split('CRenderer::ScanWorld(void)', 1)[1].split(
    'CRenderer::RequestObjectsInFrustum(void)', 1)[0]
transform = scan.index('RwV3dTransformPoints(vectors, vectors, 9, cammatrix)')
for side in ('LEFT', 'RIGHT'):
    assert scan.index('vectors[CORNER_PRIO_%s] *= priorityScale;' % side) < transform
assert 'PriorityLodDistance(LOD_DISTANCE, modernLod)' in scan[:transform]
priority_if = scan.index('if(f > priorityScanDistance)')
priority_scan = scan.index('ScanSectorPoly(poly, 3, ScanSectorList_Priority);', priority_if)
normal_if = scan.index('if(f > LOD_DISTANCE)', priority_scan)
normal_scan = scan.index('ScanSectorPoly(poly, 3, ScanSectorList);', normal_if)
assert priority_if < priority_scan < normal_if < normal_scan
