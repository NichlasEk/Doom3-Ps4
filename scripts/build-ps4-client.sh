#!/usr/bin/env bash
# Graphical DUDE diagnostic, separate from the dedicated dhewm3 build.
set -euo pipefail
root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
export OO_PS4_TOOLCHAIN=${OO_PS4_TOOLCHAIN:-/opt/openorbis/OpenOrbis/PS4Toolchain}
converter=${CREATE_FSELF:-/home/nichlas/ut99-orbis/build/create-fself-current}
[[ -x "$converter" ]] || { echo 'Set CREATE_FSELF to a source-built create-fself' >&2; exit 1; }
source_dir="$root/build/dude-client-source"
revision=3b6872fe278802496cc7a8f8209b847c68efed2a
mkdir -p "$root/build" "$root/artifacts/client" "$root/build/client-runtime"
if [[ ! -d "$source_dir/.git" ]]; then
  git clone --no-checkout https://github.com/Inkub0/dude.git "$source_dir"
  git -C "$source_dir" checkout --detach "$revision"
fi
[[ $(git -C "$source_dir" rev-parse HEAD) == "$revision" ]] || { echo 'Unexpected DUDE revision' >&2; exit 1; }
patch="$root/patches/002-ps4-client.patch"
if ! git -C "$source_dir" apply --reverse --check "$patch" 2>/dev/null; then
  git -C "$source_dir" diff --quiet || { echo 'Unrecognized DUDE edits; preserve and review them first' >&2; exit 1; }
  git -C "$source_dir" apply --check "$patch"
  git -C "$source_dir" apply "$patch"
fi
python3 "$root/scripts/build-ps4-usb.py"
"$root/scripts/build-native-vulkan.sh"
cmake -S "$source_dir/neo" -B "$root/build/ps4-client" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$root/cmake/openorbis.cmake" \
  -DCORE=ON -DDEDICATED=OFF -DBASE=ON -DD3XP=OFF -DHARDLINK_GAME=ON \
  -DIMGUI=OFF -DDHEWM3_PS4_BRINGUP=ON -DDHEWM3_VULKAN=ON \
  -DDUDE_RUNTIME_ARB_COMPILER=OFF -DDOOM3_PS4_ROOT="$root" \
  -DVulkan_INCLUDE_DIR="$root/build/native/Vulkan-Headers/include" \
  -DVulkan_LIBRARY="$root/build/native/vulkan-ps4/libvulkan_ps4.a" \
  -DVulkan_GLSLC_EXECUTABLE="$(command -v glslc)" \
  -DPS4_NATIVE_LIBS="$root/build/native/opengnm/libopengnm.a;$root/build/native/libpsbc-private.a;SceGnmDriver" \
  -DCMAKE_BUILD_TYPE=Release > "$root/artifacts/client/configure.log" 2>&1
cmake --build "$root/build/ps4-client" --target dude -j"${BUILD_JOBS:-8}" > "$root/artifacts/client/build.log" 2>&1
elf="$root/build/ps4-client/dude"
"$converter" -in="$elf" -out="$root/build/client-runtime/dude.oelf" \
  --eboot "$root/build/client-runtime/eboot.bin" --paid 0x3800000000000011 \
  > "$root/artifacts/client/fself.log" 2>&1
sha256sum "$elf" "$root/build/client-runtime/eboot.bin" > "$root/artifacts/client/sha256.txt"
echo "PS4 graphical diagnostic: $root/build/client-runtime/eboot.bin"
