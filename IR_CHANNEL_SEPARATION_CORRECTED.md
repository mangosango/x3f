# Corrected IR/Red/Green Channel Separation Implementation Plan

## Background: Foveon X3 Sensor with Triple Bandpass Filter

### Sensor Layer Physics
The Foveon X3 sensor uses silicon's natural absorption properties:
- Shorter wavelengths are absorbed near the surface
- Longer wavelengths penetrate deeper into silicon

### Your Setup: Full Spectrum + Triple Bandpass Filter
- **Filter passes**: Red (~650nm), Green (~550nm), Infrared (~850nm)
- **Filter blocks**: Blue, UV, and other wavelengths

### Actual Layer Response with Your Filter
```
Top Layer (T)    = 0.70×G + 0.25×R + 0.05×IR  (shallow, mostly green)
Middle Layer (M) = 0.45×G + 0.45×R + 0.10×IR  (balanced G/R)
Bottom Layer (B) = 0.10×G + 0.40×R + 0.50×IR  (deep, R+IR dominant)
```

## Mathematical Model for Channel Separation

### System of Equations
```
[T]   [k_TG  k_TR  k_TIR] [G]
[M] = [k_MG  k_MR  k_MIR] [R]
[B]   [k_BG  k_BR  k_BIR] [IR]
```

Where:
- T, M, B = Raw sensor layer values (measured)
- G, R, IR = Isolated channel values (to be calculated)
- k_xy = Spectral response coefficients (need calibration)

### Solution via Matrix Inversion
```
[G]        [T]
[R] = K⁻¹ × [M]
[IR]       [B]
```

### Default Coefficient Matrix
Based on typical silicon spectral response:
```c
double K[3][3] = {
    {0.70, 0.25, 0.05},  // Top layer coefficients
    {0.45, 0.45, 0.10},  // Middle layer coefficients
    {0.10, 0.40, 0.50}   // Bottom layer coefficients
};
```

## Implementation Plan

### Phase 1: Command-Line Interface
Add new options to x3f_extract.c:

```c
// Command line options
int ir_separation_mode = 0;
double ir_coeff_matrix[9] = {
    0.70, 0.25, 0.05,
    0.45, 0.45, 0.10,
    0.10, 0.40, 0.50
};
int ir_calibration_mode = 0;

// Parse options
case 'i':
    if (strcmp(optarg, "r-separate") == 0) {
        ir_separation_mode = 1;
    } else if (strcmp(optarg, "r-calibrate") == 0) {
        ir_calibration_mode = 1;
    }
    break;
case 'k':  // --ir-matrix
    // Parse 9 comma-separated values for custom matrix
    sscanf(optarg, "%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf,%lf",
           &ir_coeff_matrix[0], &ir_coeff_matrix[1], ...);
    break;
```

### Phase 2: Core Separation Algorithm
Add to x3f_process.c:

```c
// Matrix inversion using Gauss-Jordan elimination
void matrix_inverse_3x3(double *m, double *inv) {
    double det;

    // Calculate determinant
    det = m[0] * (m[4]*m[8] - m[5]*m[7]) -
          m[1] * (m[3]*m[8] - m[5]*m[6]) +
          m[2] * (m[3]*m[7] - m[4]*m[6]);

    if (fabs(det) < 1e-10) {
        x3f_printf(ERR, "Matrix is singular, cannot invert\n");
        return;
    }

    // Calculate cofactor matrix and transpose
    inv[0] = (m[4]*m[8] - m[5]*m[7]) / det;
    inv[1] = (m[2]*m[7] - m[1]*m[8]) / det;
    inv[2] = (m[1]*m[5] - m[2]*m[4]) / det;
    inv[3] = (m[5]*m[6] - m[3]*m[8]) / det;
    inv[4] = (m[0]*m[8] - m[2]*m[6]) / det;
    inv[5] = (m[2]*m[3] - m[0]*m[5]) / det;
    inv[6] = (m[3]*m[7] - m[4]*m[6]) / det;
    inv[7] = (m[1]*m[6] - m[0]*m[7]) / det;
    inv[8] = (m[0]*m[4] - m[1]*m[3]) / det;
}

// Channel separation function
void separate_ir_channels(x3f_image_t *image, double *coeff_matrix) {
    uint16_t *data = image->data;
    int width = image->width;
    int height = image->height;
    double K_inv[9];

    // Invert the coefficient matrix
    matrix_inverse_3x3(coeff_matrix, K_inv);

    // Process each pixel
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 3;

            // Get raw BMT values
            double T = (double)data[idx + 2];  // Top layer
            double M = (double)data[idx + 1];  // Middle layer
            double B = (double)data[idx + 0];  // Bottom layer

            // Apply inverse matrix to get G, R, IR
            double G  = K_inv[0]*T + K_inv[1]*M + K_inv[2]*B;
            double R  = K_inv[3]*T + K_inv[4]*M + K_inv[5]*B;
            double IR = K_inv[6]*T + K_inv[7]*M + K_inv[8]*B;

            // Clamp values to valid range
            G  = fmax(0, fmin(65535, G));
            R  = fmax(0, fmin(65535, R));
            IR = fmax(0, fmin(65535, IR));

            // Store separated channels back
            data[idx + 0] = (uint16_t)R;   // Red channel
            data[idx + 1] = (uint16_t)G;   // Green channel
            data[idx + 2] = (uint16_t)IR;  // IR channel
        }
    }
}
```

### Phase 3: Integration Point
Modify preprocess_data() in x3f_process.c:

```c
static void preprocess_data(x3f_t *x3f, x3f_area16_t *image,
                           int fix_bad, int denoise,
                           char *wb, int ir_separation_mode,
                           double *ir_coeff_matrix) {

    // Existing preprocessing...
    if (fix_bad) fix_bad_pixels(x3f, image, denoise);

    // NEW: Apply IR separation if enabled
    if (ir_separation_mode) {
        x3f_printf(INFO, "Applying IR channel separation...\n");
        separate_ir_channels(image, ir_coeff_matrix);

        // Skip standard color matrix application
        return;
    }

    // Continue with standard processing if not in IR mode...
}
```

### Phase 4: DNG Output Modifications
Update x3f_output_dng.c:

```c
// Add new profile for IR-separated mode
static profile_t profiles[] = {
    {"Default", x3f_get_bmt_to_xyz, NULL},
    {"IR-Separated", get_identity_matrix, NULL},  // Identity matrix for separated channels
    // ...
};

// Modify metadata tags
if (ir_separation_mode) {
    // Custom tags for IR mode
    TIFFSetField(tiff, TIFFTAG_SOFTWARE, "x3f_extract IR-separation mode");
    TIFFSetField(tiff, TIFFTAG_IMAGEDESCRIPTION,
                 "Ch0=Red, Ch1=Green, Ch2=Infrared (separated from BMT layers)");

    // Use identity matrix since channels are already separated
    float identity[9] = {1,0,0, 0,1,0, 0,0,1};
    TIFFSetField(tiff, TIFFTAG_COLORMATRIX1, 9, identity);
    TIFFSetField(tiff, TIFFTAG_FORWARDMATRIX1, 9, identity);
}
```

### Phase 5: Calibration Mode
For determining optimal coefficients:

```c
void output_calibration_data(x3f_image_t *image, const char *filename) {
    FILE *f = fopen(filename, "w");
    fprintf(f, "# Calibration data: T,M,B values for coefficient determination\n");
    fprintf(f, "# Capture images of pure R, G, and IR targets\n");

    // Output average values for center region
    int cx = image->width / 2;
    int cy = image->height / 2;
    int sample_size = 100;

    double avg_T = 0, avg_M = 0, avg_B = 0;
    for (int y = cy-50; y < cy+50; y++) {
        for (int x = cx-50; x < cx+50; x++) {
            int idx = (y * image->width + x) * 3;
            avg_T += image->data[idx + 2];
            avg_M += image->data[idx + 1];
            avg_B += image->data[idx + 0];
        }
    }

    avg_T /= (sample_size * sample_size);
    avg_M /= (sample_size * sample_size);
    avg_B /= (sample_size * sample_size);

    fprintf(f, "Average TMB: %.2f, %.2f, %.2f\n", avg_T, avg_M, avg_B);
    fclose(f);
}
```

## Calibration Procedure

### Step 1: Capture Calibration Images
1. Photograph a pure green target (550nm bandpass)
2. Photograph a pure red target (650nm bandpass)
3. Photograph a pure IR target (850nm LED or filter)

### Step 2: Extract TMB Values
```bash
x3f_extract -ir-calibrate green_target.x3f
x3f_extract -ir-calibrate red_target.x3f
x3f_extract -ir-calibrate ir_target.x3f
```

### Step 3: Calculate Coefficient Matrix
Given the TMB measurements for each pure target:
```
[T_green  T_red  T_ir  ]⁻¹   [1 0 0]
[M_green  M_red  M_ir  ]   ×  [0 1 0] = K
[B_green  B_red  B_ir  ]      [0 0 1]
```

### Step 4: Use Custom Matrix
```bash
x3f_extract -ir-separate --ir-matrix "0.72,0.23,0.05,0.47,0.43,0.10,0.08,0.42,0.50" image.x3f
```

## Testing Validation

1. **Synthetic Test**: Create test X3F with known R, G, IR values
2. **Spectral Test**: Use monochromatic light sources
3. **Real-world Test**: Photograph scenes with known IR reflectance
4. **Cross-validation**: Compare with dedicated IR camera results

## Benefits of This Approach

1. **Physically Accurate**: Based on actual silicon absorption physics
2. **Calibratable**: Can be tuned for specific filter characteristics
3. **Preserves Resolution**: No interpolation needed
4. **Flexible**: Matrix can be adjusted per camera/filter combination

## Potential Improvements

1. **Adaptive Coefficients**: Vary by ISO or exposure
2. **Noise Reduction**: Apply denoising before separation
3. **Advanced Calibration**: Use more calibration points for non-linear response
4. **Metadata Storage**: Save calibration matrix in DNG metadata

## Files Modified Summary

1. **x3f_extract.c**: ~150 lines (command-line parsing)
2. **x3f_process.c**: ~250 lines (separation algorithm)
3. **x3f_process.h**: ~10 lines (function declarations)
4. **x3f_output_dng.c**: ~100 lines (DNG metadata)
5. **x3f_output_dng.h**: ~5 lines (profile addition)

Total: ~515 lines of code changes

## Next Steps

1. Implement command-line parsing
2. Add matrix inversion function
3. Implement channel separation
4. Test with synthetic data
5. Calibrate with real filter
6. Validate results