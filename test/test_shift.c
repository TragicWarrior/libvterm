/*
    Content-verifying harness for vtty_buffer_shift_up/_shift_down.

    Drives the shift primitives directly (like test_bufapi.c) and checks
    the resulting cell content row by row, including:

      - full-buffer and sub-region shifts at stride 1
      - sub-region shifts at stride > 1
      - stride >= region no-op behavior
      - dirty-bit side effects (set for STANDARD, untouched for HISTORY)
      - the vtty_set_history_size grow path, which calls
        shift_down(HISTORY, -1, -1, delta) with delta > 1.  The
        pre-memmove shift_down looped `region` iterations with no
        lower-bound guard, walking its source pointer below cells[0] --
        an out-of-bounds read for any stride > 1 (only reachable via
        vtty_set_history_size jumps > 1; vshell only ever steps by 1).
        Run under ASan to see it fault on the old code.

    Build: cmake -DBUILD_TESTS=ON . && make test_shift
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

static int failures = 0;

#define CHECK(_cond, ...)                                   \
            do                                              \
            {                                               \
                if(!(_cond))                                \
                {                                           \
                    fprintf(stderr, "FAIL: " __VA_ARGS__);  \
                    fprintf(stderr, "\n");                  \
                    failures++;                             \
                }                                           \
            }                                               \
            while(0)

/* fill row r with a single identifying char across the logical width */
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

/* TRUE when every logical cell of row r holds ch */
static int
row_is(vtty_desc_t *d, int r, wchar_t ch)
{
    int c;

    for(c = 0; c < d->cols; c++)
    {
        if(d->cells[r][c].wch[0] != ch) return 0;
    }

    return 1;
}

static void
fill_ident(vtty_desc_t *d)
{
    int r;

    for(r = 0; r < d->rows; r++) fill_row(d, r, L'A' + r);
}

static void
clear_dirty(vtty_desc_t *d)
{
    int r;

    for(r = 0; r < d->rows; r++) VCELL_DIRTY_CLR_ROW(d, r);
}

static int
any_dirty(vtty_desc_t *d)
{
    int r, c;

    for(r = 0; r < d->rows; r++)
    {
        for(c = 0; c < d->cols; c++)
        {
            if(VCELL_DIRTY_TEST(d, r, c)) return 1;
        }
    }

    return 0;
}

int
main(void)
{
    vtty_t          *vt;
    vtty_desc_t     *std;
    vtty_desc_t     *hist;
    int             retval;
    int             r;

    vt = vtty_alloc();

    vtty_buffer_alloc(vt, VTTY_BUF_STANDARD, 8, 10);
    std = &vt->vtty_desc[VTTY_BUF_STANDARD];

    /* ---- full-buffer shift up, stride 1 ---- */
    fill_ident(std);
    clear_dirty(std);

    retval = vtty_buffer_shift_up(vt, VTTY_BUF_STANDARD, -1, -1, 1);
    CHECK(retval == 0, "shift_up stride 1 retval %d", retval);

    for(r = 0; r < 9; r++)
        CHECK(row_is(std, r, L'A' + r + 1), "up1: row %d", r);
    CHECK(row_is(std, 9, L'A' + 9), "up1: vacated row 9 unchanged");
    CHECK(any_dirty(std), "up1: STANDARD shift must set dirty");

    /* ---- sub-region shift up, stride 3 (rows 2..7) ---- */
    fill_ident(std);

    retval = vtty_buffer_shift_up(vt, VTTY_BUF_STANDARD, 2, 7, 3);
    CHECK(retval == 0, "shift_up region stride 3 retval %d", retval);

    CHECK(row_is(std, 0, L'A'), "up3: row 0 untouched");
    CHECK(row_is(std, 1, L'B'), "up3: row 1 untouched");
    for(r = 2; r <= 4; r++)
        CHECK(row_is(std, r, L'A' + r + 3), "up3: row %d", r);
    for(r = 5; r <= 9; r++)
        CHECK(row_is(std, r, L'A' + r), "up3: row %d untouched", r);

    /* ---- full-buffer shift down, stride 1 ---- */
    fill_ident(std);

    retval = vtty_buffer_shift_down(vt, VTTY_BUF_STANDARD, -1, -1, 1);
    CHECK(retval == 0, "shift_down stride 1 retval %d", retval);

    CHECK(row_is(std, 0, L'A'), "down1: vacated row 0 unchanged");
    for(r = 1; r < 10; r++)
        CHECK(row_is(std, r, L'A' + r - 1), "down1: row %d", r);

    /* ---- sub-region shift down, stride 3 (rows 2..7) ---- */
    fill_ident(std);

    retval = vtty_buffer_shift_down(vt, VTTY_BUF_STANDARD, 2, 7, 3);
    CHECK(retval == 0, "shift_down region stride 3 retval %d", retval);

    CHECK(row_is(std, 0, L'A'), "down3: row 0 untouched");
    CHECK(row_is(std, 1, L'B'), "down3: row 1 untouched");
    for(r = 2; r <= 4; r++)
        CHECK(row_is(std, r, L'A' + r), "down3: row %d untouched", r);
    for(r = 5; r <= 7; r++)
        CHECK(row_is(std, r, L'A' + r - 3), "down3: row %d", r);
    for(r = 8; r <= 9; r++)
        CHECK(row_is(std, r, L'A' + r), "down3: row %d untouched", r);

    /* ---- stride >= region: nothing to copy, content intact ---- */
    fill_ident(std);

    retval = vtty_buffer_shift_up(vt, VTTY_BUF_STANDARD, -1, -1, 10);
    CHECK(retval == 0, "shift_up stride==rows retval %d", retval);
    for(r = 0; r < 10; r++)
        CHECK(row_is(std, r, L'A' + r), "up-noop: row %d", r);

    /* ---- degenerate region (bottom == top) rejected ---- */
    retval = vtty_buffer_shift_up(vt, VTTY_BUF_STANDARD, 4, 4, 1);
    CHECK(retval == -1, "shift_up empty region retval %d", retval);

    /* ---- ratcheted stride: cols < max_cols, visible content correct ---- */
    vtty_buffer_realloc(vt, VTTY_BUF_STANDARD, 12, 10);
    vtty_buffer_realloc(vt, VTTY_BUF_STANDARD, 8, 10);
    CHECK(std->cols == 8 && std->max_cols == 12,
        "ratchet setup: cols %d max_cols %d", std->cols, std->max_cols);

    fill_ident(std);

    retval = vtty_buffer_shift_up(vt, VTTY_BUF_STANDARD, -1, -1, 1);
    CHECK(retval == 0, "ratchet shift_up retval %d", retval);
    for(r = 0; r < 9; r++)
        CHECK(row_is(std, r, L'A' + r + 1), "ratchet up1: row %d", r);

    /* ---- HISTORY: dirty bits must NOT be touched ---- */
    vtty_buffer_alloc(vt, VTTY_BUF_HISTORY, 8, 6);
    hist = &vt->vtty_desc[VTTY_BUF_HISTORY];

    fill_ident(hist);
    clear_dirty(hist);

    retval = vtty_buffer_shift_up(vt, VTTY_BUF_HISTORY, -1, -1, 1);
    CHECK(retval == 0, "hist shift_up retval %d", retval);
    CHECK(!any_dirty(hist), "hist: shift must not set dirty");

    /* ---- vtty_set_history_size grow: 6 -> 20 rows must land the 6
       original rows at the LOGICAL tail (14..19) with blanks above.
       checked through vtty_desc_row_phys so the assertion holds for
       both the legacy shift implementation (head always 0) and a ring
       implementation (head rotates).  legacy note: the shift_down
       (-1, -1, 14) path read cells[-13..-1] here (ASan faults) before
       the rows_moved fix. */
    fill_ident(hist);

    vtty_set_history_size(vt, 20);
    CHECK(hist->rows == 20, "grow: rows %d", hist->rows);

    for(r = 6; r <= 13; r++)
        CHECK(row_is(hist, vtty_desc_row_phys(hist, r), L' '),
            "grow: row %d blank", r);
    for(r = 14; r <= 19; r++)
        CHECK(row_is(hist, vtty_desc_row_phys(hist, r), L'A' + r - 14),
            "grow: row %d content", r);

    /* ---- vtty_set_history_size shrink: 20 -> 9 rows keeps the
       NEWEST 9 logical rows (blanks at 0..2, original 6 at 3..8) ---- */
    vtty_set_history_size(vt, 9);
    CHECK(hist->rows == 9, "shrink: rows %d", hist->rows);

    for(r = 0; r <= 2; r++)
        CHECK(row_is(hist, vtty_desc_row_phys(hist, r), L' '),
            "shrink: row %d blank", r);
    for(r = 3; r <= 8; r++)
        CHECK(row_is(hist, vtty_desc_row_phys(hist, r), L'A' + r - 3),
            "shrink: row %d content", r);

    vtty_buffer_dealloc(vt, VTTY_BUF_STANDARD);
    vtty_buffer_dealloc(vt, VTTY_BUF_HISTORY);
    free(vt);

    if(failures > 0)
    {
        fprintf(stderr, "FAILED: %d check(s)\n", failures);
        return 1;
    }

    printf("OK: shift test completed\n");
    return 0;
}
