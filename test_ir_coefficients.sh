#!/bin/bash

# Test script for IR separation with calibrated coefficients

echo "Testing IR separation with calibrated coefficients"
echo "================================================="
echo

# Path to test file
TEST_FILE="x3f_test_files/aerochrome/calibrate/GREEN.X3F"
OUTPUT_DIR="_local/ir_test_output"

# Create output directory
mkdir -p "$OUTPUT_DIR"

# Test 1: Extract with IR calibration mode to check coefficients
echo "1. Extracting calibration data from GREEN.X3F..."
./bin/osx-universal/x3f_extract -ir-calibrate "$TEST_FILE" > "$OUTPUT_DIR/green_calibration.txt" 2>&1
echo "   Calibration data:"
grep "Average TMB:" calibration_data.txt
echo

# Test 2: Extract as DNG with IR separation
echo "2. Creating DNG with IR separation enabled..."
./bin/osx-universal/x3f_extract -dng -ir-separate -o "$OUTPUT_DIR" "$TEST_FILE" 2>&1
if [ -f "$OUTPUT_DIR/GREEN.X3F.dng" ]; then
    echo "   ✓ IR-separated DNG created successfully"
    mv "$OUTPUT_DIR/GREEN.X3F.dng" "$OUTPUT_DIR/green_ir_separated.dng"
    ls -lh "$OUTPUT_DIR/green_ir_separated.dng"
else
    echo "   ✗ Failed to create IR-separated DNG"
fi
echo

# Test 3: Extract as DNG without IR separation (normal mode)
echo "3. Creating normal DNG for comparison..."
./bin/osx-universal/x3f_extract -dng -o "$OUTPUT_DIR" "$TEST_FILE" 2>&1
if [ -f "$OUTPUT_DIR/GREEN.X3F.dng" ]; then
    echo "   ✓ Normal DNG created successfully"
    mv "$OUTPUT_DIR/GREEN.X3F.dng" "$OUTPUT_DIR/green_normal.dng"
    ls -lh "$OUTPUT_DIR/green_normal.dng"
else
    echo "   ✗ Failed to create normal DNG"
fi
echo

# Test 4: Test with a RED sample
echo "4. Testing with RED calibration target..."
./bin/osx-universal/x3f_extract -dng -ir-separate -o "$OUTPUT_DIR" "x3f_test_files/aerochrome/calibrate/RED.X3F" 2>&1
if [ -f "$OUTPUT_DIR/RED.X3F.dng" ]; then
    echo "   ✓ RED IR-separated DNG created"
    mv "$OUTPUT_DIR/RED.X3F.dng" "$OUTPUT_DIR/red_ir_separated.dng"
fi
echo

# Test 5: Test with NIR sample (should show strong IR response)
echo "5. Testing with NIR calibration target..."
./bin/osx-universal/x3f_extract -dng -ir-separate -o "$OUTPUT_DIR" "x3f_test_files/aerochrome/calibrate/NIR.X3F" 2>&1
if [ -f "$OUTPUT_DIR/NIR.X3F.dng" ]; then
    echo "   ✓ NIR IR-separated DNG created"
    mv "$OUTPUT_DIR/NIR.X3F.dng" "$OUTPUT_DIR/nir_ir_separated.dng"
    echo "   This should show strong red channel in IR-separated mode"
fi
echo

echo "================================================="
echo "Test complete. Output files in: $OUTPUT_DIR"
echo
echo "To verify the IR separation visually:"
echo "1. Open the DNG files in an image editor"
echo "2. In IR-separated mode:"
echo "   - NIR target should appear red (IR -> Red channel)"
echo "   - RED target should appear green (Red -> Green channel)"
echo "   - GREEN target should appear blue (Green -> Blue channel)"
echo "3. Compare with normal DNG to see the difference"