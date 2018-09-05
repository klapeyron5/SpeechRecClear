#include <stdio.h>

#include "prim_type.h"

/**
 * States in utterance processing.
 */
typedef enum acmod_state_e {
    ACMOD_IDLE,			/**< Not in an utterance. */
    ACMOD_STARTED,      /**< Utterance started, no data yet. */
    ACMOD_PROCESSING,   /**< Utterance in progress. */
    ACMOD_ENDED         /**< Utterance ended, still buffering. */
} acmod_state_t;

/**
 * Acoustic model structure.
 *
 * This object encapsulates all stages of acoustic processing, from
 * raw audio input to acoustic score output.  The reason for grouping
 * all of these modules together is that they all have to "agree" in
 * their parameterizations, and the configuration of the acoustic and
 * dynamic feature computation is completely dependent on the
 * parameters used to build the original acoustic model (which should
 * by now always be specified in a feat.params file).
 *
 * Because there is not a one-to-one correspondence from blocks of
 * input audio or frames of input features to frames of acoustic
 * scores (due to dynamic feature calculation), results may not be
 * immediately available after input, and the output results will not
 * correspond to the last piece of data input.
 *
 * TODO: In addition, this structure serves the purpose of queueing
 * frames of features (and potentially also scores in the future) for
 * asynchronous passes of recognition operating in parallel.
 */
struct ac_mod_s {
    /* A whole bunch of flags and counters: */
    uint8 state;        /**< State of utterance processing. */

    /* Utterance processing: */
    FILE *mfcñ_fh;		/**< File for writing acoustic feature data. */
    FILE *raw_fh;		/**< File for writing raw audio data. */
    FILE *sen_fh;        /**< File for writing senone score data. */
};

typedef struct ac_mod_s ac_mod_t;

/**
 * Initialize an acoustic model.
 *
 * @return a newly initialized acmod_t, or NULL on failure.
 */
ac_mod_t *ac_mod_init();

/**
 * Finalize an acoustic model.
 */
void ac_mod_free(ac_mod_t *ac_mod);

/**
 * Start logging MFCCs to a filehandle.
 *
 * @param acmod Acoustic model object.
 * @param logfh Filehandle to log to.
 * @return 0 for success, other on error.
 */
int ac_mod_set_mfcñ_fh(ac_mod_t *ac_mod, FILE *log_fh);

/**
 * Start logging raw audio to a filehandle.
 *
 * @param acmod Acoustic model object.
 * @param logfh Filehandle to log to.
 * @return 0 for success, <0 on error.
 */
int ac_mod_set_raw_fh(ac_mod_t *ac_mod, FILE *log_fh);

/**
 * Start logging senone scores to a filehandle.
 *
 * @param acmod Acoustic model object.
 * @param logfh Filehandle to log to.
 * @return 0 for success, <0 on error.
 */
int ac_mod_set_sen_fh(ac_mod_t *ac_mod, FILE *sen_fh);