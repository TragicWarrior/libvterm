#ifndef NOUTF8
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <wchar.h>

#ifdef __FreeBSD__
#include <ncurses/ncurses.h>
#endif

#include "vtty.h"
#include "vtty_private.h"
#include "vtty_buffer.h"
#include "vtty_error.h"

// this is necessary for FreeBSD in some compilation scenarios
#ifndef CCHARW_MAX
#define CCHARW_MAX  5
#endif


void
vtty_utf8_start(vtty_t *vtty)
{
    vtty->internal_state |= STATE_UTF8_MODE;

    // zero out the utf-8 buffer just in case
    vtty->utf8_buf_len = 0;
    vtty->utf8_buf[0] = '\0';

    return;
}

void
vtty_utf8_cancel(vtty_t *vtty)
{
    vtty->internal_state &= ~STATE_UTF8_MODE;

    // zero out the utf-8 buffer for the next run
    vtty->utf8_buf_len = 0;
    vtty->utf8_buf[0] = '\0';

    return;
}

int
vtty_utf8_decode(vtty_t *vtty, wchar_t *wch)
{
    unsigned char       lead = (unsigned char)vtty->utf8_buf[0];
    wchar_t             cp;
    int                 byte_count;
    int                 i;

    if(lead < 0xC0) byte_count = 1;
    else if(lead < 0xE0) byte_count = 2;
    else if(lead < 0xF0) byte_count = 3;
    else byte_count = 4;

    if(vtty->utf8_buf_len < byte_count) return -1;

    cp = lead & (0x7F >> byte_count);

    for(i = 1; i < byte_count; i++)
    {
        unsigned char b = (unsigned char)vtty->utf8_buf[i];

        if((b & 0xC0) != 0x80)
        {
            vtty_error(vtty, VTTY_ECODE_UTF8_MARKER_ERR, (void *)wch);
            return byte_count;
        }

        cp = (cp << 6) | (b & 0x3F);
    }

    wch[0] = cp;
    wch[1] = L'\0';

    return byte_count;
}
#endif
