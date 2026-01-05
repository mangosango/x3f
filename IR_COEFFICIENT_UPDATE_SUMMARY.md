# IR Coefficient Update Summary

## What Was Updated

### 1. `x3f_output_dng.c` - IR Separation Matrix Updated ✓

**Location:** [src/x3f_output_dng.c:183-190](src/x3f_output_dng.c#L183-L190)

**Previous Values (placeholder):**
```c
double ir_separation_matrix[9] = {
    1.3, -0.2, -0.1,   /* NIR from BMT */
    0.5,  0.6, -0.1,   /* Red from BMT */
   -0.2,  0.5,  0.7    /* Green from BMT */
};
```

**Updated Values (calibrated):**
```c
double ir_separation_matrix[9] = {
    /* NIR from BMT (using IR row with B,M,T column order) */
     0.00026760, -0.00094418,  0.00071984,
    /* Red from BMT (using RED row with B,M,T column order) */
    -0.00222127,  0.01377234, -0.01141360,
    /* Green from BMT (using GREEN row with B,M,T column order) */
     0.00036064, -0.00296548,  0.00317307
};
```

### 2. Header File Inclusion Added ✓

**Location:** [src/x3f_output_dng.c:17](src/x3f_output_dng.c#L17)

Added `#include "x3f_ir_coefficients.h"` to make the calibrated coefficients available throughout the file.

## Calibration Source

The coefficients were derived from actual calibration targets:
- **BLACK.X3F**: Dark reference (T=109.20, M=111.02, B=111.78)
- **GREEN.X3F**: Green target (T=1643.93, M=1517.34, B=945.32)
- **RED.X3F**: Red target (T=547.00, M=680.68, B=944.04)
- **NIR.X3F**: Near-infrared target (T=1674.92, M=2944.32, B=9633.65)

The NIR target showed the expected strong bottom layer response (6.08x higher than top layer), confirming the Foveon sensor's IR sensitivity pattern.

## Matrix Transformation Details

The calibrated matrix was originally in **TMB → RGB+IR** format (Top, Middle, Bottom sensor layers to Red, Green, IR channels).

For `x3f_output_dng.c`, which expects **BMT** input order (Bottom, Middle, Top), the matrix columns were reordered:
- Column 0 and 2 were swapped to convert from TMB to BMT ordering
- Rows were reordered for NIR, Red, Green output sequence

## Testing Completed

Successfully tested with calibration targets:
- ✓ IR-separated DNG files created for all targets
- ✓ IR separation mode properly applies the calibrated coefficients
- ✓ Normal mode (without IR separation) still works correctly

## Test Files Generated

Location: `_local/ir_test_output/`
- `green_ir_separated.dng` - Green target with IR separation
- `green_normal.dng` - Green target without IR separation (for comparison)
- `red_ir_separated.dng` - Red target with IR separation
- `nir_ir_separated.dng` - NIR target with IR separation (should appear red)

## How to Verify

1. Open the generated DNG files in an image editor that supports DNG format
2. In IR-separated mode:
   - **NIR target** should appear predominantly **red** (IR mapped to red channel)
   - **RED target** should appear predominantly **green** (red mapped to green channel)
   - **GREEN target** should appear predominantly **blue** (green mapped to blue channel)
3. Compare with normal (non-IR-separated) versions to see the channel remapping effect

## Build Command

The updated code compiles successfully with:
```bash
make TARGET=osx-universal
```

## Usage

To use IR separation with the calibrated coefficients:
```bash
./bin/osx-universal/x3f_extract -dng -ir-separate input.x3f
```

The IR-Separated camera profile will be automatically selected in the DNG metadata when using `-ir-separate` flag.