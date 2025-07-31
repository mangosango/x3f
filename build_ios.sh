#!/bin/bash

# iOS Build Script for x3f_extract
# Based on IMPLEMENTATION_PLAN.md

set -e

# Configuration
IOS_DEPLOYMENT_TARGET="12.0"
ARCHITECTURE="arm64"
BUILD_DIR="build_ios"
INSTALL_PREFIX="/tmp/x3f_ios_install"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Building x3f_extract for iOS/iPadOS${NC}"
echo "Target: iOS ${IOS_DEPLOYMENT_TARGET}, Architecture: ${ARCHITECTURE}"
echo ""

# Check for Xcode
if ! command -v xcodebuild &> /dev/null; then
    echo -e "${RED}Error: Xcode is required but not found${NC}"
    exit 1
fi

# Clean previous build
if [ -d "$BUILD_DIR" ]; then
    echo -e "${YELLOW}Cleaning previous build directory...${NC}"
    rm -rf "$BUILD_DIR"
fi

# Create build directory
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo -e "${GREEN}Configuring CMake for iOS...${NC}"

# Configure with CMake
cmake -G Xcode \
    -DCMAKE_TOOLCHAIN_FILE=../ios.toolchain.cmake \
    -DIOS=ON \
    -DPORTABLE=ON \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="$IOS_DEPLOYMENT_TARGET" \
    -DCMAKE_OSX_ARCHITECTURES="$ARCHITECTURE" \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    -DCMAKE_BUILD_TYPE=Release \
    ..

if [ $? -eq 0 ]; then
    echo -e "${GREEN}CMake configuration successful!${NC}"
    echo ""
    echo -e "${YELLOW}To build the project:${NC}"
    echo "cd $BUILD_DIR"
    echo "xcodebuild -configuration Release"
    echo ""
    echo -e "${YELLOW}Or open in Xcode:${NC}"
    echo "open x3f_tools.xcodeproj"
else
    echo -e "${RED}CMake configuration failed!${NC}"
    exit 1
fi

echo ""
echo -e "${GREEN}iOS build environment setup complete!${NC}"
echo "Note: You will need to configure code signing in Xcode before building for device."
