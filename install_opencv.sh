#!/bin/bash

set -e

# Architectures to build
ARCHS="x86_64 arm64"

# Common settings
ROOT=$PWD
SRC=$ROOT/deps/src
UNIVERSAL_LIB_DIR=$ROOT/deps/lib/osx-universal
OCV_SRC=$SRC/opencv
OCV_URL=https://github.com/opencv/opencv.git
OCV_HASH=4.5.5

# Clean up old builds
rm -rf $SRC/build
rm -rf $UNIVERSAL_LIB_DIR

# Clone or update OpenCV source
if [ -d "$OCV_SRC" ]; then
    echo "Fetching OpenCV"
    cd $OCV_SRC
    git fetch
else
    echo "Cloning OpenCV"
    mkdir -p $SRC
    git clone $OCV_URL $OCV_SRC
    cd $OCV_SRC
fi

git checkout $OCV_HASH

# Patch zutil.h to avoid fdopen macro conflict
sed -i.bak 's/#        define fdopen(fd,mode) NULL \/\* No fdopen() \*\//\/\* #        define fdopen(fd,mode) NULL \/\* No fdopen() \*\//' 3rdparty/zlib/zutil.h
# Patch pngpriv.h to avoid fp.h include error
sed -i.bak 's/#      include <fp.h>/ \/\*#      include <fp.h>\*\//' 3rdparty/libpng/pngpriv.h

# Build each architecture
for ARCH in $ARCHS; do
    BUILD_DIR=$SRC/build/$ARCH
    INSTALL_DIR=$SRC/install/$ARCH

    echo "Building for $ARCH"
    mkdir -p $BUILD_DIR
    cd $BUILD_DIR

    cmake -G "Unix Makefiles" \
        -D CMAKE_BUILD_TYPE=Release \
        -D CMAKE_OSX_ARCHITECTURES=$ARCH \
        -D BUILD_SHARED_LIBS=OFF \
        -D BUILD_opencv_apps=OFF \
        -D BUILD_TESTS=OFF \
        -D BUILD_PERF_TESTS=OFF \
        -D WITH_OPENCL=OFF \
        -D WITH_IPP=OFF \
        -D BUILD_TIFF=ON \
        -D WITH_PNG=OFF \
        -D BUILD_opencv_videoio=OFF \
        -D BUILD_opencv_python=OFF \
        -D BUILD_opencv_python2=OFF \
        -D BUILD_opencv_python3=OFF \
        -D CMAKE_INSTALL_PREFIX=$INSTALL_DIR \
        $OCV_SRC

    make -j$(sysctl -n hw.ncpu)
    make install
done

# Create universal libraries
echo "Creating universal libraries"
UNIVERSAL_INSTALL_DIR=$UNIVERSAL_LIB_DIR/opencv
mkdir -p $UNIVERSAL_INSTALL_DIR/lib
mkdir -p $UNIVERSAL_INSTALL_DIR/include

# Combine libraries with lipo
for LIB in $SRC/install/x86_64/lib/*.a; do
    LIB_NAME=$(basename $LIB)
    if [ -f "$SRC/install/arm64/lib/$LIB_NAME" ]; then
        lipo -create \
            $SRC/install/x86_64/lib/$LIB_NAME \
            $SRC/install/arm64/lib/$LIB_NAME \
            -output $UNIVERSAL_INSTALL_DIR/lib/$LIB_NAME
    fi
done

ls -l $UNIVERSAL_INSTALL_DIR/lib

# Copy headers
cp -R $SRC/install/x86_64/include/opencv4 $UNIVERSAL_INSTALL_DIR/include/

# Create a success marker
touch $UNIVERSAL_INSTALL_DIR/.success

echo "Successfully built and installed universal OpenCV libraries."
