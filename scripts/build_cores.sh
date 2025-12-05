#!/usr/bin/env bash
set -e
NDK_PATH=${ANDROID_NDK_HOME:-/path/to/ndk}
echo "NDK: $NDK_PATH"
if [ ! -d libretro-super ]; then
  git clone https://github.com/libretro/libretro-super.git
fi
cd libretro-super
# Example: build picoDrive and mGBA for arm64
./libretro-fetch.sh picodrive
./libretro-fetch.sh mgba
make platform=android-aarch64 cores=picodrive,mgba -j$(nproc)
# built cores will be in libretro-super/dist/<platform>/
