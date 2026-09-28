#!/usr/bin/env bash
set -euo pipefail
root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
sdk=${OO_PS4_TOOLCHAIN:-/opt/openorbis/OpenOrbis/PS4Toolchain}
converter=${CREATE_FSELF:-/home/nichlas/ut99-orbis/build/create-fself-current}
stack="$root/build/native"
mode=${1:-demote}
case "$mode" in
  demote|generic|cube|ambient|shadow|texops|interaction|interactionshadow|cubeshadow) ;;
  *) echo 'Use demote, generic, cube, ambient, shadow, texops, interaction, interactionshadow or cubeshadow' >&2; exit 1 ;;
esac
out="$root/build/shader-probe-$mode"
defines=()
if [[ "$mode" != demote ]]; then defines=(-DPROBE_DUDE_GENERIC); fi
case "$mode" in
  cube) defines+=(-DPROBE_LIGHTING) ;;
  ambient) defines+=(-DPROBE_LIGHTING -DPROBE_DUDE_AMBIENT) ;;
  interaction|interactionshadow|cubeshadow) defines+=(-DPROBE_LIGHTING -DPROBE_DUDE_AMBIENT -DPROBE_INTERACTION) ;;
  shadow) defines+=(-DPROBE_SHADOW) ;;
esac
if [[ "$mode" == interactionshadow ]]; then defines+=(-DPROBE_INTERACTION_SHADOW); fi
"$root/scripts/build-native-vulkan.sh"
mkdir -p "$out"
if [[ "$mode" != demote ]]; then python3 "$root/scripts/audit-dude-shaders.py"; fi
for stage in vert frag; do
  if [[ "$mode" == generic || "$mode" == ambient || "$mode" == interaction || "$mode" == interactionshadow ]]; then
    shader=generic
    if [[ "$mode" == interaction || "$mode" == interactionshadow ]]; then shader=interaction; fi
    if [[ "$mode" == ambient ]]; then shader=ambientlight; fi
    cp "$root/build/dude-shaders/spv/$shader.$stage.spv" "$out/$stage.spv"
  else
    glslc --target-env=vulkan1.4 "$root/tests/shaders/$mode.$stage" -o "$out/$stage.spv"
  fi
  spirv-val --target-env vulkan1.4 "$out/$stage.spv"
done
spirv-dis "$out/frag.spv" -o "$out/frag.spvasm"
if [[ "$mode" == demote || "$mode" == generic ]]; then rg -q 'OpDemoteToHelperInvocation' "$out/frag.spvasm"; fi
python3 - "$out" "$root" "$mode" <<'PY'
import pathlib,struct,sys
out=pathlib.Path(sys.argv[1]); text='#include <stdint.h>\n'
for stage in ('vert','frag'):
    data=(out/(stage+'.spv')).read_bytes()
    words=struct.unpack('<'+'I'*(len(data)//4), data)
    text+='static const uint32_t g_'+stage+'_spv[] = {'+','.join(hex(w) for w in words)+'};\n'
(out/'probe_spirv.h').write_text(text)
if sys.argv[3] != 'demote':
    import re
    source=(pathlib.Path(sys.argv[2])/'.tools/dude-reference/neo/shaders/renderparms.glsl').read_text()
    offset=0; params=''
    for kind,name in re.findall(r'^\s*(mat4|vec4)\s+(u_\w+)\s*;', source, re.M):
        params+=f'#define PARAM_{name} {offset}\n'
        offset+=16 if kind=='mat4' else 4
    if not offset: raise RuntimeError('No RenderParams members found')
    params+=f'#define PROBE_PARAM_FLOATS {offset}\n'
    (out/'probe_params.h').write_text(params)
PY
clang --target=x86_64-pc-freebsd12-elf --sysroot="$sdk" -fPIC -D__PS4__ -D__ORBIS__ \
  -I"$sdk/include" -I"$stack/Vulkan-Headers/include" -I"$stack/vulkan-ps4/include" -I"$out" \
  "${defines[@]}" -O2 -c "$root/tests/ps4_shader_probe.c" -o "$out/probe.o"
ld.lld -m elf_x86_64 --script "$sdk/link.x" --eh-frame-hdr -pie -z max-page-size=0x4000 \
  -L"$sdk/lib" "$sdk/lib/crt1.o" "$out/probe.o" \
  --start-group "$stack/vulkan-ps4/libvulkan_ps4.a" "$stack/opengnm/libopengnm.a" \
  "$stack/libpsbc-private.a" --end-group \
  -lc++ -lc++abi -lunwind -lc -lSceGnmDriver -lSceVideoOut -lkernel \
  "$sdk/lib/crtn.o" -o "$out/probe.elf"
OO_PS4_TOOLCHAIN="$sdk" "$converter" -in="$out/probe.elf" -out="$out/probe.oelf" \
  --eboot "$out/eboot.bin" --paid 0x3800000000000011 > "$out/fself.log" 2>&1
sha256sum "$out/probe.elf" "$out/eboot.bin"
