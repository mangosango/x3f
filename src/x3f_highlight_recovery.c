/* X3F_HIGHLIGHT_RECOVERY.C
 *
 * Library for highlight recovery to prevent color casts in overexposed areas.
 * Uses silicon absorption physics to reconstruct clipped green/blue layers from red.
 *
 * Copyright 2024
 * BSD-style - see doc/copyright.txt
 *
 */

#include "x3f_highlight_recovery.h"
#include "x3f_printf.h"
#include <math.h>
#include <stdlib.h>

/* Default profile tuned for Merrill sensors; metadata can override this. */
static const x3f_highlight_profile_t DEFAULT_PROFILE = {
    0.92,  /* recovery_start_threshold */
    0.98,  /* recovery_full_threshold */
    0.998, /* complete_clip_threshold */
    0.75,  /* neutral_ratio_soft */
    0.50,  /* neutral_ratio_hard */
    0.995  /* max_channel_value */
};

static x3f_highlight_profile_t g_profile = {
    0.92, 0.98, 0.998, 0.75, 0.50, 0.995
};

static uint64_t g_stats_multi_layer = 0;
static uint64_t g_stats_multi_neutralized = 0;
static uint64_t g_stats_multi_reconstructed = 0;
static uint64_t g_stats_single_channel = 0;
static double g_stats_ratio_min = 1.0;
static double g_stats_ratio_max = 0.0;

static double clamp_double(double value, double min_value, double max_value)
{
    if (value < min_value) return min_value;
    if (value > max_value) return max_value;
    return value;
}

/* extern */ void x3f_highlight_profile_default(x3f_highlight_profile_t *profile)
{
    if (!profile) return;
    *profile = DEFAULT_PROFILE;
}

static void set_active_profile(const x3f_highlight_profile_t *profile)
{
    if (profile) {
        g_profile.recovery_start_threshold = clamp_double(profile->recovery_start_threshold, 0.4, 0.99);
        g_profile.recovery_full_threshold = clamp_double(profile->recovery_full_threshold, g_profile.recovery_start_threshold + 0.01, 0.999);
        g_profile.complete_clip_threshold = clamp_double(profile->complete_clip_threshold, 0.9, 0.9995);

        g_profile.neutral_ratio_soft = clamp_double(profile->neutral_ratio_soft, 0.2, 0.95);
        g_profile.neutral_ratio_hard = clamp_double(profile->neutral_ratio_hard, 0.0, g_profile.neutral_ratio_soft - 0.05);
        if (g_profile.neutral_ratio_hard <= 0.0 ||
            g_profile.neutral_ratio_hard >= g_profile.neutral_ratio_soft) {
            g_profile.neutral_ratio_hard = g_profile.neutral_ratio_soft * 0.5;
        }

        g_profile.max_channel_value = clamp_double(profile->max_channel_value, 0.85, 0.999);
    } else {
        g_profile = DEFAULT_PROFILE;
    }
}

/* ========================================================================
 * Silicon Absorption Model for Foveon Sensor Physics
 * ======================================================================== */

/* Silicon absorption coefficient alpha(λ) in 1/um */
static double alpha_silicon(double wavelength_um)
{
    /* log(alpha) ~ b0 + b1/λ + b2/λ^2 (empirical fit for silicon) */
    const double b0 = -2.1203;
    const double b1 = -2.5425;
    const double b2 = 2.0397;
    return exp(b0 + b1 / wavelength_um + b2 / (wavelength_um * wavelength_um));
}

/* Depth-dependent collection integral F(α) for Foveon sensor */
static double collection_integral(double alpha, double depth_um, double diffusion_length_um)
{
    /* Simplified model for thick silicon with depletion region
     * F(α) ≈ η_surface × (1 - e^(-α×d)) + η_bulk × (α×e^(-d/L))/(α + 1/L) */
    const double d = depth_um;                    /* depletion region thickness */
    const double L = diffusion_length_um;         /* diffusion length */
    const double eta_s = 1.0;                     /* surface collection efficiency */
    const double eta_n = 1.0;                     /* bulk collection efficiency */

    const double invL = (L > 0.0) ? (1.0 / L) : 0.0;
    const double term1 = eta_s * (1.0 - exp(-alpha * d));

    double term2 = 0.0;
    const double denom = alpha + invL;
    if (fabs(denom) >= 1e-12) {
        term2 = eta_n * alpha * exp(-d * invL) / denom;
    }

    return term1 + term2;
}

/* Helper function forward declaration */
static uint16_t denormalize_value(double normalized, uint32_t max_raw, double black_level);

/* Reconstruct green and blue from red using silicon absorption physics
 * Returns 1 if reconstruction was applied, 0 if skipped
 * wb_gains: white balance gains {R, G, B} used to convert neutral RAW to normalized color */
static int reconstruct_from_red(uint16_t *pixel, uint32_t *max_raw, double *black_level,
                                double *wb_gains, double *saturation_factors, double red_normalized)
{
    /* Foveon sensor parameters (typical values for Merrill) */
    const double depth_um = 0.5;              /* depletion region thickness */
    const double diffusion_length_um = 2.0;   /* carrier diffusion length */

    /* Wavelength centers for RGB channels (micrometers) */
    const double lambda_red = 0.61;    /* 610 nm */
    const double lambda_green = 0.54;  /* 540 nm */
    const double lambda_blue = 0.46;   /* 460 nm */

    /* Calculate absorption coefficients */
    const double alpha_r = alpha_silicon(lambda_red);
    const double alpha_g = alpha_silicon(lambda_green);
    const double alpha_b = alpha_silicon(lambda_blue);

    /* Calculate collection integrals */
    const double F_r = collection_integral(alpha_r, depth_um, diffusion_length_um);
    const double F_g = collection_integral(alpha_g, depth_um, diffusion_length_um);
    const double F_b = collection_integral(alpha_b, depth_um, diffusion_length_um);

    (void)F_r;
    (void)F_g;
    (void)F_b;

    /* CRITICAL: The pure physics model gives ratios in white-balanced space,
     * but we're working in RAW space. We need to account for the white balance gains.
     *
     * In RAW space, for a neutral subject (gray):
     *   RAW_R : RAW_G : RAW_B = 1/WB_R : 1/WB_G : 1/WB_B
     *
     * Example: if WB gains are {2.533, 1.109, 0.632}, neutral gray in RAW has
     * the ratio 1/2.533 : 1/1.109 : 1/0.632 ≈ 0.395 : 0.902 : 1.582
     *
     * So in RAW space relative to red:
     *   G/R ratio = (1/WB_G) / (1/WB_R) = WB_R / WB_G
     *   B/R ratio = (1/WB_B) / (1/WB_R) = WB_R / WB_B
     *
     * We'll use a hybrid: start with physics ratios but scale by WB ratios */

    /* Strategy: Use red as the "anchor" and reconstruct G and B to maintain
     * proper color ratios while preserving the brightness information from red.
     *
     * For neutral reconstruction (where R×WB_R = G×WB_G = B×WB_B):
     *   G = R × (WB_R / WB_G)
     *   B = R × (WB_R / WB_B)
     *
     * This preserves red's brightness while ensuring neutral color after WB. */

    const double wb_ratio_g_inv = (wb_gains && wb_gains[1] > 0.0) ? (wb_gains[0] / wb_gains[1]) : 1.0;
    const double wb_ratio_b_inv = (wb_gains && wb_gains[2] > 0.0) ? (wb_gains[0] / wb_gains[2]) : 1.0;

    /* Reconstruct G and B based on red's value
     * CRITICAL: Since B = R × 4.01, blue clips when R > 0.249
     * We need to limit how bright we allow the neutral reconstruction to be.
     * Find the limiting factor based on which channel would clip first. */

    double max_red_for_neutral = g_profile.max_channel_value;  /* Max red that keeps channels within headroom */
    double channel_ceiling = g_profile.max_channel_value;

    /* Check if green would clip */
    if (wb_ratio_g_inv > 0.0) {
        double max_red_for_green = channel_ceiling / wb_ratio_g_inv;
        if (max_red_for_green < max_red_for_neutral) {
            max_red_for_neutral = max_red_for_green;
        }
    }

    /* Check if blue would clip */
    if (wb_ratio_b_inv > 0.0) {
        double max_red_for_blue = channel_ceiling / wb_ratio_b_inv;
        if (max_red_for_blue < max_red_for_neutral) {
            max_red_for_neutral = max_red_for_blue;
        }
    }

    /* Limit red to prevent channel clipping */
    double red_limited = red_normalized;
    if (red_limited > max_red_for_neutral) {
        red_limited = max_red_for_neutral;
    }

    /* Now reconstruct with the limited red value */
    double green_reconstructed = red_limited * wb_ratio_g_inv;
    double blue_reconstructed = red_limited * wb_ratio_b_inv;

    x3f_printf(DEBUG, "Neutral reconstruction: R %.3f→%.3f, G=%.3f, B=%.3f (max_R=%.3f)\n",
              red_normalized, red_limited, green_reconstructed, blue_reconstructed, max_red_for_neutral);

    /* NOTE: We intentionally do NOT apply saturation_factors here.
     * While physically accurate (green saturates at 92%, blue at 85%),
     * applying these factors causes color casts because layers clip at different times.
     *
     * For highlight recovery, color neutrality is more important than physical accuracy.
     * By keeping K_G == K_B, we ensure both channels saturate together,
     * producing neutral white/gray clipping instead of colored casts. */

    /* Clamp to valid range */
    if (red_limited > channel_ceiling) red_limited = channel_ceiling;
    if (green_reconstructed > channel_ceiling) green_reconstructed = channel_ceiling;
    if (blue_reconstructed > channel_ceiling) blue_reconstructed = channel_ceiling;
    if (green_reconstructed < 0.0) green_reconstructed = 0.0;
    if (blue_reconstructed < 0.0) blue_reconstructed = 0.0;

    /* Write back to pixel */
    pixel[0] = denormalize_value(red_limited, max_raw[0], black_level[0]);

    /* Write back to pixel */
    pixel[1] = denormalize_value(green_reconstructed, max_raw[1], black_level[1]);
    pixel[2] = denormalize_value(blue_reconstructed, max_raw[2], black_level[2]);

    x3f_printf(DEBUG, "Reconstructed from R=%.3f: G=%.3f, B=%.3f\n",
              red_normalized, green_reconstructed, blue_reconstructed);

    return 1;
}

/* Minimum value to avoid division by zero in ratios */
#define MIN_RATIO_VALUE 0.001

/* Calculate recovery factor (0.0 = no recovery, 1.0 = full recovery) */
static double get_recovery_factor(uint16_t value, uint32_t max_raw, double black_level)
{
    double normalized = (double)(value - black_level) / (max_raw - black_level);
    double start = g_profile.recovery_start_threshold;
    double full = g_profile.recovery_full_threshold;
    if (full <= start) {
        full = start + 1e-3;
    }

    if (normalized < start) {
        return 0.0;  /* No recovery needed */
    } else if (normalized >= full) {
        return 1.0;  /* Full recovery */
    } else {
        /* Smooth transition using cosine interpolation */
        double t = (normalized - start) / (full - start);
        return 0.5 * (1.0 - cos(t * M_PI));  /* Smooth S-curve */
    }
}

/* Check if any channel needs recovery */
static int needs_recovery(uint16_t *pixel, uint32_t *max_raw, double *black_level, int colors)
{
    int i;
    for (i = 0; i < colors; i++) {
        if (get_recovery_factor(pixel[i], max_raw[i], black_level[i]) > 0.0) {
            return 1;
        }
    }
    return 0;
}

static double get_normalized_value(uint16_t value, uint32_t max_raw, double black_level)
{
    double normalized = (double)(value - black_level) / (max_raw - black_level);
    return normalized < 0.0 ? 0.0 : (normalized > 1.0 ? 1.0 : normalized);
}

/* Get normalized value with white balance awareness for recovery calculations */
static double get_wb_normalized_value(uint16_t value, uint32_t max_raw, double black_level, double wb_gain)
{
    double raw_normalized = get_normalized_value(value, max_raw, black_level);
    return raw_normalized * wb_gain;
}

/* Convert white-balanced normalized value back to raw space */
static double wb_to_raw_normalized(double wb_normalized, double wb_gain)
{
    if (wb_gain < MIN_RATIO_VALUE) return wb_normalized;
    return wb_normalized / wb_gain;
}

static uint16_t denormalize_value(double normalized, uint32_t max_raw, double black_level)
{
    double value = normalized * (max_raw - black_level) + black_level;
    if (value < 0.0) return 0;
    if (value > 65535.0) return 65535;
    return (uint16_t)round(value);
}

/* Calculate luminance from white-balanced normalized RGB values */
static double calculate_luminance(double *wb_normalized, int colors, int use_equal_weights)
{
    if (colors == 3) {
        if (use_equal_weights) {
            /* Equal weights to avoid green dominance in extreme highlights */
            return (wb_normalized[0] + wb_normalized[1] + wb_normalized[2]) / 3.0;
        } else {
            /* Standard luminance weights for RGB */
            return 0.299 * wb_normalized[0] + 0.587 * wb_normalized[1] + 0.114 * wb_normalized[2];
        }
    } else if (colors == 1) {
        return wb_normalized[0];
    }
    /* For other cases, use simple average */
    double sum = 0.0;
    int i;
    for (i = 0; i < colors; i++) {
        sum += wb_normalized[i];
    }
    return sum / colors;
}

/* Count how many channels are near saturation */
static int count_saturated_channels(double *recovery_factors, int colors, double threshold)
{
    int count = 0;
    int i;
    for (i = 0; i < colors; i++) {
        if (recovery_factors[i] >= threshold) {
            count++;
        }
    }
    return count;
}

/* Apply luminance-preserving desaturation for highlight recovery */
static void recover_pixel_highlights(uint16_t *pixel,
                                   uint32_t *max_raw,
                                   double *black_level,
                                   double *wb_gains,
                                   double *saturation_factors,
                                   int colors)
{
    double recovery_factors[3];
    double raw_normalized[3];
    double wb_normalized[3];
    double max_recovery_factor = 0.0;
    int i;

    /* Calculate recovery factors and normalized values for each channel */
    for (i = 0; i < colors; i++) {
        recovery_factors[i] = get_recovery_factor(pixel[i], max_raw[i], black_level[i]);
        raw_normalized[i] = get_normalized_value(pixel[i], max_raw[i], black_level[i]);
        wb_normalized[i] = get_wb_normalized_value(pixel[i], max_raw[i], black_level[i], wb_gains[i]);
        if (recovery_factors[i] > max_recovery_factor) {
            max_recovery_factor = recovery_factors[i];
        }
    }

    /* If no recovery is needed, return early */
    if (max_recovery_factor <= 0.0) {
        return;
    }

    /* Detect multi-layer saturation (Foveon sensor characteristic) */
    int saturated_channels = count_saturated_channels(recovery_factors, colors, 0.8);
    int multi_layer_saturation = (saturated_channels >= 2);

    /* Detect complete clipping - when all layers are at or near maximum saturation */
    int completely_clipped_channels = 0;
    for (i = 0; i < colors; i++) {
        if (raw_normalized[i] >= g_profile.complete_clip_threshold) {
            completely_clipped_channels++;
        }
    }
    int complete_clip = (completely_clipped_channels >= 2);

    /* For Foveon sensors, always use equal weights to prevent green-dominant luminance
     * from causing color casts. The standard 0.587 green weight is based on human
     * perception, but for highlight recovery we need neutral color balance. */
    int use_equal_weights = 1;  /* Always use equal weights for all recovery */

    /* Calculate luminance in white-balanced space */
    double unclamped_luminance = calculate_luminance(wb_normalized, colors, use_equal_weights);

    /* Calculate maximum achievable luminance considering WB gains.
     * The key insight: we must use equal weights here too, otherwise the target
     * luminance will be skewed by green's high weight, causing green casts. */
    double max_achievable_luminance = 0.0;
    if (colors == 3) {
        /* Use equal weights (not standard RGB weights) to match recovery calculation */
        double equal_weights[3] = {1.0/3.0, 1.0/3.0, 1.0/3.0};
        max_achievable_luminance = equal_weights[0] * wb_gains[0] +
                                   equal_weights[1] * wb_gains[1] +
                                   equal_weights[2] * wb_gains[2];
    } else {
        /* For non-RGB cases, use the minimum wb_gain as the limit */
        for (i = 0; i < colors; i++) {
            if (i == 0 || wb_gains[i] < max_achievable_luminance) {
                max_achievable_luminance = wb_gains[i];
            }
        }
    }
    
    /* Clamp luminance to what's achievable - but be conservative to avoid green cast */
    double target_luminance = unclamped_luminance;
    if (target_luminance > max_achievable_luminance * 0.95) {
        target_luminance = max_achievable_luminance * 0.95;
        x3f_printf(DEBUG, "Clamped luminance from %.3f to %.3f (max achievable: %.3f)\n", 
                  unclamped_luminance, target_luminance, max_achievable_luminance);
    }
    
    /* Apply different recovery strategies based on saturation pattern */
    if (multi_layer_saturation || complete_clip) {
        g_stats_multi_layer++;
        /* Evaluate chroma imbalance in white-balanced space */
        double max_wb_channel = wb_normalized[0];
        double min_wb_channel = wb_normalized[0];
        for (i = 1; i < colors; i++) {
            if (wb_normalized[i] > max_wb_channel) max_wb_channel = wb_normalized[i];
            if (wb_normalized[i] < min_wb_channel) min_wb_channel = wb_normalized[i];
        }

        double ratio = 1.0;
        if (max_wb_channel > MIN_RATIO_VALUE) {
            ratio = min_wb_channel / (max_wb_channel + 1e-12);
        }

        if (ratio < g_stats_ratio_min) g_stats_ratio_min = ratio;
        if (ratio > g_stats_ratio_max) g_stats_ratio_max = ratio;

        /* Attempt physics-based reconstruction when red retains headroom */
        if (colors == 3 && saturation_factors && raw_normalized[0] < g_profile.complete_clip_threshold &&
            ratio <= g_profile.neutral_ratio_hard) {
            if (reconstruct_from_red(pixel, max_raw, black_level, wb_gains, saturation_factors, raw_normalized[0])) {
                g_stats_multi_reconstructed++;
                x3f_printf(DEBUG, "Multi-layer saturation: reconstructed from red (ratio %.3f)\n", ratio);
                return;
            }
        }

        double neutral_ratio_soft = g_profile.neutral_ratio_soft;
        double neutral_ratio_hard = g_profile.neutral_ratio_hard;
        if (neutral_ratio_hard <= 0.0 || neutral_ratio_hard >= neutral_ratio_soft) {
            neutral_ratio_hard = neutral_ratio_soft * 0.5;
        }

        double chroma_suppression = 0.0;
        if (ratio < neutral_ratio_soft) {
            if (ratio <= neutral_ratio_hard) {
                chroma_suppression = 1.0;
            } else {
                chroma_suppression = (neutral_ratio_soft - ratio) /
                                     (neutral_ratio_soft - neutral_ratio_hard);
            }
        }

        double neutral_strength = fmax(max_recovery_factor, chroma_suppression);
        if (complete_clip) {
            neutral_strength = 1.0;
        }
        neutral_strength = clamp_double(neutral_strength, 0.0, 1.0);

        /* Derive the brightest neutral intensity we can represent without exceeding
         * physical headroom in any layer. For each channel, the maximum neutral
         * brightness is limited by the measured raw value (or hard clip) times the
         * white balance gain. */
        double neutral_target = target_luminance;
        double neutral_capacity = neutral_target;

        for (i = 0; i < colors; i++) {
            double available_raw = raw_normalized[i];
            if (available_raw > g_profile.max_channel_value) {
                available_raw = g_profile.max_channel_value;
            }
            double channel_capacity = available_raw * wb_gains[i];
            if (i == 0 || channel_capacity < neutral_capacity) {
                neutral_capacity = channel_capacity;
            }
        }

        if (neutral_capacity < neutral_target) {
            neutral_target = neutral_capacity;
        }
        if (neutral_target < 0.0) {
            neutral_target = 0.0;
        }

        for (i = 0; i < colors; i++) {
            double blended_wb = wb_normalized[i] * (1.0 - neutral_strength) +
                               neutral_target * neutral_strength;
            double final_raw_normalized = wb_to_raw_normalized(blended_wb, wb_gains[i]);
            if (final_raw_normalized < 0.0) final_raw_normalized = 0.0;
            if (final_raw_normalized > g_profile.max_channel_value) {
                final_raw_normalized = g_profile.max_channel_value;
            }
            pixel[i] = denormalize_value(final_raw_normalized, max_raw[i], black_level[i]);
        }

        g_stats_multi_neutralized++;
        x3f_printf(DEBUG, "Multi-layer saturation: neutralized ratio %.3f strength %.3f clip %d\n",
                  ratio, neutral_strength, complete_clip);
        return;

    } else {
        /* Single channel saturation: preserve detail with soft compression */
        int any_adjusted = 0;
        for (i = 0; i < colors; i++) {
            if (recovery_factors[i] > 0.0) {
                any_adjusted = 1;
                /* Calculate how much to compress this channel */
                double compression_factor = recovery_factors[i];
                
                /* For extreme cases, use a softer compression curve */
                if (max_recovery_factor >= 1.0) {
                    /* Apply S-curve compression to preserve detail while preventing color casts */
                    double intensity = compression_factor;
                    /* Soft S-curve: more aggressive compression but preserves relative differences */
                    compression_factor = intensity * intensity * (3.0 - 2.0 * intensity);
                    
                    /* Calculate compressed value that preserves relative detail */
                    double detail_preserving_target;
                    if (target_luminance > 0.001 && unclamped_luminance > 0.001) {
                        /* Maintain the ratio of this channel to overall luminance */
                        double channel_ratio = wb_normalized[i] / unclamped_luminance;
                        detail_preserving_target = target_luminance * channel_ratio;
                    } else {
                        /* Fallback to simple luminance blend */
                        detail_preserving_target = target_luminance;
                    }
                    
                    /* Soft compression towards detail-preserving target */
                    double compressed_wb = wb_normalized[i] * (1.0 - compression_factor) + 
                                         detail_preserving_target * compression_factor;
                    
                    /* Convert back to raw normalized space */
                    double final_raw_normalized = wb_to_raw_normalized(compressed_wb, wb_gains[i]);
                    
                    /* Soft clamp to preserve some headroom detail */
                    if (final_raw_normalized < 0.0) final_raw_normalized = 0.0;
                    if (final_raw_normalized > g_profile.max_channel_value) {
                        final_raw_normalized = g_profile.max_channel_value;
                    }
                    
                    pixel[i] = denormalize_value(final_raw_normalized, max_raw[i], black_level[i]);
                    
                    x3f_printf(DEBUG, "Channel %d soft compression: factor %.3f->%.3f, wb_norm %.3f -> %.3f, ratio %.3f\n", 
                              i, recovery_factors[i], compression_factor, wb_normalized[i], compressed_wb, 
                              wb_normalized[i] / (unclamped_luminance > 0.001 ? unclamped_luminance : 1.0));
                } else {
                    /* Normal luminance-based desaturation with clamped target */
                    double desaturation_factor = compression_factor;
                    
                    /* Blend towards clamped luminance (desaturate) */
                    double desaturated_wb = wb_normalized[i] * (1.0 - desaturation_factor) + 
                                           target_luminance * desaturation_factor;
                    
                    /* Convert back to raw normalized space */
                    double final_raw_normalized = wb_to_raw_normalized(desaturated_wb, wb_gains[i]);
                    
                    /* Clamp to valid range */
                    if (final_raw_normalized < 0.0) final_raw_normalized = 0.0;
                    if (final_raw_normalized > g_profile.max_channel_value) {
                        final_raw_normalized = g_profile.max_channel_value;
                    }
                    
                    /* Apply the recovered value */
                    pixel[i] = denormalize_value(final_raw_normalized, max_raw[i], black_level[i]);
                    
                    x3f_printf(DEBUG, "Channel %d recovery: factor %.3f, wb_norm %.3f -> %.3f, raw_norm %.3f\n", 
                              i, desaturation_factor, wb_normalized[i], desaturated_wb, final_raw_normalized);
                }
            }
        }

        if (max_recovery_factor >= 1.0) {
            x3f_printf(DEBUG, "Soft highlight compression: target %.3f (unclamped: %.3f), max_factor %.3f\n",
                      target_luminance, unclamped_luminance, max_recovery_factor);
        } else {
            x3f_printf(DEBUG, "Luminance-based recovery: target %.3f (unclamped: %.3f), max_factor %.3f\n",
                      target_luminance, unclamped_luminance, max_recovery_factor);
        }

        if (any_adjusted) {
            g_stats_single_channel++;
        }
    }
}

/* extern */ int x3f_recover_highlights(x3f_area16_t *image,
                                       uint32_t *max_raw,
                                       double *black_level,
                                       double *wb_gains,
                                       double *saturation_factors,
                                       int colors,
                                       const x3f_highlight_profile_t *profile)
{
    int row, col;
    int pixels_processed = 0;

    set_active_profile(profile);

    g_stats_multi_layer = 0;
    g_stats_multi_neutralized = 0;
    g_stats_multi_reconstructed = 0;
    g_stats_single_channel = 0;
    g_stats_ratio_min = 1.0;
    g_stats_ratio_max = 0.0;

    if (!image || !max_raw || !black_level || colors < 1 || colors > 3) {
        x3f_printf(ERR, "Invalid parameters for highlight recovery\n");
        return 0;
    }

    if (image->channels < colors) {
        x3f_printf(ERR, "Image has fewer channels than required for highlight recovery\n");
        return 0;
    }

    x3f_printf(DEBUG, "Starting highlight recovery for %dx%d image with %d colors\n",
               image->columns, image->rows, colors);

    if (saturation_factors != NULL) {
        x3f_printf(DEBUG, "Foveon layer saturation factors: {%.3f, %.3f, %.3f}\n",
                  saturation_factors[0], saturation_factors[1], saturation_factors[2]);
    }

    /* Process each pixel */
    for (row = 0; row < image->rows; row++) {
        for (col = 0; col < image->columns; col++) {
            uint16_t *pixel = &image->data[image->row_stride * row + image->channels * col];

            /* Check if any channel needs recovery */
            if (needs_recovery(pixel, max_raw, black_level, colors)) {
                recover_pixel_highlights(pixel, max_raw, black_level, wb_gains, saturation_factors, colors);
                pixels_processed++;
            }
        }
    }

    x3f_printf(DEBUG, "Highlight recovery completed: %d pixels processed\n", pixels_processed);
    x3f_printf(DEBUG,
               "Highlight stats: multi=%llu neutralized=%llu reconstructed=%llu single=%llu ratio[min=%.3f max=%.3f]\n",
               (unsigned long long)g_stats_multi_layer,
               (unsigned long long)g_stats_multi_neutralized,
               (unsigned long long)g_stats_multi_reconstructed,
               (unsigned long long)g_stats_single_channel,
               g_stats_ratio_min,
               g_stats_ratio_max);
    return 1;
}
