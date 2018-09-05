#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void _calloc_fail(char *format, ...) {
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

	exit(-1);
}

void* __calloc_log__(size_t n_elem, size_t elem_size, const char *caller_file, int caller_line) {
    void *mem;
    if ((mem = calloc(n_elem, elem_size)) == NULL) {
        _calloc_fail("calloc(%d,%d) failed from %s(%d)\n", n_elem, elem_size, caller_file, caller_line);
	}
    return mem;
}
