# PS4 optimization investigation 0.15 — 2026-10-10

## Controlled FPS tests on installed 0.14

Same stationary Mars City Hangar viewpoint, PID224. No captures or movement
during each 20-second counter sample. Console settings read back via condump
before sampling; two seconds settling after each change. Original values:
r_shadows=1, r_screenFraction=100, r_parallelImageLoad=0,
r_parallelImageLoadThreads=0. Only one rendering variable changed at a time.

| Test order | Shadows | Screen fraction | Engine FPS |
|---|---:|---:|---:|
| Baseline A | 1 | 100 | 6.651 |
| Shadows disabled | 0 | 100 | 8.575 |
| Baseline B | 1 | 100 | 6.660 |
| Half width and height | 1 | 50 | 7.469 |
| Baseline C | 1 | 100 | 6.605 |

Disabling shadows improved FPS by 28.8% relative to adjacent controls. Halving
width and height improved it by 12.6%. Neither test isolates CPU versus GPU:
shadow work affects both and resolution does not scale every pass. These are
diagnostic quality reductions, not adopted defaults or a delivered FPS fix.
All settings were restored and read back before stopping 0.14.
The native breadcrumb logger flushes every line; its cost is another candidate
for a controlled follow-up, not yet measured independently.

## Load instrumentation in 0.15

The engine already has a parallel image loader, but PS4 startup explicitly
disables it. Keep that setting: the existing implementation retains decoded
mip pyramids for all queued images before upload, so simply enabling threads
could significantly increase peak memory. A bounded producer/upload queue is
a possible later design if decode dominates; no such change is included here.

`platform/ps4/load_profile.{h,cpp}` provides scoped, atomic counters active only
inside `idImageManager::EndLevelLoad`. One aggregate report per load, not one
log line per image. Uses native monotonic clock ID4, matching the 0.14 fix.

Measured stages:

- `file_read_inflate`: engine ReadFile calls, including archive work; not pure
  physical-disk time and not every possible direct file API.
- `image_program`: top-level image program evaluation, including file reads,
  decode and image operations. Does not count recursive programs separately.
- `image_hash`: serial duplicate-image MD4 checksum.
- `mip_chain`: CPU mipmap construction in serial and worker paths.
- `texture_upload`: native Vulkan 2D uploader, including allocation, staging,
  command construction and submit/wait.
- `submit_wait`: nested queue-submit plus fence-wait interval in that uploader.

Counters are inclusive: file_read is inside image_program; submit_wait is
inside texture_upload. Do not add all stages to infer total wall time. Parallel
thread sums may exceed wall time. Cubemaps/early initialization and time before
EndLevelLoad are not fully covered. No GPU timestamp instrumentation is used.
Rendering, synchronization, audio and image quality are unchanged.

## Build and deployment evidence

Final external-data PKG: `dist/Doom3-PS4-Playable-Alpha-0.15.pkg`, 38862848 bytes.
SHA256: `d6746bab1237bd20fede8762d4825de1c650e9d86c67d319a46a53106d9bb225`.
Corrected package extracted/verified; upload and complete installed app.pkg
match. Live `D3LoadStart` code fingerprint matches the converted local ELF in
Doom PID230. Stable 0.14 PKG and matching ELF remain available for rollback.

A packaging defect was caught during this investigation: a manually invoked
create-fself command failed because OO_PS4_TOOLCHAIN was unset, while the next
shell command packaged an old eboot. The initial 0.15-labelled artifact actually
ran 0.14 code in PID227; it is **invalid for load profiling**. It is quarantined
under `artifacts/optimization-015/stale-package/`, not the current dist package.
The packager now requires the current build marker in raw ELF, converted OELF
and eboot before staging. The stale files demonstrably failed this check.
Conversion was then rerun with the SDK environment, checked successfully, and
the corrected package installed. Normal build script already uses `set -e`.
Marker checks detect this cross-version mismatch; they do not substitute for
full build provenance or live code checks for same-version edits.

13 data/config/diagnostic files were backed up after stopping each old process.
The first replacement preserved all13; separate backups retain original0.14
and the intermediate stale-runtime session. Logs and keys remain local/ignored.

Receiver's retained completed installation tasks565 and567 blocked subsequent
registration. Verified each old installed package by full hash, then reset only
our receiver's `ownedTask` with code fingerprint, compare-write and readback.
No BGFT cancellation or ShellCore credential/code changes. Final install task569.

Native launch requests can return0x80020023 without a ticket. Do not treat a
returned submission as a successful resume: check the return code and receiver
health before invoking RPC in its worker. One completion read timed out while
the receiver/worker were still suspended after a rejected resume. Preserved
that RPC and resumed receiver through ShellCore; verified rpc_go=0 afterward.
Do not overwrite a running RPC. Final Doom launch succeeded as ticket3, PID230;
worker credential restoration verified. Final evidence in `corrected/`.

Artifacts: `artifacts/optimization-015/`, final profiling/deployment evidence in
its `corrected/` subdirectory. Console commands now accept an explicit
`--console-timeout` (1–300 seconds) for blocking map loads. No command retries.

## Completed physical load profile

Corrected 0.15/PID230, first Mars City map in the process. OS/disk caches were
not reset. Total engine load122608ms: proc795, game18515, media103144,
settle154. Image phase wall100768.223ms, 2076 new images reported.

| Inclusive measured stage | Calls | Seconds |
|---|---:|---:|
| File read/inflate | 2247 | 18.013 |
| Image program, including reads | 2067 | 22.107 |
| Image hash | 2067 | 0.851 |
| Mip construction | 15023 | 0.437 |
| Texture upload, including submit/wait | 2067 | 73.390 |
| Submit/wait inside upload | 2067 | 32.907 |

Upload accounts for approximately73% of image-phase wall time in this serial
run. Its measured interval outside submit/wait is40.483s, including staging,
allocation and recording; this does not yet attribute each of those separately.
File reads overlap image-program timing; wait overlaps upload timing. The
image count differs because cube/generator/fallback paths are not all profiled.
122.6s versus the older131.4s is NOT evidence of optimization: no such change
was made and cache state is not controlled.

Source inspection explains a useful next experiment: generic buffer-to-image
upload computes a tiled address for every texel and emits many short GPU copies
for contiguous spans. Each 2D image then submits and waits separately. There
are2067 such upload/wait pairs in this sample. Test bounded staging batches or
a validated packed-tile staging path for static mip chains, preserving fences
and buffer lifetimes. The existing video-only packed path cannot simply be
used for multi-mip textures: layouts, padding and mip offsets need readback
validation first. Do not remove GPU waits or enable unbounded parallel decoding.

Priority based on this evidence: (1) upload preparation and small-copy overhead,
(2) safe batching of upload submissions, (3) archive/image input. Hash skipping
and mip generation are comparatively small targets. Runtime FPS still needs
finer frontend/render-command timing; shadows and resolution controls do not
establish a single hardware bottleneck.

Validation: engine build/link passed; corrected package extraction passed;
all three build markers checked; live profiler code verified; physical counters
completed; 21 existing tool tests passed, Python compilation and patch reverse
check passed. Old-runtime package was rejected by the new packaging guard.

Post-load smoke check: skipped arrival, captured visible playable hangar at
1280x720 and sampled another20s. Counter estimate6.984FPS, but the final
read took1661ms and the ship animation differed from the older stationary
controls. Retained as liveness evidence, not an A/B performance result.
Doom230 remains running with original quality settings; no save progress was
claimed beyond the map's ordinary autosave. Raw sample retained for inspection.
