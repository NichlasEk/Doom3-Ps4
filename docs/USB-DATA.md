# External Doom 3 data

Playable Alpha 0.04 contains no retail PK4 archives. Put your owned files at
`DOOM3/base/pak000.pk4` through `pak004.pk4` on USB, keeping lowercase archive
names. Alternatively copy them to `/data/doom3-game/base/` on the PS4.
The internal data directory takes priority; USB slots 0 through 7 are searched.
Missing data is logged in `/data/client-vulkan.log`, then startup stops.
Settings and logs remain in `/data/doom3-client`.

USB uses the same read-only sandbox mapping adapter as UT99, with distinct
`/doom3_usbN` aliases and temporary credentials restored before engine startup.
The existing compatible homebrew kernel service is required; no service is
installed by this application. Only mappings owned by this process are released.

Build dependency: bucanero's ps4-libjbc, revision
`835fe016ff0ae5dd89b9249f39cc0fe093fd07dd` from
https://github.com/bucanero/ps4-libjbc.
Set `DOOM3_JBC_SOURCE` to that checkout or use the local ScummVM source cache.
The upstream dependency has no declared license; this is a private hardware
test artifact. Source and archives remain outside Git. Build adaptations and
input hashes follow the existing UT99 script.

The package runs continuously. Use the right stick as the menu cursor and R2
to click; Options opens the menu. See PLAYABLE-ALPHA.md in the repository for
controls and known limitations. Audio is currently disabled. Do not copy the
PC executable or game00.pk4; the game module is compiled into the PS4 client.
