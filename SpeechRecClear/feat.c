#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "feat.h"
#include "prim_type.h"
#include "logged_alloc.h"

float32 *** feat_array_alloc(feat_t * fcb, int32 nfr) {
    int32 i, j, k;
    float32 *data, *d, ***feat;

    assert(fcb);
    assert(nfr > 0);
    assert(fcb->out_dim > 0);

    /* Make sure to use the dimensionality of the features *before*
       LDA and subvector projection. */
    k = 0;
    for (i = 0; i < fcb->n_stream; ++i)
        k += fcb->stream_len[i];
    assert(k >= fcb->out_dim);
    assert(k >= fcb->sv_dim);

    feat =
        (float32 ***) calloc_2d_logged_fail(nfr, feat_dimension1(fcb), sizeof(float32 *));
    data = (float32 *) calloc_logged_fail(nfr * k, sizeof(float32));

    for (i = 0; i < nfr; i++) {
        d = data + i * k;
        for (j = 0; j < feat_dimension1(fcb); j++) {
            feat[i][j] = d;
            d += feat_dimension2(fcb, j);
        }
    }

    return feat;
}

float32 *** feat_array_realloc(feat_t *fcb, float32 ***old_feat, int32 o_fr, int32 n_fr) {
    int32 i, k, cf;
    float32*** new_feat;

    assert(fcb);
    assert(n_fr > 0);
    assert(o_fr > 0);
    assert(fcb->out_dim > 0);

    /* Make sure to use the dimensionality of the features *before*
       LDA and subvector projection. */
    k = 0;
    for (i = 0; i < fcb->n_stream; ++i)
        k += fcb->stream_len[i];
    assert(k >= fcb->out_dim);
    assert(k >= fcb->sv_dim);
    
    new_feat = feat_array_alloc(fcb, n_fr);

    cf = (n_fr < o_fr) ? n_fr : o_fr;
    memcpy(new_feat[0][0], old_feat[0][0], cf * k * sizeof(float32));

    feat_array_free(old_feat);
    
    return new_feat;
}

void feat_array_free(float32 ***feat) {
    free(feat[0][0]);
    free_2d((void **)feat);
}