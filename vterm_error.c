#include <stdio.h>

#include "macros.h"
#include "vterm_private.h"
#include "vterm_error.h"

int
vterm_error(vterm_t *vterm, VtermError ecode, void *anything)
{
    VAR_UNUSED(vterm);

    switch(ecode)
    {
        case VTERM_ECODE_UNHANDLED_CSI:
            fprintf(stderr, "Unrecogized CSI: <%s>\n", (char *)anything);
            return 0;

        case VTERM_ECODE_PTY_WRITE_ERR:
            fprintf(stderr, "WARNING: Failed to write buffer to pty\n");
            return 0;

        case VTERM_ECODE_UNHANDLED_CTRL_CHAR:
            fprintf(stderr,
                "Unrecognized control char: %d (^%c)\n",
                *((char *)anything), *((char *)anything) + '@');
            return 0;
    }

   return 0;
}
