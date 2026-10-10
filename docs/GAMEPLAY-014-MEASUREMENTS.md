# Physical Mars City measurements, 2026-10-10

Doom 0.14, physical PS4 9.60 at 192.168.32.229, PID224.
Source/tools snapshot initially pushed as `92d338b`; local ELF fingerprints
were matched before every timing sample. No new package installed this run.

## Experiment and attribution

The first attempted map command did not execute. The remote tool assumed the
console was closed, but the console was already open. Its blind toggle closed
it before typing. The initial 45-second sample (9.948 FPS) therefore belongs
to the prior reception session, with console state not controlled. It is NOT
a fresh-game startup benchmark and is excluded from the new-game result.
A screenshot request during the same state error also produced no output.

Fixed `doom_sdl.py` to inspect `idConsoleLocal::Active` through a fingerprinted
5-byte accessor and read-only access to its field. It now opens only when
closed, waits for the requested state and confirms closure. The confirmed
map-command journal records `console_before=true`; the engine log subsequently
contains `]map game/mars_city1`, a new load report and game ticks restarting.
Screenshot retrieval then succeeded. No timed-out command was blindly retried.

Started the actual map through `map game/mars_city1`. This invokes the map's
in-engine arrival cinematic, not the separate `testGUI guis/intro.gui` video.
Sampled 29.810 seconds after loading: 512 engine frames, **17.175 FPS**.
Approximate 5-second windows varied (e.g. 12.34–19.76 FPS); these are sampled
counter deltas, not per-frame percentiles or 1% lows. Then pressed Escape to
skip the cinematic. A 1280×720 screenshot confirmed the playable Mars City
Hangar with health100 and visible HUD. A separate stationary gameplay sample
followed. No screenshots or injected inputs during that gameplay interval.

## Loading evidence

The earlier first map load is preserved in the same PID224/0.14 engine log:

- Total engine load phases: 131395 ms.
- proc=809 ms; game=19136 ms; media=111157 ms; settle=293 ms.
- 2076 newly loaded images; image loading reports 109.0 seconds.

The fresh map restart in this experiment reused loaded resources:

- Total engine load phases: 2272 ms.
- proc=42 ms; game=2124 ms; media=36 ms; settle=70 ms.
- A surrounding engine PERF window reported peakMs=7057. That is a long
  engine frame and is a different boundary from the 2272 ms load-phase timer.

The earlier first-load figure is retrospective log evidence, not a newly
controlled cold boot. Roughly 84.6% of that load-phase total was media work.
This points to initial image/media processing for the next investigation,
but does not distinguish FTP/disk I/O, decompression, texture conversion,
upload, allocation or synchronization. Do not label it a proven disk bottleneck.

## Evidence and limitations

Local ignored artifacts: `artifacts/gameplay-014/` (raw timing samples,
command journals, original engine and Vulkan logs, summaries). Screenshot
manifest: `gameplay-scene.json`, referring to the retained TGA/PNG capture.

Counters sampled about 5Hz using host monotonic time through read-only
ps4debug. Engine frame count is not an independent display-present counter.
Existing renderer frontend/backend counters remain zero; they cannot provide
GPU execution cost. Debugger and engine logging overhead remain present.
These are scene-specific baselines, not a whole-game performance claim.

## Stationary gameplay result

Mars City Hangar: **6.624 engine FPS**, 297 frames over 44.838 seconds
(220 samples, complete). Engine time advanced 44.817 seconds, consistent with
host time. Log confirms cinematic=0, health100, move=0,0, position
(1267,-1501,0.25), yaw=-180. This scene is much heavier than the arrival camera;
it is not a comparison of different builds. The console remained alive.

Final 200 complete presentation-log cycles (a separate trailing window after
capture, not an exactly synchronized copy of the sampler window):
- frame_ms: mean 152.570 ms, median 150.151 ms.
- prepare_until_command_end_ms: mean 109.246 ms, median 108.895 ms.
- command_end_to_submit_ms: mean 7.369 ms, median 5.386 ms.
- submit_to_present_ms: mean 27.877 ms, median 27.731 ms.
- blocking_present_ms: mean 8.078 ms, median 8.313 ms.

These CPU wall-clock boundaries include waits, scheduling and log overhead.
The large interval before command end deserves finer instrumentation; it
does not independently prove a CPU-only or GPU-only bottleneck. No performance
change was made. The game remains running at the measured hangar viewpoint.

Validation after the console-state correction: map execution confirmed in
engine log, screenshot successfully retrieved, 15 debugger/credential unit
tests pass and Python compilation passes. The earlier 6 remote-tool tests
also passed before the initial commit.
