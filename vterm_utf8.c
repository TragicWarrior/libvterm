/*
LICENSE INFORMATION:
This program is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License (LGPL) as published by the Free Software Foundation.

Please refer to the COPYING file for more information.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

Copyright (c) 2017 Bryan Christ
*/

#include <string.h>
#include <inttypes.h>

#include "vterm.h"
#include "vterm_private.h"

/*
    the way UTF-8 is encoded, we can just test the highest bit, pop it
    off, and count, in order to know how many bytes are in the code
    sequence.
*/
#define UTF8_BIT_MASK       0x80        // binary 10000000

void
vterm_utf8_start(vterm_t *vterm)
{
    vterm->state |= STATE_UTF8_MODE;

    // zero out the utf-8 buffer just in case
    vterm->utf8_buf_len = 0;
    vterm->utf8_buf[0] = '\0';

    return;
}

void
vterm_utf8_cancel(vterm_t *vterm)
{
    vterm->state &= ~STATE_UTF8_MODE;

    // zero out the utf-8 buffer for the next run
    vterm->utf8_buf_len = 0;
    vterm->utf8_buf[0] = '\0';

    return;
}

int
vterm_utf8_decode(vterm_t *vterm, chtype *utf8_char)
{
    uint32_t            utf8_code = 0;
    uint8_t             first_byte = 0;
    int                 byte_count = 0;
    int                 i = 0;

    first_byte = (uint8_t)vterm->utf8_buf[0];

    while (first_byte & UTF8_BIT_MASK)
    {
        byte_count++;
        first_byte = first_byte << 1;
    }

    // too early to decode.  return back for more reading.
    if (vterm->utf8_buf_len < byte_count) return -1;

    // TODO:  this is a band-aid

    for (i = 0; i < byte_count; i++)
    {
        utf8_code |= (uint8_t)vterm->utf8_buf[i];

        // don't shift on last byte
        if (i + 1 >= byte_count) break;

        utf8_code = utf8_code << 8;
    }

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

        case 0x00E29480:
        case 0x00E29481:    { *utf8_char = ACS_HLINE;           break;}

        case 0x00E29482:
        case 0x00E29483:    { *utf8_char = ACS_VLINE;           break;}

        case 0x00E2948C:    { *utf8_char = ACS_ULCORNER;        break;}
        case 0x00E29490:    { *utf8_char = ACS_URCORNER;        break;}
        case 0x00E29494:    { *utf8_char = ACS_LLCORNER;        break;}
        case 0x00E29498:    { *utf8_char = ACS_LRCORNER;        break;}

        case 0x00E2949C:    { *utf8_char = ACS_LTEE;            break;}
        case 0x00E294A4:    { *utf8_char = ACS_RTEE;            break;}
        case 0x00E294AC:	{ *utf8_char = ACS_TTEE;            break;}
        case 0x00E294B4:    { *utf8_char = ACS_BTEE;            break;}

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

/*
    {
        FILE *f;
        f = fopen("value.dump", "a");
        fprintf(f, "%02x %02x %02x %02x\n",
            vterm->utf8_buf[0], vterm->utf8_buf[1],
            vterm->utf8_buf[2], vterm->utf8_buf[3]);
        fprintf(f, "utf-8 char: %08x\n", utf8_code);
        fclose(f);
    }
*/
    return byte_count;
}
