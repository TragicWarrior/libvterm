#ifndef _VTERM_ESCAPE_SUFFIXES_H_
#define _VTERM_ESCAPE_SUFFIXES_H_

bool
validate_csi_escape_suffix(char *lastchar);

bool
validate_xterm_escape_suffix(char *lastchar);

bool
validate_scs_escape_suffix(char *lastchar);

bool
validate_dec_private_suffix(char *lastchar);

#endif
