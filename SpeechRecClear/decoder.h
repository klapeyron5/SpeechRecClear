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
 //   search_t *search;     /**< Currently active search module. */
 //   search_t *phone_loop; /**< Phone loop search for lookahead. */

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
 * Decode raw audio data.
 *
 * @param dc Decoder.
 * @param no_search If non-zero, perform feature extraction but don't
 *                  do any recognition yet.  This may be necessary if
 *                  your processor has trouble doing recognition in
 *                  real-time.
 * @param full_utt If non-zero, this block of data is a full utterance
 *                 worth of data.  This may allow the recognizer to
 *                 produce more accurate results.
 * @return Number of frames of data searched, or <0 for error.
 */
int dc_process_raw(decoder_t *dc,
                   int16 const *data,
                   size_t n_samples,
                   int no_search,
                   int full_utt);

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

/**
 * Base structure for search module.
 */
struct search_s {
    searchfuncs_t *vt;  /**< V-table of search methods. */
    
    char *type;
    char *name;
    
    search_t *pls;      /**< Phoneme loop for lookahead. */
 //   cmd_ln_t *config;      /**< Configuration. */
    ac_mod_t *acmod;        /**< Acoustic model. */
  //  dict_t *dict;        /**< Pronunciation dictionary. */
  //  dict2pid_t *d2p;       /**< Dictionary to senone mappings. */
   // char *hyp_str;         /**< Current hypothesis string. */
  //  ps_lattice_t *dag;	   /**< Current hypothesis word graph. */
 //   ps_latlink_t *last_link; /**< Final link in best path. */
    int32 post;            /**< Utterance posterior probability. */
    int32 n_words;         /**< Number of words known to search (may be less than in the dictionary) */

    /* Magical word IDs that must exist in the dictionary: */
    int32 start_wid;       /**< Start word ID. */
    int32 silence_wid;     /**< Silence word ID. */
    int32 finish_wid;      /**< Finish word ID. */
};

/**
 * Search algorithm structure.
 */
typedef struct search_s search_t;


/**
 * V-table for search algorithm.
 */
typedef struct searchfuncs_s {
    int (*start)(search_t *search);
    int (*step)(search_t *search, int frame_idx);
    int (*finish)(search_t *search);
    int (*reinit)(search_t *search); //, dict_t *dict, dict2pid_t *d2p);
    void (*free)(search_t *search);

   // lattice_t *(*lattice)(search_t *search);
    char const *(*hyp)(search_t *search, int32 *out_score, int32 *out_is_final);
    int32 (*prob)(search_t *search);
  //  seg_t *(*seg_iter)(search_t *search);
} searchfuncs_t;