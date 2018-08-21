#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "err.h"
#include "prim_type.h"

typedef void (*err_cb_f)(void* user_data, err_lvl_t, const char *, ...);

void err_logfp_cb(void* user_data, err_lvl_t lvl, const char* fmt, ...);

static err_cb_f err_cb = err_logfp_cb;
static void* err_user_data;

static FILE*  logfp = NULL;
static int    logfp_disabled = FALSE;

void err_msg(err_lvl_t lvl, const char *path, long ln, const char *fmt, ...)
{
    static const char *err_prefix[ERR_MAX] = {
        "DEBUG", "INFO", "INFOCONT", "WARN", "ERROR", "FATAL"
    };

    char msg[1024];
    va_list ap;

    if (!err_cb)
        return;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    if (path) {
        const char *fname = path2basename(path);
        if (lvl == ERR_INFOCONT)
    	    err_cb(err_user_data, lvl, "%s(%ld): %s", fname, ln, msg);
        else if (lvl == ERR_INFO)
            err_cb(err_user_data, lvl, "%s: %s(%ld): %s", err_prefix[lvl], fname, ln, msg);
        else
    	    err_cb(err_user_data, lvl, "%s: \"%s\", line %ld: %s", err_prefix[lvl], fname, ln, msg);
    } else {
        err_cb(err_user_data, lvl, "%s", msg);
    }
}

void err_msg_system(err_lvl_t lvl, const char* path, long ln, const char* fmt, ...) {
    int local_errno = errno;
    
    static const char* err_prefix[ERR_MAX] = {
        "DEBUG", "INFO", "INFOCONT", "WARN", "ERROR", "FATAL"
    };

    char msg[1024];
    va_list ap;

    if (!err_cb)
        return;

    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    if (path) {
        const char *fname = path2basename(path);
        if (lvl == ERR_INFOCONT)
    	    err_cb(err_user_data, lvl, "%s(%ld): %s: %s\n", fname, ln, msg, strerror(local_errno));
        else if (lvl == ERR_INFO)
            err_cb(err_user_data, lvl, "%s: %s(%ld): %s: %s\n", err_prefix[lvl], fname, ln, msg, strerror(local_errno));
        else
    	    err_cb(err_user_data, lvl, "%s: \"%s\", line %ld: %s: %s\n", err_prefix[lvl], fname, ln, msg, strerror(local_errno));
    } else {
        err_cb(err_user_data, lvl, "%s: %s\n", msg, strerror(local_errno));
    }
}

FILE* err_get_logfp(void) {
    if (logfp_disabled)
	return NULL;
    if (logfp == NULL)
	return stderr;

    return logfp;
}

void err_logfp_cb(void* user_data, err_lvl_t lvl, const char* fmt, ...) {
    va_list ap;
    FILE* fp = err_get_logfp();

    if (!fp)
        return;
    
    va_start(ap, fmt);
    vfprintf(fp, fmt, ap);
    va_end(ap);
}

const char* path2basename(const char *path) {
    const char *result;

#if defined(_WIN32) || defined(__CYGWIN__)
    result = strrchr(path, '\\');
#else
    result = strrchr(path, '/');
#endif

    return (result == NULL ? path : result + 1);
}