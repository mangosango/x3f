Perfect! Now I can see the full picture. The X3F files DO contain picture profile metadata, but x3f_extract is not currently transferring this information to the DNG output.

## Current Situation Analysis:

### In the X3F metadata (CAMF section), I found:

1. **Picture Profile definitions** under `ColorModeCompensations`:

   - Standard, Vivid, Neutral, Portrait, Landscape
   - FCBlue, FCYellow, Cinema, ForestGreen, SunsetRed, Monochrome

2. **For each profile**, there's:

   - Color matrix compensation (`CMCM_*`)
   - Contrast compensation (`CMCC_*`)
   - Multi-axis table compensation (`MultiAxisTable_*`)

### The in-camera DNG has:

- Multiple camera profiles (Standard, Vivid, Neutral, Portrait, Landscape, Monochrome, Original)
- Each with ProfileToneCurve and ProfileLookTableData
- Properly set AsShot profile name

### The x3f_extract DNG only has:

- Basic profiles (Default, Grayscale variants, Unconverted)
- No tone curves or look tables
- Missing the picture profile metadata

## Implementation Plan:

### 1. **Add Picture Profile Extraction**

- Create functions to read ColorModeCompensations from CAMF metadata
- Extract the color matrices, contrast values, and multi-axis tables for each profile

### 2. **Convert to DNG Format**

- Transform the X3F color matrices to DNG ColorMatrix format
- Convert contrast compensations to ProfileToneCurve data
- Transform multi-axis tables to ProfileLookTableData

### 3. **Extend Camera Profile Structure**

- Add fields for tone curves and look tables to the `camera_profile_t` struct
- Create profile definitions for each picture mode

### 4. **Write Profiles to DNG**

- Extend `write_camera_profile()` to include:

  - ProfileToneCurve (TIFFTAG_PROFILETONECURVE)
  - ProfileLookTableDims and ProfileLookTableData
  - ProfileHueSatMapDims and ProfileHueSatMapData (if needed)

- Set the AsShot profile based on the camera's current picture mode setting

### 5. **Map Picture Modes**

- Read the current picture mode from X3F metadata
- Set appropriate AsShot profile name

## Key Challenges:

1. **Format Conversion**: The X3F stores color adjustments as matrices and multi-axis tables, while DNG uses tone curves and look tables. We'll need conversion algorithms.

2. **Tone Curve Generation**: May need to generate appropriate tone curves based on the contrast compensation values.

3. **Look Table Format**: DNG uses a specific 3D LUT format that we'll need to properly format from the X3F multi-axis data.
