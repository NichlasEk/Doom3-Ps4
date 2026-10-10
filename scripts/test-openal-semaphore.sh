#!/usr/bin/env bash
set -euo pipefail
root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
mkdir -p "$root/build/tests"
clang++ -std=c++14 -pthread -D__PS4__ \
  -I"$root/build/openal-soft-1.23.1/common" -I"$root/build/ps4-openal-release" \
  "$root/tests/openal-semaphore.cpp" "$root/build/openal-soft-1.23.1/common/threads.cpp" \
  -o "$root/build/tests/openal-semaphore"
timeout 10 "$root/build/tests/openal-semaphore"
echo 'PASS: semaphore initial count, blocking, 4000 posts/four waiters, no lost wakeups'
