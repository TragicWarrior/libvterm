
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

#include "vterm.h"
#include "vterm_private.h"
#include "macros.h"

#define SIG_FDS_R       0
#define SIG_FDS_W       1

volatile int     vterm_sig_fds[2] = { -1, -1 };

void
signal_io(int num, siginfo_t *sinfo, void *anything);

int
vterm_set_async(vterm_t *vterm)
{
    struct sigaction    sa;
    int                 flags;
    int                 i;

    if(vterm == NULL) return -1;

    vterm->flags |= VTERM_FLAG_AIO;

    // common pipe needs to be initialized by first caller
    if(vterm_sig_fds[SIG_FDS_R] == -1 && vterm_sig_fds[SIG_FDS_W] == -1)
    {
        if(pipe((int *)vterm_sig_fds) == -1) return -1;

        // set both ends of the pipe to non-blocking
        for(i = 0; i < 2; i++)
        {
            flags = fcntl(vterm_sig_fds[i], F_GETFL);
            flags |= O_NONBLOCK;
            fcntl(vterm_sig_fds[i], F_SETFL, flags);
        }
    }

    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = signal_io;
    sigaction(SIGIO, &sa, NULL);

    fcntl(vterm->pty_fd, F_SETOWN, getpid());
    fcntl(vterm->pty_fd, F_SETFL, O_ASYNC);
    // fcntl(vterm->pty_fd, F_SETSIG, SIGIO);

    return vterm_sig_fds[SIG_FDS_R];
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

        retval = write(vterm_sig_fds[SIG_FDS_W], &val, sizeof(uint8_t));

        if(retval == -1)
        {
            if(errno == EINTR) continue;
            if(errno == EAGAIN) continue;

        }

        return;
    }

    return;
}
