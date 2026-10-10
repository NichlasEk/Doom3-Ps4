# Bounded texture uploads 0.16 — 2026-10-10

## Experiment

`r_ps4UploadBatch` controls PS4-only batching during `EndLevelLoad`. Final default0 (opt-in only),
not archived. Keep parallel image loading disabled. A dedicated command buffer
retains at most8 images and1MiB of source pixels. Larger images use the original
synchronous upload path. Texture formats, mip generation and copy commands are
unchanged. This is a loading optimization; no gameplay FPS gain is assumed.

A full batch, oversized texture, frame boundary, image destruction or end of
load flushes pending work. Staging allocations remain alive until queue submit
and an infinite fence wait succeed. Failure is fatal rather than releasing
in-flight storage. The dedicated command buffer is freed after the load. The
existing upload command buffer remains separate. GPU waits are preserved but
shared across multiple small images. Extra command-buffer memory is bounded
by the native allocator (32MiB); retained source pixels are bounded to1MiB.

`PS4 UPLOADBATCH` reports batched images and batch submissions. Total profiler
`submit_wait` also includes serial oversized images. Whole-load and image-wall
counters include the final flush. The final flush sits outside `texture_upload`,
so the old exact nesting relationship between upload and all waits no longer
holds; do not sum the inclusive stage counters.

## Validation and deployment

Full source build/conversion passed through `scripts/build-ps4-client.sh`.
Package extraction verified15 files. Initial experimental package0.16,38862848bytes:
`1eb3f21185d98d29f09a74d5e08056334dd1c47a2624bbd53b7b111909010121`.
Retained locally under `batch-default-on/`, along with its matching ELF.
Final opt-in package also0.16, distinguished by marker `upload-batch=opt-in`:
`36b9e5d2ee0a8967c94ca2520cc026a2d2fd0ef26b4d1766d774a34dc0995cd3`.
Use exact hash/marker rather than version alone.
Complete uploaded and installed hashes match. Stopped0.15 first, backed up13
files, verified rollback0.15 package hash and preserved config/save hashes.
Local artifacts: `artifacts/upload-batch-016/`; commercial assets remain ignored.

Fresh emulator profile loaded Mars City, completed60 frames and shut down
normally. Batched1960 images in491 submissions; total waits598 versus2067
unbatched images. Captured the traffic monitoring cutscene with visible video
inset/text artifacts; this is not evidence that all textures are correct or
that existing video corruption is fixed. The last progress capture is black
at shutdown; the explicit capture shows the rendered scene. Emulator timing
is not physical-console performance.

Receiver217/worker218 and ShellCore42 continued in the same console session.
Completed owned task569 was retired only after installed0.15 hash and completion
were verified; replacement task571 completed and installed0.16 hash matched.
ShellCore resume initially rejected with0x80020023/no ticket. Read completion
of known old ticket2, then a separate successful resume ticket3 and HTTP health
check. Worker old ticket3 completion read while active; Doom launch ticket4
created PID233. Worker credential restoration verified.

`doom_sdl.py --elf` selects the matching old ELF for controlling a running
baseline after rebuilding locally; physically verified by stopping0.15/PID230.
Saved RPC state remains valid only for the same continuously running process.

## Physical comparison

B/A/B, new process and first Mars City map for each run, same installed
0.16 package. B=batch1; A=batch0. Console/disk caches are not reset. No screenshots
or debugger FPS sampling during timed loading. Check logs and capture afterward.

Initial B1/PID233:150580ms total,116567.528ms image wall; file read/inflate
35285.036ms, upload67451.087ms, wait32129.890ms/598calls. Control A/PID236:
124608ms total,100301.077ms image wall; file18057.081ms, upload72876.790ms,
wait32877.459ms/2067calls. B1 game phase was also slower (30175ms). Thus the
first batch run does not establish an overall improvement. Its lower upload
interval does not explain the whole-load regression. B2 completed as below.

B1 reached playable Mars City Hangar (cinematic0, health100), screenshot reviewed;
no crash through clean shutdown. This is a scene smoke test, not exhaustive
texture correctness or long-session stability. Control A also reached gameplay.

| Run | Batch | PID | Total load s | Image wall s | File read s | Upload s | Submit/wait s | Wait calls |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| B1 | 1 | 233 | 150.580 | 116.568 | 35.285 | 67.451 | 32.130 | 598 |
| A | 0 | 236 | 124.608 | 100.301 | 18.057 | 72.877 | 32.877 | 2067 |
| B2 | 1 | 239 | 122.895 | 100.401 | 18.996 | 67.391 | 32.204 | 598 |

B2 game phase19342ms versus B1's30175ms. B1's higher read and game times
show substantial variability; cache/storage state was not isolated. B2 total
is1.4% lower than A, but image phase is0.1% higher. Fewer submits do not establish
an end-to-end loading improvement. Both B runs batch1960 images into491submits,
plus107 serial uploads. Wait time falls only about0.7s. Upload timing decreases
by about5.5s, but flush work can move outside that scope into frame boundaries;
use whole image-wall time as the decision criterion. Do not report that5.5s
as a delivered speedup. No crash in either batch run through normal shutdown.

Decision: keep batching opt-in, default0; do not promote as a performance fix.
Next useful investigation is reducing generic tiled-copy preparation/command
cost, with validated mip offsets and readback, not removing fences. No general
FPS or video improvement is claimed from this experiment.

Validation also passed21 existing Python tool tests, Python compilation, source
patch reverse-check and diff checks. Final package build/extraction passed again
after changing only the experiment default and marker.

## Final default-off verification

Replacement task573 completed; entire installed final package hash matched.
16-file backup retained from the experimental run; config/save hashes preserved.
Fresh PID242 launched via worker ticket7 with credentials restored. Console readback
confirmed batch0/default0, parallel images0, shadows1, screenFraction100.

Final A2, first map after replacement:150174ms total, image116500.712ms;
file read34334.443ms, image program38492.691ms, upload72859.844ms,
wait32889.361ms/2067calls. No UPLOADBATCH report, as expected with default off.
This closely matches B1's150580ms/image116567.528ms. Both first loads after
package replacement had much slower reads than later fresh-process loads.
This is consistent with a cache/storage-state effect, but does not prove its
cause. Both approximate pairs support the decision not to promote batching:
image phase is essentially unchanged with batching on versus off.

Final console remains running Mars City, PID242. Receiver217/worker218 suspended,
ShellCore42 last resume ticket6; worker last launch ticket7/scratch0x28000.
Never call suspended worker completion or reuse these identities after reboot.
Current receiver-owned install task573 is completed; retire only after verifying
its ownership and status before another package registration.

Final screenshot inspected: playable Mars City Hangar, health100, cinematic0;
console left running there. Screenshot stalls are excluded from performance
claims. Both on/off runs show the same broad scene; this is not a pixel-perfect
comparison because the ship animation and capture times differ.
