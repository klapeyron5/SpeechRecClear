#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "logged_alloc.h"


char * string_join(const char *base, ...) {
    va_list args;
    size_t len;
    const char *c;
    char *out;

    va_start(args, base);
    len = strlen(base);
    while ((c = va_arg(args, const char *)) != NULL) {
        len += strlen(c);
    }
    len++;
    va_end(args);

    out = calloc_logged_fail(len, 1);
    va_start(args, base);
    strcpy(out, base);
    while ((c = va_arg(args, const char *)) != NULL) {
        strcat(out, c);
    }
    va_end(args);

    return out;
}