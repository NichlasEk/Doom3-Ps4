# Bundled data selection and audit 0.07

## Physical update — 2026-09-30

The user confirms the game now works on physical PS4 with game data bundled
in the PKG. They identify their slow USB stick as the reason files could not
be read in time; bundling the data resolved the problem in their test.
Use bundled data as the primary local packaging path going forward.
The latest report does not explicitly name the installed version or hash.
Audio, save/load and extended play remain separate verification tasks.

## Earlier investigation and emulator evidence

User confirms actual Vulkan gameplay in0.06 but observes USB reads; game
crashes with USB absent. This remains physical evidence of a dependency or
failure requiring diagnosis. Package source intent did not establish behavior.

0.06 selected bundled mode with access(). 0.07 opens the marker with fopen,
logs DATA AUDIT0.07 bundled=1/0 and selected path, explicitly clears fs_cdpath
and fs_devpath, and logs all five filesystem roots after initialization.
First12 archive reopen operations log their actual path to engine log.
An access() problem is a hypothesis, not an established root cause.
The Vulkan renderer and audio settings are unchanged.

Validation: package validator and all19 extracted file hashes passed. Fresh
emulator profile had no external data links. With a temporary test-map control
file (not packaged), /app0/base/pak000..004 loaded, archive reopens used /app0,
fs_basepath=/app0, cdpath/devpath empty, saves/config under /data/doom3-client.
Map game/mars_city1 loaded and PS4 GAME ticks30/60/90 reached health100.
Run time-limited; no physical0.07 result claimed.

Install Doom3-PS4-Bundled-Data-0.07.pkg; fully close game, remove USB, restart
and load/play a map. If it fails, photograph Diagnose0.12 ENGINE1/6 and GPU2/6.
The startup DATA AUDIT may scroll off the viewer, so full logs are preferable
when available. Success withoutUSB proves that run independent ofUSB.
Generated retail packages remain ignored/local and are not published.
SHA256: b987a0b8943e0193d4be96426d15de0a5f7f719b657cabd138606807dd79413e
