# Doom 3 PS4 port workspace

This repository is the starting point for a Doom 3 PS4 port. The three owned
PC retail discs are kept locally under `media/original/`. Converted ISO images
and extracted files live under `media/iso/` and `media/extracted/`. The game
archives from all three discs are linked together under `media/game/base/`:
`game00.pk4` and `pak000.pk4` through `pak004.pk4`.

All `media/` content is excluded from Git because it contains commercial game
data. The extraction is reproducible with:

```sh
python3 scripts/extract_discs.py
```

The script requires `7z`. It converts the BlindWrite `.B6I` Mode 1 sectors to
ISO files, then extracts each disc separately. The `.B6T` metadata stays with
the original images. No game code has been ported yet. The original Windows
executable is present on disc 1, but a PS4 build will need ported engine code.
