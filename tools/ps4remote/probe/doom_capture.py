#!/usr/bin/env python3
"""Capture one Doom frame through its main-thread screenshot command and FTP.
Run from the Doom repository root. This stalls rendering; do not capture during
performance samples. Requires Pillow, the matching ELF, and a live SDL session.
"""
import argparse
from ftplib import FTP, error_perm
from io import BytesIO
import json
from pathlib import Path
import subprocess
import sys
import time
from PIL import Image

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--pid', type=int, required=True)
p.add_argument('--reuse-session', action='store_true')
a = p.parse_args()
name = 'remote' + str(time.time_ns()) + '.tga'
out = Path('artifacts/ps4-probe/captures')
out.mkdir(parents=True, exist_ok=True)
cmd = [sys.executable, str(Path(__file__).with_name('doom_sdl.py')),
       '--pid', str(a.pid), '--command', 'screenshot ' + name]
if a.reuse_session:
    cmd.append('--reuse-session')
# Never retry the command: only retry read-only retrieval of the unique output.
r = subprocess.run(cmd, capture_output=True, text=True)
(out / (name + '.command.json')).write_text(r.stdout)
if r.returncode:
    raise SystemExit(r.stderr or r.stdout)
remote = '/data/doom3-client/base/' + name
with FTP() as ftp:
    ftp.connect('192.168.32.229', 2121, timeout=5)
    ftp.login()
    for attempt in range(10):
        time.sleep(1)
        raw = BytesIO()
        try:
            ftp.retrbinary('RETR ' + remote, raw.write)
            image = Image.open(BytesIO(raw.getvalue()))
            image.load()
            break
        except (error_perm, OSError):
            if attempt == 9:
                raise
    (out / name).write_bytes(raw.getvalue())
    png = out / (Path(name).stem + '.png')
    image.save(png)
    # Remove only this invocation's unique screenshot after preserving it locally.
    ftp.delete(remote)
print(json.dumps(dict(image=str(png), size=image.size, source=str(out / name))))
