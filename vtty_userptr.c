#include "vtty.h"
#include "vtty_private.h"

void
vtty_set_userptr(vtty_t *vtty, void *anything)
{
    if(vtty == NULL) return;

    vtty->userptr = anything;

    return;
}

void*
vtty_get_userptr(vtty_t *vtty)
{
    if(vtty == NULL) return NULL;

    return vtty->userptr;
}
