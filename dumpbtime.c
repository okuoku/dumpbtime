#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#ifdef __linux__
#include <linux/stat.h>
#include <sys/syscall.h>
#elif defined(__CYGWIN__)
#else
#error Unknown OS
#endif

#ifndef AT_STATX_SYNC_AS_STAT
#define AT_STATX_SYNC_AS_STAT     0x0000  /* mimic stat() */
#endif

#ifndef STATX_BTIME
#define STATX_BTIME               0x0080  /* birth time */
#endif

#ifdef __linux__
/* statx syscall wrapper */
static int statx(int dirfd, const char *pathname, int flags,
          unsigned int mask, struct statx *statxbuf) {
    return syscall(SYS_statx, dirfd, pathname, flags, mask, statxbuf);
}
#endif

/* Print creation (birth) time of the given path */
static void print_btime(const char *path) {
#ifdef __linux__
    struct statx stx;
    int ret = statx(AT_FDCWD, path, AT_STATX_SYNC_AS_STAT, STATX_BTIME, &stx);
    if (ret < 0) {
        fprintf(stderr, "Error: cannot statx '%s': %s\n", path, strerror(errno));
        return;
    }

    if (stx.stx_mask & STATX_BTIME) {
        time_t sec = stx.stx_btime.tv_sec;
        suseconds_t usec = stx.stx_btime.tv_usec;
        printf("%s\t%ldu%ld\n", (long)sec, (long)usec);
    } else {
        fprintf(stderr, "%s: creation time not available\n", path);
    }
#elif defined(__CYGWIN__)
    struct stat st; /* See <cygwin/stat.h> */
    int ret = stat(path, &st);
    if(ret<0){
        fprintf(stderr, "Error: cannot statx '%s': %s\n", path, strerror(errno));
        return;
    }

    time_t sec = st.st_birthtim.tv_sec;
    long nsec = st.st_birthtim.tv_nsec;

    printf("%s\t%ldn%ld\n", path, (long)sec, (long)nsec);
#endif
}

int main(void) {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;

    /* Read each file path from stdin, one per line */
    for(;;){
        read = getline(&line, &len, stdin);
        if(read == -1){
            break;
        }
        /* Strip newline */
        if (read > 0 && (line[read - 1] == '\n' || line[read - 1] == '\r')) {
            line[read - 1] = '\0';
        }
        if (line[0] == '\0') continue;
        print_btime(line);
    }

    free(line);
    return EXIT_SUCCESS;
}

