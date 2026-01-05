#!/usr/bin/env python3
"""
Tune IR separation matrix to make foliage appear more magenta
"""

import numpy as np

# User's current matrix
current_matrix = np.array([
    [1.55391, 0.357035, -0.910933],   # NIR from BMT
    [0.367226, 1.4933, -0.860522],     # Red from BMT
    [0.123477, 0.176646, 0.699866]     # Green from BMT
])

print("=== Current Matrix Analysis ===")
print("Your current matrix:")
for i, channel in enumerate(['NIR', 'Red', 'Green']):
    row = current_matrix[i]
    print(f"{channel:5s}: B={row[0]:7.4f}, M={row[1]:7.4f}, T={row[2]:7.4f}  Sum={np.sum(row):.4f}")
print()

# Typical sensor response for foliage (high green + very high NIR)
# From K matrix: Green=[0.220, 0.371, 0.409], NIR=[0.662, 0.211, 0.127]
# Foliage is approximately: 30% green + 70% NIR
foliage_green = np.array([0.220, 0.371, 0.409])
foliage_nir = np.array([0.662, 0.211, 0.127])
foliage_sensor = 0.3 * foliage_green + 0.7 * foliage_nir  # BMT order

print("=== Foliage Analysis ===")
print(f"Typical foliage sensor response (BMT): [{foliage_sensor[0]:.3f}, {foliage_sensor[1]:.3f}, {foliage_sensor[2]:.3f}]")

# Apply current matrix to foliage
foliage_output = current_matrix @ foliage_sensor
print(f"Current matrix output for foliage:")
print(f"  NIR={foliage_output[0]:.3f}, Red={foliage_output[1]:.3f}, Green={foliage_output[2]:.3f}")
print(f"  → Displays as: R={foliage_output[0]:.3f}, G={foliage_output[1]:.3f}, B={foliage_output[2]:.3f}")
print(f"  → Color balance: R/G ratio = {foliage_output[0]/foliage_output[1]:.2f}")
print()

print("=== Problem Diagnosis ===")
print(f"Foliage R/G ratio is {foliage_output[0]/foliage_output[1]:.2f}")
print("For magenta appearance, we need:")
print("  - High R (from NIR)")
print("  - Low G (suppress actual red)")
print("  - Some B (from actual green)")
print("  - Target R/G ratio > 3.0 for good magenta")
print()

print("=== Solution Strategies ===")
print("To make foliage more magenta:")
print("1. Increase NIR extraction (boost NIR row, especially B coefficient)")
print("2. Reduce Red extraction for foliage patterns (adjust Red row)")
print("3. Keep or slightly increase Green->Blue to add blue component")
print()

# Try adjusted matrices
adjustments = [
    {
        'name': 'Boost NIR extraction',
        'matrix': np.array([
            [1.8, 0.2, -1.0],        # Increased B, reduced M
            [0.3, 1.5, -0.8],        # Slightly reduced
            [0.12, 0.18, 0.70]       # Keep similar
        ])
    },
    {
        'name': 'Suppress red in foliage',
        'matrix': np.array([
            [1.7, 0.4, -1.1],        # Strong NIR
            [0.2, 1.6, -0.8],        # Reduced B to suppress red from NIR areas
            [0.15, 0.15, 0.70]       # Balanced green
        ])
    },
    {
        'name': 'Enhanced magenta (recommended)',
        'matrix': np.array([
            [1.85, 0.25, -1.1],      # Very strong NIR from B
            [0.15, 1.65, -0.8],      # Minimize B contrib to reduce red from NIR
            [0.10, 0.20, 0.70]       # Keep green for blue component
        ])
    }
]

print("=== Testing Adjusted Matrices ===")
for adj in adjustments:
    print(f"\n{adj['name']}:")
    matrix = adj['matrix']

    # Print matrix
    for i, channel in enumerate(['NIR', 'Red', 'Green']):
        row = matrix[i]
        print(f"  {channel:5s}: B={row[0]:6.3f}, M={row[1]:6.3f}, T={row[2]:6.3f}")

    # Test with foliage
    output = matrix @ foliage_sensor
    rg_ratio = output[0]/output[1] if output[1] > 0 else float('inf')

    print(f"  Foliage output: NIR={output[0]:.3f}, Red={output[1]:.3f}, Green={output[2]:.3f}")
    print(f"  R/G ratio: {rg_ratio:.2f} {'✓ Good magenta' if rg_ratio > 3 else '✗ Too orange'}")

print("\n=== Recommended Matrix ===")
recommended = np.array([
    [1.85, 0.25, -1.1],      # NIR: Very strong B, minimal M, negative T
    [0.15, 1.65, -0.8],      # Red: Minimal B (reduce red from NIR areas)
    [0.10, 0.20, 0.70]       # Green: Gentle response for blue component
])

print("This matrix should produce better magenta foliage:")
print("```c")
print("double ir_separation_matrix[9] = {")
print("    /* NIR from BMT (enhanced for magenta foliage) */")
print(f"    {recommended[0,0]:.2f}, {recommended[0,1]:.2f}, {recommended[0,2]:.2f},")
print("    /* Red from BMT (reduced B to minimize red in NIR areas) */")
print(f"    {recommended[1,0]:.2f}, {recommended[1,1]:.2f}, {recommended[1,2]:.2f},")
print("    /* Green from BMT (balanced for blue component) */")
print(f"    {recommended[2,0]:.2f}, {recommended[2,1]:.2f}, {recommended[2,2]:.2f}")
print("};")
print("```")

print("\n=== Fine-tuning Tips ===")
print("If foliage is still too orange:")
print("  - Increase NIR B coefficient (currently 1.85 → try 2.0)")
print("  - Decrease Red B coefficient (currently 0.15 → try 0.1)")
print("If foliage is too dark:")
print("  - Scale up NIR row proportionally")
print("If sky is wrong:")
print("  - Adjust Green T coefficient (affects blue channel brightness)")