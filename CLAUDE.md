# CLAUDE.md - AI Assistant Context for X3F Tools

## Project Overview
**X3F Tools** is a C/C++ library and command-line utility for processing Sigma X3F RAW image files from Sigma cameras (Foveon X3 sensor). The project focuses on extracting, processing, and converting X3F files to standard formats like DNG, TIFF, and JPEG.

**Primary Executable:** `x3f_extract` - Command-line tool for X3F conversion
**Language:** C (90%) / C++ (10%)
**Lines of Code:** ~8,500
**Platform:** Cross-platform (macOS, Linux, Windows)

## Quick Context
- **X3F Format:** Proprietary RAW format from Sigma cameras using Foveon X3 sensors
- **Sensor Types:** Three generations - Classic (SD9-14), Merrill (DP1M-3M, SD1M), Quattro (DP0Q-3Q, SDQ/QH)
- **Key Feature:** Handles Foveon's unique 3-layer sensor data (captures RGB at each pixel location)

## Project Structure
```
x3f/
├── src/                    # Core source code
│   ├── x3f_io.[ch]        # X3F format parser (2,015 lines - main logic)
│   ├── x3f_process.[ch]   # Image processing pipeline (1,103 lines)
│   ├── x3f_extract.c      # Main CLI tool (808 lines)
│   ├── x3f_denoise_*.cpp  # Denoising algorithms (13,443 lines total)
│   ├── x3f_output_*.c     # Output format handlers (DNG, TIFF, JPEG, PPM)
│   ├── x3f_matrix.[ch]    # Color space transformations
│   ├── x3f_spatial_gain.c # Vignetting correction
│   └── x3f_meta.[ch]      # Metadata extraction
├── bin/                    # Built executables (platform-specific subdirs)
├── deps/                   # External dependencies
├── features/              # BDD test specs (Behave framework)
├── doc/                   # Documentation
└── x3f_test_files/       # Test images (gitignored, external repo)
```

## Key Files to Know
1. **[src/x3f_io.c](src/x3f_io.c)** - Main X3F format parser, Huffman decoder, data extraction
2. **[src/x3f_process.c](src/x3f_process.c)** - Image processing pipeline, color conversion
3. **[src/x3f_extract.c](src/x3f_extract.c)** - CLI tool entry point, option parsing
4. **[src/x3f_denoise_aniso.cpp](src/x3f_denoise_aniso.cpp)** - Primary denoising algorithm
5. **[src/x3f_output_dng.c](src/x3f_output_dng.c)** - Adobe DNG output format
6. **[makefile](makefile)** / **[sys.mk](sys.mk)** - Build system

## Building the Project

### macOS Universal Binary (Intel + Apple Silicon)
```bash
./install_opencv.sh        # Install OpenCV dependency
./create_universal_libs.sh # Create fat libraries
make TARGET=osx-universal  # Build universal binary
# Output: bin/osx-universal/x3f_extract
```

### Other Platforms
```bash
make                       # Native build for current platform
make TARGET=windows-x86_64 # Cross-compile for Windows
make TARGET=linux-x86_64   # Build for Linux
```

## Common Tasks

### Extract X3F to DNG
```bash
bin/osx-universal/x3f_extract -dng image.x3f
```

### Extract with denoising
```bash
bin/osx-universal/x3f_extract -dng -denoise image.x3f
```

### Extract metadata only
```bash
bin/osx-universal/x3f_extract -meta image.x3f
```

### Run tests
```bash
cd features && behave
```

## Development Workflow

### Current Status
- **Branch:** mac-universal-build
- **Main branch:** master
- **Modified files:** src/x3f_extract.c, src/x3f_process.c
- **Untracked:** _local/ directory

### Making Changes
1. Core format handling: Edit [src/x3f_io.c](src/x3f_io.c)
2. Processing pipeline: Edit [src/x3f_process.c](src/x3f_process.c)
3. New output format: Create src/x3f_output_*.c
4. CLI options: Edit [src/x3f_extract.c](src/x3f_extract.c)

### Testing
- Test files needed from: https://github.com/Kalpanika/x3f/tree/master/test/data
- Run BDD tests: `cd features && behave`
- Test outputs are hash-validated for consistency

## Important Technical Details

### X3F Format Versions
- **2.0-2.3:** Classic cameras (SD9, SD10, SD14)
- **3.0:** Merrill generation (DP1M-3M, SD1M)
- **4.0-4.1:** Quattro generation (DP0Q-3Q, SDQ/QH)

### Compression Methods
- **Huffman:** Used in older formats (lossless)
- **TRUE:** Merrill generation codec
- **Quattro:** New format with different layer resolutions

### Color Processing
- **Input:** Foveon 3-layer RGB data
- **Pipeline:** Demosaic → White Balance → Color Matrix → Denoise → Output
- **Output spaces:** sRGB, AdobeRGB, ProPhotoRGB

### Key Challenges
1. **Proprietary format:** No official documentation, reverse-engineered
2. **Multiple versions:** Each camera generation uses different encoding
3. **Firmware bugs:** Code includes workarounds for known camera firmware issues
4. **Large files:** X3F files can be 50-100MB+ each

## Dependencies
- **OpenCV 4.5.5** - Image processing (photo, imgproc, core modules)
- **libtiff** - TIFF output support
- **libjpeg-turbo** - JPEG compression
- **zlib** - Data compression
- **Accelerate.framework** (macOS) - Optimized math operations

## Command-Line Options Reference
```
x3f_extract [options] <x3f_file>

Output formats:
  -dng           Adobe DNG format
  -tiff          TIFF format
  -ppm           PPM format
  -jpeg          JPEG format
  -raw           Raw sensor data
  -meta          Metadata only
  -histogram     Generate histogram

Processing:
  -denoise       Apply denoising
  -no-denoise    Skip denoising
  -color <mode>  Color mode (sRGB, AdobeRGB, ProPhoto, none)
  -wb <mode>     White balance (auto, camera, none)
  -compress      Use compression in output
  -qtop          Quattro top layer interpolation
  -no-crop       Don't crop active area
  -gamma <val>   Apply gamma correction
  -unprocessed   Output unprocessed data
  -ocl           Use OpenCL acceleration
  -legacy        Use legacy processing

Debug:
  -v             Verbose output
  -q             Quiet mode
```

## Recent Development Focus
- Added FOV classic blue support
- Fixed compression bugs
- Created macOS universal build support
- Working on custom white balance implementation (see _local/)

## Tips for AI Assistants
1. **Always check x3f_io.h for data structures** - It defines all X3F format structures
2. **Test with both Merrill and Quattro samples** - They use different codecs
3. **Use -v flag for debugging** - Provides detailed processing information
4. **Memory usage can be high** - X3F files expand significantly when decoded
5. **Build system is makefile-based** - No CMake/autotools, check sys.mk for platform detection
6. **Denoising is computationally expensive** - The anisotropic diffusion algorithm is complex
7. **DNG output is preferred** - Most compatible with photo editing software

## Project Maintainer Notes
- Original project by Roland Karlsson and others
- Fork focuses on modern compiler support and macOS universal binaries
- Test coverage via BDD specs ensures compatibility across changes
- Performance critical sections use SIMD when available