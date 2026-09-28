#!/usr/bin/env python3
"""Run the finite graphical diagnostic; retain failures and optional capture.

Exit 0 establishes initialization + 60 frames + clean shutdown only. A capture
must be inspected separately before claiming menu/game rendering works.
"""
from pathlib import Path
import json
import os
import signal
import subprocess
import time

root = Path(__file__).resolve().parent.parent
if not os.environ.get('DISPLAY'):
    raise SystemExit('Run under xvfb-run -a -s "-screen 0 1280x720x24"')
profile = root/'build/client-profile'
out = root/'artifacts/client'
data = profile/'shadPS4/data'
data.mkdir(parents=True, exist_ok=True)
out.mkdir(parents=True, exist_ok=True)
assets = Path(os.environ.get('DOOM3_DATA', str(root/'media/game'))).resolve()
if not (assets/'base/pak000.pk4').is_file():
    raise SystemExit('Set DOOM3_DATA to the directory containing base/pak000.pk4')
link = data/'doom3-game'
if link.is_symlink():
    if link.resolve() != assets:
        raise SystemExit('Existing doom3-game symlink points to different assets')
elif link.exists():
    raise SystemExit('Refusing to replace existing doom3-game directory')
else:
    link.symlink_to(assets, target_is_directory=True)
engine_log = data/'doom3-client/dudelog.txt'
gpu_log = data/'client-vulkan.log'
for path in (engine_log, gpu_log, out/'capture.png', out/'progress.png', out/'result.json'):
    path.unlink(missing_ok=True)
emulator = os.environ.get('SHADPS4', '/home/nichlas/ScummVM-PS4/.tools/shadps4/Shadps4-sdl.AppImage')
with (out/'emulator.log').open('w') as log:
    proc = subprocess.Popen([emulator, '--fullscreen', 'false', str(root/'build/client-runtime/eboot.bin')],
        stdout=log, stderr=log, env=dict(os.environ, XDG_DATA_HOME=str(profile), SDL_VIDEODRIVER='x11'), start_new_session=True)
    captured = False
    try:
        deadline = time.monotonic()+float(os.environ.get('CLIENT_TIMEOUT', '240'))
        last_capture = 0.0
        while time.monotonic() < deadline:
            engine = engine_log.read_text(errors='replace') if engine_log.exists() else ''
            host = (out/'emulator.log').read_text(errors='replace')
            gpu = gpu_log.read_text(errors='replace') if gpu_log.exists() else ''
            if 'QueuePresent: flip done' in gpu and time.monotonic()-last_capture > 3:
                subprocess.run(['import', '-window', 'root', str(out/'progress.png')], check=True)
                last_capture = time.monotonic()
            if not captured and 'CLIENT CAPTURE READY' in gpu:
                time.sleep(.5)
                subprocess.run(['import', '-window', 'root', str(out/'capture.png')], check=True)
                captured = True
            if proc.poll() is not None or 'Exiting with status code' in host:
                break
            time.sleep(.1)
    finally:
        if proc.poll() is None:
            os.killpg(proc.pid, signal.SIGTERM)
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                os.killpg(proc.pid, signal.SIGKILL)
                proc.wait()
    engine = engine_log.read_text(errors='replace') if engine_log.exists() else ''
    host = (out/'emulator.log').read_text(errors='replace')
    for source, name in ((engine_log, 'engine.log'), (gpu_log, 'vulkan.log')):
        (out/name).write_text(source.read_text(errors='replace') if source.exists() else '')
    success = ('PASS PS4 client 60 frames' in engine and 'Exiting with status code 0' in host
               and 'Unhandled access violation' not in host and captured)
    result = dict(initialized='PASS PS4 client initialization' in engine,
                  sixty_frames='PASS PS4 client 60 frames' in engine,
                  captured=captured, clean_exit='Exiting with status code 0' in host,
                  diagnostic_passed=success, visual_validation='not performed', physical_ps4_tested=False)
    (out/'result.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result, indent=2))
    raise SystemExit(0 if success else 1)
