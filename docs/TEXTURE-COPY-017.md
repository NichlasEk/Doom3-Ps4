# Static texture copy experiment 0.17 — 2026-10-10

## Change

0.16 batching reduced submissions but not image-load wall time. The generic
upload path computes a tiled address for each texel and emits many short DMA
copies. This experiment instead packs CPU-owned RGBA8 mip levels into the image's
existing tiled layout and transfers the padded backing in large DMA chunks.
Generic Vulkan buffer-to-image semantics and the video-only path are unchanged.

`r_ps4PackedTextures` is a separate, non-archived test switch, initially default0; final default1 after the repeated physical results below.
Only active inside the level image-load window. Bounds: 2D optimal-tiled RGBA8
UNORM, one layer/sample, no image flags or render targets, at most1024x1024,
11mip levels,16MiB padded backing. Unsupported images keep the generic path.
Each mip uses the same Gpa surface size/offset and coordinate functions as the
reference uploader. Validate all mip storage intervals and allocation bounds;
zero padding; pack directly into the mapped coherent staging buffer. If packing
fails, refill that buffer with the original linear pixels and use generic copies.
No unbounded decode queue or permanent texture-copy cache is introduced.

The image is allocated before staging to query its actual padded layout. Source
pointers still belong to the caller. Staging remains alive and unmodified until
the original blocking submit/fence completes. Existing barriers are preserved.
Batching remains separately off. No general gameplay FPS or video gain assumed.

`PS4 TEXTUREPACK` reports successful packed versus generic uploads. `cpu_pack`
is a new inclusive profiler stage inside texture_upload. The final batch flush
can still sit outside texture_upload if a future experiment enables batching.

## Correctness gates

New `textureupload` shader-probe mode compares GPU-written image backing for the
new path against unchanged generic upload. Nine shapes on each of Onion/Garlic:
1x1,3x5,8x8,17x9,513x509,1024x1,1x1024,256x128,1024x1024. Full mip chains,
two changing patterns, nonzero image binding and staging offsets. Prefix/suffix
guards, padded bytes and nonzero output are checked after a fence. Reject null
images, SRGB, oversized images, invalid mip/dimensions and insufficient capacity.
Explicit flush/invalidate calls included for the noncoherent memory type.

Emulator PASS:18 shape/heap cases,36 updates,38813696 backing bytes match; also
921600/921600 video sampling pixels match, clean exit. Initial Onion-only run
retained separately. This is emulator GPU evidence, not physical readback.

Full Mars City emulator run uses a fresh private profile with packed upload on:
2062 packed images,5 generic;60 frames, normal shutdown. Image wall26.881s,
upload14.694s including CPU pack13.746s and submit/wait0.628s. Timings are not
physical-console performance. Capture shows the traffic monitoring scene with
existing video inset/text artifacts, not a full-game pixel correctness claim.

## Deployment

Initial opt-in0.17 PKG,38862848bytes:
`7313eb593b3a8316e0d977d0c51ec5056dbe6cdf4722ef53a8026fd7639274b2`.
Full native build/conversion and extracted package validation passed. Backup of
17 files after stopping0.16; rollback0.16 package hash verified. Staged hash and
full installed hash verified, config/save hashes preserved. Install task575.
Artifacts: `artifacts/texture-copy-017/`; probe: `artifacts/shader-probe-textureupload/`.

Physical comparisons: fresh process/first map, same installed package,
old and packed path interleaved. OS/disk caches are not reset. Separate first
load after package replacement from subsequent process restarts, as0.16 showed
substantially different read times in those contexts. No screenshot/FPS sampling
during timed loads.

## Initial physical results (same opt-in package)

A1/PID245, packed0:122082ms total,100351.148ms image wall; file18164.079ms,
upload72819.942ms, wait32915.264ms/2067calls, CPU pack0. Report0 packed/2067generic.
B1/PID248, packed1:116678ms total,84768.772ms image wall; file34842.701ms,
upload40468.461ms, wait1904.515ms/2067calls, CPU pack37645.043ms/2062calls.
2062packed/5generic. Game phase27728ms in B1, also slower before image packing.
B1 reached playable hangar, screenshot reviewed, then cleanly quit.

The upload interval drops32.351s (44.4%), while read time nearly doubles in B1.
Do not extrapolate overall map-load benefit from this one pair. The slow-read
condition is not exclusive to the first load after package installation (B1
is a later restart), so prior cache explanations remain hypotheses. A2/B2 completed below.

## Repeated physical comparison and adoption

All four use the exact opt-in package hash above, first Mars City load in a new
process. Only r_ps4PackedTextures changes. Batch0, parallel images0, shadows1,
screenFraction100. Settings read back via condump. No FPS sampler/screenshots
during load. File/system cache state was not reset or isolated.

| Run | PID | Packed | Total s | Image wall s | File read s | Upload s | Submit/wait s | CPU pack s |
|---|---:|---:|---:|---:|---:|---:|---:|---:|
| A1 | 245 | 0 | 122.082 | 100.351 | 18.164 | 72.820 | 32.915 | 0 |
| B1 | 248 | 1 | 116.678 | 84.769 | 34.843 | 40.468 | 1.905 | 37.645 |
| A2 | 251 | 0 | 148.468 | 115.617 | 33.466 | 72.822 | 32.907 | 0 |
| B2 | 254 | 1 | 87.706 | 65.433 | 15.726 | 40.472 | 1.946 | 37.641 |

Each packed run:2062 packed/5generic,2067 total submissions. No batching. Both
old-path runs return essentially the same upload interval, as do both packed
runs. Upload saves32.35s (44.4%); submit/wait saves about31s with fences retained.
These are CPU-side inclusive intervals, not GPU timestamp measurements.

Total load varies considerably with read and game initialization time. Runs
with broadly similar read times show122.082→87.706s and148.468→116.678s. These
are descriptive comparisons, not cold-cache controls or a universal percentage.
The repeated upload reduction supports adopting the path despite I/O variation.
This change targets loading only; gameplay FPS and cinematic decode are unchanged.
Both packed runs reached gameplay and exited cleanly. B1 hangar screenshot
shows expected textures; no exhaustive physical pixel/readback test was done.

Final package keeps version0.17 but changes the marker to `static-texture=packed`
and default to1. Opt-in candidate and matching ELF retained under `opt-in/`.
Final SHA256:
`12b0eabf59efc5cfe26730b413facbac8e1e21631a57e875850237396c486f60`.
Rebuilt through full build script, package extraction passed15 files. The default
and marker are the only engine changes after the four measured runs. Final
installation/first-load validation follows separately; do not mix package hashes.

## Final installed verification

Final task577 completed. Full installed package hash matches the final hash
above; config/save hashes preserved against the21-file pre-replacement backup.
Doom PID257, launch ticket12, worker credentials restored. Console defaults read
back: packed1/default1, batch0, parallel images0, shadows1, fraction100.

Final first map:120918ms total; proc798/game32526/media87430/settle164ms.
Image wall84785.462ms, file34962.568ms, upload40431.055ms, submit/wait1883.252ms,
CPU pack37655.531ms;2062 packed/5generic. Upload improvement repeats in the final
binary without a manual enable command. Whole load is slower than B2 because
file/game initialization varies; this result is retained, not omitted.
Reached playable hangar; screenshot reviewed. No physical full-image byte
comparison or long-session stability claim. Emulator differential probe and
native console smoke remain separate evidence gates.

Current receiver217/worker218 suspended while Doom257 runs. ShellCore42 last
resume ticket11, worker last launch ticket12/scratch0x3c000. Completed owned
install task577 remains retained. Never reuse these identities across reboot
or poll worker completion while suspended. Rollback0.16 and opt-in0.17 retained.

Build/conversion, package extraction, GPU probe, full-map emulator, both native
patch reverse-checks, Python compilation,15 existing debugger tests and diff
checks passed. Commercial data, packages, private session state/logs stay ignored.

Final stationary hangar sample after screenshot:133frames/19.8315s=6.7065FPS,
99 samples, maximum debugger read 7.044ms. This is a health/baseline sample,
not an interleaved gameplay FPS comparison. No gameplay FPS improvement claimed.
