#include <string.h>

#include "fe.h"
#include "gen_rand.h"
#include "fe_noise.h"
#include "fe_prespch_buf.h"

int fe_process_frames(fe_t *fe,
                  int16 const **inout_spch,
                  size_t *inout_nsamps,
                  mfcc_t **buf_cep,
                  int32 *inout_nframes,
                  int32 *out_frameidx) {
    return fe_process_frames_ext(fe, inout_spch, inout_nsamps, buf_cep, inout_nframes, NULL, NULL, out_frameidx);
}

int  fe_process_frames_ext(fe_t *fe,
                  int16 const **inout_spch, //PCM data
                  size_t *inout_nsamps, //-> max number to process -> number of leftover
                  mfcc_t **buf_cep, //frames of samples
                  int32 *inout_nframes, //-> p to max numebr of frames to generate -> p to actually generated
                  int16 *voiced_spch,
                  int32 *voiced_spch_nsamps,
                  int32 *out_frameidx) {
    int outidx, n_overflow, orig_n_overflow;
    int16 const *orig_spch;
    size_t orig_nsamps;
    
    /* The logic here is pretty complex, please be careful with modifications */

    /* FIXME: Dump PCM data if needed */

    /* In the special case where there is no output buffer, return the
     * maximum number of frames which would be generated. */
    if (buf_cep == NULL) {
        if (*inout_nsamps + fe->num_overflow_samps < (size_t)fe->frame_size)
            *inout_nframes = 0;
        else
            *inout_nframes = 1
                + ((*inout_nsamps + fe->num_overflow_samps - fe->frame_size)
                   / fe->frame_shift);
        if (!fe->vad_data->in_speech)
            *inout_nframes += fe->vad_data->prespch_buf->ncep;
        return *inout_nframes;
    }

    if (out_frameidx)
        *out_frameidx = 0;

    /* Are there not enough samples to make at least 1 frame? */
    if (*inout_nsamps + fe->num_overflow_samps < (size_t)fe->frame_size) {
        if (*inout_nsamps > 0) {
            /* Append them to the overflow buffer. */
            memcpy(fe->overflow_samps + fe->num_overflow_samps,
                   *inout_spch, *inout_nsamps * (sizeof(int16)));
            fe->num_overflow_samps += *inout_nsamps;
            /* Update input-output pointers and counters. */
            *inout_spch += *inout_nsamps;
            *inout_nsamps = 0;
        }
        /* We produced no frames of output, sorry! */
        *inout_nframes = 0;
        return 0;
    }

    /* Can't write a frame?  Then do nothing! */
    if (*inout_nframes < 1) {
        *inout_nframes = 0;
        return 0;
    }

    /* Index of output frame. */
    outidx = 0;

    /* Try to read from prespeech buffer */
    if (fe->vad_data->in_speech && fe->vad_data->prespch_buf->ncep > 0) {
    	outidx = fe_copy_from_prespch(fe, inout_nframes, buf_cep, outidx);
        if ((*inout_nframes) < 1) {
            /* mfcc buffer is filled from prespeech buffer */
            *inout_nframes = outidx;
            return 0;
        }
    }

    /* Keep track of the original start of the buffer. */
    orig_spch = *inout_spch;
    orig_nsamps = *inout_nsamps;
    orig_n_overflow = fe->num_overflow_samps;

    /* Start processing, taking care of any incoming overflow. */
    if (fe->num_overflow_samps > 0) {
        int offset = fe->frame_size - fe->num_overflow_samps;
        /* Append start of spch to overflow samples to make a full frame. */
        memcpy(fe->overflow_samps + fe->num_overflow_samps,
               *inout_spch, offset * sizeof(**inout_spch));
        fe_read_frame(fe, fe->overflow_samps, fe->frame_size);
        /* Update input-output pointers and counters. */
        *inout_spch += offset;
        *inout_nsamps -= offset;
    } else {
        fe_read_frame(fe, *inout_spch, fe->frame_size);
        /* Update input-output pointers and counters. */
        *inout_spch += fe->frame_size;
        *inout_nsamps -= fe->frame_size;
    }

    fe_write_frame(fe, buf_cep[outidx], voiced_spch != NULL);
    outidx = fe_check_prespeech(fe, inout_nframes, buf_cep, outidx, out_frameidx, inout_nsamps, orig_nsamps);

    /* Process all remaining frames. */
    while (*inout_nframes > 0 && *inout_nsamps >= (size_t)fe->frame_shift) {
        fe_shift_frame(fe, *inout_spch, fe->frame_shift);
        fe_write_frame(fe, buf_cep[outidx], voiced_spch != NULL);

	outidx = fe_check_prespeech(fe, inout_nframes, buf_cep, outidx, out_frameidx, inout_nsamps, orig_nsamps);

        /* Update input-output pointers and counters. */
        *inout_spch += fe->frame_shift;
        *inout_nsamps -= fe->frame_shift;
    }

    /* How many relevant overflow samples are there left? */
    if (fe->num_overflow_samps <= 0) {
        /* Maximum number of overflow samples past *inout_spch to save. */
        n_overflow = *inout_nsamps;
        if (n_overflow > fe->frame_shift)
            n_overflow = fe->frame_shift;
        fe->num_overflow_samps = fe->frame_size - fe->frame_shift;
        /* Make sure this isn't an illegal read! */
        if (fe->num_overflow_samps > *inout_spch - orig_spch)
            fe->num_overflow_samps = *inout_spch - orig_spch;
        fe->num_overflow_samps += n_overflow;
        if (fe->num_overflow_samps > 0) {
            memcpy(fe->overflow_samps,
                   *inout_spch - (fe->frame_size - fe->frame_shift),
                   fe->num_overflow_samps * sizeof(**inout_spch));
            /* Update the input pointer to cover this stuff. */
            *inout_spch += n_overflow;
            *inout_nsamps -= n_overflow;
        }
    } else {
        /* There is still some relevant data left in the overflow buffer. */
        /* Shift existing data to the beginning. */
        memmove(fe->overflow_samps,
                fe->overflow_samps + orig_n_overflow - fe->num_overflow_samps,
                fe->num_overflow_samps * sizeof(*fe->overflow_samps));
        /* Copy in whatever we had in the original speech buffer. */
        n_overflow = *inout_spch - orig_spch + *inout_nsamps;
        if (n_overflow > fe->frame_size - fe->num_overflow_samps)
            n_overflow = fe->frame_size - fe->num_overflow_samps;
        memcpy(fe->overflow_samps + fe->num_overflow_samps,
               orig_spch, n_overflow * sizeof(*orig_spch));
        fe->num_overflow_samps += n_overflow;
        /* Advance the input pointers. */
        if (n_overflow > *inout_spch - orig_spch) {
            n_overflow -= (*inout_spch - orig_spch);
            *inout_spch += n_overflow;
            *inout_nsamps -= n_overflow;
        }
    }

    /* Finally update the frame counter with the number of frames
     * and global sample counter with number of samples we procesed*/
    *inout_nframes = outidx; /* FIXME: Not sure why I wrote it this way... */
    fe->sample_counter += orig_nsamps - *inout_nsamps;

    return 0;
}

int32 fe_start_utt(fe_t * fe) {
    fe->num_overflow_samps = 0;
    memset(fe->overflow_samps, 0, fe->frame_size * sizeof(int16));
    fe->start_flag = 1;
    fe->prior = 0;
    fe_reset_vad_data(fe->vad_data);
    return 0;
}

static void fe_reset_vad_data(vad_data_t * vad_data) {
    vad_data->in_speech = 0;
    vad_data->pre_speech_frames = 0;
    vad_data->post_speech_frames = 0;
    fe_prespch_reset_cep(vad_data->prespch_buf);
}

/**
 * Copy frames collected in prespeech buffer
 */
static int fe_copy_from_prespch(fe_t *fe, int32 *inout_nframes, mfcc_t **buf_cep, int outidx) {
    while ((*inout_nframes) > 0 && fe_prespch_read_cep(fe->vad_data->prespch_buf, buf_cep[outidx]) > 0) {
	    outidx++;
    	    (*inout_nframes)--;
    }
    return outidx;    
}