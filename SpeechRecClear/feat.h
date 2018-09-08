#include "prim_type.h"

/*
 * feat.h -- Cepstral features computation.
 */

/**
 * \struct feat_t
 * \brief Structure for describing a speech feature type
 * Structure for describing a speech feature type (no. of streams and stream widths),
 * as well as the computation for converting the input speech (e.g., Sphinx-II format
 * MFC cepstra) into this type of feature vectors.
 */
typedef struct feat_s {
    int refcount;       /**< Reference count. */
    char *name;		/**< Printable name for this feature type */
    int32 cepsize;	/**< Size of input speech vector (typically, a cepstrum vector) */
    int32 n_stream;	/**< Number of feature streams; e.g., 4 in Sphinx-II */
    uint32 *stream_len;	/**< Vector length of each feature stream */
    int32 window_size;	/**< Number of extra frames around given input frame needed to compute
                           corresponding output feature (so total = window_size*2 + 1) */
    int32 n_sv;         /**< Number of subvectors */
    uint32 *sv_len;      /**< Vector length of each subvector */
    int32 **subvecs;    /**< Subvector specification (or NULL for none) */
    float32 *sv_buf;      /**< Temporary copy buffer for subvector projection */
    int32 sv_dim;       /**< Total dimensionality of subvector (length of sv_buf) */

    cmn_type_t cmn;	/**< Type of CMN to be performed on each utterance */
    int32 varnorm;	/**< Whether variance normalization is to be performed on each utt;
                           Irrelevant if no CMN is performed */
    agc_type_t agc;	/**< Type of AGC to be performed on each utterance */

    /**
     * Feature computation function. 
     * @param fcb the feat_t describing this feature type
     * @param input pointer into the input cepstra
     * @param feat a 2-d array of output features (n_stream x stream_len)
     * @return 0 if successful, -ve otherwise.
     *
     * Function for converting window of input speech vector
     * (input[-window_size..window_size]) to output feature vector
     * (feat[stream][]).  If NULL, no conversion available, the
     * speech input must be feature vector itself.
     **/
    void (*compute_feat)(struct feat_s *fcb, float32 **input, float32 **feat);
    cmn_t *cmn_struct;	/**< Structure that stores the temporary variables for cepstral 
                           means normalization*/
    agc_t *agc_struct;	/**< Structure that stores the temporary variables for acoustic
                           gain control*/

    float32 **cepbuf;    /**< Circular buffer of MFCC frames for live feature computation. */
    float32 **tmpcepbuf; /**< Array of pointers into cepbuf to handle border cases. */
    int32   bufpos;     /**< Write index in cepbuf. */
    int32   curpos;     /**< Read index in cepbuf. */

    float32 ***lda; /**< Array of linear transformations (for LDA, MLLT, or whatever) */
    uint32 n_lda;   /**< Number of linear transformations in lda. */
    uint32 out_dim; /**< Output dimensionality */
} feat_t;

/**
 * Number of streams or subvectors in feature output.
 */
#define feat_dimension1(f)	((f)->n_sv ? (f)->n_sv : f->n_stream)

/**
 * Dimensionality of stream/subvector i in feature output.
 */
#define feat_dimension2(f,i)	((f)->lda ? (f)->out_dim : ((f)->sv_len ? (f)->sv_len[i] : f->stream_len[i]))

/**
 * Allocate an array to hold several frames worth of feature vectors.  The returned value
 * is the float32 ***data array, organized as follows:
 *
 * - data[0][0] = frame 0 stream 0 vector, data[0][1] = frame 0 stream 1 vector, ...
 * - data[1][0] = frame 1 stream 0 vector, data[0][1] = frame 1 stream 1 vector, ...
 * - data[2][0] = frame 2 stream 0 vector, data[0][1] = frame 2 stream 1 vector, ...
 * - ...
 *
 * NOTE: For I/O convenience, the entire data area is allocated as one contiguous block.
 * @return pointer to the allocated space if successful, NULL if any error.
 */
float32 ***feat_array_alloc(feat_t *fcb,	/**< In: Descriptor from feat_init(), used to obtain number of streams and stream sizes */
                           int32 n_fr	/**< In: Number of frames for which to allocate */
    );

/**
 * Realloate the array of features. Requires us to know the old size
 */
float32 ***feat_array_realloc(feat_t *fcb, /**< In: Descriptor from feat_init(), used to obtain number of streams and stream sizes */
							  float32 ***old_feat, /**< Feature array. Freed */
                              int32 o_fr,	/**< In: Previous number of frames */
                              int32 n_fr	/**< In: Number of frames for which to allocate */
    );

/**
 * Free a buffer allocated with feat_array_alloc()
 */
void feat_array_free(float32 ***feat);

/**
 * Types of cepstral mean normalization to apply to the features.
 */
typedef enum cmn_type_e {
    CMN_NONE = 0,
    CMN_CURRENT,
    CMN_PRIOR
} cmn_type_t;

/** \struct cmn_t
 *  \brief wrapper of operation of the cepstral mean normalization. 
 */
typedef struct {
    float32 *cmn_mean;   /**< Temporary variable: current means */
    float32 *cmn_var;    /**< Temporary variables: stored the cmn variance */
    float32 *sum;        /**< The sum of the cmn frames */
    int32 nframe;	/**< Number of frames */
    int32 veclen;	/**< Length of cepstral vector */
} cmn_t;


/**
 * Types of acoustic gain control to apply to the features.
 */
typedef enum agc_type_e {
    AGC_NONE = 0,
    AGC_MAX,
    AGC_EMAX,
    AGC_NOISE
} agc_type_t;

/**
 * Structure holding data for doing AGC.
 **/
typedef struct agc_s {
    float32 max;      /**< Estimated max for current utterance (for AGC_EMAX) */
    float32 obs_max;  /**< Observed max in current utterance */
    int32 obs_frame; /**< Whether any data was observed after prev update */
    int32 obs_utt;   /**< Whether any utterances have been observed */
    float32 obs_max_sum;
    float32 noise_thresh; /**< Noise threshold (for AGC_NOISE only) */
} agc_t;