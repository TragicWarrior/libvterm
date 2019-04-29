#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_osc_DA.h"
#include "strings.h"

int
interpret_osc_DA(vterm_t *vterm)
{
    ssize_t     bytes_written = 0;
    int         retval;

    bytes_written = write(vterm->pty_fd, OSC_DA_REPLY, sizeof(OSC_DA_REPLY));
    if(bytes_written != sizeof(OSC_DA_REPLY))
    {
        fprintf(stderr, "WARNING: Failed to write buffer to pty\n");
            retval = -1;
    }

   return retval;
}

