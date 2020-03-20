#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>

#include "macros.h"
#include "color_list.h"
#include "utlist.h"

#ifndef TRUE
#define TRUE    true
#endif

#ifndef FALSE
#define FALSE   false
#endif

void
color_list_init(int color_limit)
{
    extern color_list_t     *color_list;
    color_entry_t           *color_entry;
    int                     i;

    if(color_list == NULL)
    {
        color_list = CALLOC_PTR(color_list);

        for(i = 0; i < color_limit; i++)
        {
            color_entry = CALLOC_PTR(color_entry);
            color_entry->color = i;
            color_entry->vacant = TRUE;

            CDL_PREPEND(color_list->head, color_entry);
        }
    }

    return;
}

int
color_list_get_free(bool lease)
{
    extern color_list_t     *color_list;
    color_entry_t           *color_entry;

    if(color_list == NULL) return -1;

    CDL_FOREACH(color_list->head, color_entry)
    {
        if(color_entry->vacant == TRUE) break;
    }

    if(color_entry == NULL)
    {
        color_entry = color_list->head->prev;
    }

    if(lease == TRUE) color_entry->vacant = FALSE;

    return color_entry->color;
}

void
color_list_promote_color(int color)
{
    extern color_list_t     *color_list;
    color_entry_t           *color_entry;

    if(color_list == NULL) return;

    CDL_FOREACH(color_list->head, color_entry)
    {
        if(color_entry->color == color) break;
    }

    if(color_entry->color == color)
    {
        CDL_DELETE(color_list->head, color_entry);
        CDL_PREPEND(color_list->head, color_entry);
    }

    return;
}

void
color_list_add_color(int color)
{
    extern color_list_t     *color_list;
    color_entry_t           *color_entry;

    if(color_list == NULL) return;

    CDL_FOREACH(color_list->head, color_entry)
    {
        if(color_entry->vacant == TRUE) break;
    }

    CDL_DELETE(color_list->head, color_entry);
    color_entry->color = color;

    CDL_PREPEND(color_list->head, color_entry);
    color_entry->vacant = FALSE;
    //endwin(); fprintf(stderr, "added color\n\r");

    return;
}

void
color_list_del_color(int color)
{
    extern color_list_t     *color_list;
    color_entry_t           *color_entry;

    if(color_list == NULL) return;

    CDL_FOREACH(color_list->head, color_entry)
    {
        if(color_entry->color == color)
        {
            color_entry->vacant = TRUE;
            break;
        }
    }

    return;
}
