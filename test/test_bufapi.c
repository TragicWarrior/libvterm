/*
    Direct internal-API exercise for the contiguous buffer change.

    The headless resize harness (test_resize.c) cannot reach the
    alt-screen path -- vtty_buffer_set_active needs a real PTY winsize.
    This drives the buffer primitives directly with explicit sizes so the
    clone-between-buffers-of-different-max_cols and the contiguous-block
    dealloc(ALT) paths get deterministic coverage.  Run under ASan.

    Build: gcc -fsanitize=address -O1 -g -I. test/test_bufapi.c <lib .c> ...
*/

#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif
#include <wchar.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "vtty.h"
#include "vtty_private.h"
#include "vtty_buffer.h"

static void
fill_row(vtty_desc_t *d, int r, wchar_t ch)
{
    int c;
    for(c = 0; c < d->cols; c++)
    {
        d->cells[r][c].wch[0] = ch;
        d->cells[r][c].wch[1] = L'\0';
    }
}

int
main(void)
{
    vtty_t *vt;
    vtty_desc_t *std, *alt, *hist;
    int r;

    vt = vtty_alloc();

    /* STANDARD 80x24, HISTORY 80x96 (4x), like vtty_init does */
    vtty_buffer_alloc(vt, VTTY_BUF_STANDARD, 80, 24);
    vtty_buffer_alloc(vt, VTTY_BUF_HISTORY, 80, 96);

    std  = &vt->vtty_desc[VTTY_BUF_STANDARD];
    hist = &vt->vtty_desc[VTTY_BUF_HISTORY];

    for(r = 0; r < std->rows; r++) fill_row(std, r, L'S');
    for(r = 0; r < hist->rows; r++) fill_row(hist, r, L'h');

    /* grow STANDARD wider than its history, then shrink back -- ratchets
       max_cols and forces survivor copies at a changed stride */
    vtty_buffer_realloc(vt, VTTY_BUF_STANDARD, 200, 40);
    for(r = 0; r < std->rows; r++) fill_row(std, r, L'W');
    vtty_buffer_realloc(vt, VTTY_BUF_STANDARD, 60, 30);

    /* allocate ALT at a DIFFERENT width than HISTORY, fill, clone into
       history (clone stride = min(max_cols)), then dealloc ALT --
       mirrors the ?1049l return path in vtty_buffer_set_active */
    vtty_buffer_alloc(vt, VTTY_BUF_ALTERNATE, 120, 24);
    alt = &vt->vtty_desc[VTTY_BUF_ALTERNATE];
    for(r = 0; r < alt->rows; r++) fill_row(alt, r, L'A');

    vtty_buffer_shift_up(vt, VTTY_BUF_HISTORY, -1, -1, alt->rows);
    vtty_buffer_clone(vt, VTTY_BUF_ALTERNATE, VTTY_BUF_HISTORY,
        0, hist->rows - alt->rows, alt->rows);

    vtty_buffer_dealloc(vt, VTTY_BUF_ALTERNATE);

    /* a couple more reallocs on STANDARD + HISTORY for good measure */
    vtty_buffer_realloc(vt, VTTY_BUF_STANDARD, 100, 50);
    vtty_buffer_realloc(vt, VTTY_BUF_HISTORY, 100, -1);

    vtty_buffer_dealloc(vt, VTTY_BUF_STANDARD);
    vtty_buffer_dealloc(vt, VTTY_BUF_HISTORY);
    /* ALT already deallocated -- exercise the double-dealloc guard */
    vtty_buffer_dealloc(vt, VTTY_BUF_ALTERNATE);

    free(vt->read_buf);
    free(vt);

    printf("OK: bufapi test completed\n");
    return 0;
}
