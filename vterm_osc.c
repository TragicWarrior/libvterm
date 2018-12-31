
#include <stdlib.h>
#include <string.h>

#include "vterm.h"
#include "vterm_private.h"

void
vterm_get_title(vterm_t *vterm, char *buf, int buf_sz)
{
    if(vterm == NULL) return;
    if(buf == NULL) return;
    if(buf_sz < 2) return;

    memset(buf, 0, buf_sz);
    strncpy(buf, vterm->title, buf_sz - 1);

    return;
}
