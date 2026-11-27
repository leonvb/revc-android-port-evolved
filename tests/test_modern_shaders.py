#!/usr/bin/env python3
"""Verify generated shader includes and compile/link librw GL variants."""
import ast
from pathlib import Path
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parent.parent
shader_dir = root / "src/extras/shaders"
source = (shader_dir / "modernAA.frag").read_text()
expected = 'const char *modernAA_frag_src =\n'
for line in source.splitlines():
    expected += ('"' + line + r'\n"' if line else '') + '\n'
expected += ';\n'
actual = (shader_dir / "obj/modernAA_frag.inc").read_text()
if expected != actual:
    raise SystemExit('modernAA_frag.inc is stale; run makeinc_glsl.sh modernAA.frag')

bytecode = (shader_dir / "obj/modernAA_PS.cso").read_bytes()
inc = (shader_dir / "obj/modernAA_PS.inc").read_text()
embedded = bytes(int(v, 16) for v in re.findall(r'0x([0-9a-fA-F]{2})', inc))
if bytecode != embedded or bytecode[:4] != bytes.fromhex('0003ffff'):
    raise SystemExit('modernAA_PS.inc is stale or is not shader model 3 bytecode')


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
        frag = Path(tmp) / (name + '.frag')
        vert.write_text(prefix + vertex_header + vertex)
        frag.write_text(prefix + fragment_header + source)
        subprocess.run(['glslangValidator', '-l', str(vert), str(frag)], check=True)
