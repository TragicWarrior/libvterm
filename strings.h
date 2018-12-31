#ifndef _STR_TOOLS_H_
#define _STR_TOOLS_H_

#include <stdbool.h>

char*   strdup_printf(char *fmt, ...);

char**  strsplitv(char *string, char *delim);

char**  strdupv(char **array, int limit);

void    strfreev(char **array);

#endif
