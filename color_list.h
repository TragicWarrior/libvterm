#ifndef _COLOR_LIST_H_
#define _COLOR_LIST_H_

#include <stdbool.h>

typedef struct _color_list_s    color_list_t;

typedef struct _color_entry_s   color_entry_t;

struct _color_entry_s
{
    int             color;
    bool            vacant;

    color_entry_t   *next;
    color_entry_t   *prev;
};

struct _color_list_s
{
    color_entry_t   *head;
};

void    color_list_init(int color_limit);

int     color_list_get_free(bool lease);

void    color_list_promote_color(int color);

void    color_list_add_color(int color);

void    color_list_del_color(int color);

#endif
