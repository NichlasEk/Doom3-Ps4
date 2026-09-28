#!/usr/bin/env bash
set -euo pipefail
root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
export OO_PS4_TOOLCHAIN=${OO_PS4_TOOLCHAIN:-/opt/openorbis/OpenOrbis/PS4Toolchain}
stack="$root/build/native"
mkdir -p "$stack" "$root/artifacts/native"
pin() {
  local url=$1 name=$2 revision=$3
  if [[ ! -d "$stack/$name/.git" ]]; then
    git clone --no-checkout "$url" "$stack/$name"
    git -C "$stack/$name" checkout --detach "$revision"
  fi
  [[ $(git -C "$stack/$name" rev-parse HEAD) == "$revision" ]] || { echo "Unexpected $name revision" >&2; exit 1; }
}
pin https://github.com/PS4-OpenGNM/opengnm.git opengnm c51243f333a1a964894903da080a508f7af4d91f
pin https://github.com/PS4-OpenGNM/opengnm-psbc.git opengnm-psbc a92a1228ea3a64e4be9f0e61c2a65a5aa7ffed92
pin https://github.com/PS4-OpenGNM/vulkan-ps4.git vulkan-ps4 f4d940b723771b3fe637e8ac0704bb5d6ab19622
pin https://github.com/KhronosGroup/Vulkan-Headers.git Vulkan-Headers 3c65a01745e4a1134d32b9c2c456472212dba16d
pin https://github.com/KhronosGroup/SPIRV-Headers.git SPIRV-Headers cb42dec3830d3ac67fa449ecdc0c0f73d5e74498
for name in opengnm opengnm-psbc vulkan-ps4; do
  patch="$root/patches/ps4-native/$name.patch"
  if ! git -C "$stack/$name" apply --reverse --check "$patch" 2>/dev/null; then
    git -C "$stack/$name" apply --check "$patch"
    git -C "$stack/$name" apply "$patch"
  fi
done
python="$stack/codegen/bin/python3"
if [[ ! -x "$python" ]]; then python3 -m venv --system-site-packages "$stack/codegen"; fi
if ! "$python" -c 'import lxml.etree, rnc2rng, mako, yaml' 2>/dev/null; then
  "$python" -m pip install -r "$root/thirdparty/ps4-native/codegen-requirements.txt"
fi
fingerprint=$( { sha256sum "$root/thirdparty/ps4-native/prepare-codegen.py" "$root/thirdparty/ps4-native/codegen-requirements.txt"; git -C "$stack/opengnm-psbc" rev-parse HEAD; } | sha256sum)
if [[ ! -f "$stack/codegen.stamp" || $(cat "$stack/codegen.stamp") != "$fingerprint" ]]; then
  "$python" "$root/thirdparty/ps4-native/prepare-codegen.py" "$stack/opengnm-psbc" > "$root/artifacts/native/codegen.log" 2>&1
  echo "$fingerprint" > "$stack/codegen.stamp"
fi
gnm_fingerprint=$( { sha256sum "$root/patches/ps4-native/opengnm.patch"; clang --version; printf '%s\n' "$OO_PS4_TOOLCHAIN"; } | sha256sum)
if [[ ! -f "$stack/opengnm/libopengnm.a" || ! -f "$stack/gnm.stamp" || $(cat "$stack/gnm.stamp") != "$gnm_fingerprint" ]]; then
  cp "$stack/opengnm/config.orbis.mak" "$stack/opengnm/config.mak"
  make -C "$stack/opengnm" CC=clang AR=llvm-ar lib > "$root/artifacts/native/opengnm-build.log" 2>&1
  echo "$gnm_fingerprint" > "$stack/gnm.stamp"
fi
make -C "$stack/opengnm-psbc" -f Makefile.orbis PYTHON="$python" -j"${BUILD_JOBS:-8}" libpsbc.orbis.a > "$root/artifacts/native/psbc-build.log" 2>&1
if ! cmp -s "$root/platform/ps4/spirv_push_constants.h" "$stack/vulkan-ps4/src/vk_ps4_push_spirv.h"; then
  cp "$root/platform/ps4/spirv_push_constants.h" "$stack/vulkan-ps4/src/vk_ps4_push_spirv.h"
fi
make -C "$stack/vulkan-ps4" -f Makefile.orbis -j"${BUILD_JOBS:-8}" libvulkan_ps4.a > "$root/artifacts/native/vulkan-build.log" 2>&1
python3 - "$stack" <<'PY'
from pathlib import Path
import subprocess, sys
stack = Path(sys.argv[1])
archive = stack/'vulkan-ps4/libvulkan_ps4.a'
names = subprocess.check_output(['llvm-nm','--defined-only','--extern-only','--just-symbol-name',str(archive)],text=True).split()
names = sorted({n for n in names if n.startswith('vk') and len(n)>2 and n[2].isupper()})
assert 'vkGetInstanceProcAddr' in names
mapping = stack/'icd-symbols.txt'
mapping.write_text(''.join(f'{n} ps4_icd_{n}\n' for n in names))
subprocess.run(['llvm-objcopy','--redefine-syms='+str(mapping),str(archive),str(stack/'libvulkan-icd.a')],check=True)
archive = stack/'opengnm-psbc/libpsbc.orbis.a'
obj = stack/'psbc-threads.o'
obj.write_bytes(subprocess.check_output(['llvm-ar','p',str(archive),'threads_posix.orbis.o']))
names = subprocess.check_output(['llvm-nm','--defined-only','--extern-only','--just-symbol-name',str(obj)],text=True).split()
assert 'thrd_create' in names and all(n=='call_once' or n.startswith(('cnd_','mtx_','thrd_','tss_')) for n in names)
mapping = stack/'psbc-thread-symbols.txt'
mapping.write_text(''.join(f'{n} doom3_psbc_{n}\n' for n in names))
subprocess.run(['llvm-objcopy','--redefine-syms='+str(mapping),str(archive),str(stack/'libpsbc-private.a')],check=True)
PY
sha256sum "$stack/libvulkan-icd.a" "$stack/opengnm/libopengnm.a" "$stack/libpsbc-private.a" > "$root/artifacts/native/archives.sha256"
echo 'PS4 native Vulkan archives built from source'
