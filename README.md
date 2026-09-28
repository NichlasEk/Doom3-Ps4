# Doom 3 PS4 port

This repository ports [dhewm3](https://github.com/dhewm/dhewm3) toward PS4 with
OpenOrbis. The current milestone is a **dedicated diagnostic**, not a graphical
game build. It links the Doom 3 engine and base game code into a PS4 ELF. In
shadPS4 it reads the owned retail game archives, compiles `doom_main.script`,
runs 60 engine frames, and exits with guest status 0. See [port status](docs/PORT_STATUS.md)
for the exact evidence and remaining work.

The three owned PC retail discs are kept locally under `media/original/`.
Converted ISO images and extracted files live under `media/iso/` and
`media/extracted/`. The game archives are linked together under
`media/game/base/`: `game00.pk4` and `pak000.pk4` through `pak004.pk4`.

All `media/` content is excluded from Git because it contains commercial game
data. Recreate the extracted data with:

```sh
python3 scripts/extract_discs.py
```

The script requires `7z`. It converts the BlindWrite `.B6I` Mode 1 sectors to
ISO files, then extracts each disc separately. The `.B6T` metadata stays with
the original images.

Build the diagnostic with `scripts/build-ps4-diagnostic.sh`; run and verify it
with `scripts/run-emulator.sh`. The build uses the local OpenOrbis toolchain,
the source-built `create-fself` from `/home/nichlas/ut99-orbis`, and shadPS4.
`CREATE_FSELF`, `OO_PS4_TOOLCHAIN`, and `SHADPS4` can override those paths.
The emulator runner uses its own profile under `build/` and links to local game
data; it does not package or upload the commercial assets.

The native graphics stack has its own build: `scripts/build-native-vulkan.sh`.
Run its CPU-side driver checks with `scripts/test-vulkan-features.sh`.
See [Vulkan upgrade status](docs/VULKAN_UPGRADE.md) for implemented features,
validation, and remaining work toward DUDE's Vulkan 1.4 renderer.

A standalone shader probe now renders DUDE's real generic GUI/material shaders
in shadPS4 with an exact texture/alpha-test pixel check. Build it with
`scripts/build-shader-probe.sh generic`, then run
`xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-shader-probe.py generic`.
This validates a renderer component; the graphical game client is still pending.

The same probe supports `cube` (six-face upload/sampling) and `ambient`
(DUDE's original ambient-light shader pair). Both pass exact 1280×720 image
checks in shadPS4. See the upgrade status for the bounded cube implementation.

Additional modes: `interaction` runs DUDE's original direct-light shader pair;
`interactionshadow` enables its projected-shadow branch. `shadow`, `cubeshadow`
and `texops` isolate depth comparisons and texture fetch/gradient operations.
All use the same build/run commands above with the mode substituted. Those
fixtures use uploaded or host-written shadow depths.

`shadowcast` adds a real depth-rendering pass: overlapping geometric quads write
a 128×64 shadow map, then DUDE's interaction shaders sample it. The pixel check
also verifies depth ordering and clearing when the projection moves between
frames. This remains a standalone scene, separate from the Doom game client.
