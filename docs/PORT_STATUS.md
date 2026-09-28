# Doom 3 PS4 port status

Assessment: 2026-09-28. Engine: dhewm3 pinned at
`455b88e8dff2be822f08eb498f51b383e851fa38`, with the changes in
`patches/001-ps4-diagnostic.patch`. The owned retail data is local and ignored.

## Verified

- Desktop CMake build of dhewm3 with statically linked base game code succeeds.
  A desktop run read `pak000.pk4` through `pak004.pk4`, initialized OpenGL and
  OpenAL, and compiled `script/doom_main.script` from the disc data.
- OpenOrbis Clang links the dedicated engine and base game code into a PS4 PIE
  (`build/ps4/dhewm3ded`). `create-fself` produces `build/runtime/eboot.bin`.
  Imports are open firmware stubs for UserService, Pad, AudioOut, VideoOut, Net,
  and libkernel. No PRX/SPRX from another project is packaged.
- In an isolated shadPS4 profile, the guest read all five retail `pak` archives
  with checksums `28d208f1`, `40244be0`, `c51ecdcd`, `cd79d028`, `765e4f8b`.
  It compiled `doom_main.script`, completed initialization, advanced 60 engine
  frames, shut down, and requested exit status 0. The emulator host may linger
  until its timeout after guest exit; the runner checks guest and host markers.

The diagnostic uses upstream's dedicated server stubs for GL and OpenAL. It
does **not** show Doom 3 graphics or prove gameplay. Physical PS4 execution is
unverified.

## Changes in the diagnostic patch

- Select SDK SDL2 headers/static library and link the required firmware stubs.
- Keep game code in the executable (`HARDLINK_GAME`) to avoid a game DLL.
- Use `/data/doom3-game` for read-only game data and `/data/doom3-ps4` for logs
  and settings; use `/app0/eboot.bin` as the executable path.
- Avoid `FIONBIO`, unavailable in this SDK, by using `fcntl(O_NONBLOCK)`.
- Keep network discovery local for this single-player diagnostic. shadPS4's
  `getifaddrs` path otherwise reaches an unsupported socket type.
- Disable the asynchronous download and game timer threads only when
  `DHEWM3_PS4_BRINGUP=ON`. The SDK SDL thread starts in shadPS4 but crashes in
  `SDL_RunThread`; a real gameplay build needs a correct threading path.
- Avoid the unsupported SDL `dummy` video driver in dedicated mode. The
  diagnostic initializes no video subsystem and intentionally skips rendering.

## Next engineering gates

1. Build the normal client with a PS4 video/input backend, stable threading,
   and an AudioOut mixer. Verify it reaches game initialization in shadPS4.
2. Port Doom 3's ARB2 renderer. Its ARB vertex/fragment programs and OpenGL
   state use more than the current ScummVM PS4 GL subset. Translate shaders and
   implement required depth, stencil, shadow, compressed texture, buffer, and
   render target behavior on OpenGNM/Vulkan; validate actual rendered frames.
3. Validate menu, map load, controls, sound, saves, gameplay, then physical PS4
   packaging and execution separately.
4. Check game-data version requirements. These original CDs provide
   `pak000`–`pak004`; the latest dhewm3 documentation recommends Doom 3 1.3.1
   data. The diagnostic's 29 missing string IDs are a visible data-version gap.

Sources: [dhewm3](https://github.com/dhewm/dhewm3),
[original Doom 3 GPL release](https://github.com/id-Software/DOOM-3),
[OpenOrbis](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain).

## Renderer reference found

[DUDE](https://github.com/Inkub0/dude) is a GPLv3 dhewm3 fork that already
translates the original Doom 3 ARB shaders into GLSL and has GL3 and Vulkan
renderers. A local read-only checkout at `.tools/dude-reference` is pinned for
inspection at `3b6872fe278802496cc7a8f8209b847c68efed2a`. This can save
shader translation and RHI design work. Its current Vulkan backend explicitly
requires Vulkan 1.4, while the inspected `vulkan-ps4` driver reports Vulkan
1.1. It is therefore a reference and possible donor for a reduced 1.1 backend,
not a ready PS4 renderer. No DUDE code has been merged into this build.

The graphics upgrade has started in an independent native stack in this project.
[Upgrade status](VULKAN_UPGRADE.md) records the new memory-mapping entry points,
feature negotiation, tests, and remaining Vulkan 1.4 requirements. It is not yet
connected to the dedicated diagnostic or a graphical Doom client.

A standalone PS4 graphics probe now runs DUDE's unmodified generic GUI/material
shader pair in shadPS4. Its texture/alpha-test image matches all 921600 expected
pixels. A separate demote/derivative/two-set probe also passes. The source audit
compiles 127/172 DUDE graphics stages to Liverpool GCN. See the
[shader milestone](VULKAN_UPGRADE.md#shader-and-gpu-milestone--2026-09-28).
These graphics results are separate from engine integration and gameplay.

The native graphics probe also runs DUDE's unmodified ambient-light shader pair.
Both its expected light result and a separate six-face cubemap test match all
921600 pixels in shadPS4. The shader audit now compiles 132/172 stages to PS4 GCN.
The cube path is deliberately bounded to a single RGBA8 cube and mip; direct
lighting/shadow sampling and integration into the graphical game client remain.
