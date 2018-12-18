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
#include "vterm_buffer.h"

/*
    the way UTF-8 is encoded, we can just test the highest bit, pop it
    off, and count, in order to know how many bytes are in the code
    sequence.
*/
#define UTF8_2BYTES         0xC0        // binary 11000000
#define UTF8_3BYTES         0xE0        // binary 11100000
#define UTF8_4BYTES         0xF0        // binary 11110000
#define UTF8_MASK           0xF0        // binary 11110000

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
vterm_utf8_decode(vterm_t *vterm, chtype *utf8_char)
{
    uint32_t            utf8_code = 0;
    uint8_t             first_byte = 0;
    int                 byte_count = 0;
    int                 i = 0;

    first_byte = (uint8_t)vterm->utf8_buf[0];

    /*
        in UTF8, the high order bits in the first byte
        are actually the total number of bytes to expect.
    */
    switch(first_byte & UTF8_MASK)
    {
        case UTF8_2BYTES:       byte_count = 2;     break;
        case UTF8_3BYTES:       byte_count = 3;     break;
        case UTF8_4BYTES:       byte_count = 4;     break;
    }

    // too early to decode.  return back for more reading.
    if (vterm->utf8_buf_len < byte_count) return -1;

    // store first byte in the sequence
    utf8_code = (uint32_t)first_byte;

    // push in the rest of the bytes
    for (i = 1; i < byte_count; i++)
    {
        // make room for the next byte
        utf8_code = utf8_code << 8;

        // OR it in
        utf8_code |= (uint8_t)vterm->utf8_buf[i];
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


/*
    {
        FILE *f;
        f = fopen("value.dump", "a");
        fprintf(f, "first byte: %02x, byte count: 0%d\n",
            first_byte, byte_count);
        fprintf(f, "utf-8 char: %08x\n", utf8_code);
        fclose(f);
    }
*/

    return byte_count;
}
