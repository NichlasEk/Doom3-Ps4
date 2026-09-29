#!/usr/bin/env python3
"""Regenerate the small checked-in fullscreen presentation shaders."""
from pathlib import Path
import struct
import subprocess
import tempfile
root = Path(__file__).resolve().parent.parent
lines = ['#pragma once']
with tempfile.TemporaryDirectory() as tmp:
    for stage in ('vert', 'frag'):
        binary = Path(tmp) / (stage + '.spv')
        subprocess.run(['glslangValidator', '-V', str(root / 'platform/ps4/shaders' / ('present.' + stage)), '-o', str(binary)], check=True)
        subprocess.run(['spirv-val', str(binary)], check=True)
        data = binary.read_bytes()
        words = struct.unpack('<' + 'I' * (len(data) // 4), data)
        lines.append('static const uint32_t present_' + stage + '[]={' + ','.join(hex(word) for word in words) + '};')
(root / 'platform/ps4/present_spirv.h').write_text('\n'.join(lines) + '\n')
