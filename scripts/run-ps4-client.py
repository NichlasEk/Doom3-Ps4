#!/usr/bin/env python3
"""Run the finite graphical diagnostic; retain failures and optional capture.

Exit 0 establishes initialization + 60 frames + clean shutdown only. A capture
must be inspected separately before claiming menu/game rendering works.
"""
from pathlib import Path
import json
import ctypes as c
import re
import os
import signal
import subprocess
import time

root = Path(__file__).resolve().parent.parent
if not os.environ.get('DISPLAY'):
    raise SystemExit('Run under xvfb-run -a -s "-screen 0 1280x720x24"')
profile = Path(os.environ.get('CLIENT_PROFILE', str(root/'build/client-profile'))).resolve()
out = Path(os.environ.get('CLIENT_ARTIFACTS', str(root/'artifacts/client'))).resolve()
eboot = Path(os.environ.get('CLIENT_EBOOT', str(root/'build/client-runtime/eboot.bin'))).resolve(strict=True)
# Fresh profiles must never prompt to migrate another project's saves.
for user in range(1000,1004):
    for folder in ('savedata','trophy','inputs'):
        (profile/f'shadPS4/home/{user}/{folder}').mkdir(parents=True,exist_ok=True)
frames = int(os.environ.get('CLIENT_TEST_FRAMES', '60'))
if not 1 <= frames <= 100000: raise SystemExit('Invalid finite test frame count')
(eboot.parent/'doom3-test-frames.txt').write_text(str(frames)+'\n')
map_name = os.environ.get('CLIENT_TEST_MAP', '')
map_marker = eboot.parent/'doom3-test-map.txt'
if map_name:
    if any(not (c.isalnum() or c in '_/') for c in map_name): raise SystemExit('Invalid map name')
    map_marker.write_text(map_name+'\n')
else: map_marker.unlink(missing_ok=True)
data = profile/'shadPS4/data'
data.mkdir(parents=True, exist_ok=True)
out.mkdir(parents=True, exist_ok=True)
if os.environ.get('CLIENT_USE_PACKAGED_DATA') == '1':
    if (data/'doom3-game').exists() or (data/'doom3-game').is_symlink():
        raise SystemExit('Packaged-data validation requires a profile without external doom3-game data')
    if not (eboot.parent/'base/pak000.pk4').is_file():
        raise SystemExit('Packaged eboot must have base/pak000.pk4 beside it')
else:
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
input_test = os.environ.get('CLIENT_INPUT_TEST') == '1'
if input_test:
    inputs=profile/'shadPS4/input_config';inputs.mkdir(parents=True,exist_ok=True)
    (inputs/'default.ini').write_text('axis_left_y_minus = w\naxis_right_x_plus = l\nr2 = o\ncross = n\noptions = enter\n')
    x=c.CDLL('libX11.so.6'); xt=c.CDLL('libXtst.so.6')
    x.XOpenDisplay.restype=c.c_void_p; display=x.XOpenDisplay(None)
    x.XDefaultRootWindow.argtypes=[c.c_void_p];x.XDefaultRootWindow.restype=c.c_ulong
    x.XKeysymToKeycode.argtypes=[c.c_void_p,c.c_ulong];x.XKeysymToKeycode.restype=c.c_uint
    x.XFlush.argtypes=[c.c_void_p]
    xt.XTestFakeKeyEvent.argtypes=[c.c_void_p,c.c_uint,c.c_int,c.c_ulong]
    def keys(down):
        for key in ('w','l','o'):xt.XTestFakeKeyEvent(display,x.XKeysymToKeycode(display,ord(key)),int(down),0)
        x.XFlush(display)
    x.XQueryTree.argtypes=[c.c_void_p,c.c_ulong,c.POINTER(c.c_ulong),c.POINTER(c.c_ulong),c.POINTER(c.POINTER(c.c_ulong)),c.POINTER(c.c_uint)]
    x.XSetInputFocus.argtypes=[c.c_void_p,c.c_ulong,c.c_int,c.c_ulong]
    x.XFree.argtypes=[c.c_void_p]
    def focus():
        rr,parent,count=c.c_ulong(),c.c_ulong(),c.c_uint();children=c.POINTER(c.c_ulong)()
        x.XQueryTree(display,x.XDefaultRootWindow(display),c.byref(rr),c.byref(parent),c.byref(children),c.byref(count))
        if not count.value: raise RuntimeError('No guest window')
        x.XSetInputFocus(display,children[count.value-1],1,0);x.XFree(children);x.XFlush(display)
    input_started=False; input_released=False; input_tick=0
engine_log = data/'doom3-client/dudelog.txt'
gpu_log = data/'client-vulkan.log'
for path in (engine_log, gpu_log, out/'capture.png', out/'progress.png', out/'result.json'):
    path.unlink(missing_ok=True)
emulator = os.environ.get('SHADPS4', '/home/nichlas/ScummVM-PS4/.tools/shadps4/Shadps4-sdl.AppImage')
with (out/'emulator.log').open('w') as log:
    proc = subprocess.Popen([emulator, '--fullscreen', 'false', str(eboot)],
        stdout=log, stderr=log, env=dict(os.environ, XDG_DATA_HOME=str(profile), SDL_VIDEODRIVER='x11'), start_new_session=True)
    captured = False
    try:
        deadline = time.monotonic()+float(os.environ.get('CLIENT_TIMEOUT', '240'))
        last_capture = 0.0
        while time.monotonic() < deadline:
            engine = engine_log.read_text(errors='replace') if engine_log.exists() else ''
            host = (out/'emulator.log').read_text(errors='replace')
            gpu = gpu_log.read_text(errors='replace') if gpu_log.exists() else ''
            ticks=re.findall(r'PS4 GAME tick=(\d+)',engine)
            if input_test and ticks:
                tick=int(ticks[-1])
                if not input_started:
                    # Focus the only guest window in this isolated Xvfb display.
                    focus()
                    keys(True);input_started=True;input_tick=tick
                elif not input_released and tick-input_tick>=120:
                    keys(False);input_released=True
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
    success = (f'PASS PS4 client {frames} frames' in engine and 'Exiting with status code 0' in host
               and 'Unhandled access violation' not in host and captured
               and (not map_name or ('PS4 GAME tick=' in engine and 'ERROR:' not in engine)))
    input_evidence={}
    if input_test:
        samples=re.findall(r'PS4 GAME tick=(\d+) pos=([-\d.]+),([-\d.]+),([-\d.]+) yaw=([-\d.]+) buttons=(\d+) move=([-\d]+),([-\d]+)',engine)
        input_evidence=dict(samples=len(samples),movement_command=any(int(s[6])!=0 for s in samples),
            moved=len({s[1:4] for s in samples})>1,turned=len({s[4] for s in samples})>1,
            fire_command=any(int(s[5])&1 for s in samples),released=input_released,
            stopped_after_release=bool(samples) and int(samples[-1][6])==0 and int(samples[-1][7])==0 and not (int(samples[-1][5])&1))
        success=success and all(input_evidence.values())
    result = dict(input_evidence=input_evidence, map=map_name, initialized='PASS PS4 client initialization' in engine,
                  requested_frames=frames, frames_completed=f'PASS PS4 client {frames} frames' in engine,
                  captured=captured, clean_exit='Exiting with status code 0' in host,
                  diagnostic_passed=success, visual_validation='not performed', physical_ps4_tested=False)
    (out/'result.json').write_text(json.dumps(result, indent=2)+'\n')
    print(json.dumps(result, indent=2))
    raise SystemExit(0 if success else 1)
