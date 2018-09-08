#include "wav_rec.h"

#include <stdio.h>
#include <string.h>

#include "decoder.h"
#include "err.h"

int recognise_from_file() {
	decoder_t* dc;
    int16 adbuf[2048];	//буфер для волны
    const char* fname;	//имя файла из которого распознавать
    const char* hyp;	//гипотеза распознавания из файла
	FILE* rawfile;		//файл, из которого надо распознавать
	int32 k;

	dc = dc_init();
	if (dc == NULL) {
		return 1;
	}

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

	dc_start_utt(dc);
    
	while ((k = fread(adbuf, sizeof(int16), 2048, rawfile)) > 0) {
        dc_process_raw(dc, adbuf, k, FALSE, FALSE);
	}

	dc_free(dc);
	return 0;
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