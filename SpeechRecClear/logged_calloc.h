/*
 * Called when calloc returns NULL - err msg + exit.
 */
void _calloc_fail(char *format, ...);

/*
 * The following functions are similar to the malloc family, except
 * that they have two additional parameters, caller_file and
 * caller_line, for error reporting.  All functions print a diagnostic
 * message if any error occurs, with any other behaviour determined by
 * ckd_set_jump(), above.
 */
void* __calloc_log__(size_t n_elem, size_t elem_size, const char *caller_file, int caller_line);

/*
 * Macro for __calloc_log__
 */
#define calloc_logged_fail(n,sz)	__calloc_log__((n),(sz),__FILE__,__LINE__)