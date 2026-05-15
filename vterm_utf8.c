#ifndef NOUTF8
#include <string.h>
#include <inttypes.h>
#include <stdlib.h>
#include <wchar.h>

#ifdef __FreeBSD__
#include <ncurses/ncurses.h>
#endif

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_buffer.h"
#include "vterm_error.h"

// this is necessary for FreeBSD in some compilation scenarios
#ifndef CCHARW_MAX
#define CCHARW_MAX  5
#endif


void
vterm_utf8_start(vterm_t *vterm)
{
    vterm->internal_state |= STATE_UTF8_MODE;

    // zero out the utf-8 buffer just in case
    vterm->utf8_buf_len = 0;
    vterm->utf8_buf[0] = '\0';

    return;
}

void
vterm_utf8_cancel(vterm_t *vterm)
{
    vterm->internal_state &= ~STATE_UTF8_MODE;

    // zero out the utf-8 buffer for the next run
    vterm->utf8_buf_len = 0;
    vterm->utf8_buf[0] = '\0';

    return;
}

int
vterm_utf8_decode(vterm_t *vterm, chtype *utf8_char, wchar_t *wch)
{
    unsigned char       lead = (unsigned char)vterm->utf8_buf[0];
    uint32_t            utf8_code;
    wchar_t             cp;
    int                 byte_count;
    int                 i;

    if(lead < 0xC0) byte_count = 1;
    else if(lead < 0xE0) byte_count = 2;
    else if(lead < 0xF0) byte_count = 3;
    else byte_count = 4;

    if(vterm->utf8_buf_len < byte_count) return -1;

    utf8_code = lead;
    cp = lead & (0x7F >> byte_count);

    for(i = 1; i < byte_count; i++)
    {
        unsigned char b = (unsigned char)vterm->utf8_buf[i];

        if((b & 0xC0) != 0x80)
        {
            vterm_error(vterm, VTERM_ECODE_UTF8_MARKER_ERR, (void *)wch);
            return byte_count;
        }

        utf8_code = (utf8_code << 8) | b;
        cp = (cp << 6) | (b & 0x3F);
    }

    /*
        this is for legacy mode.  it will probably be removed in
        the future.
    */
    switch (utf8_code)
    {
        /*
            these should map to ACS Teletype 5410v1 symbols but that
            doesn't seem to work on many terminals so just render the
            ascii doppelganger.
        */
        case 0x00E28092:
        case 0x00E28093:    { *utf8_char = '-';                 break;}

        case 0x00E28690:    { *utf8_char = '<';                 break;}
        case 0x00E28691:    { *utf8_char = '^';                 break;}
        case 0x00E28692:    { *utf8_char = '>';                 break;}
        case 0x00E28693:    { *utf8_char = 'v';                 break;}

        case 0x00E296AE:    { *utf8_char = '#';                 break;}

#ifndef NOCURSES
        case 0x0000C2B7:    { *utf8_char = ACS_BULLET;          break;}

        case 0x00E28094:
        case 0x00E28095:    { *utf8_char = ACS_HLINE;           break;}

        // double line and single line (heavy and light) horizontal line
        case 0x00E29590:
        case 0x00E29480:
        case 0x00E29481:    { *utf8_char = ACS_HLINE;           break;}

        // double line and single line (heavy and light) vertical line
        case 0x00E29591:
        case 0x00E29482:
        case 0x00E29483:    { *utf8_char = ACS_VLINE;           break;}

        // double line and single line upper-left corner
        case 0x00E29594:
        case 0x00E2948C:    { *utf8_char = ACS_ULCORNER;        break;}

        // double line and single line upper-right corner
        case 0x00E29597:
        case 0x00E29490:    { *utf8_char = ACS_URCORNER;        break;}

        // double line and single line lower-left corner
        case 0x00E2959A:
        case 0x00E29494:    { *utf8_char = ACS_LLCORNER;        break;}

        // double line and single line lower-right corner
        case 0x00E2959D:
        case 0x00E29498:    { *utf8_char = ACS_LRCORNER;        break;}

        // variations of double and single line top tees
        case 0x00E2959E:
        case 0x00E2959F:
        case 0x00E295A0:
        case 0x00E2949C:    { *utf8_char = ACS_LTEE;            break;}

        case 0x00E295A1:
        case 0x00E295A2:
        case 0x00E295A3:
        case 0x00E294A4:    { *utf8_char = ACS_RTEE;            break;}

        // variations of double and single line top tees
        case 0x00E295A4:
        case 0x00E295A5:
        case 0x00E295A6:
        case 0x00E294AC:	{ *utf8_char = ACS_TTEE;            break;}

        // variations of double and single line bottom tees
        case 0x00E295A7:
        case 0x00E295A8:
        case 0x00E295A9:
        case 0x00E294B4:    { *utf8_char = ACS_BTEE;            break;}

        // variations of double and single line 4-way tees
        case 0x00E295AA:
        case 0x00E295AB:
        case 0x00E295AC:
        case 0x00E294BC:    { *utf8_char = ACS_PLUS;            break;}
        case 0x00E2958B:    { *utf8_char = ACS_PLUS;            break;}

        case 0x00E29688:    { *utf8_char = ACS_CKBOARD;         break;}

        case 0x00E29691:
        case 0x00E29692:
        case 0x00E29693:    { *utf8_char = ACS_CKBOARD;         break;}
#else
        case 0x0000C2B7:    { *utf8_char = 'o';                 break;}

        case 0x00E28094:
        case 0x00E28095:    { *utf8_char = '-';                 break;}

        case 0x00E29480:
        case 0x00E29481:    { *utf8_char = '-';                 break;}

        case 0x00E29482:
        case 0x00E29483:    { *utf8_char = '|';                 break;}

        case 0x00E2948C:    { *utf8_char = '+';                 break;}
        case 0x00E29490:    { *utf8_char = '+';                 break;}
        case 0x00E29494:    { *utf8_char = '+';                 break;}
        case 0x00E29498:    { *utf8_char = '+';                 break;}

        case 0x00E29688:    { *utf8_char = '#';                 break;}

        case 0x00E2949C:    { *utf8_char = '+';                 break;}
        case 0x00E294A4:    { *utf8_char = '+';                 break;}
        case 0x00E294AC:	{ *utf8_char = '+';                 break;}
        case 0x00E294B4:    { *utf8_char = '+';                 break;}

        case 0x00E29691:
        case 0x00E29692:
        case 0x00E29693:    { *utf8_char = '#';                 break;}
#endif

        // render a harmless space when we don't know what to do
        default:            { *utf8_char = ' ';                 break;}
    }

    wch[0] = cp;
    wch[1] = L'\0';

    return byte_count;
}
#endif
