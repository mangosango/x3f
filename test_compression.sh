#!/bin/bash

# Test script to verify compression fix for x3f_extract

EXTRACT_BIN="./bin/osx-universal/x3f_extract"
TEST_DIR="./test_compression_output"

# Create test directory
mkdir -p "$TEST_DIR"

# Find a test X3F file
TEST_FILE=$(find ./x3f_test_files -name "*.x3f" -o -name "*.X3F" | head -1)

if [ -z "$TEST_FILE" ]; then
    echo "No X3F test files found in ./x3f_test_files"
    echo "Please provide an X3F file to test with"
    exit 1
fi

echo "Testing compression fix with file: $TEST_FILE"
echo "==========================================="

# Extract without compression (DNG)
echo -e "\n1. Extracting DNG without compression..."
$EXTRACT_BIN -dng -o "$TEST_DIR" "$TEST_FILE"
DNG_UNCOMPRESSED=$(ls -la "$TEST_DIR"/*.dng | awk '{print $5}')
echo "   Uncompressed DNG size: $DNG_UNCOMPRESSED bytes"

# Extract with compression (DNG)
echo -e "\n2. Extracting DNG with compression..."
$EXTRACT_BIN -dng -compress -o "$TEST_DIR" "$TEST_FILE"
DNG_COMPRESSED=$(ls -la "$TEST_DIR"/*.dng | awk '{print $5}')
echo "   Compressed DNG size: $DNG_COMPRESSED bytes"

# Calculate compression ratio
if [ "$DNG_UNCOMPRESSED" -gt 0 ]; then
    RATIO=$(echo "scale=2; ($DNG_UNCOMPRESSED - $DNG_COMPRESSED) * 100 / $DNG_UNCOMPRESSED" | bc)
    echo "   Compression saved: $RATIO%"
fi

# Clean up between tests
rm -f "$TEST_DIR"/*.dng

echo -e "\n==========================================="

# Extract without compression (TIFF)
echo -e "\n3. Extracting TIFF without compression..."
$EXTRACT_BIN -tiff -o "$TEST_DIR" "$TEST_FILE"
TIFF_UNCOMPRESSED=$(ls -la "$TEST_DIR"/*.tif | awk '{print $5}')
echo "   Uncompressed TIFF size: $TIFF_UNCOMPRESSED bytes"

# Extract with compression (TIFF)
echo -e "\n4. Extracting TIFF with compression..."
$EXTRACT_BIN -tiff -compress -o "$TEST_DIR" "$TEST_FILE"
TIFF_COMPRESSED=$(ls -la "$TEST_DIR"/*.tif | awk '{print $5}')
echo "   Compressed TIFF size: $TIFF_COMPRESSED bytes"

# Calculate compression ratio
if [ "$TIFF_UNCOMPRESSED" -gt 0 ]; then
    RATIO=$(echo "scale=2; ($TIFF_UNCOMPRESSED - $TIFF_COMPRESSED) * 100 / $TIFF_UNCOMPRESSED" | bc)
    echo "   Compression saved: $RATIO%"
fi

echo -e "\n==========================================="
echo "Test Results:"
echo "============="

# Check if compression is working
if [ "$DNG_COMPRESSED" -lt "$DNG_UNCOMPRESSED" ]; then
    echo "✓ DNG compression is working correctly!"
    echo "  Compressed file is smaller than uncompressed"
else
    echo "✗ DNG compression issue detected!"
    echo "  Compressed file is NOT smaller than uncompressed"
fi

if [ "$TIFF_COMPRESSED" -lt "$TIFF_UNCOMPRESSED" ]; then
    echo "✓ TIFF compression is working correctly!"
    echo "  Compressed file is smaller than uncompressed"
else
    echo "✗ TIFF compression issue detected!"
    echo "  Compressed file is NOT smaller than uncompressed"
fi

# Clean up
rm -rf "$TEST_DIR"
