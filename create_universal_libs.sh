#!/bin/bash

# Create universal libraries for macOS universal build
# This script combines arm64 and x86_64 libraries into universal binaries

DEPS_DIR="deps/src/build"

# Check if both architectures exist
if [ ! -d "$DEPS_DIR/arm64" ] || [ ! -d "$DEPS_DIR/x86_64" ]; then
    echo "Error: Missing architecture directories. Please build both arm64 and x86_64 first."
    exit 1
fi

echo "Creating universal libraries..."

# Create universal libtiff
if [ -f "$DEPS_DIR/arm64/3rdparty/lib/liblibtiff.a" ] && [ -f "$DEPS_DIR/x86_64/3rdparty/lib/liblibtiff.a" ]; then
    echo "Creating liblibtiff-universal.a..."
    lipo -create \
        "$DEPS_DIR/arm64/3rdparty/lib/liblibtiff.a" \
        "$DEPS_DIR/x86_64/3rdparty/lib/liblibtiff.a" \
        -output "$DEPS_DIR/liblibtiff-universal.a"
else
    echo "Warning: liblibtiff.a not found in both architectures"
fi

# Create universal libjpeg-turbo
if [ -f "$DEPS_DIR/arm64/3rdparty/lib/liblibjpeg-turbo.a" ] && [ -f "$DEPS_DIR/x86_64/3rdparty/lib/liblibjpeg-turbo.a" ]; then
    echo "Creating liblibjpeg-turbo-universal.a..."
    lipo -create \
        "$DEPS_DIR/arm64/3rdparty/lib/liblibjpeg-turbo.a" \
        "$DEPS_DIR/x86_64/3rdparty/lib/liblibjpeg-turbo.a" \
        -output "$DEPS_DIR/liblibjpeg-turbo-universal.a"
else
    echo "Warning: liblibjpeg-turbo.a not found in both architectures"
fi

# Create universal libittnotify
if [ -f "$DEPS_DIR/arm64/3rdparty/lib/libittnotify.a" ] && [ -f "$DEPS_DIR/x86_64/3rdparty/lib/libittnotify.a" ]; then
    echo "Creating libittnotify-universal.a..."
    lipo -create \
        "$DEPS_DIR/arm64/3rdparty/lib/libittnotify.a" \
        "$DEPS_DIR/x86_64/3rdparty/lib/libittnotify.a" \
        -output "$DEPS_DIR/libittnotify-universal.a"
else
    echo "Warning: libittnotify.a not found in both architectures"
fi

# Create universal libzlib
if [ -f "$DEPS_DIR/arm64/3rdparty/lib/libzlib.a" ] && [ -f "$DEPS_DIR/x86_64/3rdparty/lib/libzlib.a" ]; then
    echo "Creating libzlib-universal.a..."
    lipo -create \
        "$DEPS_DIR/arm64/3rdparty/lib/libzlib.a" \
        "$DEPS_DIR/x86_64/3rdparty/lib/libzlib.a" \
        -output "$DEPS_DIR/libzlib-universal.a"
else
    echo "Warning: libzlib.a not found in both architectures"
fi

echo "Universal libraries created successfully!"

# Verify the created libraries
echo ""
echo "Verifying universal libraries:"
for lib in liblibtiff-universal.a liblibjpeg-turbo-universal.a libittnotify-universal.a libzlib-universal.a; do
    if [ -f "$DEPS_DIR/$lib" ]; then
        echo -n "$lib: "
        lipo -info "$DEPS_DIR/$lib" 2>/dev/null | sed 's/.*are: //'
    else
        echo "$lib: NOT FOUND"
    fi
done
