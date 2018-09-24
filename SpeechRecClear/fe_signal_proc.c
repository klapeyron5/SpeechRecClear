#include <string.h>
#include <math.h>

#include "fe.h"
#include "gen_rand.h"
#include "fe_noise.h"
#include "fe_prespch_buf.h"

#define COSMUL(x,y) ((x)*(y))

/* Macro to byteswap an int16 variable.  x = ptr to variable */
#define SWAP_INT16(x)	*(x) = ((0x00ff & (*(x))>>8) | (0xff00 & (*(x))<<8))

#define LOG_FLOOR 1e-4

int fe_read_frame(fe_t * fe, int16 const *in, int32 len) {
    int i;

    if (len > fe->frame_size)
        len = fe->frame_size;

    /* Read it into the raw speech buffer. */
    memcpy(fe->spch, in, len * sizeof(*in));
    /* Swap and dither if necessary. */
    if (fe->swap)
        for (i = 0; i < len; ++i)
            SWAP_INT16(&fe->spch[i]);
    if (fe->dither)
        for (i = 0; i < len; ++i)
            fe->spch[i] += (int16) ((!(s3_rand_int31() % 4)) ? 1 : 0);

    return fe_spch_to_frame(fe, len);
}

void fe_write_frame(fe_t * fe, mfcc_t * feat, int32 store_pcm) {
    int32 is_speech;

    fe_spec_magnitude(fe);
    fe_mel_spec(fe);
    fe_track_snr(fe, &is_speech);
    fe_mel_cep(fe, feat);
    fe_lifter(fe, feat);
    fe_vad_hangover(fe, feat, is_speech, store_pcm);
}

static int fe_spch_to_frame(fe_t * fe, int len) {
    /* Copy to the frame buffer. */
    if (fe->pre_emphasis_alpha != 0.0) {
        fe_pre_emphasis(fe->spch, fe->frame, len,
                        fe->pre_emphasis_alpha, fe->prior);
        if (len >= fe->frame_shift)
            fe->prior = fe->spch[fe->frame_shift - 1];
        else
            fe->prior = fe->spch[len - 1];
    }
    else
        fe_short_to_frame(fe->spch, fe->frame, len);

    /* Zero pad up to FFT size. */
    memset(fe->frame + len, 0, (fe->fft_size - len) * sizeof(*fe->frame));

    /* Window. */
    fe_hamming_window(fe->frame, fe->hamming_window, fe->frame_size,
                      fe->remove_dc);

    return len;
}

static void fe_pre_emphasis(int16 const *in, frame_t * out, int32 len,
                float32 factor, int16 prior) {
    int i; 
    out[0] = (frame_t) in[0] - (frame_t) prior *factor;
    for (i = 1; i < len; i++)
        out[i] = (frame_t) in[i] - (frame_t) in[i - 1] * factor;
}

static void fe_short_to_frame(int16 const *in, frame_t * out, int32 len) {
    int i;
    for (i = 0; i < len; i++)
        out[i] = (frame_t) in[i];
}

static void fe_hamming_window(frame_t * in, window_t * window, int32 in_len,
                  int32 remove_dc) {
    int i;

    if (remove_dc) {
        frame_t mean = 0;

        for (i = 0; i < in_len; i++)
            mean += in[i];
        mean /= in_len;
        for (i = 0; i < in_len; i++)
            in[i] -= (frame_t) mean;
    }

    for (i = 0; i < in_len / 2; i++) {
        in[i] = COSMUL(in[i], window[i]);
        in[in_len - 1 - i] = COSMUL(in[in_len - 1 - i], window[i]);
    }
}

static void fe_spec_magnitude(fe_t * fe) {
    frame_t *fft;
    powspec_t *spec;
    int32 j, scale, fftsize;

    /* Do FFT and get the scaling factor back (only actually used in
     * fixed-point).  Note the scaling factor is expressed in bits. */
    scale = fe_fft_real(fe);

    /* Convenience pointers to make things less awkward below. */
    fft = fe->frame;
    spec = fe->spec;
    fftsize = fe->fft_size;

    /* We need to scale things up the rest of the way to N. */
    scale = fe->fft_order - scale;

    /* The first point (DC coefficient) has no imaginary part */
    spec[0] = fft[0] * fft[0];

    for (j = 1; j <= fftsize / 2; j++) {
        spec[j] = fft[j] * fft[j] + fft[fftsize - j] * fft[fftsize - j];
    }
}

static int fe_fft_real(fe_t * fe) {
    int i, j, k, m, n;
    frame_t *x, xt;

    x = fe->frame;
    m = fe->fft_order;
    n = fe->fft_size;

    /* Bit-reverse the input. */
    j = 0;
    for (i = 0; i < n - 1; ++i) {
        if (i < j) {
            xt = x[j];
            x[j] = x[i];
            x[i] = xt;
        }
        k = n / 2;
        while (k <= j) {
            j -= k;
            k /= 2;
        }
        j += k;
    }

    /* Basic butterflies (2-point FFT, real twiddle factors):
     * x[i]   = x[i] +  1 * x[i+1]
     * x[i+1] = x[i] + -1 * x[i+1]
     */
    for (i = 0; i < n; i += 2) {
        xt = x[i];
        x[i] = (xt + x[i + 1]);
        x[i + 1] = (xt - x[i + 1]);
    }

    /* The rest of the butterflies, in stages from 1..m */
    for (k = 1; k < m; ++k) {
        int n1, n2, n4;

        n4 = k - 1;
        n2 = k;
        n1 = k + 1;
        /* Stride over each (1 << (k+1)) points */
        for (i = 0; i < n; i += (1 << n1)) {
            /* Basic butterfly with real twiddle factors:
             * x[i]          = x[i] +  1 * x[i + (1<<k)]
             * x[i + (1<<k)] = x[i] + -1 * x[i + (1<<k)]
             */
            xt = x[i];
            x[i] = (xt + x[i + (1 << n2)]);
            x[i + (1 << n2)] = (xt - x[i + (1 << n2)]);

            /* The other ones with real twiddle factors:
             * x[i + (1<<k) + (1<<(k-1))]
             *   = 0 * x[i + (1<<k-1)] + -1 * x[i + (1<<k) + (1<<k-1)]
             * x[i + (1<<(k-1))]
             *   = 1 * x[i + (1<<k-1)] +  0 * x[i + (1<<k) + (1<<k-1)]
             */
            x[i + (1 << n2) + (1 << n4)] = -x[i + (1 << n2) + (1 << n4)];
            x[i + (1 << n4)] = x[i + (1 << n4)];

            /* Butterflies with complex twiddle factors.
             * There are (1<<k-1) of them.
             */
            for (j = 1; j < (1 << n4); ++j) {
                frame_t cc, ss, t1, t2;
                int i1, i2, i3, i4;

                i1 = i + j;
                i2 = i + (1 << n2) - j;
                i3 = i + (1 << n2) + j;
                i4 = i + (1 << n2) + (1 << n2) - j;

                /*
                 * cc = real(W[j * n / (1<<(k+1))])
                 * ss = imag(W[j * n / (1<<(k+1))])
                 */
                cc = fe->ccc[j << (m - n1)];
                ss = fe->sss[j << (m - n1)];

                /* There are some symmetry properties which allow us
                 * to get away with only four multiplications here. */
                t1 = COSMUL(x[i3], cc) + COSMUL(x[i4], ss);
                t2 = COSMUL(x[i3], ss) - COSMUL(x[i4], cc);

                x[i4] = (x[i2] - t2);
                x[i3] = (-x[i2] - t2);
                x[i2] = (x[i1] - t1);
                x[i1] = (x[i1] + t1);
            }
        }
    }

    /* This isn't used, but return it for completeness. */
    return m;
}

static void fe_mel_spec(fe_t * fe) {
    int whichfilt;
    powspec_t *spec, *mfspec;

    /* Convenience poitners. */
    spec = fe->spec;
    mfspec = fe->mfspec;
    for (whichfilt = 0; whichfilt < fe->mel_fb->num_filters; whichfilt++) {
        int spec_start, filt_start, i;

        spec_start = fe->mel_fb->spec_start[whichfilt];
        filt_start = fe->mel_fb->filt_start[whichfilt];

        mfspec[whichfilt] = 0;
        for (i = 0; i < fe->mel_fb->filt_width[whichfilt]; i++)
            mfspec[whichfilt] +=
                spec[spec_start + i] * fe->mel_fb->filt_coeffs[filt_start +
                                                               i];
    }
}

static void fe_mel_cep(fe_t * fe, mfcc_t * mfcep) {
    int32 i;
    powspec_t *mfspec;

    /* Convenience pointer. */
    mfspec = fe->mfspec;

    for (i = 0; i < fe->mel_fb->num_filters; ++i) {
        mfspec[i] = log(mfspec[i] + LOG_FLOOR);
    }

    /* If we are doing LOG_SPEC, then do nothing. */
    if (fe->log_spec == RAW_LOG_SPEC) {
        for (i = 0; i < fe->feature_dimension; i++) {
            mfcep[i] = (mfcc_t) mfspec[i];
        }
    }
    /* For smoothed spectrum, do DCT-II followed by (its inverse) DCT-III */
    else if (fe->log_spec == SMOOTH_LOG_SPEC) {
        /* FIXME: This is probably broken for fixed-point. */
        fe_dct2(fe, mfspec, mfcep, 0);
        fe_dct3(fe, mfcep, mfspec);
        for (i = 0; i < fe->feature_dimension; i++) {
            mfcep[i] = (mfcc_t) mfspec[i];
        }
    }
    else if (fe->transform == DCT_II)
        fe_dct2(fe, mfspec, mfcep, FALSE);
    else if (fe->transform == DCT_HTK)
        fe_dct2(fe, mfspec, mfcep, TRUE);
    else
        fe_spec2cep(fe, mfspec, mfcep);

    return;
}

void fe_dct2(fe_t * fe, const powspec_t * mflogspec, mfcc_t * mfcep, int htk) {
    int32 i, j;

    /* Compute C0 separately (its basis vector is 1) to avoid
     * costly multiplications. */
    mfcep[0] = mflogspec[0];
    for (j = 1; j < fe->mel_fb->num_filters; j++)
        mfcep[0] += mflogspec[j];
    if (htk)
        mfcep[0] = COSMUL(mfcep[0], fe->mel_fb->sqrt_inv_2n);
    else                        /* sqrt(1/N) = sqrt(2/N) * 1/sqrt(2) */
        mfcep[0] = COSMUL(mfcep[0], fe->mel_fb->sqrt_inv_n);

    for (i = 1; i < fe->num_cepstra; ++i) {
        mfcep[i] = 0;
        for (j = 0; j < fe->mel_fb->num_filters; j++) {
            mfcep[i] += COSMUL(mflogspec[j], fe->mel_fb->mel_cosine[i][j]);
        }
        mfcep[i] = COSMUL(mfcep[i], fe->mel_fb->sqrt_inv_2n);
    }
}

void fe_dct3(fe_t * fe, const mfcc_t * mfcep, powspec_t * mflogspec) {
    int32 i, j;

    for (i = 0; i < fe->mel_fb->num_filters; ++i) {
        mflogspec[i] = COSMUL(mfcep[0], SQRT_HALF);
        for (j = 1; j < fe->num_cepstra; j++) {
            mflogspec[i] += COSMUL(mfcep[j], fe->mel_fb->mel_cosine[j][i]);
        }
        mflogspec[i] = COSMUL(mflogspec[i], fe->mel_fb->sqrt_inv_2n);
    }
}

void fe_spec2cep(fe_t * fe, const powspec_t * mflogspec, mfcc_t * mfcep) {
    int32 i, j, beta;

    /* Compute C0 separately (its basis vector is 1) to avoid
     * costly multiplications. */
    mfcep[0] = mflogspec[0] / 2;        /* beta = 0.5 */
    for (j = 1; j < fe->mel_fb->num_filters; j++)
        mfcep[0] += mflogspec[j];       /* beta = 1.0 */
    mfcep[0] /= (frame_t) fe->mel_fb->num_filters;

    for (i = 1; i < fe->num_cepstra; ++i) {
        mfcep[i] = 0;
        for (j = 0; j < fe->mel_fb->num_filters; j++) {
            if (j == 0)
                beta = 1;       /* 0.5 */
            else
                beta = 2;       /* 1.0 */
            mfcep[i] += COSMUL(mflogspec[j],
                               fe->mel_fb->mel_cosine[i][j]) * beta;
        }
        /* Note that this actually normalizes by num_filters, like the
         * original Sphinx front-end, due to the doubled 'beta' factor
         * above.  */
        mfcep[i] /= (frame_t) fe->mel_fb->num_filters * 2;
    }
}

void fe_lifter(fe_t * fe, mfcc_t * mfcep) {
    int32 i;

    if (fe->mel_fb->lifter_val == 0)
        return;

    for (i = 0; i < fe->num_cepstra; ++i) {
        mfcep[i] = MFCCMUL(mfcep[i], fe->mel_fb->lifter[i]);
    }
}

void fe_vad_hangover(fe_t * fe, mfcc_t * feat, int32 is_speech, int32 store_pcm) {
    if (!fe->vad_data->in_speech) {
        fe_prespch_write_cep(fe->vad_data->prespch_buf, feat);
        if (store_pcm)
            fe_prespch_write_pcm(fe->vad_data->prespch_buf, fe->spch);
    }
    
    /* track vad state and deal with cepstrum prespeech buffer */
    if (is_speech) {
        fe->vad_data->post_speech_frames = 0;
        if (!fe->vad_data->in_speech) {
            fe->vad_data->pre_speech_frames++;
            /* check for transition sil->speech */
            if (fe->vad_data->pre_speech_frames >= fe->start_speech) {
                fe->vad_data->pre_speech_frames = 0;
                fe->vad_data->in_speech = 1;
            }
        }
    } else {
        fe->vad_data->pre_speech_frames = 0;
        if (fe->vad_data->in_speech) {
            fe->vad_data->post_speech_frames++;
            /* check for transition speech->sil */
            if (fe->vad_data->post_speech_frames >= fe->post_speech) {
                fe->vad_data->post_speech_frames = 0;
                fe->vad_data->in_speech = 0;
    	        fe_prespch_reset_cep(fe->vad_data->prespch_buf);
        	fe_prespch_reset_pcm(fe->vad_data->prespch_buf);
            }
        }
    }
}