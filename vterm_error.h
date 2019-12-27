#ifndef _VTERM_ERROR_H_
#define _VTERM_ERROR_H_

#include "vterm.h"

typedef enum
{
    VTERM_ECODE_UNHANDLED_CSI           =   100,
    VTERM_ECODE_PTY_WRITE_ERR,
    VTERM_ECODE_UNHANDLED_CTRL_CHAR,
}
VtermError;

int     vterm_error(vterm_t *vterm, VtermError ecode, void *anything);

#endif
