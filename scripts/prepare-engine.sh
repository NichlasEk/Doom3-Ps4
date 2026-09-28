#!/usr/bin/env bash
set -euo pipefail
root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
revision=455b88e8dff2be822f08eb498f51b383e851fa38
git -C "$root" submodule update --init engine
[[ $(git -C "$root/engine" rev-parse HEAD) == "$revision" ]] || {
  echo "Unexpected dhewm3 revision" >&2; exit 1;
}
patch="$root/patches/001-ps4-diagnostic.patch"
if git -C "$root/engine" apply --reverse --check "$patch" 2>/dev/null; then
  exit 0
fi
git -C "$root/engine" diff --quiet || {
  echo "Engine has local changes other than the known PS4 patch" >&2; exit 1;
}
git -C "$root/engine" apply --check "$patch"
git -C "$root/engine" apply "$patch"
