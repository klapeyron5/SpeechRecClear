#include <stdlib.h>
#include <stdio.h>

#include "ac_mod.h"
#include "logged_alloc.h"

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
