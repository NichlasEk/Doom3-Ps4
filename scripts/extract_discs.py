#!/usr/bin/env python3
"""Convert the owned BlindWrite Doom 3 CDs to ISO and extract each disc.

BlindWrite B6I files here contain 2352-byte Mode 1 sectors with a 150-sector
lead-in. ISO data is the 2048 bytes beginning at offset 16 in each sector.
"""

from pathlib import Path
import os
import subprocess


ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "media" / "original"
ISO = ROOT / "media" / "iso"
EXTRACTED = ROOT / "media" / "extracted"
GAME_BASE = ROOT / "media" / "game" / "base"
SECTOR_SIZE = 2352
DATA_OFFSET = 16
DATA_SIZE = 2048
LEAD_IN_SECTORS = 150
SYNC = bytes.fromhex("00 ff ff ff ff ff ff ff ff ff ff 00")


def convert(source: Path, target: Path) -> None:
    size = source.stat().st_size
    if size % SECTOR_SIZE:
        raise ValueError(f"{source}: size is not a multiple of {SECTOR_SIZE}")
    sectors = size // SECTOR_SIZE
    target.parent.mkdir(parents=True, exist_ok=True)
    temporary = target.with_suffix(".iso.part")
    with source.open("rb") as src, temporary.open("wb") as dst:
        src.seek(LEAD_IN_SECTORS * SECTOR_SIZE)
        for index in range(LEAD_IN_SECTORS, sectors):
            sector = src.read(SECTOR_SIZE)
            if len(sector) != SECTOR_SIZE or sector[:12] != SYNC or sector[15] != 1:
                raise ValueError(f"{source}: invalid Mode 1 sector {index}")
            dst.write(sector[DATA_OFFSET : DATA_OFFSET + DATA_SIZE])
    temporary.replace(target)
    expected = (sectors - LEAD_IN_SECTORS) * DATA_SIZE
    if target.stat().st_size != expected:
        raise ValueError(f"{target}: incorrect output size")


def main() -> None:
    for disc in range(1, 4):
        source = SOURCE / f"DOOM3PC_CD{disc}of3.B6I"
        target = ISO / f"doom3-cd{disc}.iso"
        destination = EXTRACTED / f"cd{disc}"
        if not target.exists():
            print(f"Converting CD {disc}: {source} -> {target}", flush=True)
            convert(source, target)
        destination.mkdir(parents=True, exist_ok=True)
        print(f"Extracting CD {disc}: {target} -> {destination}", flush=True)
        subprocess.run(["7z", "x", "-y", f"-o{destination}", str(target)], check=True)

    GAME_BASE.mkdir(parents=True, exist_ok=True)
    for disc, names in {
        1: ("game00.pk4", "pak002.pk4"),
        2: ("pak000.pk4", "pak001.pk4"),
        3: ("pak003.pk4", "pak004.pk4"),
    }.items():
        for name in names:
            source = EXTRACTED / f"cd{disc}" / "Setup" / "Data" / "base" / name
            if not source.is_file():
                raise FileNotFoundError(source)
            link = GAME_BASE / name
            relative = os.path.relpath(source, GAME_BASE)
            if link.is_symlink():
                if os.readlink(link) != relative:
                    raise ValueError(f"{link}: points to unexpected location")
            elif link.exists():
                raise FileExistsError(link)
            else:
                link.symlink_to(relative)
    print(f"Game archives ready in {GAME_BASE}", flush=True)


if __name__ == "__main__":
    main()
