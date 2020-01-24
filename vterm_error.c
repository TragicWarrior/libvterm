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
#ifdef DEBUG
            fprintf(stderr, "Unrecogized CSI: <%s>\n", (char *)anything);
#endif
            return 0;

        case VTERM_ECODE_PTY_WRITE_ERR:
            fprintf(stderr, "WARNING: Failed to write buffer to pty\n");
            return 0;

        case VTERM_ECODE_UNHANDLED_CTRL_CHAR:
            fprintf(stderr,
                "Unrecognized control char: %d (^%c)\n",
                *((char *)anything), *((char *)anything) + '@');
            return 0;

        case VTERM_ECODE_UTF8_MARKER_ERR:
            /*
                U+FFFD is the Unicode designated replacement character
                "used to indicate problems when a system is unable to render
                a stream of data to a correct symbol"

                TODO:  Check the UTF8 error handling mode (strict, lazy,
                none, etc...) and respond accordingly.
            */
            *((wchar_t *)anything) = 0xFFFD;
            return 0;
    }

   return 0;
}
