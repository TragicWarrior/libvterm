
#define _GNU_SOURCE

// for SIGIO
#ifdef __FreeBSD__
#define __XSI_VISIBLE 1
#endif

// for SIGIO
#if defined(__APPLE__) && defined(__MACH__)
#define _DARWIN_C_SOURCE 1
#endif

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include "vtty.h"
#include "vtty_private.h"
#include "macros.h"

#define SIG_FDS_R       0
#define SIG_FDS_W       1

volatile int     vtty_sig_fds[2] = { -1, -1 };

void
signal_io(int num, siginfo_t *sinfo, void *anything);

int
vtty_set_async(vtty_t *vtty)
{
    struct sigaction    sa;
    int                 flags;
    int                 i;

    if(vtty == NULL) return -1;

    vtty->flags |= VTTY_FLAG_AIO;

    // common pipe needs to be initialized by first caller
    if(vtty_sig_fds[SIG_FDS_R] == -1 && vtty_sig_fds[SIG_FDS_W] == -1)
    {
        if(pipe((int *)vtty_sig_fds) == -1) return -1;

        // set both ends of the pipe to non-blocking
        for(i = 0; i < 2; i++)
        {
            flags = fcntl(vtty_sig_fds[i], F_GETFL);
            flags |= O_NONBLOCK;
            fcntl(vtty_sig_fds[i], F_SETFL, flags);
        }
    }

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = signal_io;
    sigaction(SIGIO, &sa, NULL);

    fcntl(vtty->pty_fd, F_SETOWN, getpid());
    fcntl(vtty->pty_fd, F_SETFL, O_ASYNC);
    // fcntl(vtty->pty_fd, F_SETSIG, SIGIO);

    return vtty_sig_fds[SIG_FDS_R];
}

void
signal_io(int num, siginfo_t *sinfo, void *anything)
{
    int             retval;
    uint8_t         val;

    VAR_UNUSED(sinfo);
    VAR_UNUSED(anything);

    for(;;)
    {
        val = num;

        retval = write(vtty_sig_fds[SIG_FDS_W], &val, sizeof(uint8_t));

        if(retval == -1)
        {
            if(errno == EINTR) continue;
            if(errno == EAGAIN) continue;

        }

        return;
    }

    return;
}
