# X3F Image Processing Project - Comprehensive Documentation

## Project Overview

**X3F Tools** is a comprehensive C/C++ library and toolset for reading, processing, and converting Sigma X3F raw image files. The project specifically targets Sigma camera formats including:
- SD-series (SD9, SD10, SD14, SD1, SD1 Merrill, SDQ, SDQH)
- DP-series (DP1, DP1 Merrill, DP2, DP2 Merrill, DP3 Merrill, DP0Q, DP1Q, DP2Q, DP3Q)

**Repository Location:** /Users/sang/Developer/x3f
**Current Branch:** mac-universal-build
**Total Lines of Code:** ~8,500 (C/C++)

---

## 1. Project Structure

### Root Level Directories

```
x3f/
├── src/                      # Main source code (C/C++ libraries and tools)
├── doc/                      # Documentation files
├── bin/                      # Compiled binaries (platform-specific)
├── deps/                     # External dependencies (OpenCV, libtiff, etc.)
├── features/                 # BDD test specifications (Behave/Gherkin format)
├── x3f_test_files/          # Test sample X3F images (from external repo)
├── x3f.xcodeproj/           # Xcode project for macOS
├── makefile                 # Main build configuration
├── sys.mk                   # Platform detection and compiler setup
├── install_opencv.sh        # OpenCV dependency installation script
├── create_universal_libs.sh # macOS universal binary creation script
├── test_compression.sh      # Compression testing script
├── requirements.txt         # Python dependencies (behave for BDD tests)
├── Vagrantfile              # Vagrant configuration for cross-platform building
├── *.cmake                  # CMake toolchain files for cross-compilation
└── .gitignore, .git/        # Git configuration
```

### Key Binaries Location
- **OSX Universal:** `/Users/sang/Developer/x3f/bin/osx-universal/`

---

## 2. Build System & Compilation

### Build Configuration: sys.mk
The system detection makefile handles:
- **Host Detection:** Automatically detects Linux, macOS (Darwin), or Windows
- **CPU Detection:** Universal, x86_64, i386, or ARM64 (arm64)
- **Cross-compilation:** Supports building Windows and macOS binaries from Linux
- **Compiler Setup:** 
  - Native: gcc, g++
  - Windows: MinGW (x86_64-w64-mingw32, i686-w64-mingw32)
  - macOS: x86_64-apple-darwin11, i386-apple-darwin11, universal

### Main Makefile Targets

```bash
make                          # Build for current platform
make TARGET=osx-universal     # Build universal macOS binary
make TARGET=windows-x86_64    # Cross-compile for Windows 64-bit
make dist                     # Create distribution package
make clean                    # Clean object files
make clobber                  # Clean binaries and packages
```

### Dependencies

**External Libraries:**
- **OpenCV 4.5.5** - Image processing (photo, imgproc, core modules)
- **libtiff** - TIFF image format support
- **libjpeg-turbo** - JPEG compression/decompression
- **zlib** - Compression support
- **Accelerate Framework** (macOS) - Optimized mathematics
- **OpenCL** - GPU acceleration support (optional)

**Build Scripts:**
- `install_opencv.sh` - Downloads, configures, and compiles OpenCV for the target platform
- `create_universal_libs.sh` - Creates fat/universal macOS libraries combining x86_64 and arm64

---

## 3. Core Source Modules

### Total: ~45 Source Files, ~7,800 lines of C/C++

#### 3.1 Input/Output & Format Handling (x3f_io.*)

**Files:**
- `x3f_io.h` (482 lines) - Main I/O header with comprehensive data structures
- `x3f_io.c` (2,015 lines) - Primary I/O implementation

**Responsibilities:**
- Parse X3F file format (header, directory sections, subsections)
- Handle multiple X3F versions (2.0, 2.1, 2.2, 2.3, 3.0, 4.0, 4.1)
- Read different image types:
  - **Thumbnails:** Plain, Huffman-encoded, JPEG
  - **RAW data:** Huffman compressed, TRUE codec, Merrill format, Quattro format
  - **Special formats:** SDQ, SDQH (newer sensor versions)
- Huffman tree decoding for lossless and lossy compression
- Endianness handling (assumes little-endian X3F files)
- CAMF (camera firmware) metadata parsing
- Property list extraction with UTF-16 to UTF-8 conversion

**Key Data Structures:**
```c
typedef struct x3f_s {
  x3f_info_t info;                          // Error info and file handles
  x3f_header_t header;                      // File header (FOVb)
  x3f_directory_section_t directory_section; // Directory entries (SECd)
} x3f_t;
```

**Key Functions:**
- `x3f_new_from_file()` - Load and parse X3F file
- `x3f_delete()` - Free X3F resources
- `x3f_get_raw()` - Get RAW image data section
- `x3f_get_thumb_*()` - Get thumbnail images
- `x3f_load_image_block()` - Decode image data
- `x3f_load_data()` - Load compressed data

#### 3.2 Image Processing (x3f_process.*)

**Files:**
- `x3f_process.h` (22 lines) - Processing API
- `x3f_process.c` (1,103 lines) - Core processing engine

**Responsibilities:**
- Convert RAW X3 Bayer data to standard color spaces
- Color encoding modes:
  - `NONE` - Preprocessed but unconverted
  - `SRGB` - sRGB color space (default)
  - `ARGB` - Adobe RGB
  - `PPRGB` - ProPhoto RGB
  - `UNPROCESSED` - Raw data without preprocessing
  - `QTOP` - Quattro top layer without preprocessing
- White balance adjustment with preset options (Auto, Sunlight, Shade, Overcast, etc.)
- Black level and white level correction
- Spatial gain (vignetting) compensation
- Denoising operations
- Bad pixel fixing
- Cropping to active image area
- Gain matrix application (camera calibration data)
- Matrix transformations (color conversion matrices)

**Key Functions:**
```c
int x3f_get_image(x3f_t *x3f,
                 x3f_area16_t *image,
                 x3f_image_levels_t *ilevels,
                 x3f_color_encoding_t encoding,
                 int crop,
                 int fix_bad,
                 int denoise,
                 int apply_sgain,
                 char *wb);
int x3f_get_preview(...); // Generate preview image
int x3f_get_gain(...);    // Get white balance gains
```

#### 3.3 Output Formats

**DNG Output (x3f_output_dng.* - 565 lines)**
- Converts RAW to Adobe DNG (Digital Negative) format
- Preserves color conversion matrices in DNG metadata
- Supports ZIP compression
- Preserves raw sensor data for lossless workflows

**TIFF Output (x3f_output_tiff.c - 67 lines)**
- RGB TIFF export (3x16-bit)
- Color-space aware conversion
- ZIP compression option
- Better for color-correct images than PPM

**PPM Output (x3f_output_ppm.c - 76 lines)**
- Portable Pixmap format (P3 ASCII, P6 binary)
- 3x16-bit format (not universally supported)
- Useful for intermediate processing

**JPEG Output (via x3f_dump.c)**
- Extracts embedded JPEG thumbnail
- Useful for quick preview

#### 3.4 Metadata & Camera Information (x3f_meta.*, x3f_dngtags.*)

**Files:**
- `x3f_meta.h` / `x3f_meta.c` (417 lines) - CAMF/PROP access
- `x3f_dngtags.h` (141 lines) - DNG tag definitions
- `x3f_dngtags.c` (54 lines)

**Responsibilities:**
- Parse CAMF (camera firmware) data sections
- Extract camera matrices (color, gain correction)
- Read property lists
- Support for camera-specific color matrices (by ISO, white balance)
- DNG TIFF tag generation
- Camera model identification

**Key Functions:**
```c
int x3f_get_camf_text(x3f_t *x3f, char *name, char **text);
int x3f_get_camf_matrix(...);  // Get color/gain matrices
int x3f_get_camf_float(...);   // Extract floating point values
char *x3f_get_wb(x3f_t *x3f);  // Get white balance info
```

#### 3.5 Spatial Gain Correction (x3f_spatial_gain.* - 473 lines)

**Purpose:** Correct for vignetting and sensor non-uniformity

**Features:**
- Merrill-type gain correction (SD1M, DP1M, DP2M, DP3M)
- Quattro sensor-specific handling
- Interpolation for smooth gain maps
- Per-channel gain application
- High Pass (HP) filter support for certain models

#### 3.6 Denoising (x3f_denoise.*, x3f_denoise_aniso.*, x3f_denoise_utils.*)

**Files:**
- `x3f_denoise.h` / `x3f_denoise.cpp` - Main API
- `x3f_denoise_aniso.cpp` (13,443 lines) - Anisotropic diffusion filtering
- `x3f_denoise_utils.cpp` - Utility functions
- `x3f_denoise.cpp` (4,105 lines)

**Denoising Types:**
- `X3F_DENOISE_STD` - Standard denoising
- `X3F_DENOISE_F20` - F20 sensor variant
- `X3F_DENOISE_F23` - F23 sensor variant

**Features:**
- Anisotropic diffusion filtering (edge-preserving)
- Quattro sensor expansion (interpolate top layer to full resolution)
- OpenCL acceleration support

#### 3.7 Color Space Conversion (x3f_matrix.*)

**Files:** `x3f_matrix.h` / `x3f_matrix.c` (272 lines)

**Operations:**
- 3x3 matrix operations (multiply, invert, transpose)
- 3x1 vector operations
- Color space conversions:
  - XYZ ↔ ProPhoto RGB
  - XYZ ↔ Adobe RGB
  - XYZ ↔ sRGB
  - Bradford chromatic adaptation (D50 ↔ D65)
- Lookup Table (LUT) generation for gamma correction
- sRGB gamma encoding/decoding

#### 3.8 Image Area Management (x3f_image.*)

**Files:** `x3f_image.h` / `x3f_image.c` (237 lines)

**Responsibilities:**
- Raw image area extraction
- Quattro top layer extraction
- Cropping to active sensor area
- Dark shield/black reference area identification
- Column-based calibration area access

#### 3.9 Histogram Generation (x3f_histogram.* - 108 lines)

**Features:**
- Per-channel histogram computation
- CSV output format
- Linear and logarithmic exposure scale options
- Support for all color encodings

#### 3.10 Metadata Printing (x3f_print_meta.* - 508 lines)

**Features:**
- Structured metadata output
- Camera model and settings information
- CAMF entry analysis and display
- Debugging information for development

#### 3.11 Dump Functions (x3f_dump.* - 70 lines)

**Functions:**
- `x3f_dump_raw_data()` - Export raw sensor data unmodified
- `x3f_dump_jpeg()` - Extract embedded JPEG thumbnail

#### 3.12 Utilities

**x3f_printf.* - Printf-level logging system**
- Verbosity levels: DEBUG, INFO, WARN, ERR
- Prefix formatting for output clarity

**x3f_version.* - Version information**
- Build-time version injection
- Git hash/tag tracking

#### 3.13 Testing Programs

**x3f_io_test.c (99 lines)**
- I/O and metadata parsing verification
- Development/debugging tool

**x3f_matrix_test.c (60 lines)**
- Matrix operation validation
- Color space conversion testing

---

## 4. Main Executable: x3f_extract

**File:** `src/x3f_extract.c` (464 lines)

### Purpose
Main command-line tool for converting X3F files to various output formats.

### Command-Line Interface

```bash
x3f_extract [SWITCHES] <file1> [file2] ...

FORMAT SWITCHES (mutually exclusive):
  -meta               Extract metadata only
  -jpg                Extract embedded JPEG thumbnail
  -raw                Dump raw sensor data unencoded
  -tiff               Output as 16-bit TIFF
  -dng                Output as DNG (default)
  -ppm-ascii          Output as ASCII PPM/P3 (16-bit)
  -ppm                Output as binary PPM/P6 (16-bit)
  -histogram          Generate CSV histogram
  -loghist            Generate CSV histogram (log scale)

COLOR & PROCESSING SWITCHES:
  -o <DIR>            Output directory
  -v                  Verbose debug output
  -q                  Quiet (errors only)
  -color <CS>         Color space (none, sRGB, AdobeRGB, ProPhotoRGB)
  -unprocessed        Raw data without preprocessing
  -qtop               Quattro top layer (no preprocessing)
  -no-crop            Don't crop to active area
  -no-denoise         Disable denoising
  -no-sgain           Disable spatial gain correction
  -no-fix-bad         Don't fix bad pixels
  -sgain              Enable spatial gain (default except Quattro)
  -wb <WB>            White balance (preset or R,G,B values)
  -compress           Enable ZIP compression (DNG/TIFF)
  -ocl                Use OpenCL for acceleration

SPECIAL/LEGACY SWITCHES:
  -offset <N>         Legacy offset for old cameras (SD14, etc.)
  -matrixmax <N>      Max printed matrix elements (default 100)
```

### Processing Flow

1. **Parse command-line arguments** - Set output format, color space, processing options
2. **File processing loop** - For each input X3F file:
   - Open file with error handling
   - Parse X3F structure via x3f_io
   - Extract RAW image data via x3f_process
   - Optionally denoise, apply spatial gain, crop
   - Convert to selected color space
   - Write output file(s)
   - Handle errors and report results

### Output File Generation

For each input file, multiple outputs can be generated:
- Standard DNG (default)
- Compressed DNG (with -compress)
- Cropped or uncropped variants
- Different color space versions
- Metadata/histogram files
- Temporary files with atomic write (write to .tmp, then rename)

---

## 5. Supported Camera Models

### X3 TRUE Engines (2003-2008)
- SD9, SD10, SD14

### TRUE I (2008-2012)
- SD1, SD1 Merrill
- DP1, DP1 Merrill, DP2, DP2 Merrill, DP3 Merrill

### TRUE II / Quattro (2013+)
- SD Quattro, SD Quattro H
- DP0 Quattro, DP1 Quattro, DP2 Quattro, DP3 Quattro

### Image Sensor Variations
- **Merrill**: Single-layer Bayer sensor (4 colors: R, G, B, custom green)
- **Quattro**: Three-layer sensor (bottom/middle/top layers) with extended color range
- **SDQ/SDQH**: Higher resolution Quattro sensors

---

## 6. Testing & Quality Assurance

### Test Framework
- **Framework:** Behave (BDD - Behavior-Driven Development)
- **Language:** Python with gherkin scenarios
- **Location:** `/Users/sang/Developer/x3f/features/consistency.feature`

### Test Coverage

**consistency.feature** - ~87 test scenarios covering:

1. **Basic Conversions** - MD5 hash validation for:
   - DNG, TIFF, PPM (binary/ASCII), JPEG extraction
   - Metadata extraction
   - Raw data dumps
   - Histograms (linear and log scale)

2. **Compressed Output** - ZIP compression for DNG/TIFF with validation

3. **Denoising** - Denoised output validation for DNG and TIFF

4. **Color Space Variations:**
   - Cropped vs. uncropped
   - Unprocessed raw data
   - Color-encoded (sRGB, Adobe RGB, ProPhoto RGB)
   - Quattro top layer

### Test Sample Images

**Location:** `/Users/sang/Developer/x3f/x3f_test_files/`

Primary test images:
- `_SDI8040.X3F` - Merrill image with color diversity (rainbow)
- `_SDI8284.X3F` - DP2 Quattro image
- `SDQH5085-87.X3F` - Quattro H images

Extended test set (~90 MB):
- Multiple cameras across product lines
- Various firmware versions
- Edge cases and special scenarios

### Running Tests

```bash
make check              # Run all BDD tests
make check_deps         # Set up test dependencies (virtual env, test files)
```

**Test Execution:**
- Tests are parameter-driven (tabular data in .feature file)
- Each test runs x3f_extract with specific flags
- Output validated via MD5 hash comparison
- Tests ensure reproducibility and detect regressions

---

## 7. Build Targets & Platforms

### Native Builds
```bash
make              # Build for host platform (auto-detected)
```

### Explicit Targets

```bash
make TARGET=osx-universal           # macOS Intel + Apple Silicon
make TARGET=linux-x86_64            # Linux 64-bit
make TARGET=windows-x86_64          # Windows 64-bit
make TARGET=windows-i686            # Windows 32-bit
make TARGET=osx-x86_64              # macOS Intel only
make TARGET=osx-i386                # macOS PowerPC (legacy)
```

### Distribution Packaging

```bash
make dist          # Create platform-specific distribution
make dist-all      # Build all distributions (Linux host only)
```

Output:
- **Windows:** `.zip` file
- **Other:** `.tar.gz` file

### macOS Universal Binary Creation

**Script:** `create_universal_libs.sh`

Process:
1. Builds OpenCV and dependencies for both x86_64 and arm64
2. Uses `lipo` to combine binaries
3. Creates universal libraries in `/deps/lib/osx-universal/`
4. Produces x3f_extract binary supporting both architectures

---

## 8. Current Development Status

### Current Branch
- **Branch:** `mac-universal-build`
- **Latest commits (from newest):**
  - `7b9300b` - feat: add fov classic blue (color matrix)
  - `817b8c3` - bugfix: fix compression bugs
  - `0ac1cac` - feat: fix build. add docs
  - `2ac704d` - feat: add universal build script

### Modified Files (Uncommitted Changes)
- `src/x3f_extract.c` - Command-line interface modifications
- `src/x3f_process.c` - Processing logic updates

### Uncommitted Untracked Files
- `_local/` - Local development directory (gitignored)

---

## 9. Key Technical Decisions

### 1. Little-Endian Assumption
X3F files are always stored in little-endian format. Multi-byte values are handled with endianness conversion as needed.

### 2. Modular Architecture
Code is organized into functional modules with clear separation:
- I/O parsing (x3f_io)
- Processing (x3f_process)
- Output formats (x3f_output_*)
- Supporting utilities (metadata, matrices, etc.)

### 3. Memory Management
- Explicit allocation/deallocation pattern
- Buffer management with separate data and metadata pointers
- Support for in-file and in-memory buffers

### 4. Camera-Specific Workarounds
- Firmware bugs accommodated (e.g., DP2 DarkShieldBottom issue)
- Model-specific code paths for Merrill vs. Quattro sensors
- Legacy offset handling for older cameras

### 5. Compression Support
- Huffman coding for RAW data
- TRUE codec for newer cameras
- ZIP compression for output formats
- Lossless and lossy compression modes

### 6. OpenCL Acceleration
- Optional GPU acceleration for denoising
- Graceful fallback to CPU if not available
- Runtime flag control

---

## 10. Dependencies Summary

### External Libraries (Built from Source)

| Library | Version | Purpose |
|---------|---------|---------|
| OpenCV | 4.5.5 | Image processing (photo, imgproc, core) |
| libtiff | Bundled with OpenCV | TIFF I/O and format handling |
| libjpeg-turbo | Bundled with OpenCV | JPEG compression/decompression |
| zlib | Bundled with OpenCV | Data compression |
| libittnotify | Bundled with OpenCV | Threading instrumentation |

### System Frameworks (macOS)
- OpenCL - GPU acceleration
- Accelerate - Optimized math operations
- Core Foundation - System utilities

### Build Tools
- gcc/g++ - C/C++ compilation
- cmake - Build system configuration
- make - Build orchestration
- lipo - Universal binary creation

### Runtime Dependencies
- Virtual environment with Python 2.7+ (for tests)
- behave 1.2.5 (BDD test runner)
- Git (for version tracking)

---

## 11. File Organization Summary

### Core Libraries (src/)
- **I/O & Format:** x3f_io.*, x3f_dump.*
- **Processing:** x3f_process.*, x3f_matrix.*, x3f_image.*
- **Output:** x3f_output_*.*, x3f_histogram.*
- **Analysis:** x3f_meta.*, x3f_spatial_gain.*
- **Enhancement:** x3f_denoise*.*, x3f_printf.*
- **Applications:** x3f_extract.c (main tool), x3f_*_test.c (testing)

### Configuration & Build
- **Makefiles:** makefile, sys.mk, src/makefile
- **Toolchains:** *.cmake files
- **Scripts:** install_opencv.sh, create_universal_libs.sh

### Documentation
- **doc/:** README files, copyright, changelog, licensing
- **README.md:** Quick start guide
- **Inline comments:** Comprehensive code documentation

### Testing & Validation
- **features/:** BDD test specifications
- **x3f_test_files/:** Sample X3F images and expected outputs
- **test_*.sh:** Specific test scripts

---

## 12. Code Statistics

| Metric | Value |
|--------|-------|
| Total C/C++ Files | 45 |
| Total Lines of Code | ~8,500 |
| Largest Module | x3f_io.c (2,015 lines) |
| Largest Algorithm | x3f_denoise_aniso.cpp (13,443 lines, C++) |
| Build Configurations | 6+ (linux, windows-32/64, osx-universal/x86/i386) |
| Test Scenarios | 87+ |
| Supported Cameras | 15+ models across 3 sensor generations |

---

## 13. Development Workflow Notes

### Building for macOS Universal
```bash
./install_opencv.sh          # Download and build OpenCV for arm64 & x86_64
./create_universal_libs.sh   # Combine into universal binaries
make TARGET=osx-universal    # Build x3f_extract universal binary
```

### Expected Output Locations
- Binaries: `bin/osx-universal/`
- Libraries: `deps/lib/osx-universal/`

### Adding New Camera Support
1. Identify X3F format variant in file headers
2. Add structure definitions to x3f_io.h if needed
3. Implement parsing in x3f_io.c
4. Add processing logic in x3f_process.c if unique algorithm needed
5. Create test images and update features/*.feature
6. Add model identification in x3f_meta.c for proper metadata

---

## 14. Known Issues & Workarounds

### Firmware Bugs Handled
1. **DP2 DarkShieldBottom** - Incorrectly specified in firmware, skipped in code
2. **Merrill Right Column** - Bright "shielded" region causes color cast, disabled
3. **Strip on VirtualBox** - Known issue, build may need multiple runs in Vagrant

### Legacy Support
- Old cameras (SD14, SD9, SD10) use legacy offset handling
- Auto-detection available with `-offset` flag for manual override

### Compression Bugs
- Recently fixed (commit 817b8c3) - ensure up-to-date build

---

## 15. Additional Resources

### Documentation Files
- `/Users/sang/Developer/x3f/doc/README-OLD.txt` - Original documentation
- `/Users/sang/Developer/x3f/doc/readme.txt` - Current readme
- `/Users/sang/Developer/x3f/doc/copyright.txt` - BSD license
- `/Users/sang/Developer/x3f/doc/changes.txt` - Version history

### External References
- Sigma Camera X3F Format Documentation
- OpenCV Library Documentation
- DNG (Digital Negative) Format Specification
- TIFF Format Specification

---

This documentation provides AI assistants with comprehensive context for understanding, modifying, extending, and debugging the X3F Tools project.
