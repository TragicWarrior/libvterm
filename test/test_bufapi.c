/*
    Direct internal-API exercise for the contiguous buffer change.

    The headless resize harness (test_resize.c) cannot reach the
    alt-screen path -- vterm_buffer_set_active needs a real PTY winsize.
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

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_buffer.h"

static void
fill_row(vterm_desc_t *d, int r, wchar_t ch)
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
    vterm_t *vt;
    vterm_desc_t *std, *alt, *hist;
    int r;

    vt = vterm_alloc();

    /* STANDARD 80x24, HISTORY 80x96 (4x), like vterm_init does */
    vterm_buffer_alloc(vt, VTERM_BUF_STANDARD, 80, 24);
    vterm_buffer_alloc(vt, VTERM_BUF_HISTORY, 80, 96);

    std  = &vt->vterm_desc[VTERM_BUF_STANDARD];
    hist = &vt->vterm_desc[VTERM_BUF_HISTORY];

    for(r = 0; r < std->rows; r++) fill_row(std, r, L'S');
    for(r = 0; r < hist->rows; r++) fill_row(hist, r, L'h');

    /* grow STANDARD wider than its history, then shrink back -- ratchets
       max_cols and forces survivor copies at a changed stride */
    vterm_buffer_realloc(vt, VTERM_BUF_STANDARD, 200, 40);
    for(r = 0; r < std->rows; r++) fill_row(std, r, L'W');
    vterm_buffer_realloc(vt, VTERM_BUF_STANDARD, 60, 30);

    /* allocate ALT at a DIFFERENT width than HISTORY, fill, clone into
       history (clone stride = min(max_cols)), then dealloc ALT --
       mirrors the ?1049l return path in vterm_buffer_set_active */
    vterm_buffer_alloc(vt, VTERM_BUF_ALTERNATE, 120, 24);
    alt = &vt->vterm_desc[VTERM_BUF_ALTERNATE];
    for(r = 0; r < alt->rows; r++) fill_row(alt, r, L'A');

    vterm_buffer_shift_up(vt, VTERM_BUF_HISTORY, -1, -1, alt->rows);
    vterm_buffer_clone(vt, VTERM_BUF_ALTERNATE, VTERM_BUF_HISTORY,
        0, hist->rows - alt->rows, alt->rows);

    vterm_buffer_dealloc(vt, VTERM_BUF_ALTERNATE);

    /* a couple more reallocs on STANDARD + HISTORY for good measure */
    vterm_buffer_realloc(vt, VTERM_BUF_STANDARD, 100, 50);
    vterm_buffer_realloc(vt, VTERM_BUF_HISTORY, 100, -1);

    vterm_buffer_dealloc(vt, VTERM_BUF_STANDARD);
    vterm_buffer_dealloc(vt, VTERM_BUF_HISTORY);
    /* ALT already deallocated -- exercise the double-dealloc guard */
    vterm_buffer_dealloc(vt, VTERM_BUF_ALTERNATE);

    free(vt->read_buf);
    free(vt);

    printf("OK: bufapi test completed\n");
    return 0;
}
