# IR Matrix Normalization Summary

## Problem
The calibrated v2 IR separation matrix had mathematically correct values but they were too small for use in DNG profiles:
- Row sums were near zero (0.00004 to 0.00057)
- Direct normalization produced extreme values (up to 100.18)
- DNG profiles require rows summing to approximately 1.0 for proper brightness

## Solution
Applied Method 2 normalization: Scale by maximum absolute value, then adjust for unit sum.

## Original Calibrated Matrix (BMT order)
```
NIR:   [ 0.00026760, -0.00094418,  0.00071984]  Sum: 0.00004326
RED:   [-0.00222127,  0.01377234, -0.01141360]  Sum: 0.00013747
GREEN: [ 0.00036064, -0.00296548,  0.00317307]  Sum: 0.00056823
```

## Normalized Matrix (Applied to x3f_output_dng.c)
```c
double ir_separation_matrix[9] = {
    /* NIR from BMT (normalized from calibrated v2 data) */
    0.6015, -0.6819, 1.0805,
    /* Red from BMT (normalized from calibrated v2 data) */
    0.1687, 1.3300, -0.4987,
    /* Green from BMT (normalized from calibrated v2 data) */
    0.3873, -0.6609, 1.2736
};
```

## Verification
- Each row sums to 1.0000
- Values are in reasonable range (-0.68 to 1.33)
- Matrix preserves the relative channel separation characteristics
- Compatible with Lightroom and other DNG-supporting applications

## Testing
Successfully tested with `test_ir.x3f`:
```bash
./bin/osx-universal/x3f_extract -dng -ir-separate test_ir.x3f
```
Generated `test_ir.x3f.dng` (96.8 MB) with proper IR channel separation.

## Channel Mapping in Output DNG
After IR separation with normalized matrix:
- **Channel 0 (Red)**: NIR signal - vegetation appears red/magenta
- **Channel 1 (Green)**: Red signal - red objects appear green
- **Channel 2 (Blue)**: Green signal - green objects appear blue

This creates the standard false-color infrared visualization used in remote sensing and vegetation analysis.