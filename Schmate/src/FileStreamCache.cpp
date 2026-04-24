#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include "Logger.hpp"

// This file contains fopen_high
//
// this tires to assure that a FILE gets a handle > WATERMARK
// as some libs close down handles under 10 for their own use..
// Why? Just treating the symptom!


namespace schmate_util {
  FILE * fopen_high(const char *path, const char *mode) ;

#define FD_WATERMARK 100  /* Minimum FD number we want */

/**
 * fopen_high - Open a file, ensuring the underlying fd is >= FD_WATERMARK.
 *
 * Some libraries sometimes close low-numbered fds internally.
 * By duplicating the fd to a high slot we stay out of their reach.
 *
 * Returns a FILE* on success, NULL on failure (errno is set).
 */
FILE *fopen_high(const char *path, const char *mode)
{
    /* 1. Open normally */
    FILE *f = fopen(path, mode);
    if (!f)
        return NULL;

    int old_fd = fileno(f);

    /* 2. Already high enough – nothing to do */
    if (old_fd >= FD_WATERMARK)
        return f;

    /* 3. dup the fd to the first free slot at or above the watermark */
    int new_fd = fcntl(old_fd, F_DUPFD, FD_WATERMARK);
    if (new_fd < 0) {
        /* Could not raise – return as-is, or treat as fatal: your call */
       LOG_ERROR_S() <<  "fopen_high: fcntl F_DUPFD\n";
        return f;           /* fall back to the low fd */
    }

    /* 4. Reopen the FILE* around the new (high) fd.
     *    fdopen takes ownership of new_fd. */
    FILE *f_high = fdopen(new_fd, mode);
    if (!f_high) {
        LOG_ERROR_S() << "fopen_high: fdopen\n";
        close(new_fd);
        return f;           /* fall back */
    }

    /* 5. Close the original low-numbered FILE* (also closes old_fd) */
    fclose(f);
    return f_high;
}
    
} // namespace}
