# iOS CMake Toolchain File
# Based on the implementation plan for x3f_extract iPad support

set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0")
set(CMAKE_OSX_ARCHITECTURES arm64)
set(CMAKE_OSX_SYSROOT iphoneos)
set(CMAKE_C_COMPILER xcrun clang)
set(CMAKE_CXX_COMPILER xcrun clang++)

# Set iOS specific flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -arch arm64 -miphoneos-version-min=12.0")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -arch arm64 -miphoneos-version-min=12.0")

# Force the compilers to work
set(CMAKE_C_COMPILER_WORKS TRUE)
set(CMAKE_CXX_COMPILER_WORKS TRUE)

# Set the find root path mode
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# iOS specific settings
set(IOS TRUE)
set(APPLE TRUE)

# Code signing identity for development
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")
set(CMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "")
