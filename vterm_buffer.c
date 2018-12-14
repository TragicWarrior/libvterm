/*
  Copyright (C) 2009 Bryan Christ
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
  This library is based on ROTE written by Bruno Takahashi C. de Oliveira
*/

#include <stdlib.h>
#include <string.h>

#include <sys/ioctl.h>

#include "vterm.h"
#include "vterm_private.h"
#include "vterm_buffer.h"

void
vterm_alloc_buffer(vterm_t *vterm, int idx, int width, int height)
{
    vterm_desc_t    *v_desc;
    int             i;

    if(vterm == NULL) return;
    if(idx != VTERM_BUFFER_STD && idx != VTERM_BUFFER_ALT) return;

    if(width < 0 || height < 0) return;

    v_desc = &vterm->vterm_desc[idx];

    v_desc->cells =
        (vterm_cell_t**)calloc(1, (sizeof(vterm_cell_t*) * height));

    for(i = 0;i < height;i++)
    {
        v_desc->cells[i] = 
            (vterm_cell_t*)calloc(1, (sizeof(vterm_cell_t) * width));
    }

    v_desc->rows = height;
    v_desc->cols = width;

    v_desc->scroll_min = 0;
    v_desc->scroll_max = height - 1;

    return;
}

void
vterm_realloc_buffer(vterm_t *vterm, int idx, int width, int height)
{
    vterm_desc_t    *v_desc;
    int             delta_y = 0;
    // int             delta_x = 0;
    int             start_x = 0;
    uint16_t        i;
    uint16_t        j;

    if(vterm == NULL) return;
    if(idx != VTERM_BUFFER_STD && idx != VTERM_BUFFER_ALT) return;

    if(width == 0 || height == 0) return;

    // set the vterm description buffer selector
    v_desc = &vterm->vterm_desc[idx];

    delta_y = height - v_desc->rows;
    // delta_x = width - v_desc->cols;

    // realloc to accomodate the new matrix size
    v_desc->cells = (vterm_cell_t**)realloc(v_desc->cells,
        sizeof(vterm_cell_t*) * height);

    for(i = 0; i < height; i++)
    {
        // when adding new rows, we can just calloc() them.
        if((delta_y > 0) && (i > (v_desc->rows - 1)))
        {
            v_desc->cells[i] =
                (vterm_cell_t*)calloc(1, (sizeof(vterm_cell_t) * width));

            // fill new row with blanks
            for(j = 0; j < width; j++)
            {
                v_desc->cells[i][j].ch = ' ';
            }

            continue;
        }

        // this handles existing rows
        v_desc->cells[i] = (vterm_cell_t*)realloc(v_desc->cells[i],
            sizeof(vterm_cell_t) * width);

        // fill new row with blanks
        start_x = v_desc->cols - 1;
        for(j = start_x; j < width; j++)
        {
            v_desc->cells[i][j].ch = ' ';
            v_desc->cells[i][j].attr = 0;
        }
    }

    v_desc->rows = height;
    v_desc->cols = width;

    v_desc->scroll_max = height - 1;

    return;
}


void
vterm_dealloc_buffer(vterm_t *vterm, int idx)
{
    vterm_desc_t    *v_desc;
    int             i;

    if(vterm == NULL) return;
    if(idx != VTERM_BUFFER_STD && idx != VTERM_BUFFER_ALT) return;

    v_desc = &vterm->vterm_desc[idx];

    // prevent a double-free
    if(v_desc->cells == NULL) return;

    for(i = 0; i < v_desc->rows; i++)
    {
        free(v_desc->cells[i]);
        v_desc->cells[i] = NULL;
    }

    free(v_desc->cells);
    v_desc->cells = NULL;

    return;
}

int
vterm_set_active_buffer(vterm_t *vterm, int idx)
{
    vterm_desc_t    *v_desc = NULL;
    int             curr_idx;
    struct winsize  ws = {.ws_xpixel = 0, .ws_ypixel = 0};
    int             width;
    int             height;
    int             std_width, std_height;
    int             curr_width, curr_height;

    if(vterm == NULL) return -1;
    if(idx != VTERM_BUFFER_STD && idx != VTERM_BUFFER_ALT) return -1;

    curr_idx = vterm_get_active_buffer(vterm);

    /*
        check to see if current buffer index is the same as the
        requested one.  if so, this is a no-op.
    */
    if(idx == curr_idx) return 0;

    /*
        get current terminal size using the best methods available.  typically,
        that means using TIOCGWINSZ ioctl which is pretty portable.  an
        alternative method could be getmaxyx() which is found in most curses
        implementations.  however, that causes problems for uses of libvterm
        where the rendering apparatus is not a curses WINDOW.
    */
    ioctl(vterm->pty_fd, TIOCGWINSZ, &ws);
    height = ws.ws_row;
    width = ws.ws_col;

    // treat the standard buffer special -- it never goes away
    if(idx == VTERM_BUFFER_STD)
    {
        /*
            if the current buffer not the STD buffer, we need to handle
            a few housekeeping items and then tear down the other
            buffer before switching.
        */
        if(curr_idx != VTERM_BUFFER_STD)
        {
            // check to see if a resize happened
            std_width = vterm->vterm_desc[VTERM_BUFFER_STD].cols;
            std_height = vterm->vterm_desc[VTERM_BUFFER_STD].rows;
            curr_width = vterm->vterm_desc[curr_idx].cols;
            curr_height = vterm->vterm_desc[curr_idx].rows;

            if(std_height != curr_height || std_width != curr_width)
            {
                vterm_realloc_buffer(vterm, VTERM_BUFFER_STD,
                    curr_width, curr_height);
            }

            vterm_dealloc_buffer(vterm, curr_idx);
        }
    }

    /*
        given the above conditional, this could have been an if-else.
        however, this is more readable and it should optimize
        about the same.
    */
    if(idx != VTERM_BUFFER_STD)
    {
        vterm_alloc_buffer(vterm, idx, width, height);
        v_desc = &vterm->vterm_desc[idx];

        // copy some defaults from standard buffer
        v_desc->colors = vterm->vterm_desc[VTERM_BUFFER_STD].colors;

        // erase the newly created buffer
        vterm_erase(vterm, idx);
    }


    // update the vterm buffer desc index
    vterm->vterm_desc_idx = idx;

    return 0;
}

int
vterm_get_active_buffer(vterm_t *vterm)
{
    if(vterm == NULL) return -1;

    return vterm->vterm_desc_idx;
}

