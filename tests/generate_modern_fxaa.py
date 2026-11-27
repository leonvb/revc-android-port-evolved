#!/usr/bin/env python3
"""Generate/check the runtime GLSL include from the FXAA 3.11 source."""
import argparse
import json
from pathlib import Path

root = Path(__file__).resolve().parent.parent
shader_dir = root / 'src/extras/shaders'
wrapper = (shader_dir / 'modernFXAA.frag').read_text()
reference = (shader_dir / 'fxaa311.h').read_text()
assert wrapper.count('#include "fxaa311.h"') == 1
source = wrapper.replace('#include "fxaa311.h"', reference)
output = 'const char *modernFXAA_frag_src =\n'
output += ''.join(json.dumps(line + '\n', ensure_ascii=True) + '\n' for line in source.splitlines())
output += ';\n'
path = shader_dir / 'obj/modernFXAA_frag.inc'
parser = argparse.ArgumentParser()
parser.add_argument('--check', action='store_true')
if parser.parse_args().check:
    if not path.exists() or path.read_text() != output:
        raise SystemExit('modernFXAA_frag.inc is stale')
else:
    path.write_text(output)
