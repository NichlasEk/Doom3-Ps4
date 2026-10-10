# Doom 3: remote input and screenshots, 2026-10-10

Physical console: 192.168.32.229, firmware 9.60, ps4debug-NG on 744,
FTP on 2121. Doom 0.14 remained running as PID 224 throughout this experiment.
No new PKG, executable patch or console restart was needed.

User feedback: the cutscene looks better with 0.14; loading sequences remain
heavy. This is subjective feedback, separate from the earlier measured intro
GUI result. Loading performance has not been fixed in this change.

## Verified on PS4

- SDL key down/up reached Doom's own key table: J and W changed false → true →
  false. The observer reads memory after matching `idKeyInput::IsDown` code to
  the local ELF; this build uses a 56-byte key record with `down` at offset 0.
- Screenshot command produced an actual 1280×720 image, retrieved over FTP.
  The first image showed the PDA. Movement while the PDA was open had no
  gameplay effect, despite correctly accepted events.
- Escape closed the PDA. A subsequent W hold (requested 0.5 seconds) moved
  position from (-880,-1352,132.25) to (-883.75,-1352,132.25). The reception
  desk is directly ahead, so the short displacement is not a speed benchmark.
- Relative mouse delta (80,0) changed yaw from -180 to 171.20 degrees: an
  8.8-degree turn across the wrap boundary. The gameplay screenshot shows
  Mars City reception. Health stayed 100; later log entries show move=0,0.
- Log output is buffered: a retrieval immediately after the input initially
  showed older ticks. Later retrieval confirmed the changes. Do not interpret
  a stale log tail as failed input.

Evidence (ignored local artifacts):

- `artifacts/ps4-probe/control-walk-game.json`
- `artifacts/ps4-probe/control-look-game.json`
- `artifacts/ps4-probe/control-engine-latest.log`
- `artifacts/ps4-probe/remotecontrol.png` (PDA)
- `artifacts/ps4-probe/captures/remote1791621827166083713.png` (gameplay)

## Commands

Run from the Doom repository root. FIRST query current processes and match
DOOM00001 and the local `build/client-runtime/dude.oelf`. PID 224 is historical;
never reuse a saved session after reboot, process restart or PID reuse.
`--reuse-session` explicitly means the same continuously running process.

```sh
python3 tools/ps4remote/probe/debug_client.py processes
python3 tools/ps4remote/probe/doom_sdl.py --pid 224 --reuse-session --keys w --hold .5
python3 tools/ps4remote/probe/doom_sdl.py --pid 224 --reuse-session --move 80 0
python3 tools/ps4remote/probe/doom_capture.py --pid 224 --reuse-session
```

Keyboard supports letters, arrows, enter, escape, space, tab, shift, ctrl,
alt and grave, with up to four comma-separated keys. `--hold` is 0.05–2 seconds
between completed press delivery and starting release; RPC overhead adds time.
`--click left|middle|right` delivers mouse buttons. `--point X Y` provides
absolute SDL coordinates **only when the engine is in absolute mouse mode**;
this PS4 build normally uses relative input, including its custom pad path.
Click and absolute cursor behaviour have not yet been physically verified.

Recovery example after restoring connectivity and confirming the same session:

```sh
python3 tools/ps4remote/probe/doom_sdl.py --pid 224 --reuse-session --keys w --release
```

Each input invocation writes a journal and takes a local per-PID exclusive
lock. Normal holds release their keys. A timed-out RPC might already have
queued its event: no automatic retry or further calls over that uncertain
stream. The journal records pending events/release requirements. This is a
host-driven control tool, not a console-side watchdog: loss of connectivity
while holding a key can leave it down until explicitly released.

`doom_capture.py` needs Pillow. It issues one uniquely named screenshot,
retrieves and decodes the TGA, preserves TGA+PNG locally and removes only that
unique remote screenshot after successful local storage. Retrieval retries
are read-only; it never retries the screenshot command. Screenshot capture
stalls rendering and must not run during FPS sampling. This is step-by-step
visual feedback, not a real-time video stream.

Local validation: both scripts compile; 15 existing debugger/credential tests
pass. Physical tests above verify the new keyboard/mouse movement and image
path; the existing unit tests do not themselves cover SDL input.

Follow-up: `--command` now reads and verifies the console's actual state,
opens it only if necessary, and confirms closure. Blind toggling failed when
the console was already open; see [the follow-up measurements](GAMEPLAY-014-MEASUREMENTS.md).
