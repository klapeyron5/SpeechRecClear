#include <math.h>
#include <stdlib.h>

#include "fe_noise.h"
#include "fe_internal.h"
#include "logged_alloc.h"

/* Noise supression constants */
#define SMOOTH_WINDOW 4
#define LAMBDA_POWER 0.7
#define LAMBDA_A 0.995
#define LAMBDA_B 0.5
#define LAMBDA_T 0.85
#define MU_T 0.2
#define MAX_GAIN 20
#define SLOW_PEAK_FORGET_FACTOR 0.9995
#define SLOW_PEAK_LEARN_FACTOR 0.9
#define SPEECH_VOLUME_RANGE 8.0

struct noise_stats_s {
    /* Smoothed power */
    powspec_t *power;
    /* Noise estimate */
    powspec_t *noise;
    /* Signal floor estimate */
    powspec_t *floor;
    /* Peak for temporal masking */
    powspec_t *peak;

    /* Initialize it next time */
    uint8 undefined;
    /* Number of items to process */
    uint32 num_filters;

    /* Sum of slow peaks for VAD */
    powspec_t slow_peak_sum;

    /* Precomputed constants */
    powspec_t lambda_power;
    powspec_t comp_lambda_power;
    powspec_t lambda_a;
    powspec_t comp_lambda_a;
    powspec_t lambda_b;
    powspec_t comp_lambda_b;
    powspec_t lambda_t;
    powspec_t mu_t;
    powspec_t max_gain;
    powspec_t inv_max_gain;

    powspec_t smooth_scaling[2 * SMOOTH_WINDOW + 3];
};

#ifndef DEFAULT_RADIX
#define DEFAULT_RADIX 12
#endif

/** Fixed-point computation type. */
typedef int32 fixed32;
/** Convert floating point to fixed point. */
#define FLOAT2FIX_ANY(x,radix) \
	(((x)<0.0) ? \
	((fixed32)((x)*(float32)(1<<(radix)) - 0.5)) \
	: ((fixed32)((x)*(float32)(1<<(radix)) + 0.5)))
#define FLOAT2FIX(x) FLOAT2FIX_ANY(x,DEFAULT_RADIX)
/** Convert fixed point to floating point. */
#define FIX2FLOAT_ANY(x,radix) ((float32)(x)/(1<<(radix)))
#define FIX2FLOAT(x) FIX2FLOAT_ANY(x,DEFAULT_RADIX)

/**
 * For fixed point we are doing the computation in a fixlog domain,
 * so we have to add many processing cases.
 */
void fe_track_snr(fe_t * fe, int32 *in_speech) {
    powspec_t *signal;
    powspec_t *gain;
    noise_stats_t *noise_stats;
    powspec_t *mfspec;
    int32 i, num_filts;
    int16 is_quiet;
    powspec_t lrt, snr;

    if (!(fe->remove_noise || fe->remove_silence)) {
        *in_speech = TRUE;
        return;
    }

    noise_stats = fe->noise_stats;
    mfspec = fe->mfspec;
    num_filts = noise_stats->num_filters;

    signal = (powspec_t *) __calloc_log__(num_filts, sizeof(powspec_t));

    if (noise_stats->undefined) {
        noise_stats->slow_peak_sum = FIX2FLOAT(0.0);
        for (i = 0; i < num_filts; i++) {
            noise_stats->power[i] = mfspec[i];
            noise_stats->noise[i] = mfspec[i] / noise_stats->max_gain;
            noise_stats->floor[i] = mfspec[i] / noise_stats->max_gain;
            noise_stats->peak[i] = 0.0;
        }
        noise_stats->undefined = FALSE;
    }

    /* Calculate smoothed power */
    for (i = 0; i < num_filts; i++) {
        noise_stats->power[i] =
            noise_stats->lambda_power * noise_stats->power[i] + noise_stats->comp_lambda_power * mfspec[i];
    }

    /* Noise estimation and vad decision */
    fe_lower_envelope(noise_stats, noise_stats->power, noise_stats->noise, num_filts);

    lrt = FLOAT2FIX(0.0);
    for (i = 0; i < num_filts; i++) {
        signal[i] = noise_stats->power[i] - noise_stats->noise[i];
        if (signal[i] < 1.0)
            signal[i] = 1.0;
        snr = log(noise_stats->power[i] / noise_stats->noise[i]);
        if (snr > lrt)
            lrt = snr;
    }
    is_quiet = fe_is_frame_quiet(noise_stats, signal, num_filts);

    if (fe->remove_silence && (lrt < fe->vad_threshold || is_quiet)) {
        *in_speech = FALSE;
    } else {
        *in_speech = TRUE;
    }

    fe_lower_envelope(noise_stats, signal, noise_stats->floor, num_filts);

    fe_temp_masking(noise_stats, signal, noise_stats->peak, num_filts);

    if (!fe->remove_noise) {
        /* no need for further calculations if noise cancellation disabled */
        free(signal);
        return;
    }

    for (i = 0; i < num_filts; i++) {
        if (signal[i] < noise_stats->floor[i])
            signal[i] = noise_stats->floor[i];
    }

    gain = (powspec_t *) __calloc_log__(num_filts, sizeof(powspec_t));
#ifndef FIXED_POINT
    for (i = 0; i < num_filts; i++) {
        if (signal[i] < noise_stats->max_gain * noise_stats->power[i])
            gain[i] = signal[i] / noise_stats->power[i];
        else
            gain[i] = noise_stats->max_gain;
        if (gain[i] < noise_stats->inv_max_gain)
            gain[i] = noise_stats->inv_max_gain;
    }
#else
    for (i = 0; i < num_filts; i++) {
        gain[i] = signal[i] - noise_stats->power[i];
        if (gain[i] > noise_stats->max_gain)
            gain[i] = noise_stats->max_gain;
        if (gain[i] < noise_stats->inv_max_gain)
            gain[i] = noise_stats->inv_max_gain;
    }
#endif

    /* Weight smoothing and time frequency normalization */
    fe_weight_smooth(noise_stats, mfspec, gain, num_filts);

    free(gain);
    free(signal);
}

static void fe_lower_envelope(noise_stats_t *noise_stats, powspec_t * buf, powspec_t * floor_buf, int32 num_filt) {
    int i;
    for (i = 0; i < num_filt; i++) {
        if (buf[i] >= floor_buf[i]) {
            floor_buf[i] =
                noise_stats->lambda_a * floor_buf[i] + noise_stats->comp_lambda_a * buf[i];
        }
        else {
            floor_buf[i] =
                noise_stats->lambda_b * floor_buf[i] + noise_stats->comp_lambda_b * buf[i];
        }
    }
}

/* update slow peaks, check if max signal level big enough compared to peak */
static int16 fe_is_frame_quiet(noise_stats_t *noise_stats, powspec_t *buf, int32 num_filt) {
    int i;
    int16 is_quiet;
    powspec_t sum;
    double smooth_factor;

    sum = 0.0;
    for (i = 0; i < num_filt; i++) {
        sum += buf[i];
    }
    sum = log(sum);
    smooth_factor = (sum > noise_stats->slow_peak_sum) ? SLOW_PEAK_LEARN_FACTOR : SLOW_PEAK_FORGET_FACTOR;
    noise_stats->slow_peak_sum = noise_stats->slow_peak_sum * smooth_factor +
                                 sum * (1 - smooth_factor);
    is_quiet = noise_stats->slow_peak_sum - SPEECH_VOLUME_RANGE > sum;
    return is_quiet;
}

/* temporal masking */
static void fe_temp_masking(noise_stats_t *noise_stats, powspec_t * buf, powspec_t * peak, int32 num_filt) {
    powspec_t cur_in;
    int i;

    for (i = 0; i < num_filt; i++) {
        cur_in = buf[i];

        peak[i] *= noise_stats->lambda_t;
        if (buf[i] < noise_stats->lambda_t * peak[i])
            buf[i] = peak[i] * noise_stats->mu_t;

        if (cur_in > peak[i])
            peak[i] = cur_in;
    }
}

/* spectral weight smoothing */
static void fe_weight_smooth(noise_stats_t *noise_stats, powspec_t * buf, powspec_t * coefs, int32 num_filt) {
    int i, j;
    int l1, l2;
    powspec_t coef;

    for (i = 0; i < num_filt; i++) {
        l1 = ((i - SMOOTH_WINDOW) > 0) ? (i - SMOOTH_WINDOW) : 0;
        l2 = ((i + SMOOTH_WINDOW) <
              (num_filt - 1)) ? (i + SMOOTH_WINDOW) : (num_filt - 1);

        coef = 0;
        for (j = l1; j <= l2; j++) {
            coef += coefs[j];
        }
        buf[i] = buf[i] * (coef / (l2 - l1 + 1));
    }
}