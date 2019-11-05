
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <poll.h>
#include <errno.h>
#include <limits.h>
#include <fcntl.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_render.h"
#include "vterm_reply.h"

/*
    PIPE_BUF is defined as the maximum number of bytes that can be
    written to a pipe atomically.  On many systems, this value
    is relatively low (around 4KB).  We'll set our read size to
    a multiple of this to prevent blocking in the child
    process.
*/
// #define MAX_PIPE_READ   (PIPE_BUF * 8)


ssize_t
vterm_read_pipe(vterm_t *vterm, int timeout)
{
    struct pollfd   fd_array[1];
    char            *pos;
    int             bytes_peek = 0;
	size_t			bytes_waiting = 0;
    ssize_t			bytes_read = 0;
    ssize_t         bytes_written = 0;
    size_t          bytes_remaining = 0;
    size_t          bytes_total = 0;
    int             retval;
    pid_t           child_pid;
    int             pid_status;
    int             errcpy = 0;
#ifdef _DEBUG
    FILE            *f_debug;
    char            debug_file[NAME_MAX];
#endif

    // saftey precaution
    if(vterm == NULL) return -1;

    // make sure pty file descriptor is good
    if(vterm->pty_fd < 0) return -1;

    // check to see if child pid has exited
    child_pid = waitpid(vterm->child_pid, &pid_status, WNOHANG);

    if(child_pid == vterm->child_pid || child_pid == -1)
    {
        vterm->internal_state |= STATE_CHILD_EXITED;
        return -1;
    }

    // don't use poll in async io mode
    if(!(vterm->flags & VTERM_FLAG_AIO))
    {
        fd_array[0].fd = vterm->pty_fd;
        fd_array[0].events = POLLIN;

        // wait 10 millisecond for data on pty file descriptor.
        retval = poll(fd_array, 1, timeout);

        // no data or poll() error.
        if(retval <= 0)
        {
            if(errno == EINTR) return 0;
            return retval;
        }
    }

    retval = 0;
    bytes_peek = 0;

#ifdef TIOCINQ
    retval = ioctl(vterm->pty_fd, TIOCINQ, &bytes_peek);
#else
    retval = ioctl(vterm->pty_fd, FIONREAD, &bytes_peek);
#endif

    if(retval == -1) return 0;


#if defined(__APPLE__) && defined(__MACH__)
    /*
        the FIONREAD ioctl() seems to be busted on Mac OS.  the
        ioctl will claim 0 bytes to read.  setting it to a value
        of PIPE buf and then expecting what read() actually
        does is the best way to handle this.
    */
    if(bytes_peek == 0) bytes_peek = PIPE_BUF;
#endif

    if(bytes_peek == 0) return 0;

    bytes_waiting = bytes_peek;

    // see notes at top of file regarding MAX_PIPE_READ and PIPE_BUF
    if(bytes_waiting > MAX_PIPE_READ) bytes_waiting = MAX_PIPE_READ;
    bytes_remaining = bytes_waiting;

	// 10 byte padding
	// buf = (char *)calloc(bytes_waiting + 10, sizeof(char));
    // pos = buf;
    pos = vterm->read_buf;

    do
    {
        bytes_read = read(vterm->pty_fd, pos, bytes_remaining);
        if(bytes_read == -1)
        {
            // retry if interrupted by a signal
            if(errno == EINTR)
            {
                bytes_read = 0;
                continue;
            }
            else errcpy = errno;
        }

        if(bytes_read <= 0) break;

#if defined(__APPLE__) && defined(__MACH__)
        // shore up bytes read (see MacOS ioctl comment above)
        bytes_remaining = bytes_read;
#endif

        bytes_remaining -= bytes_read;
        bytes_total += bytes_read;
        pos += bytes_read;
    }
    while(bytes_remaining > 0);

    // render the data to the offscreen terminal buffer.
    if((bytes_waiting > 0) && (bytes_read != -1))
    {
        // write debug information if enabled
        if(vterm->flags & VTERM_FLAG_DUMP)
        {
            bytes_written = write(vterm->debug_fd,
                (const void *)vterm->read_buf, bytes_read);
            if( bytes_written != bytes_read )
            {
                fprintf(stderr,
                    "ERROR: wrote fewer bytes than read (%d w / %d r)\n",
                    (int)bytes_written, (int)bytes_read);

                if(bytes_written == -1)
                    fprintf(stderr, "%s\n", strerror(errno));
            }
        }

        // vterm_render(vterm, buf, bytes_read);
        vterm_render(vterm, vterm->read_buf, bytes_read);
    }

    // release memory
    // if(buf != NULL) free(buf);

    if(bytes_read == -1 && errcpy != EINTR) return -1;

    if(vterm->event_mask & VTERM_MASK_PIPE_READ)
    {
        if(vterm->event_hook != NULL)
        {
            vterm->event_hook(vterm,
                VTERM_EVENT_PIPE_READ, (void *)&bytes_read);
        }
    }

    return bytes_waiting - bytes_remaining;
}

