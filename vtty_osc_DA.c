#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "vtty.h"
#include "vtty_private.h"
#include "vtty_osc_DA.h"
#include "strings.h"
#include "vtty_reply.h"

int
interpret_osc_DA(vtty_t *vtty, int param[], int pcount)
{
    int     verb;

    if(pcount == 0) verb = 0;
    else verb = param[0];

    // linux behaves very poorly to DA replies.  don't do it.
    if(vtty->flags & VTTY_FLAG_LINUX) return 0;


    /*
        a valid inquiry is verb 0 and 1 (in extended mode which we're not
        supporting right now.
    */
    if(verb == 0)
    {
        memset(vtty->reply_buf, 0, sizeof(vtty->reply_buf));

        if(vtty->flags & VTTY_FLAG_VT100)
        {
            vtty->reply_buf_sz = sizeof(OSC_DA_REPLY_VT100);
            strncpy(vtty->reply_buf,
                OSC_DA_REPLY_VT100,
                vtty->reply_buf_sz);
        }
        else
        {
            vtty->reply_buf_sz = sizeof(OSC_DA_REPLY_XTERM);
            strncpy(vtty->reply_buf,
                OSC_DA_REPLY_XTERM,
                vtty->reply_buf_sz);
        }

        vtty_reply_buffer(vtty);
    }

   return 0;
}

