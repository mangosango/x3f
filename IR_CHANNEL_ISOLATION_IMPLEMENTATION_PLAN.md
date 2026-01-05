# Implementation Plan: Infrared/Red/Green Channel Isolation for x3f_extract

## Executive Summary

This document provides a detailed implementation plan for adding infrared (IR) and red/green channel isolation capabilities to the x3f_extract application. The goal is to support full-spectrum cameras with triple bandpass filters that pass only red, green, and infrared light, allowing separation of these channels from the Foveon sensor's three layers.

## Background: Understanding the Foveon X3F Sensor Architecture

### Sensor Layer Structure

The Foveon sensor has three stacked layers (referenced throughout the code as "BMT"):
- **B (Bottom)**: Blue-sensitive layer - BUT with IR filter removed, this captures Red + IR
- **M (Middle)**: Green-sensitive layer - captures Green
- **T (Top)**: Red-sensitive layer - BUT with IR filter removed, this captures mostly IR

Key constant in code: `TRUE_PLANES 3` (defined in x3f_io.h:202)
Comment in x3f_io.h:201: `/* 0=bottom, 1=middle, 2=top */`

### Current Processing Pipeline Flow

1. **x3f_extract.c:main()** - Entry point, command-line parsing
2. **x3f_load_data()** - Loads raw sensor data from X3F file
3. **x3f_image_area()** - Provides access to decoded raw 3-channel BMT data
4. **preprocess_data()** - Black level subtraction, scaling, bad pixel fixing
5. **convert_data()** - Applies color matrix (BMT → XYZ → RGB)
6. **x3f_dump_raw_data_as_dng()** - Writes DNG file with appropriate metadata

## Problem Statement

With a full-spectrum camera and triple bandpass filter (R+G+IR only):
- **Bottom layer**: Receives Red + IR (no blue due to filter)
- **Middle layer**: Receives Green only
- **Top layer**: Receives primarily IR (some red leakage)

We need to:
1. Separate Red and IR from the bottom layer
2. Extract clean Green from middle layer
3. Handle IR from top layer
4. Output these as isolated channels in a DNG file

## Detailed Implementation Plan

### Phase 1: Understanding Current Data Flow

#### 1.1 Raw Data Access Point
**Location**: `x3f_process.c:preprocess_data()` (line 656)

This is where we first access the raw BMT sensor data BEFORE color matrix application:

```c
static int preprocess_data(x3f_t *x3f, int fix_bad, char *wb, x3f_image_levels_t *ilevels)
{
  x3f_area16_t image, qtop;  // Raw sensor data structure
  int row, col, color;
  
  // Get raw BMT data
  if (!x3f_image_area(x3f, &image) || image.channels < 3) return 0;
  
  // image.data now contains raw BMT values as uint16_t
  // Layout: [B0, M0, T0, B1, M1, T1, B2, M2, T2, ...]
  // Access via: image.data[row*image.row_stride + col*image.channels + color]
  // where color: 0=Bottom(R+IR), 1=Middle(G), 2=Top(IR)
```

**Key Insight**: This is our interception point. We need to:
1. Access `image.data` after black level correction but before color matrix
2. Apply our IR separation algorithm here
3. Create new channel layout before passing to DNG output

#### 1.2 Current Color Transformation

**Location**: `x3f_process.c:x3f_get_bmt_to_xyz()` (line 274)

The standard BMT → XYZ transformation is computed here:
```c
/* extern */ int x3f_get_bmt_to_xyz(x3f_t *x3f, char *wb, double *bmt_to_xyz)
{
  // This gets the color correction matrix from camera metadata
  // For IR mode, we'll bypass this entirely
}
```

### Phase 2: New Command-Line Options

#### 2.1 Required Switches

Add to `x3f_extract.c:usage()` and `x3f_extract.c:main()`:

```c
// New output mode
typedef enum
  { META      = 0,
    JPEG      = 1,
    RAW       = 2,
    TIFF      = 3,
    DNG       = 4,
    PPMP3     = 5,
    PPMP6     = 6,
    HISTOGRAM = 7,
    DNG_IR    = 8}  // NEW: IR channel isolation mode
  output_file_type_t;

// New command-line switches
"   -dng-ir         Dump RAW as DNG with IR/R/G channel isolation\n"
"   -ir-coeff <R>,<IR>  IR separation coefficients (default: 0.5,0.5)\n"
"                       R_isolated = Bottom - (IR_coeff * Top)\n"
"                       IR_isolated = Top - (R_coeff * Bottom)\n"
```

#### 2.2 Parsing Logic

```c
// In main()
double ir_r_coeff = 0.5;  // Coefficient for R channel isolation
double ir_ir_coeff = 0.5; // Coefficient for IR channel isolation
int ir_mode = 0;

// Parse loop
else if (!strcmp(argv[i], "-dng-ir"))
  Z, extract_raw = 1, file_type = DNG_IR, ir_mode = 1;
else if ((!strcmp(argv[i], "-ir-coeff")) && (i+1)<argc) {
  char *coeff_str = argv[++i];
  if (sscanf(coeff_str, "%lf,%lf", &ir_r_coeff, &ir_ir_coeff) != 2) {
    fprintf(stderr, "Invalid IR coefficients: %s\n", coeff_str);
    usage(argv[0]);
  }
}
```

### Phase 3: IR Separation Algorithm Design

#### 3.1 Mathematical Model

Given:
- `B` = Bottom layer raw value (Red + IR)
- `M` = Middle layer raw value (Green)
- `T` = Top layer raw value (IR with red leakage)

Goal: Separate into:
- `R_isolated` = Pure red
- `G_isolated` = Pure green (already isolated)
- `IR_isolated` = Pure infrared

**Algorithm**:
```
R_isolated = B - (k_ir * T)
IR_isolated = T - (k_r * B)
G_isolated = M
```

Where:
- `k_ir` = IR contamination coefficient in bottom layer (default 0.5)
- `k_r` = Red leakage coefficient in top layer (default 0.1)

**Calibration**: These coefficients should be determined by:
1. Photographing a scene with known IR/R content
2. Analyzing the correlation between T and B layers
3. Adjusting until clean separation is achieved

#### 3.2 Implementation Location

Create new function in `x3f_process.c`:

```c
/**
 * Separate IR and Red channels from BMT sensor data
 * 
 * @param image Preprocessed BMT image data (in-place modification)
 * @param ir_r_coeff Coefficient for removing IR from bottom layer
 * @param ir_ir_coeff Coefficient for removing R from top layer
 * @return 1 on success, 0 on failure
 */
static int separate_ir_channels(x3f_area16_t *image, 
                                double ir_r_coeff, 
                                double ir_ir_coeff)
{
  int row, col;
  
  if (image->channels != 3) return 0;
  
  // Create temporary buffer for original values
  uint16_t *temp_bottom = malloc(image->rows * image->columns * sizeof(uint16_t));
  uint16_t *temp_top = malloc(image->rows * image->columns * sizeof(uint16_t));
  
  if (!temp_bottom || !temp_top) {
    free(temp_bottom);
    free(temp_top);
    return 0;
  }
  
  // Copy original bottom and top values
  for (row = 0; row < image->rows; row++) {
    for (col = 0; col < image->columns; col++) {
      int idx = row * image->row_stride + col * image->channels;
      temp_bottom[row * image->columns + col] = image->data[idx + 0];
      temp_top[row * image->columns + col] = image->data[idx + 2];
    }
  }
  
  // Apply separation algorithm
  for (row = 0; row < image->rows; row++) {
    for (col = 0; col < image->columns; col++) {
      int idx = row * image->row_stride + col * image->channels;
      int temp_idx = row * image->columns + col;
      
      uint16_t B = temp_bottom[temp_idx];
      uint16_t T = temp_top[temp_idx];
      
      // R_isolated = B - (k_ir * T)
      int32_t R_isolated = (int32_t)B - (int32_t)(ir_r_coeff * T);
      if (R_isolated < 0) R_isolated = 0;
      if (R_isolated > 65535) R_isolated = 65535;
      
      // IR_isolated = T - (k_r * B)  
      int32_t IR_isolated = (int32_t)T - (int32_t)(ir_ir_coeff * B);
      if (IR_isolated < 0) IR_isolated = 0;
      if (IR_isolated > 65535) IR_isolated = 65535;
      
      // Update image data: [R_isolated, G, IR_isolated]
      image->data[idx + 0] = (uint16_t)R_isolated;
      // image->data[idx + 1] = G (unchanged)
      image->data[idx + 2] = (uint16_t)IR_isolated;
    }
  }
  
  free(temp_bottom);
  free(temp_top);
  
  x3f_printf(DEBUG, "IR channel separation applied with coefficients: k_ir=%f, k_r=%f\n",
             ir_r_coeff, ir_ir_coeff);
  
  return 1;
}
```

#### 3.3 Integration Point

Modify `preprocess_data()` to call IR separation:

```c
static int preprocess_data(x3f_t *x3f, int fix_bad, char *wb, 
                          x3f_image_levels_t *ilevels,
                          int ir_mode, double ir_r_coeff, double ir_ir_coeff)
{
  // ... existing black level and scaling code ...
  
  if (fix_bad) interpolate_bad_pixels(x3f, &image, 3);
  
  // NEW: Apply IR separation if in IR mode
  if (ir_mode) {
    if (!separate_ir_channels(&image, ir_r_coeff, ir_ir_coeff)) {
      x3f_printf(ERR, "Could not separate IR channels\n");
      return 0;
    }
  }
  
  return 1;
}
```

### Phase 4: DNG Output Modifications

#### 4.1 New Camera Profile for IR Mode

Add to `x3f_output_dng.c`:

```c
static int get_bmt_to_xyz_ir(x3f_t *x3f, char *wb, double *bmt_to_xyz)
{
  // For IR mode, we use an identity-like matrix since channels are already separated
  // This assumes the DNG will be processed as "linear raw" without color conversion
  
  // Map R, G, IR to XYZ space (simplified)
  // This is a placeholder - proper calibration needed
  bmt_to_xyz[0] = 1.0; bmt_to_xyz[1] = 0.0; bmt_to_xyz[2] = 0.0;  // R -> X
  bmt_to_xyz[3] = 0.0; bmt_to_xyz[4] = 1.0; bmt_to_xyz[5] = 0.0;  // G -> Y
  bmt_to_xyz[6] = 0.0; bmt_to_xyz[7] = 0.0; bmt_to_xyz[8] = 0.3;  // IR -> Z (scaled)
  
  x3f_printf(INFO, "Using IR-separated channel mapping\n");
  return 1;
}

// Add to camera_profiles array:
static const camera_profile_t camera_profiles[] = {
  {"Default", x3f_get_bmt_to_xyz, NULL},
  {"FOV Classic Blue", get_bmt_to_xyz_fcblue, NULL},
  {"IR Separated R/G/IR", get_bmt_to_xyz_ir, NULL},  // NEW
  {"Grayscale", get_bmt_to_xyz_noconvert, grayscale_mix_std},
  {"Grayscale (red filter)", get_bmt_to_xyz_noconvert, grayscale_mix_red},
  {"Grayscale (blue filter)", get_bmt_to_xyz_noconvert, grayscale_mix_blue},
  {"Unconverted", get_bmt_to_xyz_noconvert, NULL},
};
```

#### 4.2 DNG Metadata Tags

In `x3f_dump_raw_data_as_dng()`, add IR-specific metadata:

```c
if (ir_mode) {
  // Override CFAPlaneColor to indicate non-standard channel layout
  uint8_t cfa_plane_color[3] = {0, 1, 5}; /* R=0, G=1, IR=5 (custom) */
  TIFFSetField(f_out, TIFFTAG_CFAPLANECOLOR, 3, cfa_plane_color);
  
  // Add custom metadata to indicate IR mode
  TIFFSetField(f_out, TIFFTAG_IMAGEDESCRIPTION, 
               "IR-separated channels: R, G, IR. Processed with x3f_extract -dng-ir");
  
  // Use IR camera profile
  // Modify profile selection to use "IR Separated R/G/IR" profile
}
```

### Phase 5: Parameter Passing and Plumbing

#### 5.1 Modified Function Signatures

Update these functions to pass IR mode parameters:

**x3f_process.h**:
```c
extern int x3f_get_image(x3f_t *x3f,
                        x3f_area16_t *image,
                        x3f_image_levels_t *ilevels,
                        x3f_color_encoding_t encoding,
                        int crop,
                        int fix_bad,
                        int denoise,
                        int apply_sgain,
                        char *wb,
                        int ir_mode,           // NEW
                        double ir_r_coeff,     // NEW
                        double ir_ir_coeff);   // NEW
```

**x3f_output_dng.h** (create/update):
```c
x3f_return_t x3f_dump_raw_data_as_dng(x3f_t *x3f,
                                     char *outfilename,
                                     int fix_bad,
                                     int denoise,
                                     int apply_sgain,
                                     char *wb,
                                     int compress,
                                     int ir_mode,        // NEW
                                     double ir_r_coeff,  // NEW
                                     double ir_ir_coeff); // NEW
```

#### 5.2 Call Chain Updates

Update all callers in the chain:

1. **x3f_extract.c:main()** → calls `x3f_dump_raw_data_as_dng()`
2. **x3f_output_dng.c:x3f_dump_raw_data_as_dng()** → calls `x3f_get_image()`
3. **x3f_process.c:x3f_get_image()** → calls `preprocess_data()`
4. **x3f_process.c:preprocess_data()** → calls `separate_ir_channels()`

### Phase 6: Calibration and Testing Strategy

#### 6.1 Calibration Process

Create a calibration utility or document the manual process:

1. **Capture test images**:
   - Pure IR scene (use 850nm IR LED panel)
   - Pure red scene (use 660nm deep red LED)
   - Pure green scene (use 530nm green LED)
   - Mixed natural scene

2. **Analyze channel correlations**:
   - Extract raw BMT values from each test image
   - Plot B vs T for IR scene (should show high correlation)
   - Plot B vs T for R scene (should show B >> T)
   - Calculate optimal `k_ir` and `k_r` coefficients

3. **Iterative refinement**:
   - Start with default values (0.5, 0.1)
   - Process test images
   - Adjust coefficients to minimize cross-contamination
   - Validate with natural scenes

#### 6.2 Validation Metrics

Implement validation functions to assess separation quality:

```c
// Measure channel correlation (should be low after separation)
double measure_channel_correlation(x3f_area16_t *image, int ch1, int ch2);

// Measure channel isolation (pure scenes should activate only one channel)
double measure_channel_isolation(x3f_area16_t *image, int expected_channel);
```

### Phase 7: Advanced Features (Future Enhancements)

#### 7.1 Auto-Calibration Mode

Add switch: `-ir-auto-calibrate <reference_images>`
- Analyze multiple reference images
- Compute optimal coefficients automatically
- Save to configuration file

#### 7.2 Per-Pixel Adaptive Separation

Instead of global coefficients, use spatially-varying separation:
```c
// Account for spectral variations across sensor
double k_ir_spatial[rows][cols];
double k_r_spatial[rows][cols];
```

#### 7.3 Multi-Channel DNG Output

Extend to 4-channel output: R, G, IR_near, IR_far
- Use top layer for IR_far
- Use bottom layer for R + IR_near
- Apply more sophisticated separation

## Files Requiring Modification

### Core Files (MUST MODIFY):

1. **x3f_extract.c**
   - Add new command-line options: `-dng-ir`, `-ir-coeff`
   - Add IR mode variables and parsing
   - Pass IR parameters to output functions
   - ~100 lines of changes

2. **x3f_process.c**
   - Add `separate_ir_channels()` function
   - Modify `preprocess_data()` signature and logic
   - Modify `x3f_get_image()` signature
   - ~150 lines of new code + ~50 lines of modifications

3. **x3f_process.h**
   - Update `x3f_get_image()` function declaration
   - ~5 lines of changes

4. **x3f_output_dng.c**
   - Add `get_bmt_to_xyz_ir()` function
   - Update camera profiles array
   - Modify `x3f_dump_raw_data_as_dng()` signature and logic
   - Add IR-specific DNG metadata
   - ~100 lines of new code + ~50 lines of modifications

5. **x3f_output_dng.h** (may need to create)
   - Update function declaration
   - ~5 lines of changes

### Supporting Files (REVIEW ONLY):

6. **x3f_io.h** - Review data structures (no changes needed)
7. **x3f_image.c** - Review data access (no changes needed)
8. **x3f_meta.c** - Review metadata access (no changes needed)

## Implementation Phases

### Phase 1: Basic Infrastructure (Week 1)
- Add command-line parsing for IR mode
- Add parameter passing through call chain
- Test with dummy IR separation (identity function)

### Phase 2: Core Algorithm (Week 2)
- Implement `separate_ir_channels()` function
- Test with simple fixed coefficients
- Validate data flow and channel mapping

### Phase 3: DNG Output (Week 3)
- Add IR camera profile
- Update DNG metadata
- Test with Lightroom/RawTherapee

### Phase 4: Calibration & Refinement (Week 4)
- Capture calibration images
- Refine coefficients
- Document calibration process

### Phase 5: Testing & Documentation (Week 5)
- Comprehensive testing with various scenes
- Performance optimization
- User documentation
- Code comments

## Potential Issues and Solutions

### Issue 1: Negative Values After Separation
**Problem**: R_isolated = B - k_ir*T could go negative
**Solution**: Clamp to 0 (already in implementation)

### Issue 2: Noise Amplification
**Problem**: Subtraction can amplify noise
**Solution**: 
- Apply stronger denoising in IR mode
- Consider bilateral filtering on separated channels

### Issue 3: Coefficient Sensitivity
**Problem**: Wrong coefficients cause poor separation
**Solution**:
- Provide calibration guide
- Add validation warnings
- Consider auto-calibration mode

### Issue 4: DNG Compatibility
**Problem**: Non-standard channel layout may confuse software
**Solution**:
- Use standard RGB tags but document mapping
- Provide custom ICC profile
- Test with multiple DNG processors

## Testing Checklist

- [ ] Compile with new flags
- [ ] Parse IR mode command-line correctly
- [ ] Access raw BMT data correctly
- [ ] Apply IR separation without crashes
- [ ] Generate valid DNG files
- [ ] DNG files open in Lightroom/RawTherapee
- [ ] Channels appear correctly separated
- [ ] Green channel is clean
- [ ] Red channel has no IR contamination
- [ ] IR channel is isolated
- [ ] White balance still works
- [ ] Metadata is correct
- [ ] Performance is acceptable (<2x slowdown)

## Alternative Approaches Considered

### Approach 1: Post-Processing Script
**Pros**: No C code changes
**Cons**: Limited access to raw data, slower
**Verdict**: Rejected - need direct access to BMT layers

### Approach 2: Separate Utility
**Pros**: Cleaner separation of concerns
**Cons**: Duplicated code, harder to maintain
**Verdict**: Rejected - better to integrate into main tool

### Approach 3: Plugin Architecture
**Pros**: Very extensible
**Cons**: Over-engineered for single feature
**Verdict**: Deferred - good future enhancement

## Success Criteria

1. **Functional**: Successfully separates R, G, IR channels from full-spectrum X3F files
2. **Quality**: Minimal cross-contamination between channels (<5% measured by correlation)
3. **Usable**: Simple command-line interface with sensible defaults
4. **Compatible**: Produces standard-compliant DNG files
5. **Performant**: <50% performance overhead vs. normal DNG conversion
6. **Maintainable**: Clean code with documentation
7. **Calibratable**: Users can adjust coefficients for their specific filter setup

## Conclusion

This implementation plan provides a comprehensive roadmap for adding IR channel isolation to x3f_extract. The approach:

- Leverages existing preprocessing pipeline
- Minimizes code changes (estimated ~400 new lines, ~150 modified lines)
- Maintains backward compatibility
- Provides clear extension points for future enhancements
- Follows existing code style and patterns

The key insight is intercepting the raw BMT data after preprocessing but before color conversion, applying a simple but effective mathematical separation, and properly labeling the output channels in the DNG metadata.
