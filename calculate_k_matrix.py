#!/usr/bin/env python3
"""
Calculate K matrix from calibration data for TB550/660/850 filter
"""

import numpy as np

# Calibration measurements from the three targets
# TMB values: Top, Middle, Bottom sensor layer responses
calibration_data = {
    "GREEN": {"T": 946.55, "M": 859.03, "B": 508.95},  # 550nm response
    "RED":   {"T": 318.42, "M": 394.68, "B": 540.25},  # 660nm response
    "NIR":   {"T": 302.83, "M": 501.40, "B": 1576.50}, # 850nm response
}

print("Raw calibration data:")
print("Target    Top      Middle   Bottom")
print("-" * 40)
for target, values in calibration_data.items():
    print(f"{target:<8} {values['T']:8.2f} {values['M']:8.2f} {values['B']:8.2f}")
print()

# Build K matrix
# Columns: Green, Red, NIR
# Rows: Top, Middle, Bottom layers
K_raw = np.array([
    [calibration_data["GREEN"]["T"], calibration_data["RED"]["T"], calibration_data["NIR"]["T"]],    # Top
    [calibration_data["GREEN"]["M"], calibration_data["RED"]["M"], calibration_data["NIR"]["M"]],    # Middle
    [calibration_data["GREEN"]["B"], calibration_data["RED"]["B"], calibration_data["NIR"]["B"]]     # Bottom
])

print("Raw K matrix (sensor response to each wavelength):")
print("         Green    Red      NIR")
print("Top:    ", K_raw[0])
print("Middle: ", K_raw[1])
print("Bottom: ", K_raw[2])
print()

# Normalize each column to sum to 1
K_normalized = np.zeros_like(K_raw)
for col in range(3):
    column_sum = np.sum(K_raw[:, col])
    K_normalized[:, col] = K_raw[:, col] / column_sum

print("Normalized K matrix (each column sums to 1):")
print("         Green    Red      NIR")
print(f"Top:     {K_normalized[0,0]:.4f}   {K_normalized[0,1]:.4f}   {K_normalized[0,2]:.4f}")
print(f"Middle:  {K_normalized[1,0]:.4f}   {K_normalized[1,1]:.4f}   {K_normalized[1,2]:.4f}")
print(f"Bottom:  {K_normalized[2,0]:.4f}   {K_normalized[2,1]:.4f}   {K_normalized[2,2]:.4f}")
print()

# Format for command line use
matrix_values = K_normalized.flatten()
matrix_string = ",".join(f"{v:.4f}" for v in matrix_values)
print("Matrix for x3f_extract -ir-matrix parameter:")
print(f'"{matrix_string}"')
print()

# Full command example
print("Example command:")
print(f"x3f_extract -o output -dng -ir-separate \\")
print(f'  -ir-matrix "{matrix_string}" \\')
print(f"  your_image.x3f")
print()

# Verify column sums
print("Verification - column sums (should all be 1.0):")
for col in range(3):
    col_sum = np.sum(K_normalized[:, col])
    print(f"Column {col} ({['Green', 'Red', 'NIR'][col]}): {col_sum:.6f}")
print()

# Calculate inverse for verification
print("Calculating inverse matrix for channel separation...")
K_inv = np.linalg.inv(K_normalized)
print("\nInverse K matrix (for BMT -> GRN conversion):")
print("K_inv =")
for i, channel in enumerate(["Green", "Red", "NIR"]):
    print(f"{channel:6s}: [{K_inv[i,0]:7.4f}, {K_inv[i,1]:7.4f}, {K_inv[i,2]:7.4f}]")

# Test the separation
print("\nSeparation test - if we input pure signals:")
test_cases = [
    ("Pure Green", [K_normalized[0,0], K_normalized[1,0], K_normalized[2,0]]),
    ("Pure Red",   [K_normalized[0,1], K_normalized[1,1], K_normalized[2,1]]),
    ("Pure NIR",   [K_normalized[0,2], K_normalized[1,2], K_normalized[2,2]])
]

for name, bmt in test_cases:
    result = np.dot(K_inv, bmt)
    print(f"{name} BMT {[f'{v:.4f}' for v in bmt]} -> GRN [{result[0]:.3f}, {result[1]:.3f}, {result[2]:.3f}]")