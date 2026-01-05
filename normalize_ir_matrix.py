#!/usr/bin/env python3
"""
Normalize IR separation matrix so each row sums to approximately 1.0
This is required for compatibility with DNG profiles in Lightroom and other applications
"""

import numpy as np

# Original calibrated v2 data (BMT column order)
original_matrix = np.array([
    # NIR from BMT
    [0.00026760, -0.00094418, 0.00071984],
    # Red from BMT
    [-0.00222127, 0.01377234, -0.01141360],
    # Green from BMT
    [0.00036064, -0.00296548, 0.00317307]
])

print("=== Original Calibrated Matrix (BMT order) ===")
for i, channel in enumerate(['NIR', 'RED', 'GREEN']):
    row = original_matrix[i]
    row_sum = np.sum(row)
    print(f"{channel:5s}: [{row[0]:11.8f}, {row[1]:11.8f}, {row[2]:11.8f}]  Sum: {row_sum:.8f}")
print()

# Method 1: Scale to preserve relative magnitudes while summing to 1
# Find the scaling factor that makes the dominant coefficients reasonable
print("=== Method 1: Proportional Scaling to Sum=1 ===")
print("Note: This may produce extreme values due to near-zero sums")

for i, channel in enumerate(['NIR', 'RED', 'GREEN']):
    row = original_matrix[i]
    row_sum = np.sum(row)
    if abs(row_sum) > 1e-10:  # Avoid division by near-zero
        normalized = row / row_sum
        print(f"{channel:5s}: [{normalized[0]:8.4f}, {normalized[1]:8.4f}, {normalized[2]:8.4f}]  Sum: {np.sum(normalized):.4f}")
    else:
        print(f"{channel:5s}: Cannot normalize - sum is too close to zero ({row_sum:.8f})")
print()

# Method 2: Scale based on maximum absolute value, then adjust for sum
print("=== Method 2: Scale by Max Absolute Value ===")

normalized_matrix = np.zeros_like(original_matrix)

for i, channel in enumerate(['NIR', 'RED', 'GREEN']):
    row = original_matrix[i]

    # Find the maximum absolute value in the row
    max_abs = np.max(np.abs(row))

    # Scale so max absolute value is 1.0
    scaled = row / max_abs

    # Now adjust to sum to 1.0 by adding a constant to all elements
    current_sum = np.sum(scaled)
    adjustment = (1.0 - current_sum) / 3.0
    normalized = scaled + adjustment

    normalized_matrix[i] = normalized

    print(f"{channel:5s}: [{normalized[0]:8.4f}, {normalized[1]:8.4f}, {normalized[2]:8.4f}]  Sum: {np.sum(normalized):.4f}")
print()

# Method 3: Use the pattern from the K matrix (more reasonable approach)
# Scale to maintain relative proportions but with reasonable values
print("=== Method 3: Practical Normalization ===")
print("Scale to maintain channel separation while ensuring reasonable values")

# Calculate scaling to bring values to a reasonable range
# We want the dominant coefficient in each row to be around 0.5-1.5

practical_matrix = np.zeros_like(original_matrix)

for i, channel in enumerate(['NIR', 'RED', 'GREEN']):
    row = original_matrix[i]

    # Find the dominant (largest absolute value) coefficient
    abs_row = np.abs(row)
    max_idx = np.argmax(abs_row)
    max_val = row[max_idx]

    # Scale so the dominant value is around 1.0 in magnitude
    if abs(max_val) > 1e-10:
        scale_factor = 1.0 / abs(max_val)
        scaled = row * scale_factor

        # Adjust to sum to 1.0
        current_sum = np.sum(scaled)
        if current_sum != 0:
            # Scale to get sum = 1.0
            final_scaled = scaled / current_sum
        else:
            # If sum is zero, add constant to make sum = 1.0
            final_scaled = scaled + (1.0 / 3.0)

        practical_matrix[i] = final_scaled

        print(f"{channel:5s}: [{final_scaled[0]:8.4f}, {final_scaled[1]:8.4f}, {final_scaled[2]:8.4f}]  Sum: {np.sum(final_scaled):.4f}")
print()

# Generate C code for the most reasonable approach (Method 3)
print("=== C Code Format (Normalized for DNG Profile) ===")
print("double ir_separation_matrix[9] = {")
print("    /* NIR from BMT (normalized) */")
print(f"    {practical_matrix[0,0]:8.4f}, {practical_matrix[0,1]:8.4f}, {practical_matrix[0,2]:8.4f},")
print("    /* Red from BMT (normalized) */")
print(f"    {practical_matrix[1,0]:8.4f}, {practical_matrix[1,1]:8.4f}, {practical_matrix[1,2]:8.4f},")
print("    /* Green from BMT (normalized) */")
print(f"    {practical_matrix[2,0]:8.4f}, {practical_matrix[2,1]:8.4f}, {practical_matrix[2,2]:8.4f}")
print("};")
print()

# Verification
print("=== Verification of Normalized Matrix ===")
for i, channel in enumerate(['NIR', 'RED', 'GREEN']):
    row = practical_matrix[i]
    row_sum = np.sum(row)
    max_abs = np.max(np.abs(row))
    print(f"{channel}: Sum={row_sum:.4f}, Max|value|={max_abs:.4f}")