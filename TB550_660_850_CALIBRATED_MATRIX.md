# Calibrated Matrix for TB550/660/850 Filter on DP2M

## Your Calibrated K Matrix

Based on calibration performed with GREEN, RED, and NIR targets:

```
K = [0.4090, 0.2541, 0.1272]  # Top layer (T)
    [0.3711, 0.3149, 0.2106]  # Middle layer (M)
    [0.2199, 0.4310, 0.6622]  # Bottom layer (B)
```

## Command Line Usage

### Basic IR Separation
```bash
x3f_extract -o output -dng -ir-separate \
  -ir-matrix "0.4090,0.2541,0.1272,0.3711,0.3149,0.2106,0.2199,0.4310,0.6622" \
  your_image.x3f
```

### Batch Processing Script
```bash
#!/bin/bash
MATRIX="0.4090,0.2541,0.1272,0.3711,0.3149,0.2106,0.2199,0.4310,0.6622"
OUTPUT_DIR="processed"

for file in *.X3F; do
    echo "Processing $file..."
    x3f_extract -o "$OUTPUT_DIR" -dng -ir-separate -ir-matrix "$MATRIX" "$file"
done
```

## Channel Mapping in Output DNG (False Color Infrared)

After processing, your DNG file will contain channels mapped for false color infrared visualization:
- **Channel 0 (displays as Red)**: NIR (850nm) - vegetation appears red/magenta
- **Channel 1 (displays as Green)**: Red (660nm) - red objects appear green
- **Channel 2 (displays as Blue)**: Green (550nm) - green objects appear blue

This is the standard "Color Infrared" (CIR) mapping used in aerial photography, where healthy vegetation appears bright red/magenta due to high NIR reflectance.

## Vegetation Index Calculations

With the separated channels, you can calculate:

### Chlorophyll Vegetation Index (CVI)
```
CVI = (NIR × Red) / (Green²)
CVI = (Channel_0 × Channel_1) / (Channel_2²)
```

### Normalized Difference Vegetation Index (NDVI)
```
NDVI = (NIR - Red) / (NIR + Red)
NDVI = (Channel_0 - Channel_1) / (Channel_0 + Channel_1)
```

### Triangular Vegetation Index (TVI)
```
TVI = 0.5 × (120 × (NIR - Green) - 200 × (Red - Green))
TVI = 0.5 × (120 × (Channel_0 - Channel_2) - 200 × (Channel_1 - Channel_2))
```

### Enhanced Vegetation Index (EVI)
```
EVI = 2.5 × ((NIR - Red) / (NIR + 6×Red - 7.5×Blue + 1))
EVI = 2.5 × ((Channel_0 - Channel_1) / (Channel_0 + 6×Channel_1 - 7.5×Channel_2 + 1))
```

## Calibration Quality

The calibration shows excellent channel separation:
- Green signal: 41% top, 37% middle, 22% bottom
- Red signal: 25% top, 31% middle, 43% bottom
- NIR signal: 13% top, 21% middle, 66% bottom

This distribution matches the expected silicon absorption depths for these wavelengths.

## Notes

- This matrix is specific to your DP2M camera and TB550/660/850 filter
- Recalibrate if you change filters or cameras
- The matrix assumes linear sensor response
- For best results, use consistent white balance settings