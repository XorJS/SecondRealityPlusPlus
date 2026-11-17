#!/bin/sh
# Builds SDL3 as a static xcframework (device + simulator) into iOS/_deps;
# camera support is left out (would need CoreMedia/CoreVideo and a usage description),
# so is HIDAPI (CoreBluetooth, ditto); controllers still work through GameController
set -eu

SDL_VERSION=3.4.20

cd "$(dirname "$0")"
DEPS="$(pwd)/_deps"
SRC="$DEPS/SDL3-$SDL_VERSION"

mkdir -p "$DEPS"

if [ ! -d "$SRC" ]; then
    curl -fL -o "$DEPS/SDL3-$SDL_VERSION.tar.gz" \
        "https://github.com/libsdl-org/SDL/releases/download/release-$SDL_VERSION/SDL3-$SDL_VERSION.tar.gz"
    tar -xzf "$DEPS/SDL3-$SDL_VERSION.tar.gz" -C "$DEPS"
fi

build() {
    sysroot=$1
    cmake -S "$SRC" -B "$DEPS/build-$sysroot" -G Xcode \
        -DCMAKE_SYSTEM_NAME=iOS \
        -DCMAKE_OSX_SYSROOT="$sysroot" \
        -DCMAKE_OSX_ARCHITECTURES=arm64 \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=15.0 \
        -DSDL_SHARED=OFF -DSDL_STATIC=ON -DSDL_TEST_LIBRARY=OFF \
        -DSDL_CAMERA=OFF -DSDL_HIDAPI=OFF
    cmake --build "$DEPS/build-$sysroot" --config Release
    cmake --install "$DEPS/build-$sysroot" --config Release --prefix "$DEPS/install-$sysroot"
}

build iphoneos
build iphonesimulator

rm -rf "$DEPS/SDL3.xcframework"
xcodebuild -create-xcframework \
    -library "$DEPS/install-iphoneos/lib/libSDL3.a" \
    -library "$DEPS/install-iphonesimulator/lib/libSDL3.a" \
    -output "$DEPS/SDL3.xcframework"

# headers are the same for both slices; included as <SDL3/...>
rm -rf "$DEPS/include"
cp -R "$DEPS/install-iphoneos/include" "$DEPS/include"
