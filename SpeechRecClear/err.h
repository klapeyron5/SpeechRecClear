/*
 * Исключительно для дебага. Отладка через консоль.
 */

/**
 * Exit with non-zero status after error message
 */
#define E_FATAL_SYSTEM(...)                                                  \
    do {                                                                     \
        err_msg_system(ERR_FATAL, FILELINE, __VA_ARGS__);				     \
        exit(EXIT_FAILURE);                                                  \
    } while (0)

#define FILELINE __FILE__,__LINE__

typedef enum err_enum {
    ERR_DEBUG,
    ERR_INFO,
    ERR_INFOCONT,
    ERR_WARN,
    ERR_ERROR,
    ERR_FATAL,
    ERR_MAX
} err_lvl;

void err_msg_system(err_lvl lvl, const char* path, long ln, const char* fmt, ...);

/**
 * Returns the last part of the path, without modifying anything in memory.
 */
const char *path2basename(const char *path);