#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_osc_DA.h"
#include "strings.h"
#include "vterm_reply.h"

int
interpret_osc_DA(vterm_t *vterm, int param[], int pcount)
{
    int     verb;

    if(pcount == 0) verb = 0;
    else verb = param[0];

    // linux behaves very poorly to DA replies.  don't do it.
    if(vterm->flags & VTERM_FLAG_LINUX) return 0;


    /*
        a valid inquiry is verb 0 and 1 (in extended mode which we're not
        supporting right now.
    */
    if(verb == 0)
    {
        memset(vterm->reply_buf, 0, sizeof(vterm->reply_buf));

        if(vterm->flags & VTERM_FLAG_VT100)
        {
            vterm->reply_buf_sz = sizeof(OSC_DA_REPLY_VT100);
            strncpy(vterm->reply_buf,
                OSC_DA_REPLY_VT100,
                vterm->reply_buf_sz);
        }
        else
        {
            vterm->reply_buf_sz = sizeof(OSC_DA_REPLY_XTERM);
            strncpy(vterm->reply_buf,
                OSC_DA_REPLY_XTERM,
                vterm->reply_buf_sz);
        }

        vterm_reply_buffer(vterm);
    }

   return 0;
}

