/* X3F_HIGHLIGHT_RECOVERY.H
 *
 * Library for highlight recovery to prevent color casts in overexposed areas.
 *
 * Copyright 2024
 * BSD-style - see doc/copyright.txt
 *
 */

#ifndef X3F_HIGHLIGHT_RECOVERY_H
#define X3F_HIGHLIGHT_RECOVERY_H

#include "x3f_io.h"

/* Apply highlight recovery to prevent color casts in burned highlights
 *
 * saturation_factors: Per-layer saturation factors (e.g., {1.0, 0.92, 0.85} for Foveon)
 *                     representing the physical layer saturation characteristics.
 *                     Pass NULL to disable layer-aware recovery.
 */
typedef struct {
    double recovery_start_threshold;   /* Normalized value to start highlight blending */
    double recovery_full_threshold;    /* Normalized value where blending reaches 100% */
    double complete_clip_threshold;    /* Normalized value treated as fully clipped */
    double neutral_ratio_soft;         /* Min/max ratio where neutralization begins */
    double neutral_ratio_hard;         /* Ratio where neutralization is forced */
    double max_channel_value;          /* Maximum normalized raw value allowed post-recovery */
} x3f_highlight_profile_t;

extern void x3f_highlight_profile_default(x3f_highlight_profile_t *profile);

extern int x3f_recover_highlights(x3f_area16_t *image,
                                 uint32_t *max_raw,
                                 double *black_level,
                                 double *wb_gains,
                                 double *saturation_factors,
                                 int colors,
                                 const x3f_highlight_profile_t *profile);

#endif
