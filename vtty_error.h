#ifndef _VTTY_ERROR_H_
#define _VTTY_ERROR_H_

#include "vtty.h"

typedef enum
{
    VTTY_ECODE_UNHANDLED_CSI            =   100,
    VTTY_ECODE_PTY_WRITE_ERR,
    VTTY_ECODE_UNHANDLED_CTRL_CHAR,
    VTTY_ECODE_UTF8_MARKER_ERR,
}
VttyError;

int     vtty_error(vtty_t *vtty, VttyError ecode, void *anything);

#endif
