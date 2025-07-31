# Implementation Plan: Building x3f_extract for iPad

## Executive Summary

This document outlines the comprehensive plan for adapting the x3f_extract tool to run on iPad devices. The x3f_extract tool processes X3F files from Sigma cameras, extracting and converting raw image data. To enable iPad compatibility, we need to cross-compile the application for iOS/iPadOS, handle platform-specific dependencies, and address iOS-specific constraints.

## Project Overview

### Current State

- **Language**: C/C++ application
- **Build System**: CMake
- **Dependencies**: OpenCV, libtiff, libjpeg, zlib, zstd, liblzma, TBB, OpenBLAS, OpenCL
- **Target Platform**: Currently macOS/Linux/Windows
- **Main Executable**: x3f_extract

### Target State

- **Platform**: iOS/iPadOS (iPad)
- **Architecture**: ARM64 (Apple Silicon)
- **Minimum iOS Version**: iOS 12.0 or later
- **Deployment Options**: iOS App, Command Line Tool, or Framework

## Key Challenges and Solutions

### 1. Platform Architecture

**Challenge**: iPads use ARM64 architecture with iOS-specific system libraries and frameworks.

**Solution**:

- Use Xcode's iOS SDK for cross-compilation
- Configure CMake with iOS toolchain file
- Target arm64 architecture exclusively
- Set appropriate deployment target (iOS 12.0+)

### 2. Dependency Management

**Challenge**: All dependencies must be rebuilt for iOS platform.

**Solutions by dependency**:

| Dependency | Current Use      | iOS Solution                                |
| ---------- | ---------------- | ------------------------------------------- |
| OpenCV     | Image processing | Build OpenCV for iOS using official scripts |
| libtiff    | TIFF file I/O    | Cross-compile for iOS                       |
| libjpeg    | JPEG handling    | Cross-compile for iOS                       |
| zlib       | Compression      | Use iOS system zlib or cross-compile        |
| zstd       | Compression      | Cross-compile for iOS                       |
| liblzma    | Compression      | Cross-compile for iOS                       |
| TBB        | Parallelization  | Replace with GCD or build for iOS           |
| OpenBLAS   | Linear algebra   | Use Apple's Accelerate framework            |
| OpenCL     | GPU compute      | Replace with Metal or CPU fallback          |

### 3. System API Differences

**Challenge**: iOS has different APIs and restrictions compared to desktop platforms.

**Solutions**:

- File system: Adapt to iOS sandboxing and document directory structure
- Threading: Use Grand Central Dispatch (GCD) instead of raw pthreads
- GPU compute: Implement Metal shaders to replace OpenCL kernels
- Memory management: Respect iOS memory constraints

### 4. Code Signing and Distribution

**Challenge**: iOS requires code signing and has strict distribution methods.

**Solutions**:

- Development: Use development provisioning profile
- Distribution: App Store, TestFlight, or enterprise distribution
- Alternative: Jailbroken devices for command-line tool

## Implementation Phases

### Phase 1: iOS Build Environment Setup (Week 1)

#### 1.1 Create iOS CMake Toolchain File

Create `ios.toolchain.cmake`:

```cmake
set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
set(CMAKE_OSX_ARCHITECTURES arm64)
set(CMAKE_OSX_SYSROOT iphoneos)
set(CMAKE_C_COMPILER xcrun clang)
set(CMAKE_CXX_COMPILER xcrun clang++)
```

#### 1.2 Modify CMakeLists.txt

Add iOS-specific configurations:

```cmake
if(IOS)
  set(CMAKE_MACOSX_BUNDLE YES)
  set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")
  find_library(ACCELERATE_FRAMEWORK Accelerate)
  find_library(METAL_FRAMEWORK Metal)
endif()
```

#### 1.3 Setup Build Scripts

Create shell scripts for:

- Building dependencies
- Configuring CMake for iOS
- Creating universal binaries

### Phase 2: Dependency Building (Week 2-3)

#### 2.1 Core Libraries Build Order

1. **zlib** (if not using system)

   ```bash
   ./configure --prefix=$IOS_PREFIX --static
   make install
   ```

2. **libjpeg**

   ```bash
   ./configure --host=arm-apple-darwin --prefix=$IOS_PREFIX --disable-shared
   make install
   ```

3. **libtiff**

   ```bash
   ./configure --host=arm-apple-darwin --prefix=$IOS_PREFIX \
     --disable-shared --without-x
   make install
   ```

4. **zstd**

   ```bash
   make install PREFIX=$IOS_PREFIX CC="xcrun clang" \
     CFLAGS="-arch arm64 -isysroot $IOS_SDK"
   ```

5. **liblzma**
   ```bash
   ./configure --host=arm-apple-darwin --prefix=$IOS_PREFIX --disable-shared
   make install
   ```

#### 2.2 OpenCV iOS Build

Use OpenCV's official iOS build:

```bash
python opencv/platforms/ios/build_framework.py ios_build
```

#### 2.3 Threading Library

Replace TBB with GCD implementation or build TBB for iOS if critical.

### Phase 3: Source Code Modifications (Week 3-4)

#### 3.1 Platform Detection

Add to common header:

```c
#ifdef __APPLE__
  #include <TargetConditionals.h>
  #if TARGET_OS_IOS
    #define PLATFORM_IOS 1
  #endif
#endif
```

#### 3.2 OpenCL to Metal Conversion

Options:

1. **Metal Compute Shaders** (Recommended)

   - Convert OpenCL kernels to Metal
   - Better performance on iOS devices
   - Native GPU acceleration

2. **CPU Fallback**
   - Implement CPU versions of GPU kernels
   - Simpler but slower
   - Good for initial testing

#### 3.3 File System Adaptations

```c
#ifdef PLATFORM_IOS
const char* get_documents_directory() {
    // iOS-specific code to get documents directory
}
#endif
```

#### 3.4 Memory Management

- Add autorelease pools for Objective-C interop
- Implement memory pressure handling
- Respect iOS background execution limits

### Phase 4: iOS Wrapper Implementation (Week 4-5)

#### 4.1 Deployment Option A: Command Line Tool

**Pros**: Minimal changes, direct port
**Cons**: Requires jailbreak, limited distribution
**Implementation**: Build as standard executable

#### 4.2 Deployment Option B: iOS Application (Recommended)

**Structure**:

```
X3FExtractor.app/
├── Info.plist
├── Base.lproj/
│   └── Main.storyboard
├── ViewController.swift
├── X3FBridge.h
├── X3FBridge.m
└── libx3f_extract.a
```

**Features**:

- File picker for X3F files
- Progress indicators
- Export options (Photos app, Files app)
- Settings for output format

#### 4.3 Deployment Option C: Framework/Library

**Pros**: Reusable in other apps
**Cons**: Requires integration work
**Output**: X3FExtractor.framework

### Phase 5: Build System Integration (Week 5)

#### 5.1 Updated CMakeLists.txt Structure

```cmake
if(IOS)
  # iOS-specific settings
  set(X3F_SOURCES ${CORE_SOURCES})
  add_library(x3f_extract_ios STATIC ${X3F_SOURCES})

  # Link iOS frameworks
  target_link_libraries(x3f_extract_ios
    ${ACCELERATE_FRAMEWORK}
    ${METAL_FRAMEWORK}
    ${IOS_OPENCV_LIBS}
    # ... other dependencies
  )
else()
  # Original desktop build
endif()
```

#### 5.2 Xcode Project Generation

```bash
cmake -G Xcode \
  -DCMAKE_TOOLCHAIN_FILE=ios.toolchain.cmake \
  -DIOS=ON \
  -DPORTABLE=ON \
  ..
```

### Phase 6: Testing and Optimization (Week 6)

#### 6.1 Testing Strategy

1. **Unit Tests**: Port existing tests to iOS
2. **Integration Tests**: Test with sample X3F files
3. **Performance Tests**: Compare with desktop version
4. **Memory Tests**: Ensure iOS memory limits are respected

#### 6.2 Optimization Areas

- Metal shader optimization
- Memory usage reduction
- Background processing support
- Battery usage optimization

## Technical Specifications

### Minimum Requirements

- **iOS Version**: 12.0+
- **Device**: iPad (all models with iOS 12.0+ support)
- **Storage**: ~50MB for app + space for X3F files
- **Memory**: Minimum 2GB RAM recommended

### Build Requirements

- **macOS**: 10.15 or later
- **Xcode**: 12.0 or later
- **CMake**: 3.15 or later
- **Apple Developer Account**: For device testing

### Performance Targets

- Process standard X3F file (20MP) in under 30 seconds
- Memory usage under 500MB for typical operations
- Support background processing for large files

## Risk Assessment and Mitigation

### High Risk Items

1. **OpenCL to Metal conversion complexity**
   - Mitigation: Start with CPU fallback, optimize later
2. **Memory constraints on older iPads**
   - Mitigation: Implement streaming processing for large files
3. **App Store approval (if distributing publicly)**
   - Mitigation: Ensure compliance with App Store guidelines

### Medium Risk Items

1. **Dependency compatibility issues**
   - Mitigation: Test each dependency thoroughly
2. **Performance on older devices**
   - Mitigation: Provide quality/speed options

## Timeline and Milestones

| Week | Phase              | Deliverables                           |
| ---- | ------------------ | -------------------------------------- |
| 1    | Environment Setup  | iOS toolchain, modified CMakeLists.txt |
| 2-3  | Dependencies       | All libraries built for iOS            |
| 3-4  | Code Modifications | iOS-compatible source code             |
| 4-5  | iOS Wrapper        | Working iOS app or tool                |
| 5    | Build Integration  | Complete build system                  |
| 6    | Testing & Polish   | Tested, optimized application          |

## Success Criteria

1. **Functional**: x3f_extract runs successfully on iPad
2. **Performance**: Processes files within acceptable time
3. **Stability**: No crashes with typical X3F files
4. **Usability**: Intuitive interface (if app option)
5. **Distribution**: Can be deployed to target devices

## Next Steps

1. **Immediate Actions**:

   - Set up iOS development environment
   - Create ios.toolchain.cmake file
   - Begin dependency compilation

2. **Decision Points**:

   - Choose deployment option (App recommended)
   - Decide on Metal vs CPU for compute operations
   - Determine distribution method

3. **Resources Needed**:
   - Apple Developer account
   - Test iPad devices
   - Sample X3F files for testing

## Appendix: Code Examples

### A. iOS File Access Example

```objc
// Getting documents directory in iOS
NSArray *paths = NSSearchPathForDirectoriesInDomains(
    NSDocumentDirectory, NSUserDomainMask, YES);
NSString *documentsDirectory = [paths objectAtIndex:0];
```

### B. Metal Compute Kernel Example

```metal
kernel void processImage(
    device const float* input [[buffer(0)]],
    device float* output [[buffer(1)]],
    uint2 gid [[thread_position_in_grid]])
{
    // Metal compute shader implementation
}
```

### C. CMake iOS Detection

```cmake
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    message(STATUS "Building for iOS")
    set(IOS TRUE)
endif()
```

---

This implementation plan provides a comprehensive roadmap for successfully porting x3f_extract to iPad. The modular approach allows for incremental progress and testing at each phase.
