#include <stdbool.h>

#include "vterm.h"
#include "vterm_escape_suffixes.h"

inline bool
validate_csi_escape_suffix(char *lastchar)
{
    char    c = *lastchar;

    if(c >= 'a' && c <= 'z') return TRUE;
    if(c >= 'A' && c <= 'Z') return TRUE;
    if(c == '@') return TRUE;
    if(c == '`') return TRUE;

    return FALSE;
}

inline bool
validate_xterm_escape_suffix(char *lastchar)
{
    char    c = *lastchar;

    if(c == '\x07') return TRUE;
    if(c == '\x9c') return TRUE;

    // seems to be a VTE thing
    if(c == '\x5c')
    {
        // make sure the preceding char was ESC
        if( *(--lastchar) == '\x1b') return TRUE;
    }

    return FALSE;
}

inline bool
validate_scs_escape_suffix(char *lastchar)
{
    char c = *lastchar;

    if(c == 'A') return TRUE;
    if(c == 'B') return TRUE;
    if(c == '0') return TRUE;
    if(c == '1') return TRUE;
    if(c == '2') return TRUE;

    return FALSE;
}

inline bool
validate_dec_private_suffix(char *lastchar)
{
    char c = *lastchar;

    // the suffix must be a number between 3 and 8 inclusive
    if(c >= '3' && c <= '8') return TRUE;

    return FALSE;
}

