#!/usr/bin/env python3
"""
More aggressive matrix adjustments to achieve magenta foliage
"""

import numpy as np

# Typical foliage sensor response (30% green + 70% NIR)
foliage_sensor = np.array([0.529, 0.259, 0.212])  # BMT order

print("=== Aggressive Magenta Adjustments ===")
print("Target: R/G ratio > 3.0 for proper magenta appearance")
print()

matrices = [
    {
        'name': 'Strong NIR boost + Red suppression',
        'matrix': np.array([
            [2.2, 0.1, -1.3],        # Much stronger NIR from B
            [0.05, 1.75, -0.80],     # Minimal B to avoid red from NIR
            [0.10, 0.20, 0.70]       # Standard green
        ])
    },
    {
        'name': 'Extreme NIR isolation',
        'matrix': np.array([
            [2.5, -0.2, -1.3],       # Very strong B, negative M
            [-0.1, 1.9, -0.80],      # Negative B to suppress red in NIR areas
            [0.08, 0.22, 0.70]       # Slight green
        ])
    },
    {
        'name': 'Balanced high contrast',
        'matrix': np.array([
            [2.0, 0.2, -1.2],        # Strong NIR
            [0.0, 1.8, -0.80],       # Zero B for red (no red from NIR)
            [0.12, 0.18, 0.70]       # Moderate green
        ])
    },
    {
        'name': 'Optimized for R/G > 3',
        'matrix': np.array([
            [2.3, 0.0, -1.3],        # Pure B response for NIR
            [-0.05, 1.85, -0.80],    # Slight negative B
            [0.10, 0.20, 0.70]       # Standard green
        ])
    }
]

best_matrix = None
best_ratio = 0
best_name = ""

for config in matrices:
    matrix = config['matrix']
    name = config['name']

    print(f"{name}:")

    # Print matrix
    for i, channel in enumerate(['NIR', 'Red', 'Green']):
        row = matrix[i]
        print(f"  {channel:5s}: B={row[0]:6.2f}, M={row[1]:6.2f}, T={row[2]:6.2f}  Sum={np.sum(row):.2f}")

    # Test with foliage
    output = matrix @ foliage_sensor
    rg_ratio = output[0]/output[1] if output[1] > 0 else float('inf')

    print(f"  Foliage: R={output[0]:.3f}, G={output[1]:.3f}, B={output[2]:.3f}")
    print(f"  R/G ratio: {rg_ratio:.2f} {'✓ MAGENTA!' if rg_ratio > 3 else ('→ Better' if rg_ratio > 2.5 else '✗ Orange')}")

    if rg_ratio > best_ratio and rg_ratio < 10:  # Avoid extreme ratios
        best_ratio = rg_ratio
        best_matrix = matrix
        best_name = name

    print()

print("=== BEST MATRIX FOR MAGENTA FOLIAGE ===")
if best_ratio > 3:
    print(f"✓ Found ideal matrix: '{best_name}' with R/G ratio = {best_ratio:.2f}")
else:
    print(f"Best available: '{best_name}' with R/G ratio = {best_ratio:.2f}")

print("\nRecommended matrix for x3f_output_dng.c:")
print("```c")
print("double ir_separation_matrix[9] = {")
print("    /* NIR from BMT (optimized for magenta foliage) */")
print(f"    {best_matrix[0,0]:.3f}, {best_matrix[0,1]:.3f}, {best_matrix[0,2]:.3f},")
print("    /* Red from BMT (minimized in NIR-rich areas) */")
print(f"    {best_matrix[1,0]:.3f}, {best_matrix[1,1]:.3f}, {best_matrix[1,2]:.3f},")
print("    /* Green from BMT (provides blue component) */")
print(f"    {best_matrix[2,0]:.3f}, {best_matrix[2,1]:.3f}, {best_matrix[2,2]:.3f}")
print("};")
print("```")

print("\n=== Understanding the Changes ===")
print("Key modifications from your current matrix:")
print(f"1. NIR B coefficient: {1.55391:.2f} → {best_matrix[0,0]:.2f} (stronger NIR extraction)")
print(f"2. Red B coefficient: {0.367:.2f} → {best_matrix[1,0]:.2f} (less red from NIR-rich pixels)")
print(f"3. This pushes R/G ratio from 1.81 to {best_ratio:.2f}")
print()
print("Result: Foliage with high NIR will appear more magenta (red+blue)")
print("        instead of orange (red+green)")

# Alternative approach
print("\n=== Alternative: Scale existing NIR row ===")
scale_factor = 1.5
alt_matrix = np.array([
    [1.55391 * scale_factor, 0.357035 * scale_factor, -0.910933 * scale_factor],
    [0.1, 1.4933, -0.860522],  # Reduce B coefficient more
    [0.123477, 0.176646, 0.699866]
])

output = alt_matrix @ foliage_sensor
rg_ratio = output[0]/output[1] if output[1] > 0 else float('inf')

print("Your matrix with scaled NIR row (×1.5) and reduced Red B:")
for i, channel in enumerate(['NIR', 'Red', 'Green']):
    row = alt_matrix[i]
    print(f"  {channel:5s}: B={row[0]:6.3f}, M={row[1]:6.3f}, T={row[2]:6.3f}")

print(f"\nFoliage output: R={output[0]:.3f}, G={output[1]:.3f}, B={output[2]:.3f}")
print(f"R/G ratio: {rg_ratio:.2f} {'✓ MAGENTA!' if rg_ratio > 3 else ''}")