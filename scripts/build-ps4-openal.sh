#!/usr/bin/env bash
set -euo pipefail
root=$(CDPATH='' cd -- "$(dirname -- "$0")/.." && pwd)
archive="$root/build/openal-soft-1.23.1.tar.gz"
source_dir="$root/build/openal-soft-1.23.1"
mkdir -p "$root/build" "$root/artifacts/client"
if [[ ! -f "$archive" ]]; then
  curl -fL --retry 2 https://codeload.github.com/kcat/openal-soft/tar.gz/refs/tags/1.23.1 -o "$archive.partial"
  mv "$archive.partial" "$archive"
fi
printf '%s  %s\n' dfddf3a1f61059853c625b7bb03de8433b455f2f79f89548cbcbd5edca3d4a4a "$archive" | sha256sum -c -
if [[ ! -d "$source_dir" ]]; then tar -xzf "$archive" -C "$root/build"; fi
if ! patch -d "$source_dir" -p1 -R --dry-run < "$root/patches/003-openal-ps4.patch" >/dev/null 2>&1; then
  patch -d "$source_dir" -p1 --forward < "$root/patches/003-openal-ps4.patch"
fi
cmake -S "$source_dir" -B "$root/build/ps4-openal-release" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$root/cmake/openorbis.cmake" \
  -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_DISABLE_FIND_PACKAGE_PkgConfig=TRUE -DALSOFT_RTKIT=OFF \
  -DLIBTYPE=STATIC -DALSOFT_UTILS=OFF -DALSOFT_EXAMPLES=OFF \
  -DALSOFT_TESTS=OFF -DALSOFT_INSTALL=OFF -DALSOFT_EMBED_HRTF_DATA=OFF \
  -DALSOFT_BACKEND_PIPEWIRE=OFF -DALSOFT_BACKEND_PULSEAUDIO=OFF \
  -DALSOFT_BACKEND_OSS=OFF -DALSOFT_BACKEND_SNDIO=OFF \
  -DALSOFT_BACKEND_SOLARIS=OFF -DALSOFT_BACKEND_JACK=OFF \
  -DALSOFT_BACKEND_SDL2=OFF -DALSOFT_BACKEND_WAVE=OFF \
  > "$root/artifacts/client/openal-configure.log" 2>&1
cmake --build "$root/build/ps4-openal-release" -j"${BUILD_JOBS:-8}" \
  > "$root/artifacts/client/openal-build.log" 2>&1
