#!/bin/bash

# Test script to verify FOV Classic Blue white balance functionality
# This script tests different white balance modes with the FCBlue profile

X3F_EXTRACT="./bin/osx-universal/x3f_extract"
TEST_FILE="./x3f_test_files/DP2M0643.X3F"
OUTPUT_DIR="./test_output/fcblue_wb_test"

# Create test output directory
mkdir -p "$OUTPUT_DIR"

echo "Testing FOV Classic Blue profile with different white balance settings..."
echo "============================================================"

# Test 1: Default white balance (should use FCBlue as default)
echo "Test 1: Default WB with FCBlue profile"
$X3F_EXTRACT -dng -o "$OUTPUT_DIR" "$TEST_FILE"
mv "$OUTPUT_DIR/DP2M0643.dng" "$OUTPUT_DIR/test1_default.dng" 2>/dev/null

# Test 2: Explicit Auto white balance
echo -e "\nTest 2: Auto white balance"
$X3F_EXTRACT -dng -wb Auto -o "$OUTPUT_DIR" "$TEST_FILE"
mv "$OUTPUT_DIR/DP2M0643.dng" "$OUTPUT_DIR/test2_auto.dng" 2>/dev/null

# Test 3: Sunlight white balance
echo -e "\nTest 3: Sunlight white balance"
$X3F_EXTRACT -dng -wb Sunlight -o "$OUTPUT_DIR" "$TEST_FILE"
mv "$OUTPUT_DIR/DP2M0643.dng" "$OUTPUT_DIR/test3_sunlight.dng" 2>/dev/null

# Test 4: Overcast white balance
echo -e "\nTest 4: Overcast white balance"
$X3F_EXTRACT -dng -wb Overcast -o "$OUTPUT_DIR" "$TEST_FILE"
mv "$OUTPUT_DIR/DP2M0643.dng" "$OUTPUT_DIR/test4_overcast.dng" 2>/dev/null

# Test 5: Custom white balance values
echo -e "\nTest 5: Custom white balance (R=1.2,G=1.0,B=0.8)"
$X3F_EXTRACT -dng -wb "1.2,1.0,0.8" -o "$OUTPUT_DIR" "$TEST_FILE"
mv "$OUTPUT_DIR/DP2M0643.dng" "$OUTPUT_DIR/test5_custom.dng" 2>/dev/null

echo -e "\n============================================================"
echo "Test complete. DNGs created in: $OUTPUT_DIR"
echo ""
echo "To verify the fix:"
echo "1. Open the DNG files in a raw editor (e.g., Lightroom, RawTherapee)"
echo "2. Check that each file shows different white balance settings"
echo "3. The FCBlue profile should be active in all files"
echo "4. White balance adjustments should be visible in each file"

# Check metadata for verification
echo -e "\nExtracting metadata for verification..."
for file in "$OUTPUT_DIR"/*.dng; do
    if [ -f "$file" ]; then
        echo -e "\n$(basename $file):"
        # Use exiftool if available, otherwise use strings
        if command -v exiftool &> /dev/null; then
            exiftool "$file" 2>/dev/null | grep -E "Profile Name|As Shot Neutral|Color Mode" | head -5
        else
            strings "$file" | grep -E "FOV Classic Blue|FCBlue" | head -2
        fi
    fi
done