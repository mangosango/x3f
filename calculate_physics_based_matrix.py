#!/usr/bin/env python3
"""
Calculate physics-based IR separation matrix for Aerochrome effect
Based on actual sensor response characteristics from K matrix
"""

import numpy as np

# K matrix shows actual sensor response
# Rows: T, M, B layers
# Cols: Green(550), Red(660), NIR(850)
K = np.array([
    [0.4090, 0.2541, 0.1272],  # Top layer
    [0.3711, 0.3149, 0.2106],  # Middle layer
    [0.2199, 0.4310, 0.6622]   # Bottom layer
])

print("=== Sensor Response Analysis ===")
print("From K matrix calibration data:")
print()
print("Green (550nm) distribution:")
print(f"  Top: {K[0,0]:.1%}, Middle: {K[1,0]:.1%}, Bottom: {K[2,0]:.1%}")
print()
print("Red (660nm) distribution:")
print(f"  Top: {K[0,1]:.1%}, Middle: {K[1,1]:.1%}, Bottom: {K[2,1]:.1%}")
print()
print("NIR (850nm) distribution:")
print(f"  Top: {K[0,2]:.1%}, Middle: {K[1,2]:.1%}, Bottom: {K[2,2]:.1%}")
print()

print("=== Physics-Based Matrix Design ===")
print("Principles:")
print("1. NIR: Emphasize bottom (66%), suppress top/middle")
print("2. Red: Balance middle (31%) and bottom (43%)")
print("3. Green: Emphasize top (41%) and middle (37%), suppress bottom IR")
print()

# Design matrix based on physical understanding
# For BMT input order (Bottom, Middle, Top)

# NIR extraction: Strong bottom, negative others to remove visible light
# Since B has 66% of NIR, we want strong positive B
# T and M have mostly visible light, so negative to suppress
nir_coeffs = [2.2, -0.7, -0.5]  # B, M, T

# Red extraction: Balance M and B, suppress T
# M has 31% red, B has 43% red, but B also has lots of NIR
# So moderate B, strong M, negative T
red_coeffs = [0.2, 1.3, -0.5]  # B, M, T

# Green extraction: Emphasize T and M, suppress B
# T has 41% green, M has 37% green, B only 22% and lots of NIR
# So negative B to remove NIR, positive T and M
green_coeffs = [-0.6, 1.0, 0.6]  # B, M, T

matrix = np.array([nir_coeffs, red_coeffs, green_coeffs])

print("Physics-based separation matrix (BMT order):")
print()
for i, (channel, coeffs) in enumerate(zip(['NIR', 'Red', 'Green'], matrix)):
    row_sum = np.sum(coeffs)
    print(f"{channel:5s}: B={coeffs[0]:6.2f}, M={coeffs[1]:6.2f}, T={coeffs[2]:6.2f}  (sum={row_sum:.2f})")
print()

# Normalize to sum = 1.0 for DNG compatibility
print("=== Normalized Matrix (sum=1.0) ===")
matrix_norm = np.zeros_like(matrix)
for i in range(3):
    matrix_norm[i] = matrix[i] / np.sum(matrix[i])

print("For x3f_output_dng.c:")
print()
print("double ir_separation_matrix[9] = {")
print("    /* NIR from BMT (physics-based, normalized) */")
print(f"    {matrix_norm[0,0]:7.4f}, {matrix_norm[0,1]:7.4f}, {matrix_norm[0,2]:7.4f},")
print("    /* Red from BMT (physics-based, normalized) */")
print(f"    {matrix_norm[1,0]:7.4f}, {matrix_norm[1,1]:7.4f}, {matrix_norm[1,2]:7.4f},")
print("    /* Green from BMT (physics-based, normalized) */")
print(f"    {matrix_norm[2,0]:7.4f}, {matrix_norm[2,1]:7.4f}, {matrix_norm[2,2]:7.4f}")
print("};")
print()

# Verify against user's working values
print("=== Comparison with User's Working Matrix ===")
user_matrix = np.array([
    [2.2805, -0.6819, -0.6015],  # NIR
    [0.1687, 1.3300, -0.4987],    # Red
    [-0.6609, 1.2736, 0.3873]     # Green
])

print("User's matrix:")
for i, channel in enumerate(['NIR', 'Red', 'Green']):
    print(f"{channel:5s}: B={user_matrix[i,0]:6.3f}, M={user_matrix[i,1]:6.3f}, T={user_matrix[i,2]:6.3f}")
print()

print("Our physics-based (normalized):")
for i, channel in enumerate(['NIR', 'Red', 'Green']):
    print(f"{channel:5s}: B={matrix_norm[i,0]:6.3f}, M={matrix_norm[i,1]:6.3f}, T={matrix_norm[i,2]:6.3f}")
print()

# Test the matrix with hypothetical pure inputs
print("=== Verification with Pure Inputs ===")
print("Testing normalized matrix with sensor responses from K matrix:")
print()

# For pure NIR input, sensor sees (from K matrix):
nir_sensor = K[:, 2]  # T=0.127, M=0.211, B=0.662
nir_bmt = [nir_sensor[2], nir_sensor[1], nir_sensor[0]]  # Reorder to BMT
result_nir = matrix_norm @ nir_bmt
print(f"Pure NIR → Sensor BMT [{nir_bmt[0]:.3f}, {nir_bmt[1]:.3f}, {nir_bmt[2]:.3f}]")
print(f"         → Output [NIR={result_nir[0]:.3f}, Red={result_nir[1]:.3f}, Green={result_nir[2]:.3f}]")
print()

# For pure Red input:
red_sensor = K[:, 1]  # T=0.254, M=0.315, B=0.431
red_bmt = [red_sensor[2], red_sensor[1], red_sensor[0]]
result_red = matrix_norm @ red_bmt
print(f"Pure Red → Sensor BMT [{red_bmt[0]:.3f}, {red_bmt[1]:.3f}, {red_bmt[2]:.3f}]")
print(f"         → Output [NIR={result_red[0]:.3f}, Red={result_red[1]:.3f}, Green={result_red[2]:.3f}]")
print()

# For pure Green input:
green_sensor = K[:, 0]  # T=0.409, M=0.371, B=0.220
green_bmt = [green_sensor[2], green_sensor[1], green_sensor[0]]
result_green = matrix_norm @ green_bmt
print(f"Pure Green → Sensor BMT [{green_bmt[0]:.3f}, {green_bmt[1]:.3f}, {green_bmt[2]:.3f}]")
print(f"           → Output [NIR={result_green[0]:.3f}, Red={result_green[1]:.3f}, Green={result_green[2]:.3f}]")