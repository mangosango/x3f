#!/usr/bin/env python3
"""
Analyze the matrix transformation issue for Aerochrome false color
"""

import numpy as np

# K matrix from calibration (how each wavelength distributes across layers)
# Rows: T, M, B sensor layers
# Cols: Green(550nm), Red(660nm), NIR(850nm)
K_matrix = np.array([
    [0.4090, 0.2541, 0.1272],  # Top layer response
    [0.3711, 0.3149, 0.2106],  # Middle layer response
    [0.2199, 0.4310, 0.6622]   # Bottom layer response
])

print("=== K Matrix (Response Matrix) ===")
print("Shows how each wavelength distributes across sensor layers")
print("Rows: [Top, Middle, Bottom]")
print("Cols: [Green, Red, NIR]")
print(K_matrix)
print()

# For false color IR, we want:
# - NIR to appear as Red (vegetation bright red)
# - Red to appear as Green
# - Green to appear as Blue

# The key insight: We need to extract pure channels from mixed sensor data
# The sensor sees: T, M, B (mixed signals)
# We want to extract: Green, Red, NIR (pure channels)

# Method 1: Direct inverse (what we tried before)
print("=== Method 1: Direct Inverse ===")
try:
    K_inv = np.linalg.inv(K_matrix)
    print("Inverse matrix (TMB -> GRN):")
    print("Rows: [Green, Red, NIR]")
    print("Cols: [Top, Middle, Bottom]")
    for i, channel in enumerate(['Green', 'Red', 'NIR']):
        print(f"{channel:5s}: [{K_inv[i,0]:8.4f}, {K_inv[i,1]:8.4f}, {K_inv[i,2]:8.4f}]")
except:
    print("Matrix not invertible")
print()

# For DNG, we need BMT order (not TMB), so reorder columns
print("=== Reordered for BMT input ===")
K_inv_bmt = K_inv[:, [2, 1, 0]]  # Swap columns 0 and 2 for BMT order
for i, channel in enumerate(['Green', 'Red', 'NIR']):
    print(f"{channel:5s}: [{K_inv_bmt[i,0]:8.4f}, {K_inv_bmt[i,1]:8.4f}, {K_inv_bmt[i,2]:8.4f}]")
print()

# For false color display, we need to remap:
# NIR -> Red display, Red -> Green display, Green -> Blue display
print("=== False Color Remapping (for display) ===")
# Reorder rows: NIR first, then Red, then Green
K_false_color = K_inv_bmt[[2, 1, 0], :]
for i, channel in enumerate(['NIR->R', 'Red->G', 'Green->B']):
    print(f"{channel:8s}: [{K_false_color[i,0]:8.4f}, {K_false_color[i,1]:8.4f}, {K_false_color[i,2]:8.4f}]")
print()

# Normalize each row to sum to 1
print("=== Normalized for DNG (sum=1) ===")
K_normalized = np.zeros_like(K_false_color)
for i in range(3):
    row_sum = np.sum(K_false_color[i])
    K_normalized[i] = K_false_color[i] / row_sum if row_sum != 0 else K_false_color[i]

for i, channel in enumerate(['NIR', 'Red', 'Green']):
    print(f"{channel:5s}: [{K_normalized[i,0]:8.4f}, {K_normalized[i,1]:8.4f}, {K_normalized[i,2]:8.4f}]  Sum={np.sum(K_normalized[i]):.4f}")
print()

print("=== User's Working Matrix ===")
user_matrix = np.array([
    [2.2805, -0.6819, -0.6015],  # NIR
    [0.1687, 1.3300, -0.4987],    # Red
    [-0.6609, 1.2736, 0.3873]     # Green
])
for i, channel in enumerate(['NIR', 'Red', 'Green']):
    print(f"{channel:5s}: [{user_matrix[i,0]:8.4f}, {user_matrix[i,1]:8.4f}, {user_matrix[i,2]:8.4f}]  Sum={np.sum(user_matrix[i]):.4f}")
print()

# Compare patterns
print("=== Pattern Analysis ===")
print("User's NIR row emphasizes Bottom (2.28) and negates Top (-0.60)")
print("This makes sense: Bottom layer is most IR-sensitive (66% of NIR)")
print("User's Green row has negative Bottom (-0.66), positive Middle (1.27)")
print("This reduces IR contamination in green channel")
print()

# Let's try a different approach based on the K matrix directly
print("=== Alternative: Emphasize Dominant Responses ===")
# For each output channel, emphasize where it's strongest in K matrix
# NIR is strongest in Bottom (0.6622)
# Red is strongest in Bottom (0.4310) but also Middle (0.3149)
# Green is strongest in Top (0.4090)

alt_matrix = np.array([
    # NIR: Emphasize bottom, suppress others
    [2.0, -0.5, -0.5],  # Strong B, negative T and M
    # Red: Balance middle and bottom
    [0.2, 1.3, -0.5],   # Strong M, some B, negative T
    # Green: Emphasize top, suppress bottom
    [-0.5, 0.8, 0.7]    # Strong M and T, negative B
])

print("Alternative matrix based on dominant responses:")
for i, channel in enumerate(['NIR', 'Red', 'Green']):
    print(f"{channel:5s}: [{alt_matrix[i,0]:8.4f}, {alt_matrix[i,1]:8.4f}, {alt_matrix[i,2]:8.4f}]  Sum={np.sum(alt_matrix[i]):.4f}")