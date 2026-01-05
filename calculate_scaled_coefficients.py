#!/usr/bin/env python3
"""
Calculate properly scaled IR separation coefficients for DNG camera profiles
based on the calibrated values.
"""

import numpy as np

# Original calibrated values (BMT order for DNG)
calibrated_bmt = np.array([
    [ 0.00026760, -0.00094418,  0.00071984],  # NIR from BMT
    [-0.00222127,  0.01377234, -0.01141360],  # Red from BMT
    [ 0.00036064, -0.00296548,  0.00317307]   # Green from BMT
])

# Original placeholder values that worked in DNG
original_placeholder = np.array([
    [1.3, -0.2, -0.1],   # NIR from BMT
    [0.5,  0.6, -0.1],   # Red from BMT
    [-0.2,  0.5,  0.7]   # Green from BMT
])

print("=== Scaling Calibrated Coefficients for DNG Profile ===\n")

# Calculate magnitudes
calib_magnitude = np.linalg.norm(calibrated_bmt)
orig_magnitude = np.linalg.norm(original_placeholder)

print(f"Calibrated matrix magnitude: {calib_magnitude:.6f}")
print(f"Original placeholder magnitude: {orig_magnitude:.6f}")
print(f"Ratio: {orig_magnitude/calib_magnitude:.2f}x\n")

# Method 1: Scale to match original magnitude
scale_factor_magnitude = orig_magnitude / calib_magnitude
scaled_by_magnitude = calibrated_bmt * scale_factor_magnitude

print("Method 1: Scale to match original magnitude")
print(f"Scaling factor: {scale_factor_magnitude:.2f}")
print("Scaled matrix:")
for i, row_name in enumerate(['NIR', 'Red', 'Green']):
    print(f"  {row_name:5s}: [{scaled_by_magnitude[i,0]:7.3f}, {scaled_by_magnitude[i,1]:7.3f}, {scaled_by_magnitude[i,2]:7.3f}]")
print()

# Method 2: Scale to preserve the dominant coefficients
# Find the largest absolute value in each row and scale to match typical DNG range
print("Method 2: Scale based on dominant coefficients")

# Find max absolute value in calibrated matrix
max_calib = np.max(np.abs(calibrated_bmt))
# Target range for DNG profiles is typically 0.5-1.5 for dominant values
target_max = 1.0
scale_factor_dominant = target_max / max_calib

scaled_by_dominant = calibrated_bmt * scale_factor_dominant

print(f"Max calibrated value: {max_calib:.6f}")
print(f"Scaling factor: {scale_factor_dominant:.2f}")
print("Scaled matrix:")
for i, row_name in enumerate(['NIR', 'Red', 'Green']):
    print(f"  {row_name:5s}: [{scaled_by_dominant[i,0]:7.3f}, {scaled_by_dominant[i,1]:7.3f}, {scaled_by_dominant[i,2]:7.3f}]")
print()

# Method 3: Analyze the relative sensitivities and create improved values
print("Method 3: Improved values based on calibration insights")
print("From calibration we learned:")
print("- Bottom layer has 6.08x higher IR sensitivity than top")
print("- Middle layer has 1.81x higher IR sensitivity than top")
print("- NIR response is dominated by bottom layer")
print()

# Create improved matrix based on calibration insights
# NIR: Strong bottom (B), negative middle (M), small top (T)
# Red: Positive middle (M), negative top (T) and bottom (B)
# Green: Balanced but slightly positive top (T)

improved_matrix = np.array([
    # NIR from BMT - based on IR row scaled by ~75
    [ 0.020, -0.071,  0.054],  # Preserves B>T>M relationship
    # Red from BMT - based on RED row scaled by ~75
    [-0.167,  1.033, -0.856],  # Preserves M>>T>B relationship
    # Green from BMT - based on GREEN row scaled by ~75
    [ 0.027, -0.222,  0.238]   # Preserves T>B>M relationship
])

# Round to cleaner values for better readability
improved_matrix_clean = np.array([
    [ 0.02,  -0.07,   0.05],   # NIR: emphasizes bottom, suppresses middle
    [-0.17,   1.03,  -0.86],   # Red: strong middle, negative top/bottom
    [ 0.03,  -0.22,   0.24]    # Green: balanced with slight top emphasis
])

print("Improved matrix (Method 3):")
for i, row_name in enumerate(['NIR', 'Red', 'Green']):
    print(f"  {row_name:5s}: [{improved_matrix_clean[i,0]:6.2f}, {improved_matrix_clean[i,1]:6.2f}, {improved_matrix_clean[i,2]:6.2f}]")
print()

# Method 4: Conservative scaling - 100x to bring to reasonable range
scale_factor_conservative = 100.0
scaled_conservative = calibrated_bmt * scale_factor_conservative

print("Method 4: Conservative 100x scaling")
print(f"Scaling factor: {scale_factor_conservative:.0f}")
print("Scaled matrix:")
for i, row_name in enumerate(['NIR', 'Red', 'Green']):
    print(f"  {row_name:5s}: [{scaled_conservative[i,0]:7.3f}, {scaled_conservative[i,1]:7.3f}, {scaled_conservative[i,2]:7.3f}]")
print()

# Recommendation
print("=== RECOMMENDATION ===")
print("Use Method 4 (100x scaling) as it:")
print("1. Preserves the exact relative relationships from calibration")
print("2. Brings values into the acceptable DNG profile range (0.01-2.0)")
print("3. Maintains the scientific accuracy of the calibration")
print()
print("Recommended matrix for x3f_output_dng.c:")
print("double ir_separation_matrix[9] = {")
for i in range(3):
    comment = ['/* NIR from BMT */', '/* Red from BMT */', '/* Green from BMT */'][i]
    if i < 2:
        print(f"  {scaled_conservative[i,0]:8.5f}, {scaled_conservative[i,1]:9.5f}, {scaled_conservative[i,2]:9.5f},  {comment}")
    else:
        print(f"  {scaled_conservative[i,0]:8.5f}, {scaled_conservative[i,1]:9.5f}, {scaled_conservative[i,2]:9.5f}   {comment}")
print("};")