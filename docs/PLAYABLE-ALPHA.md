> Latest installation candidate: [0.05 package identity correction](INSTALL-005.md).

# Playable Alpha 0.04 — 2026-09-29

This is the first continuous, controller-driven Doom 3 client candidate.
It uses the newer compiled fullscreen color-clear path. It is still a silent
alpha with renderer limitations, not a completed console port.

## Install and controls

Install `dist/Doom3-PS4-Playable-Alpha-0.04.pkg` (`DM3P00001`, version 00.04).
Keep owned retail archives outside it: `DOOM3/base/pak000.pk4` through
`pak004.pk4` on USB, or `/data/doom3-game/base/` internally. See
[USB setup](USB-DATA.md). The package contains no retail PK4 files.
Settings and saves use `/data/doom3-client`; save/load has not yet been tested.

| Control | Action |
| --- | --- |
| Left stick | Move (digital WASD thresholds) |
| Right stick | Look / menu cursor |
| R2 | Attack / menu click |
| L2 | Zoom |
| Cross | Jump / menu Enter |
| Square | Reload |
| Triangle | PDA |
| L1 | Run |
| R1 / D-pad down | Crouch |
| D-pad up | Flashlight |
| D-pad left/right | Previous/next weapon |
| Options / Circle | Menu / Escape |

D-pad acts as arrow keys while a menu is active. Controls use the original
retail default key bindings; existing custom bindings may change actions.
The normal package has no frame limit. The test runner alone adds bounded
markers beside its chosen eboot. Right-stick menu navigation still needs a
hands-on console check.

## Changes

- Native ScePad events feed the engine keyboard/mouse queues, including releases
  on disconnect. Look speed uses elapsed time with a bounded stall interval.
- The real game loop runs continuously. Optional `doom3-test-frames.txt` and
  `doom3-test-map.txt` under `/app0` are test-only and excluded from the package.
- Retail CD single-player scripts may lack `WEAPON_NETFIRING`; link that field
  only when present. Multiplayer still requires it. Networking is not tested.
- PS4 startup disables parallel image loading after a reproducible worker
  access violation. Sequential loading gets through the tested maps.
- A fullscreen sampled draw presents the scene instead of a large per-pixel
  detiling command stream. It supports the fixed RGBA8 two-image PS4 bridge.
  Descriptor sets are separate per target; teardown follows device idle.
- RGBA8 UNORM clear colors are quantized before FP16 export to avoid an observed
  one-byte rounding error. The existing bounded clear-pipeline cache remains.

## Evidence and limitations

- `artifacts/input-game/`: a single-player session in `game/mp/d3dm1` rendered
  a textured, lit room with player fists and HUD. Automated controller input
  moved and turned the player and issued attack commands. Physics changed
  position and health after a fall. This is not a network multiplayer test or
  proof of successful combat against enemies.
- `artifacts/campaign/`: `game/mars_city1` loaded and completed 600 engine frames
  with clean guest exit. The inspected capture shows the opening cinematic's
  traffic-monitor screen, with corrupt overlay pixels in the upper left.
  This does not establish campaign completion or artifact-free rendering.
- `artifacts/shader-probe-sampledpresent/`: all 921,600 pixels match in the
  presentation regression, including alternating source images and clear color.
- Final extracted-package test: 360 frames, 16 gameplay telemetry samples,
  movement/turn/attack and release checks pass, clean guest exit. Its inspected
  capture shows a textured room, crosshair, fists and health HUD. Evidence:
  `artifacts/playable-package/`. All 13 package payload/metadata checks pass;
  no PK4 archives are bundled (`artifacts/package/`).

Audio is disabled. Physical PS4 gameplay, USB loading for this candidate,
save/load, extended play, controller feel and sustained performance remain
unverified. Compressed/precompressed textures, HDR and parallel image loading
are disabled. Native Vulkan support remains bounded, not full conformance.

## Reproduce

```sh
scripts/build-ps4-client.sh
python3 scripts/package-ps4-client.py --skip-build
python3 scripts/verify-ps4-package.py
CLIENT_EBOOT="$PWD/build/client-package-playable-004-extracted/uroot/eboot.bin" \
CLIENT_TEST_MAP=game/mp/d3dm1 CLIENT_TEST_FRAMES=360 CLIENT_INPUT_TEST=1 \
CLIENT_TIMEOUT=600 CLIENT_PROFILE="$PWD/build/playable-package-profile" \
CLIENT_ARTIFACTS="$PWD/artifacts/playable-package" \
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-ps4-client.py

scripts/build-shader-probe.sh sampledpresent
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-shader-probe.py sampledpresent
```

Regenerate presentation SPIR-V with `python3 scripts/generate-present-shaders.py`
(glslangValidator and spirv-val). Native/engine changes are preserved as patches;
ignored dependency checkouts and owned assets are not committed.

## USB delivery 2026-09-29

See [USB delivery checks and console test steps](USB-TEST-2026-09-29.md).
