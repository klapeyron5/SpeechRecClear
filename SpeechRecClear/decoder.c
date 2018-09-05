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
    if (dc->mfc�_log_dir) {
        char *log_fn = string_join(dc->mfc�_log_dir, "/", uttid, ".mfc", NULL);
        FILE *mfcc_fh;
        E_INFO("Writing MFCC log file: %s\n", log_fn);
        if ((mfcc_fh = fopen(log_fn, "wb")) == NULL) {
            E_ERROR_SYSTEM("Failed to open MFCC log file %s", log_fn);
            free(log_fn);
            return -1;
        }
        free(log_fn);
        ac_mod_set_mfc�_fh(dc->ac_mod, mfcc_fh);
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
      //  ac_mod_set_raw_fh(dc->ac_mod, raw_fh);
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
 //   if (dc->phone_loop)
 //       dc_search_start(dc->phone_loop);

    return 0;//dc_search_start(dc->search);
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