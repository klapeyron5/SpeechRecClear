#include <stdlib.h>

/*
 * Исключительно для дебага. Отладка через консоль.
 */

/**
 * Exit with non-zero status after error message
 */
#define E_FATAL(...)                               \
    do {                                           \
        err_msg(ERR_FATAL, FILELINE, __VA_ARGS__); \
        exit(EXIT_FAILURE);                        \
    } while (0)

/**
 * Print error text; Call perror(""); exit(errno);
 */
#define E_FATAL_SYSTEM(...)									\
    do {													\
        err_msg_system(ERR_FATAL, FILELINE, __VA_ARGS__);	\
        exit(EXIT_FAILURE);									\
    } while (0)

/**
 * Print error message to error log
 */
#define E_ERROR(...)     err_msg(ERR_ERROR, FILELINE, __VA_ARGS__)

/**
 * Print logging information to standard error stream
 */
#define E_INFO(...)      err_msg(ERR_INFO, FILELINE, __VA_ARGS__)

#define FILELINE __FILE__,__LINE__

typedef enum err_e {
    ERR_DEBUG,
    ERR_INFO,
    ERR_INFOCONT,
    ERR_WARN,
    ERR_ERROR,
    ERR_FATAL,
    ERR_MAX
} err_lvl_t;

void err_msg(err_lvl_t lvl, const char* path, long ln, const char* fmt, ...);
void err_msg_system(err_lvl_t lvl, const char* path, long ln, const char* fmt, ...);

/**
 * Returns the last part of the path, without modifying anything in memory.
 */
const char *path2basename(const char *path);