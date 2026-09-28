#!/usr/bin/env bash
set -euo pipefail
root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
sdk=${OO_PS4_TOOLCHAIN:-/opt/openorbis/OpenOrbis/PS4Toolchain}
converter=${CREATE_FSELF:-/home/nichlas/ut99-orbis/build/create-fself-current}
[[ -x "$converter" ]] || { echo "Set CREATE_FSELF to a source-built create-fself" >&2; exit 1; }
"$root/scripts/prepare-engine.sh"
mkdir -p "$root/artifacts" "$root/build/runtime"
cmake -S "$root/engine/neo" -B "$root/build/ps4" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$root/cmake/openorbis.cmake" \
  -DCORE=OFF -DDEDICATED=ON -DBASE=ON -DD3XP=OFF \
  -DHARDLINK_GAME=ON -DIMGUI=OFF -DDHEWM3_PS4_BRINGUP=ON \
  -DCMAKE_BUILD_TYPE=Release > "$root/artifacts/ps4-configure.log" 2>&1
cmake --build "$root/build/ps4" -j"${BUILD_JOBS:-8}" > "$root/artifacts/ps4-build.log" 2>&1
elf="$root/build/ps4/dhewm3ded"
readelf -h -l -d "$elf" > "$root/artifacts/ps4-elf-report.txt"
llvm-nm --undefined-only "$elf" > "$root/artifacts/ps4-undefined-symbols.txt"
OO_PS4_TOOLCHAIN="$sdk" "$converter" -in="$elf" \
  -out="$root/build/runtime/doom3ded.oelf" \
  --eboot "$root/build/runtime/eboot.bin" --paid 0x3800000000000011 \
  > "$root/artifacts/ps4-fself.log" 2>&1
sha256sum "$elf" "$root/build/runtime/eboot.bin" > "$root/artifacts/ps4-sha256.txt"
echo "PS4 diagnostic: $root/build/runtime/eboot.bin"
