#!/usr/bin/env bash

# Copies the generated TypeScript wrapper files and Emscripten build outputs
# into the package/wasm directory, matching the layout used by the legacy scripts.

set -euo pipefail

CONFIG="${1:-Debug}"
BUILD_DIR="${2:-build/Debug}"

case "$CONFIG" in
    Debug|Release|RelWithDebInfo)
        ;;
    debug)
        CONFIG="Debug"
        ;;
    release)
        CONFIG="Release"
        ;;
    relwithdebinfo)
        CONFIG="RelWithDebInfo"
        ;;
    *)
        echo "Error: CONFIG must be one of: Debug, Release, RelWithDebInfo"
        echo "Usage: $0 <Config> <BuildDir>"
        echo "Example: $0 Debug build/Debug"
        exit 1
        ;;
esac

OUTPUT_DIR="./Library/Binaries/package/wasm/connected-spaces-platform.web"
CONFIG_OUTPUT_DIR="$OUTPUT_DIR/$CONFIG"

WRAPPER_BUILD_DIR="./$BUILD_DIR/LegacyEmscriptenWrapper"
TYPESCRIPT_OUTPUT_DIR="Tools/WrapperGenerator/Output/TypeScript"

mkdir -p "$CONFIG_OUTPUT_DIR"

# Copy specific Emscripten output files into the config folder
cp "$WRAPPER_BUILD_DIR/ConnectedSpacesPlatform_WASM.js" "$CONFIG_OUTPUT_DIR/"
cp "$WRAPPER_BUILD_DIR/ConnectedSpacesPlatform_WASM.wasm" "$CONFIG_OUTPUT_DIR/"
cp "$WRAPPER_BUILD_DIR/ConnectedSpacesPlatform_WASM.worker.js" "$CONFIG_OUTPUT_DIR/"

if [ "$CONFIG" = "Debug" ]; then
    cp "$WRAPPER_BUILD_DIR/ConnectedSpacesPlatform_WASM.wasm.debug.wasm" "$CONFIG_OUTPUT_DIR/"
fi

# Copy all generated TypeScript output into the package folder
cp -R "$TYPESCRIPT_OUTPUT_DIR"/. "$OUTPUT_DIR/"