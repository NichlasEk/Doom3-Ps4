#!/usr/bin/env bash
set -euo pipefail
root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
stack="$root/build/native"
mkdir -p "$root/build/tests"
clang -std=c11 -O1 -g -ffunction-sections -fdata-sections \
  -fsanitize=address,undefined -Wl,--gc-sections \
  -I"$stack/vulkan-ps4/include" -I"$stack/Vulkan-Headers/include" \
  -I"$stack/opengnm/include" -I"$stack/opengnm-psbc/libpsbc" \
  "$root/tests/vulkan_features.c" "$stack/vulkan-ps4/src/vk_ps4_vulkan11.c" \
  "$stack/vulkan-ps4/src/vk_ps4_memory.c" "$stack/vulkan-ps4/src/vk_ps4_entrypoints.c" \
  "$stack/vulkan-ps4/src/vk_ps4_descriptor.c" \
  -o "$root/build/tests/vulkan-features"
"$root/build/tests/vulkan-features"
