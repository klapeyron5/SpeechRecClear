#include "fe_prespch_buf.h"

void fe_prespch_reset_cep(prespch_buf_t * prespch_buf) {
    prespch_buf->cep_read_ptr = 0;
    prespch_buf->cep_write_ptr = 0;
    prespch_buf->ncep = 0;
}