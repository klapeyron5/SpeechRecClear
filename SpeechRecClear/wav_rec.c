#include <stdio.h>
#include <string.h>

#include "logged_calloc.h"
#include "wav_rec.h"
#include "err.h"
#include "prim_type.h"
#include "ac_mod.h"

/**
 * Decoder object.
 */
struct ps_decoder_s {
    /* Model parameters and such. */
 //   cmd_ln_t *config;  /**< Configuration. */ //TODO
 //   int refcount;      /**< Reference count. */ //TODO

    /* Basic units of computation. */
    ac_mod_t *ac_mod;    /**< Acoustic model. */
};

/**
 * PocketSphinx speech recognizer object.
 */
typedef struct ps_decoder_s ps_decoder_t;

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
ps_decoder_t* ps_init();

ps_decoder_t* ps_init() {
	ps_decoder_t* ps;
	ps = calloc_logged_fail(1,sizeof(*ps));
	return ps;
}

/**
 * Start utterance processing.
 *
 * This function should be called before any utterance data is passed
 * to the decoder.  It marks the start of a new utterance and
 * reinitializes internal data structures.
 *
 * @param ps Decoder to be started.
 * @return 0 for success, <0 on error.
 */
//int ps_start_utt(ps_decoder_t *ps);

void recognise_from_file() {
	ps_decoder_t* ps;
    int16 adbuf[2048];	//буфер для волны
    const char* fname;	//имя файла из которого распознавать
    const char* hyp;	//гипотеза распознавания из файла
	FILE* rawfile;		//файл, из которого надо распознавать
	int32 k;

	fname = "../test.wav";
	
    if ((rawfile = fopen(fname, "rb")) == NULL) { //открываем для чтения в бинарном виде
        E_FATAL_SYSTEM("Failed to open file '%s' for reading", fname);
	}

	if (strlen(fname) > 4 && strcmp(fname + strlen(fname) - 4, ".wav") == 0) {
        char waveheader[44];
		fread(waveheader, 1, 44, rawfile);
		if (!check_wav_header(waveheader, 16000))//(int)cmd_ln_float32_r(config, "-samprate")))
			E_FATAL("Failed to process file '%s' due to format mismatch.\n", fname);
    }

    if (strlen(fname) > 4 && strcmp(fname + strlen(fname) - 4, ".mp3") == 0) {
		E_FATAL("Can not decode mp3 files, convert input file to WAV 16kHz 16-bit mono before decoding.\n");
    }

	ps = ps_init();
//	ps_start_utt(ps);
    while ((k = fread(adbuf, sizeof(int16), 2048, rawfile)) > 0) {

	}
}

int check_wav_header(char *header, int expected_sr) {
    int sr;

    if (header[34] != 0x10) {
        E_ERROR("Input audio file has [%d] bits per sample instead of 16\n", header[34]);
        return 0;
    }
    if (header[20] != 0x1) {
        E_ERROR("Input audio file has compression [%d] and not required PCM\n", header[20]);
        return 0;
    }
    if (header[22] != 0x1) {
        E_ERROR("Input audio file has [%d] channels, expected single channel mono\n", header[22]);
        return 0;
    }
    sr = ((header[24] & 0xFF) | ((header[25] & 0xFF) << 8) | ((header[26] & 0xFF) << 16) | ((header[27] & 0xFF) << 24));
    if (sr != expected_sr) {
        E_ERROR("Input audio file has sample rate [%d], but decoder expects [%d]\n", sr, expected_sr);
        return 0;
    }
    return 1;
}