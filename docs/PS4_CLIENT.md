# Graphical PS4 client

Current build: [Playable Alpha 0.04](PLAYABLE-ALPHA.md). The older milestone
below records the initial finite menu test; current packaged builds are interactive.

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
Artifacts and screenshots are under `artifacts/client/`. The runner writes a 60-frame test marker and shuts the client down after that
bounded run. The package contains no test marker and runs continuously. The default runner deadline is 240 seconds
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
  skipped. Native input is now implemented; audio and broader threading support remain pending.
- RGBA textures, one-mip cube path, D32S8 scene depth. Compressed/precompressed
  textures and HDR disabled in the diagnostic startup arguments.
- Dynamic UBO offsets are validated in binding order, with bounds/alignment
  checks and per-draw descriptor snapshots. The `dynamic` probe checks two
  differently colored draws in one recording (921600 pixels).
- Bounded D32/D32S8 depth-plane copies now use a fragment shader and depth
  attachment writes. Single-mip/layer 2D copies support different dimensions
  and nonzero source/destination offsets. Sampling uses the depth plane with
  HTILE disabled. The `depthcopy`, `depthstencil` and `depthregion` probes sample
  copied geometry in DUDE's projected-light shader. Stencil-plane copying,
  multisampling, mip chains and cube attachments remain unsupported.
- Tiled RGBA8 clears use a regenerated PSBC shader, its indirect resource table,
  immutable GPU constants and explicit viewport/scissor/depth state. Colors
  are quantized before FP16 export to preserve the requested UNORM byte value.
  The `present` probe alternates the clear color between frames and checks all
  background pixels alongside the green/blue draws.
- The alpha uses a fullscreen sampled draw for RGBA8 scene-to-VideoOut presentation. The older diagnostic used the GPU detiling/readback path. This
  records many DMA packets and is slow. Recording capacity is 32 MiB with
  packet-aligned submissions. A shader blit is the next performance task.
- Failed frame recording now stops the client before submitting that buffer.

## Known graphics defects

The tiled-color clear and D32S8 depth-copy defects are fixed in focused shadPS4
pixel tests. Scene-depth capture is enabled again in the PS4 client. These
component checks do not establish correct depth-dependent effects in gameplay.
Other scaled/flipped color blits remain incomplete and require independent
validation before gameplay claims. Original CD data also produces missing
string-ID warnings; later Doom 3 data compatibility remains a separate gate.

```sh
./scripts/build-shader-probe.sh dynamic
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-shader-probe.py dynamic
./scripts/build-shader-probe.sh depthcopy
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-shader-probe.py depthcopy
./scripts/build-shader-probe.sh present
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-shader-probe.py present
```

Additional isolation probes: `clear` (background only), `stencilcast` (direct
D32S8 attachment sampling), `depthstencil` (full D32S8 depth-plane copy), and
`depthregion` (160x96 source to 128x64 destination, then a second copy with
nonzero source/destination offsets). Each uses the same build/run commands
above with its mode name. The offset copy also checks untouched pixels and
that two recorded copies keep separate parameter snapshots.

To regenerate the embedded driver shaders with the pinned patched compiler:

```sh
python3 build/native/vulkan-ps4/scripts/generate-meta-shaders.py \
  --psbc build/native/opengnm-psbc/opengnm-psbc
```

Both GLSL sources and generated PS4 binaries are retained in the native patch.

## Checkpoint evidence (2026-09-28)

Final client run: `artifacts/client/result.json` reports initialization, 60
frames, capture and guest exit 0. `capture.png` was manually inspected and
shows the main menu. The driver recorded 60 D32S8 copies: 960x645 regions
from the 1280x720 scene into a 1024x1024 capture texture. The runner's automatic `visual_validation` field stays
`not performed`; visual inspection is a separate observation.

- Client ELF SHA-256: `d35744c11b855915c8cae2c991a2d826cf2b4f562c39f59a63f5399f1e5c4c92`
- Eboot SHA-256: `4b0a08c757e877f3ce036329506cafc4e3deeba54f28dfefbef3675d3be713c3`
- `dynamic`: all 921600 pixels correct, guest exit 0.
- `depthcopy`, `stencilcast`, `depthstencil`, `depthregion`, `present`: each checks
  all 921600 pixels, with zero mismatches and guest exit 0.
- Host ASan/UBSan feature, map-memory2 and descriptor-offset checks pass.
- Both source patches apply cleanly against their pinned upstream revisions.

Artifacts, game data and intermediate sources remain ignored by Git.
