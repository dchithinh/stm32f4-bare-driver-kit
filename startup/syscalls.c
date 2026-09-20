/**
 * @file syscalls.c
 * @brief Minimal newlib stubs so we do not pull libnosys warning objects.
 *
 * These are not drivers. _write/_read can later be wired to bdk_uart if wanted.
 */

#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>

extern char end; /* linker script */

void *_sbrk(ptrdiff_t incr)
{
    static char *heap_end;
    char *prev;

    if (heap_end == NULL) {
        heap_end = &end;
    }

    prev = heap_end;
    heap_end += incr;
    return prev;
}

int _close(int fd)
{
    (void)fd;
    errno = EBADF;
    return -1;
}

int _fstat(int fd, struct stat *st)
{
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}

int _isatty(int fd)
{
    (void)fd;
    return 1;
}

int _lseek(int fd, int offset, int whence)
{
    (void)fd;
    (void)offset;
    (void)whence;
    return 0;
}

int _read(int fd, char *buf, int count)
{
    (void)fd;
    (void)buf;
    (void)count;
    return 0;
}

int _write(int fd, char *buf, int count)
{
    (void)fd;
    (void)buf;
    return count;
}

void _exit(int status)
{
    (void)status;
    for (;;) {
    }
}

int _kill(int pid, int sig)
{
    (void)pid;
    (void)sig;
    errno = EINVAL;
    return -1;
}

int _getpid(void)
{
    return 1;
}
