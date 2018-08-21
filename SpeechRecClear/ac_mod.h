#include "prim_type.h"

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
};
typedef struct ac_mod_s ac_mod_t;