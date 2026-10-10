#!/usr/bin/env bash
set -euo pipefail
root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
stack="$root/build/native/opengnm-psbc"
out="$root/build/psbc-memory-test"
mkdir -p "$out"
# Uses the prepared host compiler library; build-native-vulkan prepares its sources.
make -C "$stack" -j"${BUILD_JOBS:-8}" libpsbc.a > "$out/build.log" 2>&1
glslc --target-env=vulkan1.4 "$root/tests/shaders/demote.vert" -o "$out/vert.spv"
clang -I"$stack/libpsbc" -c "$root/tests/psbc-memory.c" -o "$out/test.o"
clang++ "$out/test.o" "$stack/libpsbc.a" -lpthread -lm -o "$out/test"
"$out/test" "$out/vert.spv" > "$out/result.log" 2>&1
tail -1 "$out/result.log"
