# Graphical PS4 client diagnostic

The graphical client uses DUDE at
`3b6872fe278802496cc7a8f8209b847c68efed2a`, with
`patches/002-ps4-client.patch`. Sources are cloned into ignored
`build/dude-client-source`; the dedicated dhewm3 submodule remains separate.
DUDE is GPLv3; retain its license and source when distributing this derivative.

## Reproduce

Prerequisites are the existing native stack/OpenOrbis toolchain, CMake/Ninja,
`glslc`, the source-built `CREATE_FSELF` converter, shadPS4, Xvfb and ImageMagick.
The defaults use the installed tools on this development host. `CREATE_FSELF`,
`OO_PS4_TOOLCHAIN`, `SHADPS4` and `DOOM3_DATA` override their locations.

```sh
./scripts/build-ps4-client.sh
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-ps4-client.py
```

Retail data stays outside Git: `media/game/base/pak000.pk4` through `pak004.pk4`.
The runner links it into an isolated emulator profile at
`build/client-profile/shadPS4/data/doom3-game`. Logs/settings use
`/data/doom3-client`; native driver diagnostics use `/data/client-vulkan.log`.
Artifacts and screenshots are under `artifacts/client/`. The client runs 60
engine frames and shuts down. The default runner deadline is 240 seconds
(`CLIENT_TIMEOUT` overrides it); this is a slow diagnostic, not a benchmark.

## Verified milestone

In shadPS4 0.18.0, the client initializes the real engine, reads the retail
archives, and renders the Doom 3 main menu (Mars, logo and menu buttons).
The finite run completed 60 engine frames, presented 64 images including
startup, and exited with guest status 0. This establishes menu rendering,
not functional controls, map rendering, gameplay, audio or physical PS4 support.
The runner checks lifecycle separately from visual inspection; its JSON does
not automatically declare the menu correct based on a successful exit.

## Port changes and limits

- Private fixed 1280x720 VideoOut WSI bridge, no SDL window. Its null surface
  contract is specific to this native ICD.
- PS4 Vulkan baseline is 1.1, VMA 1.1 dispatch. Desktop DUDE still requests 1.4.
  This is not Vulkan 1.4 conformance. Optional tessellation/geometry are no
  longer advertised by the native driver; compute descriptors are skipped in
  this diagnostic, and optional compute effects must remain disabled.
- Hardlinked base game, sound disabled, asynchronous download/timer threads
  skipped. A playable client needs input, audio and a proper threading path.
- RGBA textures, one-mip cube path, D32S8 scene depth. Compressed/precompressed
  textures and HDR disabled in the diagnostic startup arguments.
- Dynamic UBO offsets are validated in binding order, with bounds/alignment
  checks and per-draw descriptor snapshots. The `dynamic` probe checks two
  differently colored draws in one recording (921600 pixels).
- Bounded full-image D32 depth-plane copies require matching dimensions
  and tile layout. Sampled transfer destinations use the same depth layout;
  D32S8 sampling and partial/differing-layout copies fail closed.
  The `depthcopy` probe draws depth geometry and samples its copied image in
  DUDE's projected-light shader.
- 1:1 RGBA8 scene-to-VideoOut copying uses the GPU detiling/readback path. This
  records many DMA packets and is slow. Recording capacity is 32 MiB with
  packet-aligned submissions. A shader blit is the next performance task.
- Failed frame recording now stops the client before submitting that buffer.

## Known graphics defects

The `present` probe is deliberately **not passing**: drawn green/blue pixels
survive offscreen rendering and presentation, but the intended colored clear
background is black (460800 of 921600 pixels differ). This isolates an existing
load-op clear problem for tiled color attachments. The expected image has not
been relaxed. Menu rendering does not establish this clear behavior is correct.
The separate D32S8 copy experiment rendered an incorrect shadow image.
Sampled D32S8 images are therefore rejected, and the PS4 client explicitly
reports scene-depth capture as unavailable. Depth-dependent screen effects are
not validated. The `depthcopy` fixture checks both D32 pixels and D32S8 rejection.
Other scaled/flipped color blits remain incomplete and require independent
validation before gameplay claims. Original CD data also produces missing
string-ID warnings; later Doom 3 data compatibility remains a separate gate.

```sh
./scripts/build-shader-probe.sh dynamic
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-shader-probe.py dynamic
./scripts/build-shader-probe.sh depthcopy
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-shader-probe.py depthcopy
./scripts/build-shader-probe.sh present
# Expected to fail until the tiled color clear is fixed:
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-shader-probe.py present
```

## Checkpoint evidence (2026-09-28)

Final client run: `artifacts/client/result.json` reports initialization, 60
frames, capture and guest exit 0. `capture.png` was manually inspected and
shows the main menu. The runner's automatic `visual_validation` field stays
`not performed`; visual inspection is a separate observation.

- Client ELF SHA-256: `48d422ceeabd1f5c2564f14f394a2be208dbf0c62b0851c57f0ca6e501236e60`
- Eboot SHA-256: `0cf3ee2ff3516b065740ba91685bca33a86af17bc9c61dd71ef9b1480a82e4e8`
- `dynamic`: all 921600 pixels correct, guest exit 0.
- `depthcopy`: all 921600 pixels correct, guest exit 0; sampled D32S8 creation rejected.
- `present`: 460800 incorrect pixels, guest exit 0; known clear defect retained.
- Host ASan/UBSan feature, map-memory2 and descriptor-offset checks pass.
- Both source patches apply cleanly against their pinned upstream revisions.

Artifacts, game data and intermediate sources remain ignored by Git.
