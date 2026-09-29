# External Doom 3 data

Clear External 0.03 contains no retail PK4 archives. Put your owned files at
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
