#!/usr/bin/env python3
import pathlib
import re
import shutil
import subprocess
import sys

ROOT=pathlib.Path(__file__).resolve().parents[1]
CASES=[
    (ROOT/'src/gles_ibl.cpp', [('V','vert'),('I','frag'),('G','frag')]),
    (ROOT/'src/gles_vegetation_pbr.cpp', [('V','vert'),('F','frag')]),
    (ROOT/'src/gles_probe_material.cpp', [('R','frag')]),
]
validator=shutil.which('glslangValidator')
if not validator:
    print('glslangValidator is required for GLES shader validation',file=sys.stderr);sys.exit(2)
failed=False
for path,items in CASES:
    text=path.read_text(encoding='utf-8')
    for name,stage in items:
        m=re.search(r'const char\*\s*'+re.escape(name)+r'\s*=R"GLSL\((.*?)\)GLSL";',text,re.S)
        if not m:
            print(f'Missing shader {name} in {path}',file=sys.stderr);failed=True;continue
        out=ROOT/'build'/'shader_validation'/f'{path.stem}_{name}.{stage}'
        out.parent.mkdir(parents=True,exist_ok=True)
        out.write_text(m.group(1),encoding='utf-8')
        cmd=[validator,'-S',stage,str(out)]
        result=subprocess.run(cmd,text=True,capture_output=True)
        if result.returncode:
            print(result.stdout,file=sys.stderr);print(result.stderr,file=sys.stderr)
            print(f'FAILED: {path.name}:{name}',file=sys.stderr);failed=True
        else:
            print(f'OK: {path.name}:{name} ({stage})')
sys.exit(1 if failed else 0)