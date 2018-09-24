#include "fe.h"
#include "prim_type.h"

typedef struct noise_stats_s noise_stats_t;

/* Creates noisestats object */
noise_stats_t *fe_init_noisestats(int num_filters);

/* Resets collected noise statistics */
void fe_reset_noisestats(noise_stats_t * noise_stats);

/* Frees allocated data */
void fe_free_noisestats(noise_stats_t * noise_stats);

/**
 * Process frame, update noise statistics, remove noise components if needed, 
 * and return local vad decision.
 */
void fe_track_snr(fe_t *fe, int32 *in_speech);

/**
 * Updates global state based on local VAD state smoothing the estimate.
 */
void fe_vad_hangover(fe_t *fe, mfcc_t *feat, int32 is_speech, int32 store_pcm);