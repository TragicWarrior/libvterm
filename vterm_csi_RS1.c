
#include <string.h>

#include "vterm.h"
#include "vterm_cursor.h"
#include "vterm_csi.h"
#include "vterm_private.h"
#include "vterm_buffer.h"


/*
    we're looking for a very specific sequence.  using a protothread here
    would probably make sense, but it's pretty easy to do this with some
    statics as well.
*/
int
interpret_csi_RS1_rxvt(vterm_t *vterm, char *byte)
{
    static char     *end = RXVT_RS1 + sizeof(RXVT_RS1) - 2;
    static char     *pos = RXVT_RS1;

    // if we get a stray byte, reset sequence parser
    if(byte[0] != pos[0])
    {
        pos = RXVT_RS1;
        return 0;
    }

    // do the bytes match and are we at the end
    if((pos[0] == end[0] && pos == end))
    {
        // reset to standard buffer (and add other stuff if ncessary)
        vterm_buffer_set_active(vterm, VTERM_BUF_STANDARD);

        // make cursor always visible after a reset
        vterm_cursor_show(vterm, VTERM_BUF_STANDARD);

        // reset the parser
        pos = RXVT_RS1;

        return 0;
    }

    pos++;

    return 0;
}

int
interpret_csi_RS1_xterm(vterm_t *vterm, char *data)
{
    if(vterm == NULL) return -1;

    // safety check
    if(strncmp(data, XTERM_RS1, strlen(XTERM_RS1)) != 0) return -1;

    /*
        although the RXVT sequence is a bizarre signal for resetting the
        terminal, the sequence itself is very effective.  push it through
        the renderering engine to affect a reset.
    */
    vterm_render(vterm, RXVT_RS1, sizeof(RXVT_RS1) - 1);

    // reset to standard buffer (and add other stuff if ncessary)
    vterm_buffer_set_active(vterm, VTERM_BUF_STANDARD);

    // make cursor always visible after a reset
    vterm_cursor_show(vterm, VTERM_BUF_STANDARD);

    return 0;
}
