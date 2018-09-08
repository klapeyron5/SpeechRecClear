#include "decoder.h"

#include <stdio.h>

#include "logged_alloc.h"
#include "wav_rec.h"
#include "err.h"
#include "prim_type.h"
#include "str_funcs.h"

decoder_t* dc_init() {
	decoder_t* dc;
	dc = calloc_logged_fail(1,sizeof(*dc));
    dc->ref_count = 1;
	if (dc_reinit(dc) < 0) {
		dc_free(dc);
		return NULL;
	}
	return dc;
}

int dc_reinit(decoder_t* dc) {
    /* Free old acmod. */
    ac_mod_free(dc->ac_mod);
	dc->ac_mod = NULL;
	
    if ((dc->ac_mod = ac_mod_init()) == NULL)
        return -1;
	return 0;
}

int dc_start_utt(decoder_t* dc) {
    int rv;
    char uttid[16];
    
    if (dc->ac_mod->state == ACMOD_STARTED || dc->ac_mod->state == ACMOD_PROCESSING) {
		E_ERROR("Utterance already started\n");
		return -1;
    }

    /* Start logging features and audio if requested. */
    if (dc->mfcñ_log_dir) {
        char *log_fn = string_join(dc->mfcñ_log_dir, "/", uttid, ".mfc", NULL);
        FILE *mfcc_fh;
        E_INFO("Writing MFCC log file: %s\n", log_fn);
        if ((mfcc_fh = fopen(log_fn, "wb")) == NULL) {
            E_ERROR_SYSTEM("Failed to open MFCC log file %s", log_fn);
            free(log_fn);
            return -1;
        }
        free(log_fn);
        ac_mod_set_mfcñ_fh(dc->ac_mod, mfcc_fh);
    }
    if (dc->raw_log_dir) { //TODO need wav file for listening, not raw
        char *log_fn = string_join(dc->raw_log_dir, "/", uttid, ".raw", NULL);
        FILE *raw_fh;
        E_INFO("Writing raw audio log file: %s\n", log_fn);
        if ((raw_fh = fopen(log_fn, "wb")) == NULL) {
            E_ERROR_SYSTEM("Failed to open raw audio log file %s", log_fn);
            free(log_fn);
            return -1;
        }
        free(log_fn);
        ac_mod_set_raw_fh(dc->ac_mod, raw_fh);
    }
    if (dc->sen_log_dir) {
        char *log_fn = string_join(dc->sen_log_dir, "/", uttid, ".sen", NULL);
        FILE *sen_fh;
        E_INFO("Writing senone score log file: %s\n", log_fn);
        if ((sen_fh = fopen(log_fn, "wb")) == NULL) {
            E_ERROR_SYSTEM("Failed to open senone score log file %s", log_fn);
            free(log_fn);
            return -1;
        }
        free(log_fn);
        ac_mod_set_sen_fh(dc->ac_mod, sen_fh);
    }

    /* Start auxiliary phone loop search. */
    if (dc->phone_loop)
		dc->phone_loop->vt->start;

    return dc->search->vt->start;
}

int dc_process_raw(decoder_t *dc,
               int16 const *data,
               size_t n_samples,
               int no_search,
               int full_utt) {
    int n_search_fr = 0;

    if (dc->ac_mod->state == ACMOD_IDLE) {
		E_ERROR("Failed to process data, utterance is not started. Use start_utt to start it\n");
		return 0;
    }

    if (no_search) //ïîêà ÷òî íàì ñþäà
        ac_mod_set_grow(dc->ac_mod, TRUE);

    while (n_samples) {
        int n_fr;

        /* Process some data into features. */
        if ((n_fr = ac_mod_process_raw(dc->ac_mod, &data,
                                     &n_samples, full_utt)) < 0)
            return n_fr;

        /* Score and search as much data as possible */
        if (no_search)
            continue;
        if ((n_fr = dc_search_forward(dc)) < 0)
            return n_fr;
        n_search_fr += n_fr;
    }

    return n_search_fr;
}

int dc_free(decoder_t* dc) {
    if (dc == NULL)
        return 0;
    if (--dc->ref_count > 0)
        return dc->ref_count;
    ac_mod_free(dc->ac_mod);
    free(dc);
    return 0;
}