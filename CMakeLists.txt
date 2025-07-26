cmake_minimum_required(VERSION 3.10)
project(x3f_tools C CXX)

option(PORTABLE "Build a portable binary with static linking" OFF)

if(PORTABLE)
  message(STATUS "Attempting to build a portable binary. This requires static versions of dependencies.")
  if(APPLE)
    set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ${CMAKE_FIND_LIBRARY_SUFFIXES})
  else()
    # For other platforms like Linux
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
  endif()
endif()

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB OpenCV_STATIC_LIBS "/opt/homebrew/opt/opencv/lib/*.a")
include_directories(/opt/homebrew/opt/opencv/include/opencv4)
find_package(ZLIB REQUIRED)
find_package(TIFF REQUIRED)
find_package(JPEG REQUIRED)
find_library(ZSTD_STATIC_LIBRARY NAMES libzstd.a REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LZMA REQUIRED liblzma)
find_library(TBB_STATIC_LIBRARY NAMES libtbb.a REQUIRED)
find_package(BLAS REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS} ${ZLIB_INCLUDE_DIRS} ${TIFF_INCLUDE_DIRS} ${JPEG_INCLUDE_DIR} ${ZSTD_INCLUDE_DIR} ${LZMA_INCLUDE_DIRS})

add_library(x3f_version src/x3f_version.c)
target_compile_definitions(x3f_version PRIVATE -DVERSION="0.0.1")

add_executable(x3f_extract
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

target_link_libraries(x3f_extract x3f_version ${OpenCV_STATIC_LIBS} ${TIFF_LIBRARIES} ${JPEG_LIBRARIES} ${ZSTD_STATIC_LIBRARY} ${LZMA_LIBRARIES} ${ZLIB_LIBRARIES} ${TBB_STATIC_LIBRARY} ${BLAS_LIBRARIES})

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

if(APPLE)
    target_link_libraries(x3f_extract "-framework OpenCL" iconv)
endif()
