#include "general.h"

double TimeDifference(TimeSpec *a, TimeSpec *b) {
    return (double)(a->tv_sec  - b->tv_sec)
         + (double)(a->tv_nsec - b->tv_nsec) * 1e-9;
}

char* ReadFile(const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) { perror(path); return NULL; }
    struct stat st;
    fstat(fd, &st);
    char *buf = malloc(st.st_size + 1);
    if (!buf) { close(fd); return NULL; }
    ssize_t n = read(fd, buf, st.st_size);
    close(fd);
    if (n < 0) { free(buf); return NULL; }
    buf[n] = '\0';
    return buf;
}
