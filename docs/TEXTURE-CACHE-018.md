# Bounded texture address cache (0.18)

Target: the loading screen after the main menu, before Mars City starts.
0.17 still spends about 37.7 seconds computing tiled pixel addresses during
first-map loading on the physical PS4. This cache reuses those addresses for
mips with identical dimensions, complete Gpa surface context and surface size.
It stores no pixels, image handles, allocation addresses or GPU resources.

The cache holds at most 8 MiB of uint32 offsets and 64 entries, evicting least
recently used entries before allocating. Every offset is checked against the
mip's current bounds before an entry becomes usable. Offsets are relative to
the mip; its image-specific base is added only when copying current source
pixels. A try-lock makes concurrent callers fall back to the uncached routine.
Allocation or coordinate failure also falls back. Clear at EndLevelLoad frees
all entries; synchronization and staging lifetimes are unchanged. Video uses
its existing separate upload path.

`r_ps4TextureOffsetCache` initially defaults to 0 for same-package comparisons;
`r_ps4PackedTextures` remains 1, `r_ps4UploadBatch` and parallel image loading 0.
Native `TEXTURECACHE` logs hits, misses, evictions and retained bytes at clear.
The existing `cpu_pack` timer includes lookup, cache construction and copies.
Profiler stages are inclusive; do not add them to each other.

## Correctness

Emulator GPU byte comparison against unchanged generic uploads passed 22
shape/heap cases, 44 updates and 61,190,144 backing bytes, including padding,
nonzero memory/staging offsets and guards. Eleven dimensions include odd sizes,
1024-square and both 1024x512 orientations. Observed 218 hits, 130 misses and
100 evictions, followed by two clears (second reports zero). Existing video
sampling passed all 921,600 pixels. These are emulator, not physical GPU byte
comparison results. Artifacts: `artifacts/shader-probe-textureupload/`.

Full emulator Mars City load, 60 frames and clean exit passed with cache on:
2062 packed images / 5 generic; image wall 14.790s, upload 2.862s, CPU-pack
1.903s, wait 0.626s. Hits16017/misses1011/evictions947; 1,694,472 bytes before
clear. Emulator timing is not console timing. Artifacts:
`artifacts/texture-cache-018/emulator/` and private profile
`build/texture-cache-018-profile/`.

## Physical experiment

Opt-in package SHA256:
`29c8829ed127fe16c286146160a9ebdf1857285f8e4a63583005b2da7e893389`.
Package extraction (15 files), staged readback and installed full hash passed.
24 backup files retained, config/save hashes preserved. Prior 0.17 PKG and
matching ELF/eboot retained as rollback. Installer task579.

First-map loads use fresh processes, same package and read-back cvar settings.
File/system caches are not reset. No screenshots or FPS sampler during loading.
Results follow.

Initial B1 (PID269, cache1): total81.448s, image46.197s, read/inflate29.922s,
upload7.180s, CPU-pack4.563s, wait1.722s. Game phase31.598s.
Cache hits16017/misses1011/evictions947, 1,694,472 bytes before clear.
Intro screenshot reviewed after timing, then normal shutdown. This initial
result is followed by the interleaved cache-off control and repeat below.

A1 (PID272, cache0): total121.384s, image86.571s, read/inflate36.559s,
upload40.711s, CPU-pack37.884s, wait1.903s. Game phase30.575s.
Cache counters all zero. Engine reached intro with health100. A diagnostic
condump was attempted near the end of loading, but console-open timed out;
no command text was sent or retried. The engine log subsequently provided
complete timings. Keep this input attempt disclosed for the comparison.

B2 (PID275, cache1): total53.432s, image31.165s, read/inflate14.833s,
upload7.242s, CPU-pack4.560s, wait1.788s. Game phase19.088s.
Same cache counts and retained bytes as B1;2062 packed/5generic; intro reached
with health100 and clean shutdown. B2 had faster reads and game initialization;
do not attribute the entire 67.952s difference from A1 to the cache.

| Fresh process | Cache | Total s | Read/inflate s | Upload s | CPU pack s |
|---|---:|---:|---:|---:|---:|
| B1 /269 | on |81.448|29.922|7.180|4.563|
| A1 /272 | off |121.384|36.559|40.711|37.884|
| B2 /275 | on |53.432|14.833|7.242|4.560|

The repeatable reduction is about33.3s in CPU-pack and33.5s in total upload,
with all GPU fences retained. The memory-layout transformation takes about88%
less time. No gameplay FPS gain is claimed. Same code/PKG throughout B/A/B;
final candidate changes only default to1 and build marker to offset-cache=on.
Artifacts b1-complete/, a1-later/, b2-complete/ contain engine/native logs.
Opt-in package and its matching runtime are retained under opt-in/.

## Final default-on package

Final0.18 SHA256:
`d36f4f2320730c5850d38b9ff2ac94f0fcb07cd1b1efbe72dce23b876834dcc1`.
38,862,848 bytes, external-data package; marker `offset-cache=on`.
The only runtime changes from measured opt-in0.18 are cvar default and marker.
Package extraction15files and staged full readback passed. Prior opt-in package
backup contains27files. Final installer task581. Final installed readback and
first-map smoke results are recorded below.

Final full installed hash matched and config/save hashes were preserved. Launch
ticket17 returned submission0, but debugger queries timed out before process
observation and credential restoration. Reconnection found DoomPID278; a guarded
recovery revalidated worker218/proc/credential identity and restored its temporary
system bit, verifying the exact baseline tuple. Evidence:
final/credential-recovery.json and launch journal41e5dd86... . No launch retry.

Final default-on PID278 first-map:53.012s total,31.315s image wall,
read/inflate14.802s, upload7.416s, CPU-pack4.563s, wait1.956s. Same cache counts
as B1/B2. Defaults read back cache1/packed1/batch0/parallel0; marker confirmed.
Final engine/native logs: final-load/. Existing 0.17 timing with comparable
fast file reads was87.681s; this is descriptive, not a controlled cold-cache
percentage. No screenshot or FPS sampler overlapped this load.
Final screenshot remote1791632055196643668.png inspected: arrival intro is
rendering, including its monitoring screen and inset. This single capture does
not establish all textures/videos are correct. Doom278 left running for user.
