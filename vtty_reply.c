#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "vtty_reply.h"
#include "vtty_private.h"

void
vtty_reply_buffer(vtty_t *vtty)
{
    ssize_t         bytes_written = 0;

    if(vtty == NULL) return;

    if(vtty->reply_buf_sz <= 0) return;

    for(;;)
    {
        bytes_written = write(vtty->pty_fd, vtty->reply_buf,
                                vtty->reply_buf_sz);

        // force a drain on the pty fd after every write
        tcdrain(vtty->pty_fd);

        if(bytes_written == 0) continue;

        // on some errors, we just need to retry
        if(bytes_written < 0)
        {
            if(errno == EINTR) continue;
            if(errno == EAGAIN) continue;
        }

        vtty->reply_buf_sz -= bytes_written;

        if(vtty->reply_buf_sz == 0) break;
    }

    return;
}
