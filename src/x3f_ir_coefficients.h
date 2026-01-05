/*
 * x3f_ir_coefficients.h
 *
 * IR separation transformation coefficients
 * Generated from calibration data on 2025-11-13
 *
 * Calibration samples:
 * - BLACK.X3F (dark reference: T=109.20, M=111.02, B=111.78)
 * - GREEN.X3F (green target: T=1643.93, M=1517.34, B=945.32)
 * - RED.X3F (red target: T=547.00, M=680.68, B=944.04)
 * - NIR.X3F (near-infrared target: T=1674.92, M=2944.32, B=9633.65)
 *
 * Sensor IR sensitivity analysis:
 * - Top layer: 1.00x (baseline)
 * - Middle layer: 1.81x
 * - Bottom layer: 6.08x (highest IR sensitivity)
 */

#ifndef X3F_IR_COEFFICIENTS_H
#define X3F_IR_COEFFICIENTS_H

/*
 * IR Separation Matrix
 * Transforms TMB (Top/Middle/Bottom) sensor values to RGB+IR channels
 * Matrix condition number: 189.69 (fair conditioning)
 * Verified to correctly separate pure R, G, and IR inputs
 */
static const double ir_separation_matrix[3][3] = {
    { -0.01141360, 0.01377234, -0.00222127 },  /* RED coefficients */
    { 0.00317307, -0.00296548, 0.00036064 },   /* GREEN coefficients */
    { 0.00071984, -0.00094418, 0.00026760 }    /* IR coefficients */
};

/*
 * Simplified IR Extraction Coefficients
 * Based on actual calibration data
 * These provide direct IR extraction without full matrix multiplication
 */
typedef struct {
    double top_coeff;
    double middle_coeff;
    double bottom_coeff;
} ir_coefficients_t;

/* Extract IR signal from TMB values */
static const ir_coefficients_t ir_extract_coeffs = {
    .top_coeff = 0.00072,
    .middle_coeff = -0.00094,
    .bottom_coeff = 0.00027
};

/* Extract visible signal (average of R and G) from TMB values */
static const ir_coefficients_t visible_extract_coeffs = {
    .top_coeff = -0.00412,
    .middle_coeff = 0.00540,
    .bottom_coeff = -0.00093
};

/*
 * Helper function to apply IR extraction
 * Returns isolated IR signal from TMB values
 */
static inline double extract_ir_signal(double top, double middle, double bottom) {
    return ir_extract_coeffs.top_coeff * top +
           ir_extract_coeffs.middle_coeff * middle +
           ir_extract_coeffs.bottom_coeff * bottom;
}

/*
 * Helper function to extract visible signal
 * Returns visible signal (average of R and G) from TMB values
 */
static inline double extract_visible_signal(double top, double middle, double bottom) {
    return visible_extract_coeffs.top_coeff * top +
           visible_extract_coeffs.middle_coeff * middle +
           visible_extract_coeffs.bottom_coeff * bottom;
}

#endif /* X3F_IR_COEFFICIENTS_H */