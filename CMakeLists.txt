cmake_minimum_required(VERSION 3.10)
project(x3f_tools C CXX)

option(PORTABLE "Build a portable binary with static linking" OFF)
option(IOS "Build for iOS/iPadOS" OFF)

# iOS Detection and Configuration
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    message(STATUS "Building for iOS")
    set(IOS TRUE)
endif()

if(IOS)
    set(CMAKE_MACOSX_BUNDLE YES)
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer")
    
    # Find iOS-specific frameworks
    find_library(ACCELERATE_FRAMEWORK Accelerate)
    find_library(METAL_FRAMEWORK Metal)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(UIKIT_FRAMEWORK UIKit)
    
    message(STATUS "iOS build configured with frameworks:")
    message(STATUS "  Accelerate: ${ACCELERATE_FRAMEWORK}")
    message(STATUS "  Metal: ${METAL_FRAMEWORK}")
    message(STATUS "  Foundation: ${FOUNDATION_FRAMEWORK}")
    message(STATUS "  UIKit: ${UIKIT_FRAMEWORK}")
    
    # For iOS, we need to use iOS-specific paths for libraries
    # Set the iOS SDK path
    execute_process(
        COMMAND xcrun --sdk iphoneos --show-sdk-path
        OUTPUT_VARIABLE IOS_SDK_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    
    # Set search paths for iOS libraries
    set(IOS_LIB_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ios_libs")
    message(STATUS "Looking for iOS libraries in: ${IOS_LIB_PATH}")
endif()

if(PORTABLE)
  message(STATUS "Attempting to build a portable binary. This requires static versions of dependencies.")
  if(APPLE AND NOT IOS)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ${CMAKE_FIND_LIBRARY_SUFFIXES})
  elseif(IOS)
    # For iOS, we always use static libraries
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a")
  else()
    # For other platforms like Linux
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
  endif()
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(IOS)
  # For iOS, we need OpenCV built for iOS
  set(OPENCV_IOS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/ios_libs/opencv")
  file(GLOB OpenCV_STATIC_LIBS "${OPENCV_IOS_PATH}/lib/*.a")
  include_directories(${OPENCV_IOS_PATH}/include/opencv4)
  if(NOT OpenCV_STATIC_LIBS)
    message(WARNING "OpenCV for iOS not found. You need to build it for iOS first.")
    set(OpenCV_STATIC_LIBS "")
  endif()
else()
  file(GLOB OpenCV_STATIC_LIBS "/opt/homebrew/opt/opencv/lib/*.a")
  include_directories(/opt/homebrew/opt/opencv/include/opencv4)
endif()

if(PORTABLE)
  # Add Homebrew paths for finding libraries on macOS (but not iOS)
  if(APPLE AND NOT IOS)
    list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew" "/usr/local")
    set(CMAKE_LIBRARY_PATH "/opt/homebrew/lib" "/usr/local/lib" ${CMAKE_LIBRARY_PATH})
  endif()
  
  # For portable builds, use system zlib but static versions of other libraries
  if(IOS)
    # For iOS, directly use the system zlib from the iOS SDK
    # Since it's always available in iOS SDK, we can set it directly
    set(ZLIB_LIBRARY "${IOS_SDK_PATH}/usr/lib/libz.tbd")
    set(ZLIB_INCLUDE_DIR "${IOS_SDK_PATH}/usr/include")
    set(ZLIB_LIBRARIES ${ZLIB_LIBRARY})
    set(ZLIB_INCLUDE_DIRS ${ZLIB_INCLUDE_DIR})
    set(ZLIB_FOUND TRUE)
    message(STATUS "Using iOS SDK ZLIB: ${ZLIB_LIBRARY}")
  else()
    find_package(ZLIB REQUIRED)  # Use system zlib dynamically
  endif()
  
  if(IOS)
    # For iOS, look for libraries in iOS-specific location
    # Making these optional for initial iOS setup
    find_library(TIFF_STATIC_LIBRARY NAMES libtiff.a tiff.a 
      PATHS ${IOS_LIB_PATH}/lib ${IOS_SDK_PATH}/usr/lib
      PATH_SUFFIXES lib
      NO_DEFAULT_PATH)
    find_library(JPEG_STATIC_LIBRARY NAMES libjpeg.a jpeg.a 
      PATHS ${IOS_LIB_PATH}/lib ${IOS_SDK_PATH}/usr/lib
      PATH_SUFFIXES lib
      NO_DEFAULT_PATH)
    find_library(ZSTD_STATIC_LIBRARY NAMES libzstd.a zstd.a 
      PATHS ${IOS_LIB_PATH}/lib ${IOS_SDK_PATH}/usr/lib
      PATH_SUFFIXES lib
      NO_DEFAULT_PATH)
    find_library(TBB_STATIC_LIBRARY NAMES libtbb.a tbb.a 
      PATHS ${IOS_LIB_PATH}/lib ${IOS_SDK_PATH}/usr/lib
      PATH_SUFFIXES lib
      NO_DEFAULT_PATH)
    find_library(LZMA_STATIC_LIBRARY NAMES liblzma.a lzma.a 
      PATHS ${IOS_LIB_PATH}/lib ${IOS_SDK_PATH}/usr/lib
      PATH_SUFFIXES lib
      NO_DEFAULT_PATH)
      
    # Check which libraries were found
    if(NOT TIFF_STATIC_LIBRARY)
      message(WARNING "libtiff not found for iOS. You need to build it for iOS first.")
      set(TIFF_STATIC_LIBRARY "")
    endif()
    if(NOT JPEG_STATIC_LIBRARY)
      message(WARNING "libjpeg not found for iOS. You need to build it for iOS first.")
      set(JPEG_STATIC_LIBRARY "")
    endif()
    if(NOT ZSTD_STATIC_LIBRARY)
      message(WARNING "libzstd not found for iOS. You need to build it for iOS first.")
      set(ZSTD_STATIC_LIBRARY "")
    endif()
    if(NOT TBB_STATIC_LIBRARY)
      message(WARNING "libtbb not found for iOS. You need to build it for iOS first.")
      set(TBB_STATIC_LIBRARY "")
    endif()
    if(NOT LZMA_STATIC_LIBRARY)
      message(WARNING "liblzma not found for iOS. You need to build it for iOS first.")
      set(LZMA_STATIC_LIBRARY "")
    endif()
  else()
    find_library(TIFF_STATIC_LIBRARY NAMES libtiff.a tiff.a 
      PATHS /opt/homebrew/lib /usr/local/lib /usr/lib
      PATH_SUFFIXES lib
      REQUIRED)
    find_library(JPEG_STATIC_LIBRARY NAMES libjpeg.a jpeg.a 
      PATHS /opt/homebrew/lib /usr/local/lib /usr/lib
      PATH_SUFFIXES lib
      REQUIRED)
    find_library(ZSTD_STATIC_LIBRARY NAMES libzstd.a zstd.a 
      PATHS /opt/homebrew/lib /usr/local/lib /usr/lib
      PATH_SUFFIXES lib
      REQUIRED)
    find_library(TBB_STATIC_LIBRARY NAMES libtbb.a tbb.a 
      PATHS /opt/homebrew/lib /usr/local/lib /usr/lib
      PATH_SUFFIXES lib
      REQUIRED)
    find_library(LZMA_STATIC_LIBRARY NAMES liblzma.a lzma.a 
      PATHS /opt/homebrew/lib /usr/local/lib /usr/lib
      PATH_SUFFIXES lib
      REQUIRED)
  endif()
  
  if(IOS)
    # Additional libtiff dependencies for iOS
    find_library(WEBP_STATIC_LIBRARY NAMES libwebp.a webp.a
      PATHS ${IOS_LIB_PATH}/lib ${IOS_SDK_PATH}/usr/lib
      PATH_SUFFIXES lib
      NO_DEFAULT_PATH)
    find_library(JBIG_STATIC_LIBRARY NAMES libjbig.a jbig.a
      PATHS ${IOS_LIB_PATH}/lib ${IOS_SDK_PATH}/usr/lib
      PATH_SUFFIXES lib
      NO_DEFAULT_PATH)
    find_library(DEFLATE_STATIC_LIBRARY NAMES libdeflate.a deflate.a
      PATHS ${IOS_LIB_PATH}/lib ${IOS_SDK_PATH}/usr/lib
      PATH_SUFFIXES lib
      NO_DEFAULT_PATH)
    find_library(SHARPYUV_STATIC_LIBRARY NAMES libsharpyuv.a sharpyuv.a
      PATHS ${IOS_LIB_PATH}/lib ${IOS_SDK_PATH}/usr/lib
      PATH_SUFFIXES lib
      NO_DEFAULT_PATH)
    
    # Include directories for iOS
    find_path(TIFF_INCLUDE_DIR NAMES tiff.h
      PATHS ${IOS_LIB_PATH}/include ${IOS_SDK_PATH}/usr/include)
    find_path(JPEG_INCLUDE_DIR NAMES jpeglib.h
      PATHS ${IOS_LIB_PATH}/include ${IOS_SDK_PATH}/usr/include)
  else()
    # Additional libtiff dependencies that might be needed
    find_library(WEBP_STATIC_LIBRARY NAMES libwebp.a webp.a
      PATHS /opt/homebrew/lib /usr/local/lib /usr/lib
      PATH_SUFFIXES lib)
    find_library(JBIG_STATIC_LIBRARY NAMES libjbig.a jbig.a
      PATHS /opt/homebrew/lib /usr/local/lib /usr/lib
      PATH_SUFFIXES lib)
    find_library(DEFLATE_STATIC_LIBRARY NAMES libdeflate.a deflate.a
      PATHS /opt/homebrew/lib /usr/local/lib /usr/lib
      PATH_SUFFIXES lib)
    find_library(SHARPYUV_STATIC_LIBRARY NAMES libsharpyuv.a sharpyuv.a
      PATHS /opt/homebrew/lib /usr/local/lib /usr/lib
      PATH_SUFFIXES lib)
    
    # Include directories
    find_path(TIFF_INCLUDE_DIR NAMES tiff.h
      PATHS /opt/homebrew/include /usr/local/include /usr/include)
    find_path(JPEG_INCLUDE_DIR NAMES jpeglib.h
      PATHS /opt/homebrew/include /usr/local/include /usr/include)
  endif()
  
  # Build the TIFF_LIBRARIES with all dependencies
  set(TIFF_LIBRARIES ${TIFF_STATIC_LIBRARY})
  if(WEBP_STATIC_LIBRARY)
    list(APPEND TIFF_LIBRARIES ${WEBP_STATIC_LIBRARY})
  endif()
  if(SHARPYUV_STATIC_LIBRARY)
    list(APPEND TIFF_LIBRARIES ${SHARPYUV_STATIC_LIBRARY})
  endif()
  if(JBIG_STATIC_LIBRARY)
    list(APPEND TIFF_LIBRARIES ${JBIG_STATIC_LIBRARY})
  endif()
  if(DEFLATE_STATIC_LIBRARY)
    list(APPEND TIFF_LIBRARIES ${DEFLATE_STATIC_LIBRARY})
  endif()
  
  set(JPEG_LIBRARIES ${JPEG_STATIC_LIBRARY})
  # ZLIB_LIBRARIES is already set by find_package(ZLIB)
  set(LZMA_LIBRARIES ${LZMA_STATIC_LIBRARY})
else()
  # For non-portable builds, use standard find_package
  find_package(ZLIB REQUIRED)
  find_package(TIFF REQUIRED)
  find_package(JPEG REQUIRED)
  find_library(ZSTD_STATIC_LIBRARY NAMES libzstd.a REQUIRED)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LZMA REQUIRED liblzma)
  find_library(TBB_STATIC_LIBRARY NAMES libtbb.a REQUIRED)
endif()

if(NOT IOS)
    find_package(BLAS REQUIRED)
else()
    # On iOS, use Accelerate framework instead of BLAS
    set(BLAS_LIBRARIES ${ACCELERATE_FRAMEWORK})
endif()

# Include directories - only add if they exist
include_directories(${ZLIB_INCLUDE_DIRS})

if(OpenCV_INCLUDE_DIRS)
  include_directories(${OpenCV_INCLUDE_DIRS})
endif()

if(TIFF_INCLUDE_DIRS)
  include_directories(${TIFF_INCLUDE_DIRS})
endif()

if(JPEG_INCLUDE_DIR)
  include_directories(${JPEG_INCLUDE_DIR})
endif()

if(ZSTD_INCLUDE_DIR)
  include_directories(${ZSTD_INCLUDE_DIR})
endif()

if(LZMA_INCLUDE_DIRS)
  include_directories(${LZMA_INCLUDE_DIRS})
endif()

add_library(x3f_version src/x3f_version.c)
target_compile_definitions(x3f_version PRIVATE -DVERSION="0.0.1")

# Define core source files
set(X3F_CORE_SOURCES
    src/x3f_extract.c
    src/x3f_io.c
    src/x3f_process.c
    src/x3f_meta.c
    src/x3f_image.c
    src/x3f_spatial_gain.c
    src/x3f_output_dng.c
    src/x3f_output_tiff.c
    src/x3f_output_ppm.c
    src/x3f_histogram.c
    src/x3f_print_meta.c
    src/x3f_dump.c
    src/x3f_matrix.c
    src/x3f_dngtags.c
    src/x3f_denoise_utils.cpp
    src/x3f_denoise_aniso.cpp
    src/x3f_denoise.cpp
    src/x3f_printf.c
)

if(IOS)
    # For iOS, create a static library
    add_library(x3f_extract_ios STATIC ${X3F_CORE_SOURCES})
    
    # Link iOS frameworks and libraries
    target_link_libraries(x3f_extract_ios 
        x3f_version 
        ${OpenCV_STATIC_LIBS} 
        ${TIFF_LIBRARIES} 
        ${JPEG_LIBRARIES} 
        ${ZSTD_STATIC_LIBRARY} 
        ${LZMA_LIBRARIES} 
        ${ZLIB_LIBRARIES} 
        ${TBB_STATIC_LIBRARY} 
        ${BLAS_LIBRARIES}
        ${ACCELERATE_FRAMEWORK}
        ${METAL_FRAMEWORK}
        ${FOUNDATION_FRAMEWORK}
        iconv
    )
    
    # Set iOS-specific compile definitions
    target_compile_definitions(x3f_extract_ios PRIVATE -DPLATFORM_IOS=1)
    
else()
    # Original desktop build
    add_executable(x3f_extract ${X3F_CORE_SOURCES})
    target_link_libraries(x3f_extract x3f_version ${OpenCV_STATIC_LIBS} ${TIFF_LIBRARIES} ${JPEG_LIBRARIES} ${ZSTD_STATIC_LIBRARY} ${LZMA_LIBRARIES} ${ZLIB_LIBRARIES} ${TBB_STATIC_LIBRARY} ${BLAS_LIBRARIES})
endif()

add_executable(x3f_io_test
    src/x3f_io_test.c
    src/x3f_io.c
    src/x3f_print_meta.c
    src/x3f_printf.c
)

target_link_libraries(x3f_io_test x3f_version iconv)

add_executable(x3f_matrix_test
    src/x3f_matrix_test.c
    src/x3f_matrix.c
    src/x3f_printf.c
)

target_link_libraries(x3f_matrix_test m)

if(APPLE AND NOT IOS)
    target_link_libraries(x3f_extract "-framework OpenCL" iconv)
endif()
