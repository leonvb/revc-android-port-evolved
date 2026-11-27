#!/usr/bin/env python3
"""Verify generated shader includes and compile/link librw GL variants."""
import ast
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
shader_dir = root / "src/extras/shaders"
sources = {}
for name in ("modernAA", "modernBloom"):
    source = (shader_dir / (name + ".frag")).read_text()
    sources[name] = source
    expected = 'const char *' + name + '_frag_src =\n'
    for line in source.splitlines():
        expected += ('"' + line + r'\n"' if line else '') + '\n'
    expected += ';\n'
    actual = (shader_dir / ("obj/" + name + "_frag.inc")).read_text()
    if expected != actual:
        raise SystemExit(name + '_frag.inc is stale; run makeinc_glsl.sh ' + name + '.frag')

for ext in ('vert', 'frag'):
    source = (shader_dir / ('modernWorld.' + ext)).read_text()
    expected = 'const char *modernWorld_' + ext + '_src =\n'
    for line in source.splitlines():
        expected += ('"' + line + r'\n"' if line else '') + '\n'
    expected += ';\n'
    if (shader_dir / ('obj/modernWorld_' + ext + '.inc')).read_text() != expected:
        raise SystemExit('modernWorld_' + ext + '.inc is stale')

subprocess.run(['python3', str(root / 'tests/generate_modern_fxaa.py'), '--check'], check=True)
fxaa = (shader_dir / 'modernFXAA.frag').read_text().replace(
    '#include "fxaa311.h"', (shader_dir / 'fxaa311.h').read_text())
sources['modernFXAA'] = fxaa

for name in ('modernAA', 'modernBloom', 'modernFXAA'):
    bytecode = (shader_dir / ('obj/' + name + '_PS.cso')).read_bytes()
    inc = (shader_dir / ('obj/' + name + '_PS.inc')).read_text()
    embedded = bytes(int(v, 16) for v in re.findall(r'0x([0-9a-fA-F]{2})', inc))
    if (bytecode != embedded or bytecode[:4] != bytes.fromhex('0003ffff') or
            bytecode[-4:] != bytes.fromhex('ffff0000')):
        raise SystemExit(name + '_PS.inc is stale or is not shader model 3 bytecode')


def load_c_string(path):
    text = path.read_text()
    return ''.join(ast.literal_eval(s) for s in re.findall(r'"(?:\\.|[^"\\])*"', text))


vertex_header = load_c_string(root / 'vendor/librw/src/gl/shaders/header_vs.inc')
fragment_header = load_c_string(root / 'vendor/librw/src/gl/shaders/header_fs.inc')
vertex = load_c_string(shader_dir / 'obj/im2d_vert.inc')
macros = ('#define VSIN(index) layout(location = index) in\n'
          '#define VSOUT out\n#define FSIN in\n#define FRAGCOLOR(c) (fragColor = c)\n')
variants = {
    'desktop': '#version 330\n' + macros,
    'gles': '#version 310 es\nprecision highp float;\nprecision highp int;\n' + macros,
}
with tempfile.TemporaryDirectory() as tmp:
    for name, prefix in variants.items():
        vert = Path(tmp) / (name + '.vert')
        vert.write_text(prefix + vertex_header + vertex)
        for shader_name, source in sources.items():
            frag = Path(tmp) / (name + '_' + shader_name + '.frag')
            frag.write_text(prefix + fragment_header + source)
            subprocess.run(['glslangValidator', '-l', str(vert), str(frag)], check=True)
        world_vert = Path(tmp) / (name + '_world.vert')
        world_frag = Path(tmp) / (name + '_world.frag')
        world_vert.write_text(prefix + '#define DIRECTIONALS\n#define POINTLIGHTS\n#define SPOTLIGHTS\n' +
                              vertex_header + (shader_dir / 'modernWorld.vert').read_text())
        world_frag.write_text(prefix + fragment_header + (shader_dir / 'modernWorld.frag').read_text())
        subprocess.run(['glslangValidator', '-l', str(world_vert), str(world_frag)], check=True)
