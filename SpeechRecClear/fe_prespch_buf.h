/* Buffer that maintains both features and raw audio for the VAD implementation */

#include "prim_type.h"

/** MFCC computation type. */
typedef float32 mfcc_t;

struct prespch_buf_s {
    /* saved mfcc frames */
    mfcc_t **cep_buf;
    /* saved pcm audio */
    int16 *pcm_buf;

    /* flag for pcm buffer initialization */
    int16 cep_write_ptr;
    /* read pointer for cep buffer */
    int16 cep_read_ptr;
    /* Count */
    int16 ncep;
    

    /* flag for pcm buffer initialization */
    int16 pcm_write_ptr;
    /* read pointer for cep buffer */
    int16 pcm_read_ptr;
    /* Count */
    int16 npcm;
    
    /* frames amount in cep buffer */
    int16 num_frames_cep;
    /* frames amount in pcm buffer */
    int16 num_frames_pcm;
    /* filters amount */
    int16 num_cepstra;
    /* amount of fresh samples in frame */
    int16 num_samples;
};

/* Buffer that maintains both features and raw audio for the VAD implementation */
typedef struct prespch_buf_s prespch_buf_t;

/* Creates prespeech buffer */
prespch_buf_t *fe_prespch_init(int num_frames, int num_cepstra,
                               int num_samples);

/* Reads mfcc frame from prespeech buffer */
int fe_prespch_read_cep(prespch_buf_t * prespch_buf, mfcc_t * fea);

/* Writes mfcc frame to prespeech buffer */
void fe_prespch_write_cep(prespch_buf_t * prespch_buf, mfcc_t * fea);

/* Reads pcm frame from prespeech buffer */
void fe_prespch_read_pcm(prespch_buf_t * prespch_buf, int16 *samples,
                         int32 * samples_num);

/* Writes pcm frame to prespeech buffer */
void fe_prespch_write_pcm(prespch_buf_t * prespch_buf, int16 * samples);

/* Resets read/write pointers for cepstrum buffer */
void fe_prespch_reset_cep(prespch_buf_t * prespch_buf);

/* Resets read/write pointer for pcm audio buffer */
void fe_prespch_reset_pcm(prespch_buf_t * prespch_buf);

/* Releases prespeech buffer */
void fe_prespch_free(prespch_buf_t * prespch_buf);

/* Returns number of accumulated frames */
int32 fe_prespch_ncep(prespch_buf_t * prespch_buf);