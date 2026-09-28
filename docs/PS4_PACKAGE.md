# Personal PS4 menu-test package

`dist/Doom3-PS4-Menu-Test-0.01.pkg` packages the tested graphical diagnostic,
the original project icon, SDK loader support files, license notices and the
five locally owned retail archives. It is a finite menu test: 60 engine frames,
a five-second capture hold, then exit. Controls, audio and gameplay remain
unimplemented/unverified. Physical PS4 execution is not established by packaging.

- Title: **Doom 3 PS4 - Menu Test**
- Title ID: `DM3P00001` (separate from UT99 and ScummVM)
- App version: `00.01`
- Content ID: `IV0000-DM3P00001_00-DOOM3PS4MENUTEST`
- Size: 1,594,097,664 bytes (fits the USB stick's FAT32 file-size limit).
- Package SHA-256: `c3f703260088773b8bacae624eb0cff342ec2834b21dcb89deb7e8c691c8aa53`
- Engine ELF / eboot: unchanged from the tested `44298ac` milestone.

The package uses `/app0/base` through the engine's executable-directory
fallback. No separate USB asset-directory setup is needed. Logs/settings
continue to use `/data/doom3-client`; the native driver log is
`/data/client-vulkan.log`. The three loader files come from this host's
OpenOrbis `samples/SDL2` directory, following the local UT99 package recipe.
They and the retail archives stay in ignored build/dist directories.

## Reproduce

```sh
./scripts/package-ps4-client.py --game-data media/game
# Or preserve an already tested binary:
./scripts/package-ps4-client.py --skip-build --game-data media/game
./scripts/verify-ps4-package.py
CLIENT_EBOOT="$PWD/build/client-package-extracted/uroot/eboot.bin" \
CLIENT_PROFILE="$PWD/build/package-test-profile" \
CLIENT_ARTIFACTS="$PWD/artifacts/package/emulator" \
CLIENT_USE_PACKAGED_DATA=1 \
xvfb-run -a -s '-screen 0 1280x720x24' python3 scripts/run-ps4-client.py
```

The package validator checks hashes/signatures. The extraction verifier checks
all 16 input files: byte hashes for payloads and unchanged application SFO
fields, allowing the packer's added `PUBTOOLINFO` and `PUBTOOLVER`. Icon and SFO
are separate package entries, so the verifier extracts these alongside PFS.
The emulator profile must have no external `doom3-game` directory or symlink;
this proves the test uses packaged data. Screenshots need separate inspection.

The first package passed all validation checks, booted in the isolated shadPS4
profile, displayed the main menu, completed 60 engine frames and exited 0.
Evidence is retained in `artifacts/package/`, including the manifest,
extraction check, emulator logs and screenshot. The USB copy is separately
verified by SHA-256 after flushing writes.

The icon's editable source is [doom3-icon.svg](../assets/doom3-icon.svg);
[icon0.png](../assets/icon0.png) is the required 512x512 opaque RGB PNG.
