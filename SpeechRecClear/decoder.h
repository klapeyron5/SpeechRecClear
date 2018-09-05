#include "ac_mod.h"

/**
 * Decoder object.
 */
struct decoder_s {
    /* Model parameters and such. */
    int ref_count;      /**< Reference count. */

    /* Basic units of computation. */
    ac_mod_t *ac_mod;    /**< Acoustic model. */

    /* Search modules. */
  //  ps_search_t *search;     /**< Currently active search module. */
  //  ps_search_t *phone_loop; /**< Phone loop search for lookahead. */

    /* Utterance-processing related stuff. */
    char const *mfcñ_log_dir; /**< Log directory for MFCC files. */
    char const *raw_log_dir; /**< Log directory for audio files. */
    char const *sen_log_dir; /**< Log directory for senone score files. */
};

/**
 * PocketSphinx speech recognizer object.
 */
typedef struct decoder_s decoder_t;

/**
 * Initialize the decoder from a configuration object.
 *
 * @note The decoder retains ownership of the pointer
 * <code>config</code>, so if you are not going to use it
 * elsewere, you can free it.
 *
 * @param config a command-line structure, as created by
 * cmd_ln_parse_r() or cmd_ln_parse_file_r().
 */
decoder_t* dc_init();

/**
 * Reinitialize the decoder with updated configuration.
 *
 * This function allows you to switch the acoustic model, dictionary,
 * or other configuration without creating an entirely new decoding
 * object.
 *
 * @note The decoder retains ownership of the pointer
 * <code>config</code>, so you must not attempt to free it manually.
 * If you wish to reuse it elsewhere, call cmd_ln_retain() on it.
 *
 * @param Decoder.
 * @param config An optional new configuration to use.  If this is
 *               NULL, the previous configuration will be reloaded,
 *               with any changes applied.
 * @return 0 for success, <0 for failure.
 */
int dc_reinit(decoder_t* dc);

/**
 * Start utterance processing.
 *
 * This function should be called before any utterance data is passed
 * to the decoder.  It marks the start of a new utterance and
 * reinitializes internal data structures.
 *
 * @param Decoder to be started.
 * @return 0 for success, <0 on error.
 */	
int dc_start_utt(decoder_t* dc);

/**
 * Finalize the decoder.
 *
 * This releases all resources associated with the decoder, including
 * any language models or grammars which have been added to it, and
 * the initial configuration object passed to dc_init().
 *
 * @param Decoder to be freed.
 * @return New reference count (0 if freed).
 */
int dc_free(decoder_t *dc);