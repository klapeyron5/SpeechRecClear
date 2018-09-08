/*
 * \file ckd_alloc.h
 * \brief Sphinx's memory allocation/deallocation routines.
 *
 * Implementation of efficient memory allocation deallocation for
 * multiple dimensional arrays.
 */

/*
 * Called when memory allocation returns NULL - err msg + exit.
 */
void _failed_alloc(char *format, ...);

/*
 * The following functions are similar to the malloc family, except
 * that they have two additional parameters, caller_file and
 * caller_line, for error reporting.  All functions print a diagnostic
 * message if any error occurs, with any other behaviour determined by
 * ckd_set_jump(), above.
 */
void* __calloc_log__(size_t n_elem, size_t elem_size, const char *caller_file, int caller_line);

void* __malloc_log__(size_t size, const char *caller_file, int caller_line);

void* __realloc_log__(void *ptr, size_t new_size, const char *caller_file, int caller_line);

/**
 * Allocate a 2-D array and return ptr to it (ie, ptr to vector of ptrs).
 * The data area is allocated in one block so it can also be treated as a 1-D array.
 */
void* __calloc_2d_log__(size_t d1, size_t d2,	/* In: #elements in the 2 dimensions */
                        size_t elemsize,	/* In: Size (#bytes) of each element */
                        const char *caller_file, int caller_line);	/* In */

/*
 * Free a 2-D array (ptr) previously allocated by __calloc_2d_log__
 */
void free_2d(void *ptr);

/*
 * Macro for __calloc_log__
 */
#define calloc_logged_fail(n,sz)	__calloc_log__((n),(sz),__FILE__,__LINE__)

/*
 * Macro for __calloc_2d_log__
 */
#define calloc_2d_logged_fail(d1,d2,sz)	__calloc_2d_log__((d1),(d2),(sz),__FILE__,__LINE__)