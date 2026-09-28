#!/usr/bin/env python3
"""Run the isolated PS4 shader probe and retain a display capture."""
from pathlib import Path
import json
import os
import subprocess
import time
import sys

root = Path(__file__).resolve().parent.parent
if not os.environ.get('DISPLAY'):
    raise SystemExit('Run under xvfb-run -a -s "-screen 0 1280x720x24"')
mode = sys.argv[1] if len(sys.argv) > 1 else 'demote'
if mode not in ('demote', 'generic', 'cube', 'ambient', 'shadow', 'texops', 'interaction', 'cubeshadow', 'interactionshadow'):
    raise SystemExit('Use demote, generic, cube, ambient, shadow, texops, interaction, interactionshadow or cubeshadow')
profile = root / ('build/shader-probe-profile-'+mode)
out = root / ('artifacts/shader-probe-'+mode)
out.mkdir(parents=True, exist_ok=True)
for folder in ('data', 'home/1000/savedata', 'home/1000/trophy', 'home/1000/inputs'):
    (profile / 'shadPS4' / folder).mkdir(parents=True, exist_ok=True)
guest = profile / 'shadPS4/data/vk_ps4_breadcrumb.log'
guest.unlink(missing_ok=True)
(out/'capture.png').unlink(missing_ok=True)
(out/'result.json').unlink(missing_ok=True)
emulator = os.environ.get('SHADPS4', '/home/nichlas/ScummVM-PS4/.tools/shadps4/Shadps4-sdl.AppImage')
with (out/'emulator.log').open('w') as log:
    proc = subprocess.Popen([emulator, '--fullscreen', 'false', str(root/('build/shader-probe-'+mode)/'eboot.bin')],
        stdout=log, stderr=log, env=dict(os.environ, XDG_DATA_HOME=str(profile), SDL_VIDEODRIVER='x11'), start_new_session=True)
    try:
        deadline = time.monotonic() + 60
        captured = False
        while time.monotonic() < deadline:
            text = guest.read_text(errors='replace') if guest.exists() else ''
            if not captured and 'PROBE CAPTURE READY:' in text:
                time.sleep(.5)
                (out/'windows.txt').write_text(subprocess.check_output(['xwininfo', '-root', '-tree'], text=True))
                subprocess.run(['import', '-window', 'root', str(out/'capture.png')], check=True)
                captured = True
            host = (out/'emulator.log').read_text(errors='replace')
            if 'Exiting with status code' in host or proc.poll() is not None:
                break
            time.sleep(.1)
        host = (out/'emulator.log').read_text(errors='replace')
        if not captured or 'Exiting with status code 0' not in host:
            raise RuntimeError('Probe did not capture and exit cleanly; inspect artifacts/shader-probe')
        if 'Unhandled access violation' in host or 'FAIL' in text or 'FAILED' in text:
            raise RuntimeError('Guest failure; inspect logs')
        from PIL import Image
        image = Image.open(out/'capture.png').convert('RGB')
        if image.size != (1280,720):
            raise RuntimeError('Unexpected display size')
        wrong = 0
        stripe_width = 1 if mode == 'demote' else 16
        for y in range(720):
            for x in range(1280):
                expected = (26,26,51) if (x // stripe_width) % 2 else (0,255,0)
                if mode == 'cube':
                    colors = [(255,0,0),(0,255,0),(0,0,255),(255,255,0),(0,255,255),(255,0,255)]
                    expected = colors[min(int((x+.5)*6/1280),5)]
                elif mode == 'ambient':
                    expected = (0,128,128)
                elif mode == 'interactionshadow':
                    expected = (128,128,128) if x < 640 else (0,0,0)
                elif mode == 'interaction':
                    expected = (128,128,128)
                elif mode == 'cubeshadow':
                    face = min(int((x+.5)*6/1280),5)
                    lit = y < 240 or (y < 480 and face%2 == 1)
                    expected = (0,255,0) if lit else (255,0,0)
                elif mode == 'texops':
                    expected = (255,0,0) if (x//16)%2 else (0,255,0)
                elif mode == 'shadow':
                    lit = y < 240 or (y < 480 and (x//16)%2 == 1)
                    expected = (0,255,0) if lit else (255,0,0)
                if image.getpixel((x,y)) != expected:
                    wrong += 1
        if wrong:
            raise RuntimeError(f'{wrong}/921600 pixels differ from expected {mode} pattern')
        (out/'result.json').write_text(json.dumps({'captured': True, 'guest_exit': 0,
            'pixel_validation': 'passed', 'mode': mode, 'checked_pixels': 921600, 'wrong_pixels': wrong, 'physical_ps4_tested': False}, indent=2)+'\n')
        print('PASS:', mode, '921600 pixels, guest exit 0;', out/'capture.png')
    finally:
        if proc.poll() is None:
            import signal
            os.killpg(proc.pid, signal.SIGTERM)
            try:
                proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                os.killpg(proc.pid, signal.SIGKILL)
                proc.wait()
