#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "vterm_reply.h"
#include "vterm_private.h"

void
vterm_reply_buffer(vterm_t *vterm)
{
    ssize_t         bytes_written = 0;

    if(vterm == NULL) return;

    if(vterm->reply_buf_sz <= 0) return;

    for(;;)
    {
        bytes_written = write(vterm->pty_fd, vterm->reply_buf,
                                vterm->reply_buf_sz);

        // force a drain on the pty fd after every write
        tcdrain(vterm->pty_fd);

        if(bytes_written == 0) continue;

        // on some errors, we just need to retry
        if(bytes_written < 0)
        {
            if(errno == EINTR) continue;
            if(errno == EAGAIN) continue;
        }

        vterm->reply_buf_sz -= bytes_written;

        if(vterm->reply_buf_sz == 0) break;
    }

    return;
}
