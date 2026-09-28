#!/usr/bin/env bash
set -euo pipefail
root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
emulator=${SHADPS4:-/home/nichlas/ScummVM-PS4/.tools/shadps4/Shadps4-sdl.AppImage}
profile=${EMULATOR_PROFILE:-"$root/build/emulator-profile"}
game_data=$(realpath -- "${DOOM3_DATA:-$root/media/game}")
[[ -x "$emulator" ]] || { echo "Set SHADPS4 to a shadPS4 SDL executable" >&2; exit 1; }
for archive in pak000.pk4 pak001.pk4 pak002.pk4 pak003.pk4 pak004.pk4; do
  [[ -f "$game_data/base/$archive" ]] || { echo "Missing $game_data/base/$archive" >&2; exit 1; }
done
"$root/scripts/build-ps4-diagnostic.sh"
mkdir -p "$profile/shadPS4/data" "$profile/shadPS4/home/1000"/{savedata,trophy,inputs}
link="$profile/shadPS4/data/doom3-game"
if [[ -e "$link" || -L "$link" ]]; then
  [[ -L "$link" && $(realpath -- "$link") == "$game_data" ]] || {
    echo "Existing emulator data path differs: $link" >&2; exit 1;
  }
else
  ln -s -- "$game_data" "$link"
fi
guest_log="$profile/shadPS4/data/doom3-ps4/dhewm3log.txt"
before_stamp=
if [[ -f "$guest_log" ]]; then before_stamp=$(stat -c %y "$guest_log"); fi
launcher=(xvfb-run -a -s '-screen 0 1280x720x24')
if [[ -n ${DISPLAY:-} ]]; then launcher=(); fi
status=0
SDL_VIDEODRIVER=x11 XDG_DATA_HOME="$profile" \
  timeout -k 3s "${EMULATOR_TIMEOUT:-30}s" "${launcher[@]}" \
  "$emulator" --fullscreen false "$root/build/runtime/eboot.bin" \
  > "$root/artifacts/emulator.log" 2>&1 || status=$?
[[ -f "$guest_log" && $(stat -c %y "$guest_log") != "$before_stamp" ]] || {
  echo "No fresh guest log (emulator status $status)" >&2; exit 1;
}
for marker in 'Loaded pk4 /data/doom3-game/base/pak000.pk4' \
              'Loaded pk4 /data/doom3-game/base/pak004.pk4' \
              "Compiled 'script/doom_main.script'" \
              'PASS PS4 dedicated initialization' \
              'PASS PS4 dedicated 60 frames'; do
  rg -qF -- "$marker" "$guest_log" || {
    echo "Missing guest marker: $marker (emulator status $status)" >&2; exit 1;
  }
done
rg -qF 'Exiting with status code 0' "$root/artifacts/emulator.log" || {
  echo "Guest did not exit cleanly (emulator status $status)" >&2; exit 1;
}
if rg -q 'Unhandled access violation|Sys_Error:' "$root/artifacts/emulator.log"; then
  echo "Guest crash or fatal error (emulator status $status)" >&2; exit 1;
fi
echo "PASS: Doom 3 dedicated initialization, game scripts, 60 frames, guest exit 0"
