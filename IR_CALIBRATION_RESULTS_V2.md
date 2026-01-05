# IR Separation Calibration Results (Corrected)

## Calibration Data Summary

### Raw TMB Values from Calibration Targets

| Target | Top (T) | Middle (M) | Bottom (B) |
|--------|---------|------------|------------|
| BLACK  | 109.20  | 111.02     | 111.78     |
| GREEN  | 1643.93 | 1517.34    | 945.32     |
| RED    | 547.00  | 680.68     | 944.04     |
| NIR    | 1674.92 | 2944.32    | 9633.65    |

### Normalized Values (Black-Subtracted)

| Target | Top (T) | Middle (M) | Bottom (B) |
|--------|---------|------------|------------|
| GREEN  | 1534.73 | 1406.32    | 833.54     |
| RED    | 437.80  | 569.66     | 832.26     |
| NIR    | 1565.72 | 2833.30    | 9521.87    |

## IR Sensitivity Analysis

The NIR calibration data confirms the Foveon sensor's layered IR response:

- **Top layer**: 1.00x (baseline IR sensitivity)
- **Middle layer**: 1.81x IR sensitivity
- **Bottom layer**: 6.08x IR sensitivity (strongest IR response)

This validates the physical expectation that deeper sensor layers have higher IR sensitivity due to silicon's wavelength-dependent absorption.

## Calculated Coefficient Matrices

### IR Separation Matrix

Successfully calculated from actual calibration data with **matrix condition number: 189.69** (fair conditioning).

```c
// IR Separation Coefficient Matrix
// Transforms TMB sensor values to RGB+IR channels
static const double ir_separation_matrix[3][3] = {
    { -0.01141360, 0.01377234, -0.00222127 },  // RED coefficients
    { 0.00317307, -0.00296548, 0.00036064 },   // GREEN coefficients
    { 0.00071984, -0.00094418, 0.00026760 }    // IR coefficients
};
```

### Verification Results

The matrix correctly separates known inputs:
- Pure RED → Output: R=1.000, G=0.000, IR=0.000 ✓
- Pure GREEN → Output: R=0.000, G=1.000, IR=0.000 ✓
- Pure NIR → Output: R=0.000, G=0.000, IR=1.000 ✓

### Aerochrome False Color Transformation Matrix

For Aerochrome/Color Infrared film simulation:

```c
// Aerochrome transformation matrix for false color IR
static const double aerochrome_matrix[3][3] = {
    { 0.00071984, -0.00094418, 0.00026760 },   // IR → Red output
    { -0.01141360, 0.01377234, -0.00222127 },  // Red → Green output
    { 0.00317307, -0.00296548, 0.00036064 }    // Green → Blue output
};
```

## Simplified Extraction Coefficients

For direct IR and visible signal extraction without full matrix multiplication:

### IR Extraction
```c
// Extract IR signal from TMB layers
double ir_signal = 0.00072 * top - 0.00094 * middle + 0.00027 * bottom;
```

### Visible Signal Extraction
```c
// Extract visible signal (average of R and G)
double visible = -0.00412 * top + 0.00540 * middle - 0.00093 * bottom;
```

## Implementation Guide

### Using the Full Matrix

```c
#include "x3f_ir_coefficients.h"

void separate_ir_channels(double tmb[3], double rgb_ir[3]) {
    // Apply separation matrix
    for (int i = 0; i < 3; i++) {
        rgb_ir[i] = 0;
        for (int j = 0; j < 3; j++) {
            rgb_ir[i] += ir_separation_matrix[i][j] * tmb[j];
        }
    }
}

void apply_aerochrome(double tmb[3], double output[3]) {
    // Apply Aerochrome false color transformation
    for (int i = 0; i < 3; i++) {
        output[i] = 0;
        for (int j = 0; j < 3; j++) {
            output[i] += aerochrome_matrix[i][j] * tmb[j];
        }
    }
}
```

### Using Simplified Coefficients

```c
#include "x3f_ir_coefficients.h"

// Quick IR extraction
double ir = extract_ir_signal(top, middle, bottom);

// Quick visible extraction
double visible = extract_visible_signal(top, middle, bottom);
```

## Key Findings

1. **Calibration Success**: The corrected NIR target provides valid calibration data showing strong IR response

2. **Layer Response Pattern**: Confirms the expected Foveon behavior:
   - Bottom layer is 6x more sensitive to IR than top layer
   - Middle layer shows intermediate IR sensitivity

3. **Matrix Stability**: Condition number of 189.69 indicates fair numerical stability
   - Good enough for practical use
   - May benefit from regularization in extreme cases

4. **Coefficient Magnitudes**: The small coefficient values (0.0001-0.01 range) suggest:
   - High sensor response values in raw data
   - Need for proper scaling in implementation

## Testing Recommendations

1. **Vegetation Tests**:
   - Healthy vegetation should appear bright red in Aerochrome mode
   - Chlorophyll reflects strongly in NIR (700-900nm)

2. **False Color Validation**:
   - Water bodies: Should appear dark (low IR reflectance)
   - Vegetation: Bright red (high IR reflectance)
   - Soil/concrete: Moderate tones

3. **Fine-tuning**:
   - Monitor for color shifts or artifacts
   - Adjust scaling factors if output appears too saturated
   - Consider gamma correction for display

## Files Generated

- `/Users/sang/Developer/x3f/calculate_ir_coefficients_v2.py` - Updated calculation script
- `/Users/sang/Developer/x3f/src/x3f_ir_coefficients.h` - C header with corrected coefficients
- `/Users/sang/Developer/x3f/_local/calibration_output/*_calib_v2.txt` - Raw calibration data
- This document - Corrected calibration analysis and results

## Summary

✅ Successful IR calibration with valid NIR reference
✅ Coefficients calculated and verified mathematically
✅ Ready for integration into x3f_process.c
✅ Both full matrix and simplified extraction methods provided

The coefficients are now based on actual measured IR response from the Foveon sensor, providing accurate IR channel separation for Aerochrome simulation and other IR photography applications.