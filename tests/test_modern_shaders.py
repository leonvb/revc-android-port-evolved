#!/usr/bin/env python3
"""Check generated GLSL includes and compile the post-process shader variants."""
from pathlib import Path
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

variants = {
    'desktop': '#version 330\n#define FSIN in\n#define FRAGCOLOR(c) (fragColor = c)\nout vec4 fragColor;\n',
    'gles': '#version 310 es\nprecision highp float;\nprecision highp int;\n#define FSIN in\n#define FRAGCOLOR(c) (fragColor = c)\nout vec4 fragColor;\n',
}
with tempfile.TemporaryDirectory() as tmp:
    for name, prefix in variants.items():
        path = Path(tmp) / (name + '.frag')
        path.write_text(prefix + source)
        subprocess.run(['glslangValidator', '-S', 'frag', str(path)], check=True)
