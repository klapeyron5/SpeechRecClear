#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ac_mod.h"
#include "logged_alloc.h"
#include "err.h"

ac_mod_t* ac_mod_init() {
    ac_mod_t *ac_mod;

    ac_mod = calloc_logged_fail(1, sizeof(*ac_mod));

    return ac_mod;

error_out:
    ac_mod_free(ac_mod);
    return NULL;
}

void ac_mod_free(ac_mod_t *ac_mod) {
    if (ac_mod == NULL)
        return;

    free(ac_mod); //free a 1-D array
}

int ac_mod_set_mfcñ_fh(ac_mod_t *ac_mod, FILE *log_fh) {
    int rv = 0;

    if (ac_mod->mfcñ_fh)
        fclose(ac_mod->mfcñ_fh);
    ac_mod->mfcñ_fh = log_fh;
    fwrite(&rv, 4, 1, ac_mod->mfcñ_fh);
    return rv;
}

int ac_mod_set_raw_fh(ac_mod_t *ac_mod, FILE *log_fh) {
    if (ac_mod->raw_fh)
        fclose(ac_mod->raw_fh);
    ac_mod->raw_fh = log_fh;
    return 0;
}

int ac_mod_set_sen_fh(ac_mod_t *ac_mod, FILE *log_fh) {
    if (ac_mod->sen_fh)
        fclose(ac_mod->sen_fh);
    ac_mod->sen_fh = log_fh;
    if (log_fh == NULL)
        return 0;
    return acmod_write_senfh_header(ac_mod, log_fh);
}

int acmod_write_senfh_header(ac_mod_t *acmod, FILE *logfh) {
    char nsenstr[64], logbasestr[64];

	return 0;
}

#define MAX_N_FRAMES MAX_INT32

void ac_mod_grow_feat_buf(ac_mod_t *ac_mod, int n_fr) {
    if (n_fr > MAX_N_FRAMES)
        E_FATAL("Decoder can not process more than %d frames at once, "
                "requested %d\n", MAX_N_FRAMES, n_fr);

    ac_mod->feat_buf = feat_array_realloc(ac_mod->fcb, ac_mod->feat_buf,
                                         ac_mod->n_feat_alloc, n_fr);
    ac_mod->framepos = __realloc_log__(ac_mod->framepos,
                                  n_fr * sizeof(*ac_mod->framepos));
    ac_mod->n_feat_alloc = n_fr;
}

int ac_mod_set_grow(ac_mod_t *ac_mod, int grow_feat) {
    int tmp = ac_mod->grow_feat;
    ac_mod->grow_feat = grow_feat;

    /* Expand feat_buf to a reasonable size to start with. */
    if (grow_feat && ac_mod->n_feat_alloc < 128)
        ac_mod_grow_feat_buf(ac_mod, 128);

    return tmp;
}

static int ac_mod_process_full_raw(ac_mod_t *ac_mod,
                       int16 const **inout_raw,
                       size_t *inout_n_samps) {
    int32 n_fr, ntail;
    float32 **cepptr;

    /* Write to logging file if any. */
    if (*inout_n_samps + ac_mod->rawdata_pos < ac_mod->rawdata_size) {
		memcpy(ac_mod->rawdata + ac_mod->rawdata_pos, *inout_raw, *inout_n_samps * sizeof(int16));
		ac_mod->rawdata_pos += *inout_n_samps;
    }
    if (ac_mod->raw_fh)
        fwrite(*inout_raw, sizeof(int16), *inout_n_samps, ac_mod->raw_fh);
    /* Resize mfc_buf to fit. */
    if (fe_process_frames(ac_mod->fe, NULL, inout_n_samps, NULL, &n_fr, NULL) < 0)
        return -1;
    if (ac_mod->n_mfc_alloc < n_fr + 1) {
        free_2d(ac_mod->mfc_buf);
		ac_mod->mfc_buf = calloc_2d_logged_fail(n_fr + 1, ac_mod->fe->feature_dimension,
                                       sizeof(**ac_mod->mfc_buf));
        ac_mod->n_mfc_alloc = n_fr + 1;
    }
    ac_mod->n_mfc_frame = 0;
    ac_mod->mfc_outidx = 0;
    fe_start_utt(ac_mod->fe);
    if (fe_process_frames(ac_mod->fe, inout_raw, inout_n_samps,
                          ac_mod->mfc_buf, &n_fr, NULL) < 0)
        return -1;
    fe_end_utt(ac_mod->fe, ac_mod->mfc_buf[n_fr], &ntail);
    n_fr += ntail;

    cepptr = ac_mod->mfc_buf;
    n_fr = acmod_process_full_cep(ac_mod, &cepptr, &n_fr);
    ac_mod->n_mfc_frame = 0;
    return n_fr;
}

int ac_mod_process_raw(ac_mod_t *ac_mod,
                  int16 const **inout_raw,
                  size_t *inout_n_samps,
                  int full_utt) {
    int32 ncep;
    int32 out_frameidx;
    int16 const *prev_audio_inptr;
    
    /* If this is a full utterance, process it all at once. */
    if (full_utt)
        return ac_mod_process_full_raw(ac_mod, inout_raw, inout_n_samps);

    /* Append MFCCs to the end of any that are previously in there
     * (in practice, there will probably be none) */
    if (inout_n_samps && *inout_n_samps) {
        int inptr;
        int32 processed_samples;

        prev_audio_inptr = *inout_raw;
        /* Total number of frames available. */
        ncep = ac_mod->n_mfc_alloc - ac_mod->n_mfc_frame;
        /* Where to start writing them (circular buffer) */
        inptr = (ac_mod->mfc_outidx + ac_mod->n_mfc_frame) % ac_mod->n_mfc_alloc;

        /* Write them in two (or more) parts if there is wraparound. */
        while (inptr + ncep > ac_mod->n_mfc_alloc) {
            int32 ncep1 = ac_mod->n_mfc_alloc - inptr;
            if (fe_process_frames(ac_mod->fe, inout_raw, inout_n_samps,
                                  ac_mod->mfc_buf + inptr, &ncep1, &out_frameidx) < 0)
                return -1;
	    
	    if (out_frameidx > 0)
		acmod->utt_start_frame = out_frameidx;

    	    processed_samples = *inout_raw - prev_audio_inptr;
	    if (processed_samples + ac_mod->rawdata_pos < ac_mod->rawdata_size) {
		memcpy(ac_mod->rawdata + ac_mod->rawdata_pos, prev_audio_inptr, processed_samples * sizeof(int16));
		ac_mod->rawdata_pos += processed_samples;
	    }
            /* Write to logging file if any. */
            if (ac_mod->rawfh) {
                fwrite(prev_audio_inptr, sizeof(int16),
                       processed_samples,
                       ac_mod->rawfh);
            }
            prev_audio_inptr = *inout_raw;
            
            /* ncep1 now contains the number of frames actually
             * processed.  This is a good thing, but it means we
             * actually still might have some room left at the end of
             * the buffer, hence the while loop.  Unfortunately it
             * also means that in the case where we are really
             * actually done, we need to get out totally, hence the
             * goto. */
            ac_mod->n_mfc_frame += ncep1;
            ncep -= ncep1;
            inptr += ncep1;
            inptr %= ac_mod->n_mfc_alloc;
            if (ncep1 == 0)
        	goto alldone;
        }

        assert(inptr + ncep <= ac_mod->n_mfc_alloc);        
        if (fe_process_frames(ac_mod->fe, inout_raw, inout_n_samps,
                              ac_mod->mfc_buf + inptr, &ncep, &out_frameidx) < 0)
            return -1;

	if (out_frameidx > 0)
	    ac_mod->utt_start_frame = out_frameidx;

	
	processed_samples = *inout_raw - prev_audio_inptr;
	if (processed_samples + ac_mod->rawdata_pos < ac_mod->rawdata_size) {
	    memcpy(ac_mod->rawdata + ac_mod->rawdata_pos, prev_audio_inptr, processed_samples * sizeof(int16));
	    ac_mod->rawdata_pos += processed_samples;
	}
        if (ac_mod->rawfh) {
            fwrite(prev_audio_inptr, sizeof(int16),
                   processed_samples, ac_mod->rawfh);
        }
        prev_audio_inptr = *inout_raw;
        ac_mod->n_mfc_frame += ncep;
    alldone:
        ;
    }

    /* Hand things off to acmod_process_cep. */
    return acmod_process_mfcbuf(ac_mod);
}