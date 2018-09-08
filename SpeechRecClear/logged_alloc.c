#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

void _failed_alloc(char *format, ...) {
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);

	exit(-1);
}

void* __calloc_log__(size_t n_elem, size_t elem_size, const char *caller_file, int caller_line) {
    void *mem;
    if ((mem = calloc(n_elem, elem_size)) == NULL) {
        _failed_alloc("calloc(%d,%d) failed from %s(%d)\n", n_elem, elem_size, caller_file, caller_line);
	}
    return mem;
}

void* __malloc_log__(size_t size, const char *caller_file, int caller_line) {
    void *mem;
    if ((mem = malloc(size)) == NULL)
	        _failed_alloc("malloc(%d) failed from %s(%d)\n", size,
                caller_file, caller_line);

    return mem;
}

void* __realloc_log__(void *ptr, size_t new_size, const char *caller_file, int caller_line) {
    void *mem;
    if ((mem = realloc(ptr, new_size)) == NULL) {
        _failed_alloc("malloc(%d) failed from %s(%d)\n", new_size,
                caller_file, caller_line);
    }

    return mem;
}


void* __calloc_2d_log__(size_t d1, size_t d2, size_t elemsize,
                  const char *caller_file, int caller_line) {
    char **ref, *mem;
    size_t i, offset;

    mem =
        (char *) __calloc_log__(d1 * d2, elemsize, caller_file,
                                caller_line);
    ref =
        (char **) __malloc_log__(d1 * sizeof(void *), caller_file,
                                 caller_line);

    for (i = 0, offset = 0; i < d1; i++, offset += d2 * elemsize)
        ref[i] = mem + offset;

    return ref;
}

void free_2d(void *tmpptr) {
    void **ptr = (void **)tmpptr;
    if (ptr)
        free(ptr[0]);
    free(ptr);
}