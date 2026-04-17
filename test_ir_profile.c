#include <stdio.h>
#include <math.h>

// Matrix multiplication for 3x3 matrices
void matrix_mul_3x3(double *a, double *b, double *result) {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            result[i*3+j] = 0;
            for (int k = 0; k < 3; k++) {
                result[i*3+j] += a[i*3+k] * b[k*3+j];
            }
        }
    }
}

void print_matrix(const char *name, double *mat) {
    printf("%s:\n", name);
    for (int i = 0; i < 3; i++) {
        printf("  [%8.5f, %8.5f, %8.5f]\n", mat[i*3], mat[i*3+1], mat[i*3+2]);
    }
    printf("\n");
}

double matrix_magnitude(double *mat) {
    double sum = 0;
    for (int i = 0; i < 9; i++) {
        sum += mat[i] * mat[i];
    }
    return sqrt(sum);
}

int main() {
    printf("=== Testing IR Separation Matrix Values ===\n\n");

    // Original placeholder values
    double ir_separation_original[9] = {
        1.3, -0.2, -0.1,   /* NIR from BMT */
        0.5,  0.6, -0.1,   /* Red from BMT */
       -0.2,  0.5,  0.7    /* Green from BMT */
    };

    // New calibrated values
    double ir_separation_calibrated[9] = {
        0.00026760, -0.00094418,  0.00071984,  /* NIR from BMT */
       -0.00222127,  0.01377234, -0.01141360,  /* Red from BMT */
        0.00036064, -0.00296548,  0.00317307   /* Green from BMT */
    };

    // False color remap matrix
    double false_color_remap[9] = {
        1.0, 0.0, 0.0,   /* NIR -> Red display */
        0.0, 1.0, 0.0,   /* Red -> Green display */
        0.0, 0.0, 1.0    /* Green -> Blue display */
    };

    // Adobe RGB to XYZ (approximate values)
    double adobe_to_xyz[9] = {
        0.5767309,  0.1855540, 0.1881852,
        0.2973769,  0.6273491, 0.0752741,
        0.0270343,  0.0706872, 0.9911085
    };

    printf("1. Matrix Magnitudes:\n");
    printf("   Original matrix magnitude: %.5f\n", matrix_magnitude(ir_separation_original));
    printf("   Calibrated matrix magnitude: %.5f\n", matrix_magnitude(ir_separation_calibrated));
    printf("   Ratio: %.2fx smaller\n\n", matrix_magnitude(ir_separation_original) / matrix_magnitude(ir_separation_calibrated));

    // Calculate final matrices for both
    double temp_original[9], final_original[9];
    double temp_calibrated[9], final_calibrated[9];

    matrix_mul_3x3(false_color_remap, ir_separation_original, temp_original);
    matrix_mul_3x3(adobe_to_xyz, temp_original, final_original);

    matrix_mul_3x3(false_color_remap, ir_separation_calibrated, temp_calibrated);
    matrix_mul_3x3(adobe_to_xyz, temp_calibrated, final_calibrated);

    print_matrix("2. Original Final Matrix (BMT->XYZ)", final_original);
    print_matrix("3. Calibrated Final Matrix (BMT->XYZ)", final_calibrated);

    printf("4. Final Matrix Magnitudes:\n");
    printf("   Original final magnitude: %.5f\n", matrix_magnitude(final_original));
    printf("   Calibrated final magnitude: %.5f\n", matrix_magnitude(final_calibrated));
    printf("   Ratio: %.2fx smaller\n\n", matrix_magnitude(final_original) / matrix_magnitude(final_calibrated));

    // Check if values are too small
    double max_val = 0;
    for (int i = 0; i < 9; i++) {
        double abs_val = fabs(final_calibrated[i]);
        if (abs_val > max_val) max_val = abs_val;
    }
    printf("5. Maximum absolute value in calibrated final matrix: %.8f\n", max_val);
    if (max_val < 0.01) {
        printf("   WARNING: Matrix values may be too small for DNG profile!\n");
        printf("   Typical values should be in range 0.1-2.0\n");
    }

    // Suggest scaling factor
    double suggested_scale = 1.0 / matrix_magnitude(ir_separation_calibrated);
    printf("\n6. Suggested scaling factor: %.2f\n", suggested_scale);
    printf("   This would bring the matrix to unit magnitude\n");

    return 0;
}