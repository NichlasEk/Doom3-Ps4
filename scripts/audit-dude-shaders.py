#!/usr/bin/env python3
"""Compile DUDE graphics shaders for Vulkan 1.4, then audit Liverpool GCN support.

Requires scripts/build-native-vulkan.sh. Shader sources remain in an ignored,
pinned reference clone. The required gate is generic/zfill/shadow/ambientlight/interaction, ten stages;
the report includes unsupported optional/advanced stages without hiding them.
"""
from pathlib import Path
import hashlib
import json
import os
import subprocess
import sys

root = Path(__file__).resolve().parent.parent
reference = root / '.tools/dude-reference'
pin = '3b6872fe278802496cc7a8f8209b847c68efed2a'
if not reference.exists():
    subprocess.run(['git', 'clone', '--no-checkout', 'https://github.com/Inkub0/dude.git', str(reference)], check=True)
    subprocess.run(['git', '-C', str(reference), 'checkout', '--detach', pin], check=True)
if subprocess.check_output(['git', '-C', str(reference), 'rev-parse', 'HEAD'], text=True).strip() != pin:
    raise SystemExit('Unexpected DUDE revision')
if subprocess.check_output(['git', '-C', str(reference), 'status', '--porcelain'], text=True).strip():
    raise SystemExit('Reference clone has local changes')
stack = root / 'build/native'
out = root / 'build/dude-shaders'
artifacts = root / 'artifacts/native'
artifacts.mkdir(parents=True, exist_ok=True)
with (artifacts/'psbc-host-build.log').open('w') as log:
    subprocess.run(['make', '-C', str(stack/'opengnm-psbc'), '-j'+os.environ.get('BUILD_JOBS', '8'),
                    'PYTHON='+str(stack/'codegen/bin/python3')], stdout=log, stderr=log, check=True)
subprocess.run([sys.executable, str(reference/'neo/shaders/compile_spv.py'),
                '--compiler', '/usr/bin/glslc', '--out', str(out/'spv')], check=True)
(out/'gcn').mkdir(parents=True, exist_ok=True)
required = {name+'.'+stage for name in ('generic','zfill','shadow','ambientlight','interaction') for stage in ('vert','frag')}
report = {'reference': pin, 'target': 'Liverpool PS4 base', 'gpu_test': False, 'shaders': []}
stages = {'vert':'vertex', 'frag':'fragment', 'tesc':'tess-ctrl', 'tese':'tess-eval'}
for source in sorted((reference/'neo/shaders').iterdir()):
    if source.suffix[1:] not in stages:
        continue
    spv = out/'spv'/(source.name+'.spv')
    binary = out/'gcn'/(source.name+'.sb')
    binary.unlink(missing_ok=True)
    valid = subprocess.run(['spirv-val', '--target-env', 'vulkan1.4', str(spv)], capture_output=True, text=True)
    if valid.returncode:
        raise RuntimeError(source.name+': invalid SPIR-V: '+valid.stderr)
    result = subprocess.run([str(stack/'opengnm-psbc/opengnm-psbc'), '-4', '-s', stages[source.suffix[1:]],
        '-f', str(spv), '-o', str(binary)], capture_output=True, text=True, timeout=30)
    (out/'gcn'/(source.name+'.log')).write_text(result.stdout+result.stderr)
    ok = result.returncode == 0 and binary.is_file() and binary.stat().st_size > 0
    report['shaders'].append({'name': source.name, 'required': source.name in required,
        'spirv_valid': True, 'gcn_compiled': ok, 'returncode': result.returncode,
        'spirv_sha256': hashlib.sha256(spv.read_bytes()).hexdigest(),
        'gcn_sha256': hashlib.sha256(binary.read_bytes()).hexdigest() if ok else None,
        'compiler_diagnostics': result.stdout+result.stderr})
report['compiled'] = sum(s['gcn_compiled'] for s in report['shaders'])
report['total'] = len(report['shaders'])
report['required_passed'] = required <= {s['name'] for s in report['shaders'] if s['gcn_compiled']}
(artifacts/'dude-shader-audit.json').write_text(json.dumps(report, indent=2)+'\n')
print(f"GCN: {report['compiled']}/{report['total']}; {len(required)} required stages: {report['required_passed']}")
sys.exit(0 if report['required_passed'] else 1)
