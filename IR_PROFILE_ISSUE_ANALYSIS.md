# IR-Separated Profile Issue Analysis

## Summary
The IR-Separated profile appears in Lightroom when using `-ir-separate` flag but not in normal mode due to matrix value issues.

## Root Cause
The calibrated coefficient matrix produces negative values when combined with Adobe RGB to XYZ transformation, which Lightroom rejects for camera profiles.

### Matrix Analysis
**Calibrated values (scaled 100x):**
- Produces combined BMT->XYZ matrix with values ranging from -0.67 to 0.81
- Contains significant negative values that are invalid for DNG camera profiles
- Lightroom expects positive or near-positive values in the 0-2 range

**Original placeholder values (reverted to):**
- 1.3, -0.2, -0.1 (NIR from BMT)
- 0.5, 0.6, -0.1 (Red from BMT)
- -0.2, 0.5, 0.7 (Green from BMT)
- These work because they produce a more balanced final matrix

## Why It Works with `-ir-separate` Flag
When using `-ir-separate`:
1. The data is already IR-separated by `x3f_process.c`
2. The profile function just applies Adobe RGB to XYZ conversion
3. No problematic matrix multiplication occurs

## Why It Fails in Normal Mode
Without `-ir-separate`:
1. The profile tries to apply IR separation through the color matrix
2. The calibrated values create negative results
3. Lightroom rejects or hides profiles with invalid matrix values

## Potential Solutions for Future

### Option 1: Different Matrices for Different Modes
- Use calibrated values when data is IR-separated
- Use working placeholder values for normal mode

### Option 2: Aerochrome-Style Transformation
Instead of true IR separation, use a simpler transformation:
```c
// Maps Bottom (IR-sensitive) -> Red, Middle -> Green, Top -> Blue
double aerochrome[9] = {
  0.9, 0.05, 0.05,  // Bottom -> Red
  0.05, 0.9, 0.05,  // Middle -> Green
  0.05, 0.05, 0.9   // Top -> Blue
};
```
This produces valid positive values for DNG profiles.

### Option 3: Adjusted Scaling
Find a scaling factor that keeps values in valid range while preserving calibration relationships. The 100x scaling may be too aggressive.

## Files Involved
- `src/x3f_output_dng.c` - Contains the IR-Separated profile implementation
- `src/x3f_ir_coefficients.h` - Has the calibrated coefficient values
- Line 172-180 in `x3f_output_dng.c` - The matrix that needs adjustment

## Current Status
Reverted to original working placeholder values so the IR-Separated profile works in both modes. The calibrated values are preserved in `x3f_ir_coefficients.h` for future refinement.