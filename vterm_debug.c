/*
    Compiled only into libvterm-debug.so (cmake target `vterm-debug`).
    A __attribute__((constructor)) installs signal handlers for the usual
    fatal signals; on delivery, a glibc backtrace is dumped to
        ./vterm-crash-<pid>.log
    in the process's current working directory.  After dumping, the
    handler restores the default disposition and re-raises so the process
    still terminates (and a core file lands if rlimits allow).
*/

#ifdef VTERM_DEBUG_BACKTRACE

#define _GNU_SOURCE

#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define BT_MAX  128

static void                     *bt_buf[BT_MAX];
static volatile sig_atomic_t     crashing = 0;

/* async-signal-safe unsigned-to-decimal; returns digit count written */
static int
_vd_utoa(unsigned long v, char *buf)
{
    char    tmp[32];
    int     i = 0;
    int     j = 0;

    if(v == 0) { buf[0] = '0'; return 1; }
    while(v > 0) { tmp[i++] = '0' + (v % 10); v /= 10; }
    while(i > 0) buf[j++] = tmp[--i];
    return j;
}

/* async-signal-safe unsigned-to-hex (no 0x prefix) */
static int
_vd_xtoa(unsigned long v, char *buf)
{
    static const char hex[] = "0123456789abcdef";
    char    tmp[32];
    int     i = 0;
    int     j = 0;

    if(v == 0) { buf[0] = '0'; return 1; }
    while(v > 0) { tmp[i++] = hex[v & 0xF]; v >>= 4; }
    while(i > 0) buf[j++] = tmp[--i];
    return j;
}

static const char *
_vd_signame(int sig)
{
    switch(sig)
    {
        case SIGSEGV:   return "SIGSEGV";
        case SIGABRT:   return "SIGABRT";
        case SIGBUS:    return "SIGBUS";
        case SIGFPE:    return "SIGFPE";
        case SIGILL:    return "SIGILL";
    }
    return "SIG?";
}

static void
_vd_handler(int sig, siginfo_t *si, void *ctx)
{
    char        path[64] = "vterm-crash-";
    char        num[32];
    int         fd;
    int         n;
    int         off;
    const char *name;

    (void)ctx;

    /* fault inside the handler -> bail rather than recurse */
    if(crashing) _exit(128 + sig);
    crashing = 1;

    off = (int)strlen(path);
    off += _vd_utoa((unsigned long)getpid(), &path[off]);
    memcpy(&path[off], ".log", 5);              /* includes NUL */

    fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd < 0) fd = STDERR_FILENO;

    write(fd, "libvterm crash: ", 16);
    name = _vd_signame(sig);
    write(fd, name, strlen(name));
    write(fd, " (sig ", 6);
    n = _vd_utoa((unsigned long)sig, num);
    write(fd, num, n);
    write(fd, ") pid ", 6);
    n = _vd_utoa((unsigned long)getpid(), num);
    write(fd, num, n);

    if(si != NULL && (sig == SIGSEGV || sig == SIGBUS))
    {
        write(fd, " fault_addr 0x", 14);
        n = _vd_xtoa((unsigned long)si->si_addr, num);
        write(fd, num, n);
    }

    write(fd, " time ", 6);
    n = _vd_utoa((unsigned long)time(NULL), num);
    write(fd, num, n);
    write(fd, "\n", 1);

    write(fd,
        "(resolve addresses via:  addr2line -e /usr/local/lib/libvterm.so <addr>)\n",
        73);
    write(fd, "--- backtrace ---\n", 18);

    n = backtrace(bt_buf, BT_MAX);
    backtrace_symbols_fd(bt_buf, n, fd);

    write(fd, "--- end backtrace ---\n", 22);

    if(fd != STDERR_FILENO) close(fd);

    /* restore default disposition and re-raise so the process actually
       dies (and may core), preserving normal post-mortem signal behavior */
    signal(sig, SIG_DFL);
    raise(sig);
}

__attribute__((constructor))
static void
_vd_init(void)
{
    struct sigaction    sa;
    void               *dummy[4];

    /*
        prime libgcc_s so the first call to backtrace() (which dlopens it)
        doesn't happen inside the signal handler -- dlopen is not
        async-signal-safe.
    */
    backtrace(dummy, 4);

    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = _vd_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS,  &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE,  &sa, NULL);
    sigaction(SIGILL,  &sa, NULL);
}

#endif /* VTERM_DEBUG_BACKTRACE */
