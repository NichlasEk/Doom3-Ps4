# Local bundled-data test 0.06

Physical 0.05 reached menu/video then reported:
`Couldn't reopen /doom3_usb0/DOOM3/base/pak000.pk4`.
This demonstrates an archive reopen failure, not proof of slow USB or a GPU bug.

Build the local owned-data experiment:

```
python3 scripts/package-ps4-client.py --bundle-data --game-data media/game
```

The optional flag embeds pak000 through pak004 in `/app0/base` and adds
`/app0/doom3-bundled-data.txt`. The client then explicitly selects `/app0`,
with no external/USB fallback. Default packaging still excludes retail data.
Executable graphics/audio settings and compiled clear path otherwise remain
unchanged. Title DOOM00001 is retained; version is 00.06.

Package: `dist/Doom3-PS4-Bundled-Data-0.06.pkg`, 1594294272 bytes.
SHA-256: `cde903832051194015d16f57d9ce030ce5b5695fdd096c8ef550fd34b698d229`.

Validator passed; all 19 extracted manifest files matched. A fresh isolated
shadPS4 run selected `/app0`, loaded all five packaged PK4 archives and presented
menu frames. Gameplay and physical installation/execution remain unverified.
The emulator run was time limited. Diagnose 0.12 shows engine/GPU logs on 1/6
and 2/6. Install the exact package; USB is unnecessary after installation.

Generated packages/data stay in ignored build/dist directories and must not be
published. Only build code and documentation belong in Git.
