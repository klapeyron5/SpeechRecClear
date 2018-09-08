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
 //   frame_t *ccc, *sss;
    /* Mel filter parameters. */
 //   melfb_t *mel_fb;
    /* Half of a Hamming Window. */
 //   window_t *hamming_window;

    /* Noise removal  */
//    noise_stats_t *noise_stats;

    /* VAD variables */
    int16 pre_speech;
    int16 post_speech;
    int16 start_speech;
    float32 vad_threshold;
//    vad_data_t *vad_data;

    /* Temporary buffers for processing. */
    /* FIXME: too many of these. */
    int16 *spch;
 //   frame_t *frame;
//    powspec_t *spec, *mfspec;
    int16 *overflow_samps;
    int16 num_overflow_samps;    
    int16 prior;
};

/**
 * Structure for the front-end computation.
 */
typedef struct fe_s fe_t;