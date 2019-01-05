
#ifndef _VTERM_CSI_H_
#define _VTERM_CSI_H_

#include "vterm.h"

#define MAX_CSI_ES_PARAMS   32
#define MAX_XTERM_ES_PARAMS 32

/*
    this string is emitted by for resetting an RXVT terminal (RS1).
    xterm is far more sane using a single \Ec control code to perform
    the same termcap operation.  the sequence is codified in terminfo
    database.  this can be inspected with the infocmp tool.
*/
#define RXVT_RS1    "\e>\e[1;3;4;5;6l\e[?7h\e[m\e[r\e[2J\e[H"
#define XTERM_RS1   "\ec"

void    interpret_dec_SM(vterm_t *vterm, int param[], int pcount);
void    interpret_dec_RM(vterm_t *vterm, int param[], int pcount);
void    interpret_csi_SGR(vterm_t *vterm, int param[], int pcount);
void    interpret_csi_ED(vterm_t *vterm, int param[], int pcount);
void    interpret_csi_CUP(vterm_t *vterm, int param[], int pcount);
void    interpret_csi_ED(vterm_t *vterm, int param[], int pcount);
void    interpret_csi_EL(vterm_t *vterm, int param[], int pcount);
void    interpret_csi_ICH(vterm_t *vterm, int param[], int pcount);
void    interpret_csi_DCH(vterm_t *vterm, int param[], int pcount);
void    interpret_csi_IL(vterm_t *vterm, int param[], int pcount);
void    interpret_csi_DL(vterm_t *vterm, int param[], int pcount);
void    interpret_csi_ECH(vterm_t *vterm, int param[], int pcount);
void    interpret_csi_REP(vterm_t *vterm, int param[], int pcount);
void    interpret_csi_CUx(vterm_t *vterm, char verb, int param[], int pcount);
void    interpret_csi_DECSTBM(vterm_t *vterm, int param[], int pcount);
void    interpret_csi_SAVECUR(vterm_t *vterm, int param[], int pcount);
void    interpret_csi_RESTORECUR(vterm_t *vterm, int param[], int pcount);
void    interpret_csi_CBT(vterm_t *vterm, int param[], int pcount);

int     interpret_csi_RS1_rxvt(vterm_t *vterm, char *data);
int     interpret_csi_RS1_xterm(vterm_t *vterm, char *data);

// trap for unknown escape sequences
void    interpret_csi_UNKNOWN(vterm_t *vterm, int param[], int pcount);

#endif

