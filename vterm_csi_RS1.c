
#include <string.h>

#include "vterm.h"
#include "vterm_csi.h"
#include "vterm_private.h"
#include "vterm_buffer.h"

/*
    we're looking for a very specific sequence.  using a protothread here
    would probably make sense, but it's pretty easy to do this with some
    statics as well.
*/
void
interpret_csi_RS1(vterm_t *vterm, char *byte)
{
    static char     *end = RXVT_RS1 + strlen(RXVT_RS1) - 1;
    static char     *pos = RXVT_RS1;

    // when in VT100 mode this is not set
    if(vterm->reset_rs1 == NULL) return;

    // if we get a stray byte, reset sequence parser
    if(byte[0] != pos[0])
    {
        pos = RXVT_RS1;
        return;
    }

    // do the bytes match and are we at the end
    if((pos[0] == end[0] && pos == end))
    {
        // reset to standard buffer (and add other stuff if ncessary)
        vterm_buffer_set_active(vterm, VTERM_BUFFER_STD);

        // reset the parser
        pos = RXVT_RS1;
        return;
    }

    pos++;
}
