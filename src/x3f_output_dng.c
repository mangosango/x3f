/* X3F_OUTPUT_DNG.C
 *
 * Library for writing the image as DNG.
 *
 * Copyright 2015 - Roland and Erik Karlsson
 * BSD-style - see doc/copyright.txt
 *
 */

#include "x3f_output_dng.h"
#include "x3f_process.h"
#include "x3f_dngtags.h"
#include "x3f_matrix.h"
#include "x3f_meta.h"
#include "x3f_image.h"
#include "x3f_spatial_gain.h"
#include "x3f_printf.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <tiffio.h>
#include <math.h>
#include <string.h>
#include <assert.h>

static void vec_double_to_float(double *a, float *b, int len)
{
  int i;

  for (i=0; i<len; i++)
    b[i] = a[i];
}

static int get_camf_rect_as_dngrect(x3f_t *x3f, char *name,
				    x3f_area16_t *image, int rescale,
				    uint32_t *rect)
{
  uint32_t camf_rect[4];

  if (!x3f_get_camf_rect(x3f, name, image, rescale, camf_rect))
    return 0;

  /* Translate from Sigma's to Adobe's view on rectangles */
  rect[0] = camf_rect[1];
  rect[1] = camf_rect[0];
  rect[2] = camf_rect[3] + 1;
  rect[3] = camf_rect[2] + 1;

  return 1;
}

static int write_spatial_gain(x3f_t *x3f, x3f_area16_t *image, char *wb,
			      TIFF *tiff)
{
  x3f_spatial_gain_corr_t corr[MAXCORR];
  int corr_num;
  uint32_t active_area[4];
  double originv, originh, scalev, scaleh;

  uint8_t *opcode_list, *p;
  int opcode_size[MAXCORR];
  int opcode_list_size = sizeof(dng_opcodelist_header_t);
  dng_opcodelist_header_t *header;
  int i, j;

  if (!get_camf_rect_as_dngrect(x3f, "ActiveImageArea", image, 1,
				active_area))
    return 0;

  /* Spatial gain in X3F refers to the entire image, but OpcodeList2
     in DNG is appled after cropping to ActiveArea */
  originv = -(double)active_area[0] / (active_area[2] - active_area[0]);
  originh = -(double)active_area[1] / (active_area[3] - active_area[1]);
  scalev =   (double)image->rows    / (active_area[2] - active_area[0]);
  scaleh =   (double)image->columns / (active_area[3] - active_area[1]);

  corr_num = x3f_get_spatial_gain(x3f, wb, corr);
  if (corr_num == 0) return 0;

  for (i=0; i<corr_num; i++) {
    opcode_size[i] = sizeof(dng_opcode_GainMap_t) +
      corr[i].rows*corr[i].cols*corr[i].channels*sizeof(float);
    opcode_list_size += opcode_size[i];
  }

  opcode_list = alloca(opcode_list_size);
  header = (dng_opcodelist_header_t *)opcode_list;
  PUT_BIG_32(header->count, corr_num);

  for (p = opcode_list + sizeof(dng_opcodelist_header_t), i=0;
       i<corr_num;
       p += opcode_size[i], i++) {
    dng_opcode_GainMap_t *gain_map = (dng_opcode_GainMap_t *)p;
    x3f_spatial_gain_corr_t *c = &corr[i];

    PUT_BIG_32(gain_map->header.id, DNG_OPCODE_GAINMAP_ID);
    PUT_BIG_32(gain_map->header.ver, DNG_OPCODE_GAINMAP_VER);
    PUT_BIG_32(gain_map->header.flags, 0);
    PUT_BIG_32(gain_map->header.parsize,
	       opcode_size[i] - sizeof(dng_opcode_header_t));

    PUT_BIG_32(gain_map->Top, c->rowoff);
    PUT_BIG_32(gain_map->Left, c->coloff);
    PUT_BIG_32(gain_map->Bottom, active_area[2] - active_area[0]);
    PUT_BIG_32(gain_map->Right, active_area[3] - active_area[1]);
    PUT_BIG_32(gain_map->Plane, c->chan);
    PUT_BIG_32(gain_map->Planes, c->channels);
    PUT_BIG_32(gain_map->RowPitch, c->rowpitch);
    PUT_BIG_32(gain_map->ColPitch, c->colpitch);
    PUT_BIG_32(gain_map->MapPointsV, c->rows);
    PUT_BIG_32(gain_map->MapPointsH, c->cols);
    PUT_BIG_64(gain_map->MapSpacingV, scalev/(c->rows-1));
    PUT_BIG_64(gain_map->MapSpacingH, scaleh/(c->cols-1));
    PUT_BIG_64(gain_map->MapOriginV, originv);
    PUT_BIG_64(gain_map->MapOriginH, originh);
    PUT_BIG_32(gain_map->MapPlanes, c->channels);

    for (j=0; j<c->rows*c->cols*c->channels; j++)
      PUT_BIG_32(gain_map->MapGain[j], c->gain[j]);
  }

  x3f_cleanup_spatial_gain(corr, corr_num);

  TIFFSetField(tiff, TIFFTAG_OPCODELIST2, opcode_list_size, opcode_list);

  return 1;
}

typedef struct {
  char *name;
  int (*get_bmt_to_xyz)(x3f_t *x3f, char *wb, double *raw_to_xyz);
  double *grayscale_mix;
} camera_profile_t;

/* TODO: more mixes should be defined */
static double grayscale_mix_std[3] = {1.0/3.0, 1.0/3.0, 1.0/3.0};
static double grayscale_mix_red[3] = {2.0, -1.0, 0.0};
static double grayscale_mix_blue[3] = {0.0, -1.0, 2.0};

static int get_bmt_to_xyz_noconvert(x3f_t *x3f, char *wb, double *bmt_to_xyz)
{
  /* TODO: assuming working space to be Adobe RGB. Is that acceptable? */
  x3f_AdobeRGB_to_XYZ(bmt_to_xyz);
  return 1;
}

static const camera_profile_t camera_profiles[] = {
  {"Default", x3f_get_bmt_to_xyz, NULL},
  {"Grayscale", get_bmt_to_xyz_noconvert, grayscale_mix_std},
  {"Grayscale (red filter)", get_bmt_to_xyz_noconvert, grayscale_mix_red},
  {"Grayscale (blue filter)", get_bmt_to_xyz_noconvert, grayscale_mix_blue},
  {"Unconverted", get_bmt_to_xyz_noconvert, NULL},
};

static int write_camera_profile(x3f_t *x3f, char *wb,
				const camera_profile_t *profile,
				TIFF *tiff)
{
  double bmt_to_xyz[9], xyz_to_bmt[9], bmt_to_d50[9];
  float color_matrix1[9], forward_matrix1[9];
    
  TIFFSetField(tiff, TIFFTAG_CALIBRATIONILLUMINANT1, 21); // D65, // added

  if (!profile->get_bmt_to_xyz(x3f, wb, bmt_to_xyz)) {
    x3f_printf(ERR, "Could not get bmt_to_xyz for white balance: %s\n", wb);
    return 0;
  }
  x3f_3x3_inverse(bmt_to_xyz, xyz_to_bmt);
  vec_double_to_float(xyz_to_bmt, color_matrix1, 9);
  TIFFSetField(tiff, TIFFTAG_COLORMATRIX1, 9, color_matrix1);

  if (profile->grayscale_mix) {
    double d50_xyz[3] = {0.96422, 1.00000, 0.82521};
    double grayscale_mix_mat[9], ones[9], d50_xyz_mat[9], bmt_to_grayscale[9];

    x3f_3x3_diag(profile->grayscale_mix, grayscale_mix_mat);
    x3f_3x3_ones(ones);
    x3f_3x3_3x3_mul(ones, grayscale_mix_mat, bmt_to_grayscale);
    x3f_3x3_diag(d50_xyz, d50_xyz_mat);
    x3f_3x3_3x3_mul(d50_xyz_mat, bmt_to_grayscale, bmt_to_d50);
  }
  else {
    double d65_to_d50[9];

    x3f_Bradford_D65_to_D50(d65_to_d50);
    x3f_3x3_3x3_mul(d65_to_d50, bmt_to_xyz, bmt_to_d50);
  }
  vec_double_to_float(bmt_to_d50, forward_matrix1, 9);
  TIFFSetField(tiff, TIFFTAG_FORWARDMATRIX1, 9, forward_matrix1);

  TIFFSetField(tiff, TIFFTAG_PROFILENAME, profile->name);
  /* Tell the raw converter to refrain from clipping the dark areas */
  TIFFSetField(tiff, TIFFTAG_DEFAULTBLACKRENDER, 1);

  return 1;
}

#if defined(_WIN32) || defined (_WIN64)
/* tmpfile() is broken on Windows */
#include <windows.h>

#define tmpfile tmpfile_win
static FILE *tmpfile_win(void)
{
  char dir[MAX_PATH], file[MAX_PATH];
  HANDLE h;

  if (!GetTempPath(MAX_PATH, dir)) return NULL;
  if (!GetTempFileName(dir, "x3f", 0, file)) return NULL;

  h = CreateFile(file, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
		 FILE_ATTRIBUTE_NORMAL | FILE_FLAG_DELETE_ON_CLOSE, NULL);
  if (h == INVALID_HANDLE_VALUE) return NULL;

  return fdopen(_open_osfhandle((intptr_t)h, O_RDWR | O_BINARY), "w+b");
}
#endif

static x3f_return_t write_camera_profiles(x3f_t *x3f, char *wb,
					  const camera_profile_t *profiles,
					  int num,
					  TIFF *tiff)
{
  FILE *tiff_file;
  uint32_t *profile_offsets;
  int i;

  assert(num >= 1);
  if (!write_camera_profile(x3f, wb, &profiles[0], tiff))
    return X3F_ARGUMENT_ERROR;
  TIFFSetField(tiff, TIFFTAG_ASSHOTPROFILENAME, profiles[0].name);
  if (num == 1) return X3F_OK;

  profile_offsets = alloca((num-1)*sizeof(uint32_t));

  tiff_file = fdopen(dup(TIFFFileno(tiff)), "w+b");
  if (!tiff_file) return X3F_OUTFILE_ERROR;

  for (i=1; i < num; i++) {
    FILE *tmp;
    TIFF *tmp_tiff;
#define BUFSIZE 1024
    char buf[BUFSIZE];
    int offset, count;

    if (!(tmp = tmpfile())) {
      fclose(tiff_file);
      return X3F_OUTFILE_ERROR;
    }
    if (!(tmp_tiff = TIFFFdOpen(dup(fileno(tmp)), "", "wb"))) { /* Big endian */
      fclose(tmp);
      fclose(tiff_file);
      return X3F_OUTFILE_ERROR;
    }
    if (!write_camera_profile(x3f, wb, &profiles[i], tmp_tiff)) {
      fclose(tmp);
      TIFFClose(tmp_tiff);
      fclose(tiff_file);
      return X3F_ARGUMENT_ERROR;
    }
    TIFFWriteDirectory(tmp_tiff);
    TIFFClose(tmp_tiff);

    fseek(tiff_file, 0, SEEK_END);
    offset = (ftell(tiff_file)+1) & ~1; /* 2-byte alignment */
    fseek(tiff_file, offset, SEEK_SET);
    profile_offsets[i-1] = offset;

    fputs("MMCR", tiff_file);	/* DNG camera profile magic in big endian */
    fseek(tmp, 4, SEEK_SET);	/* Skip over the standard TIFF magic */

    while((count = fread(buf, 1, BUFSIZE, tmp)))
      fwrite(buf, 1, count, tiff_file);

    fclose(tmp);
  }

  fclose(tiff_file);
  TIFFSetField(tiff, TIFFTAG_EXTRACAMERAPROFILES, num-1, profile_offsets);
  return X3F_OK;
}

#if defined(_WIN32) || defined(_WIN64)
#define BINMODE O_BINARY
#else
#define BINMODE 0
#endif

typedef enum {
    PROFILE_MODEL_SD15=1,
//    PROFILE_MODEL_DP2Q=2,
    PROFILE_MODEL_SDQH=3
} color_profile_model_t;

static x3f_return_t write_color_profile(x3f_t *x3f, TIFF *tiff_out, x3f_color_profile_t color_profile)
{
    if (color_profile != PROFILE_CALIBRATED) {
        return X3F_OK;
    }
    
    char *cammodel;

    color_profile_model_t model = 0;
    
    if (x3f_get_prop_entry(x3f, "CAMMODEL", &cammodel)) {
        if (!strcmp(cammodel, "SIGMA SD15")) {
            model = PROFILE_MODEL_SD15;
        }
    }
    uint32_t cameraid;

    if (x3f_get_camf_unsigned(x3f, "CAMERAID", &cameraid)) {
        if (cameraid == X3F_CAMERAID_SDQH) {
            model = PROFILE_MODEL_SDQH;
        }
    }
      
    
    if (model == 0) {
        x3f_printf(ERR, "not supported camera model for calibrated color profile.\n");
        return X3F_ARGUMENT_ERROR;
    }
    
    // color matrix 1, StdA
    switch (model) {
        case PROFILE_MODEL_SD15:
        {
            float color_matrix1_sd15[9] = {1.575339, -0.605818, -0.065272, 0.753385,  0.093570,  0.159122, 0.338864,  0.183074,  0.509481};
            TIFFSetField(tiff_out, TIFFTAG_COLORMATRIX1, 9, color_matrix1_sd15);
        }
            break;
        case PROFILE_MODEL_SDQH:
        {
            float color_matrix1_sdqh[9] = {1.204060, -0.193424, -0.083580, 0.341497,  0.432368,  0.144470, 0.086408,  0.421332,  0.439112};
            TIFFSetField(tiff_out, TIFFTAG_COLORMATRIX1, 9, color_matrix1_sdqh);
        }
            break;
    }
    
    
    TIFFSetField(tiff_out, TIFFTAG_CALIBRATIONILLUMINANT1, 17); // StdA, // added
    
    // color matrix 2, D65
    switch (model) {
        case PROFILE_MODEL_SD15:
        {
            float color_matrix2_sd15[9] = {1.431427, -0.160064, -0.217438, 0.622291,  0.474456,  0.016736, 0.211198,  0.496788,  0.342864};
            TIFFSetField(tiff_out, TIFFTAG_COLORMATRIX2, 9, color_matrix2_sd15);
        }
            break;
        case PROFILE_MODEL_SDQH:
        {
            float color_matrix2_sdqh[9] = {1.223357,  0.027374, -0.178520, 0.287156,  0.648912,  0.100569, 0.000699,  0.612274,  0.432974};
            TIFFSetField(tiff_out, TIFFTAG_COLORMATRIX2, 9, color_matrix2_sdqh);
        }
            break;
    }
    
    TIFFSetField(tiff_out, TIFFTAG_CALIBRATIONILLUMINANT2, 21); // D65, // added
    
    
    // SD15, StdA
//    float forward_matrix1_[9] = {1.714980, -2.401556,  1.650795, -0.197052,  0.538726,  0.658326, 1.996839, -6.351043,  5.179405};
//
//    TIFFSetField(f_out, TIFFTAG_FORWARDMATRIX1, 9, forward_matrix1_);
    
    // SD15, D65
//      float forward_matrix2_[9] = {1.376607, -1.376272,  0.963883, -0.175388,  1.347596, -0.172208, 1.107507, -3.636357,  3.354051};
//
//      TIFFSetField(f_out, TIFFTAG_FORWARDMATRIX2, 9, forward_matrix2_);

    return X3F_OK;
}

/* extern */
x3f_return_t x3f_dump_raw_data_as_dng(x3f_t *x3f,
				      char *outfilename,
				      int fix_bad,
				      int denoise,
				      int apply_sgain,
				      char *wb,
				      int compress,
                      x3f_color_profile_t color_profile)
{
  x3f_return_t ret;
  int fd = open(outfilename, O_RDWR | BINMODE | O_CREAT | O_TRUNC, 0444);
  TIFF *f_out;
  uint64_t sub_ifds[1] = {0};

  double sensor_iso, capture_iso;
  double gain[3], gain_inv[3], gain_inv_mat[9];
  float as_shot_neutral[3], camera_calibration1[9];
  float black_level[3];
  uint32_t active_area[4];
  x3f_area16_t image;
  x3f_image_levels_t ilevels;
  x3f_area8_t preview;
  int row;

  if (fd == -1) return X3F_OUTFILE_ERROR;
  if (!(f_out = TIFFFdOpen(fd, outfilename, "w"))) {
    close(fd);
    return X3F_OUTFILE_ERROR;
  }

  if (wb == NULL) wb = x3f_get_wb(x3f);
  if (!x3f_get_image(x3f, &image, &ilevels, NONE, 0,
		     fix_bad, denoise, apply_sgain, wb) ||
      image.channels != 3) {
    x3f_printf(ERR, "Could not get image\n");
    TIFFClose(f_out);
    return X3F_ARGUMENT_ERROR;
  }
  if (!x3f_get_preview(x3f, &image, &ilevels, SRGB,
		       apply_sgain, wb, 300, &preview)) {
    x3f_printf(ERR, "Could not get preview\n");
    TIFFClose(f_out);
    free(image.buf);
    return X3F_ARGUMENT_ERROR;
  }

//  TIFFSetField(f_out, TIFFTAG_SUBFILETYPE, FILETYPE_REDUCEDIMAGE);
//  TIFFSetField(f_out, TIFFTAG_IMAGEWIDTH, preview.columns);
//  TIFFSetField(f_out, TIFFTAG_IMAGELENGTH, preview.rows);
//  TIFFSetField(f_out, TIFFTAG_ROWSPERSTRIP, preview.rows);
//  TIFFSetField(f_out, TIFFTAG_SAMPLESPERPIXEL, preview.channels);
//  TIFFSetField(f_out, TIFFTAG_BITSPERSAMPLE, 8);
//  TIFFSetField(f_out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
//  TIFFSetField(f_out, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
//  TIFFSetField(f_out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
  TIFFSetField(f_out, TIFFTAG_ORIENTATION, ORIENTATION_TOPLEFT);
//  TIFFSetField(f_out, TIFFTAG_DNGVERSION, "\001\004\000\000");
//  TIFFSetField(f_out, TIFFTAG_DNGBACKWARDVERSION,
//	       compress ? "\001\004\000\000" : "\001\003\000\000");
//  TIFFSetField(f_out, TIFFTAG_SUBIFD, 1, sub_ifds);

  if (x3f_get_camf_float(x3f, "SensorISO", &sensor_iso) &&
      x3f_get_camf_float(x3f, "CaptureISO", &capture_iso)) {
    double baseline_exposure = log2(capture_iso/sensor_iso);
    TIFFSetField(f_out, TIFFTAG_BASELINEEXPOSURE, baseline_exposure);
  }

//  ret = write_camera_profiles(x3f, wb, camera_profiles,
//			      sizeof(camera_profiles)/sizeof(camera_profile_t),
//			      f_out);
  if (color_profile == PROFILE_EMBED) {
      ret = write_camera_profiles(x3f, wb, camera_profiles,
                        1, // only Default profile
                        f_out);
      if (ret != X3F_OK) {
        x3f_printf(ERR, "Could not write camera profiles\n");
        TIFFClose(f_out);
        free(image.buf);
        free(preview.buf);
        return ret;
      }
  }

  if (!x3f_get_gain(x3f, wb, gain)) {
    x3f_printf(ERR, "Could not get gain for white balance: %s\n", wb);
    TIFFClose(f_out);
    free(image.buf);
    free(preview.buf);
    return X3F_ARGUMENT_ERROR;
  }
  x3f_3x1_invert(gain, gain_inv);
  vec_double_to_float(gain_inv, as_shot_neutral, 3);
  TIFFSetField(f_out, TIFFTAG_ASSHOTNEUTRAL, 3, as_shot_neutral);

#define WB_D65 "Overcast"
  if (!x3f_get_gain(x3f, WB_D65, gain)) {
    x3f_printf(ERR, "Could not get gain for white balance: %s\n", WB_D65);
    TIFFClose(f_out);
    free(image.buf);
    free(preview.buf);
    return X3F_ARGUMENT_ERROR;
  }
  x3f_3x1_invert(gain, gain_inv);
  x3f_3x3_diag(gain_inv, gain_inv_mat);
  vec_double_to_float(gain_inv_mat, camera_calibration1, 9);
  if (color_profile == PROFILE_EMBED) {
      TIFFSetField(f_out, TIFFTAG_CAMERACALIBRATION1, 9, camera_calibration1);
  }

  ret = write_color_profile(x3f, f_out, color_profile);
  if (ret != X3F_OK) {
    x3f_printf(ERR, "Could not write color profile\n");
    TIFFClose(f_out);
    free(image.buf);
    free(preview.buf);
    return ret;
  }
//  for (row=0; row < preview.rows; row++)
//    TIFFWriteScanline(f_out, preview.data + preview.row_stride*row, row, 0);
//
//  TIFFWriteDirectory(f_out);

  TIFFSetField(f_out, TIFFTAG_SUBFILETYPE, 0);
  TIFFSetField(f_out, TIFFTAG_IMAGEWIDTH, image.columns);
  TIFFSetField(f_out, TIFFTAG_IMAGELENGTH, image.rows);
  TIFFSetField(f_out, TIFFTAG_ROWSPERSTRIP, 32);
  TIFFSetField(f_out, TIFFTAG_SAMPLESPERPIXEL, 3);
  TIFFSetField(f_out, TIFFTAG_BITSPERSAMPLE, 16);
  TIFFSetField(f_out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
  TIFFSetField(f_out, TIFFTAG_COMPRESSION,
	       compress ? COMPRESSION_ADOBE_DEFLATE : COMPRESSION_NONE);
  TIFFSetField(f_out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_LINEARRAW);
  TIFFSetField(f_out, TIFFTAG_DNGVERSION, "\001\004\000\000");
  TIFFSetField(f_out, TIFFTAG_DNGBACKWARDVERSION, "\001\004\000\000");
  /* Prevent further chroma denoising in DNG processing software */
  TIFFSetField(f_out, TIFFTAG_CHROMABLURRADIUS, 0.0);

  vec_double_to_float(ilevels.black, black_level, 3);
  TIFFSetField(f_out, TIFFTAG_BLACKLEVEL, 3, black_level);
  TIFFSetField(f_out, TIFFTAG_WHITELEVEL, 3, ilevels.white);

  if (apply_sgain)
    if (!write_spatial_gain(x3f, &image, wb, f_out))
      x3f_printf(WARN, "Could not get spatial gain\n");

  if (get_camf_rect_as_dngrect(x3f, "ActiveImageArea", &image, 1, active_area))
    TIFFSetField(f_out, TIFFTAG_ACTIVEAREA, active_area);

  for (row=0; row < image.rows; row++)
    TIFFWriteScanline(f_out, image.data + image.row_stride*row, row, 0);

  TIFFWriteDirectory(f_out);
  TIFFClose(f_out);
  free(image.buf);
  free(preview.buf);

  return X3F_OK;
}
