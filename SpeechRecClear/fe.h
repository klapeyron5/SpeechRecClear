#include "prim_type.h"

/** Structure for the front-end computation. */
struct fe_s {
 //   cmd_ln_t *config;
    int refcount;

    float32 sampling_rate;
    int16 frame_rate;
    int16 frame_shift;

    float32 window_length;
    int16 frame_size;
    int16 fft_size;

    uint8 fft_order;
    uint8 feature_dimension;
    uint8 num_cepstra;
    uint8 remove_dc;
    uint8 log_spec;
    uint8 swap;
    uint8 dither;
    uint8 transform;
    uint8 remove_noise;
    uint8 remove_silence;

    float32 pre_emphasis_alpha;
    int32 seed;

    size_t sample_counter;
    uint8 start_flag;
    uint8 reserved;

    /* Twiddle factors for FFT. */
    frame_t *ccc, *sss;
    /* Mel filter parameters. */
    melfb_t *mel_fb;
    /* Half of a Hamming Window. */
    window_t *hamming_window;

    /* Noise removal  */
    noise_stats_t *noise_stats;

    /* VAD variables */
    int16 pre_speech;
    int16 post_speech;
    int16 start_speech;
    float32 vad_threshold;
    vad_data_t *vad_data;

    /* Temporary buffers for processing. */
    /* FIXME: too many of these. */
    int16 *spch;
    frame_t *frame;
    powspec_t *spec, *mfspec;
    int16 *overflow_samps;
    int16 num_overflow_samps;
    int16 prior;
};

typedef struct vad_data_s {
    uint8 in_speech;
    int16 pre_speech_frames;
    int16 post_speech_frames;
    prespch_buf_t* prespch_buf;
} vad_data_t;

/* Values for the 'logspec' field. */
enum {
	RAW_LOG_SPEC = 1,
	SMOOTH_LOG_SPEC = 2
};

typedef struct melfb_s melfb_t;
/** Base Struct to hold all structure for MFCC computation. */
struct melfb_s {
    float32 sampling_rate;
    int32 num_cepstra;
    int32 num_filters;
    int32 fft_size;
    float32 lower_filt_freq;
    float32 upper_filt_freq;
    /* DCT coefficients. */
    mfcc_t **mel_cosine;
    /* Filter coefficients. */
    mfcc_t *filt_coeffs;
    int16 *spec_start;
    int16 *filt_start;
    int16 *filt_width;
    /* Luxury mobile home. */
    int32 doublewide;
    char const *warp_type;
    char const *warp_params;
    uint32 warp_id;
    /* Precomputed normalization constants for unitary DCT-II/DCT-III */
    mfcc_t sqrt_inv_n, sqrt_inv_2n;
    /* Value and coefficients for HTK-style liftering */
    int32 lifter_val;
    mfcc_t *lifter;
    /* Normalize filters to unit area */
    int32 unit_area;
    /* Round filter frequencies to DFT points (hurts accuracy, but is
       useful for legacy purposes) */
    int32 round_filters;
};

/* sqrt(1/2), also used for unitary DCT-II/DCT-III */
#define SQRT_HALF FLOAT2MFCC(0.707106781186548)

/* Values for the 'transform' field. */
enum {
	LEGACY_DCT = 0,
	DCT_II = 1,
        DCT_HTK = 2
};

/**
 * Structure for the front-end computation.
 */
typedef struct fe_s fe_t;

/** MFCC computation type. */
typedef float32 mfcc_t;

typedef float64 frame_t;
typedef float64 powspec_t;
typedef float64 window_t;

/** Convert a floating-point value to mfcc_t. */
#define FLOAT2MFCC(x) (x)
/** Multiply two mfcc_t values. */
#define MFCCMUL(a,b) ((a)*(b))

/**
 * Start processing an utterance.
 * @return 0 for success, <0 for error (see enum fe_error_e)
 */
int fe_start_utt(fe_t *fe);

/** 
 * Process a block of samples.
 *
 * This function generates up to <code>*inout_nframes</code> of
 * features, or as many as can be generated from
 * <code>*inout_nsamps</code> samples.
 *
 * On exit, the <code>inout_spch</code>, <code>inout_nsamps</code>,
 * and <code>inout_nframes</code> parameters are updated to point to
 * the remaining sample data, the number of remaining samples, and the
 * number of frames processed, respectively.  This allows you to call
 * this repeatedly to process a large block of audio in small (say,
 * 5-frame) chunks:
 *
 *  int16 *bigbuf, *p;
 *  mfcc_t **cepstra;
 *  int32 nsamps;
 *  int32 nframes = 5;
 *
 *  cepstra = (mfcc_t **)
 *      ckd_calloc_2d(nframes, fe_get_output_size(fe), sizeof(**cepstra));
 *  p = bigbuf;
 *  while (nsamps) {
 *      nframes = 5;
 *      fe_process_frames(fe, &p, &nsamps, cepstra, &nframes);
 *      // Now do something with these frames...
 *      if (nframes)
 *          do_some_stuff(cepstra, nframes);
 *  }
 *
 * @param inout_spch Input: Pointer to pointer to speech samples
 *                   (signed 16-bit linear PCM).
 *                   Output: Pointer to remaining samples.
 * @param inout_nsamps Input: Pointer to maximum number of samples to
 *                     process.
 *                     Output: Number of samples remaining in input buffer.
 * @param buf_cep Two-dimensional buffer (allocated with
 *                ckd_calloc_2d()) which will receive frames of output
 *                data.  If NULL, no actual processing will be done,
 *                and the maximum number of output frames which would
 *                be generated is returned in
 *                <code>*inout_nframes</code>.
 * @param inout_nframes Input: Pointer to maximum number of frames to
 *                      generate.
 *                      Output: Number of frames actually generated.
 * @param out_frameidx Index of the first frame returned in a stream
 *
 * @return 0 for success, <0 for failure (see enum fe_error_e)
 */
int fe_process_frames(fe_t *fe,
                      int16 const **inout_spch,
                      size_t *inout_nsamps,
                      mfcc_t **buf_cep,
                      int32 *inout_nframes,
                      int32 *out_frameidx);

/*
 * Do same as fe_process_frames, but also returns
 * voiced audio. Output audio is valid till next
 * fe_process_frames call.
 *
 * DO NOT MIX fe_process_frames calls
 *
 * @param voiced_spch Output: obtain voiced audio samples here
 *
 * @param voiced_spch_nsamps Output: shows voiced_spch length
 *
 * @param out_frameidx Output: index of the utterance start
 */
int fe_process_frames_ext(fe_t *fe,
                      int16 const **inout_spch,
                      size_t *inout_nsamps,
                      mfcc_t **buf_cep,
                      int32 *inout_nframes,
                      int16 *voiced_spch,
                      int32 *voiced_spch_nsamps,
                      int32 *out_frameidx);

/* Load a frame of data into the fe. */
int fe_read_frame(fe_t *fe, int16 const *in, int32 len);

/* Process a frame of data into features. */
void fe_write_frame(fe_t *fe, mfcc_t *feat, int32 store_pcm);