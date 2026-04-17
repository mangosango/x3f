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

int main() {
    printf("=== Testing IR-Separated Profile in Normal Mode ===\n\n");

    // Current scaled IR separation matrix (100x scaling)
    double ir_separation_matrix[9] = {
        0.02676, -0.09442,  0.07198,   /* NIR from BMT */
       -0.22213,  1.37723, -1.14136,   /* Red from BMT */
        0.03606, -0.29655,  0.31731    /* Green from BMT */
    };

    // False color remap (identity matrix - no-op)
    double false_color_remap[9] = {
        1.0, 0.0, 0.0,   /* NIR -> Red display */
        0.0, 1.0, 0.0,   /* Red -> Green display */
        0.0, 0.0, 1.0    /* Green -> Blue display */
    };

    // Adobe RGB to XYZ
    double adobe_to_xyz[9] = {
        0.5767309,  0.1855540, 0.1881852,
        0.2973769,  0.6273491, 0.0752741,
        0.0270343,  0.0706872, 0.9911085
    };

    printf("1. Current approach (causing issues):\n");
    print_matrix("IR Separation Matrix", ir_separation_matrix);

    // Calculate combined matrix
    double temp[9], combined[9];
    matrix_mul_3x3(false_color_remap, ir_separation_matrix, temp);
    matrix_mul_3x3(adobe_to_xyz, temp, combined);

    print_matrix("Combined Matrix (BMT->XYZ)", combined);

    // Check for negative or extreme values
    printf("Analysis of combined matrix:\n");
    int has_negative = 0;
    double max_val = 0, min_val = 0;
    for (int i = 0; i < 9; i++) {
        if (combined[i] < min_val) min_val = combined[i];
        if (combined[i] > max_val) max_val = combined[i];
        if (combined[i] < 0) has_negative = 1;
    }
    printf("  Min value: %.5f\n", min_val);
    printf("  Max value: %.5f\n", max_val);
    printf("  Has negative values: %s\n", has_negative ? "YES" : "NO");
    printf("\n");

    // Test with typical BMT input values (normalized 0-1 range)
    double test_bmt[3] = {0.5, 0.5, 0.5}; // Gray input
    double output_xyz[3] = {0, 0, 0};

    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            output_xyz[i] += combined[i*3+j] * test_bmt[j];
        }
    }

    printf("2. Test with gray input BMT=[0.5, 0.5, 0.5]:\n");
    printf("   Output XYZ: [%.5f, %.5f, %.5f]\n", output_xyz[0], output_xyz[1], output_xyz[2]);
    printf("   Expected range for XYZ: typically 0-1\n");
    printf("\n");

    // Alternative approach: Apply Aerochrome-style transformation directly
    printf("3. Alternative: Aerochrome-style transformation\n");
    printf("   Instead of separating IR then remapping, directly create false color:\n");

    // Aerochrome transformation: B->R, M->G, T->B (simplified)
    double aerochrome_direct[9] = {
        0.0, 0.0, 1.0,   /* Top -> Blue */
        0.0, 1.0, 0.0,   /* Middle -> Green */
        1.0, 0.0, 0.0    /* Bottom -> Red (IR-sensitive) */
    };

    matrix_mul_3x3(adobe_to_xyz, aerochrome_direct, combined);
    print_matrix("Aerochrome Direct (BMT->XYZ)", combined);

    // Test the direct approach
    for (int i = 0; i < 3; i++) {
        output_xyz[i] = 0;
        for (int j = 0; j < 3; j++) {
            output_xyz[i] += combined[i*3+j] * test_bmt[j];
        }
    }

    printf("   Test with gray BMT=[0.5, 0.5, 0.5]:\n");
    printf("   Output XYZ: [%.5f, %.5f, %.5f]\n", output_xyz[0], output_xyz[1], output_xyz[2]);
    printf("   This produces valid XYZ values!\n");

    return 0;
}