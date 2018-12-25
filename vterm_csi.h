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


void  interpret_dec_SM(vterm_t *vterm, int param[], int pcount);
void  interpret_dec_RM(vterm_t *vterm, int param[], int pcount);
void  interpret_csi_SGR(vterm_t *vterm, int param[], int pcount);
void  interpret_csi_ED(vterm_t *vterm, int param[], int pcount);
void  interpret_csi_CUP(vterm_t *vterm, int param[], int pcount);
void  interpret_csi_ED(vterm_t *vterm, int param[], int pcount);
void  interpret_csi_EL(vterm_t *vterm, int param[], int pcount);
void  interpret_csi_ICH(vterm_t *vterm, int param[], int pcount);
void  interpret_csi_DCH(vterm_t *vterm, int param[], int pcount);
void  interpret_csi_IL(vterm_t *vterm, int param[], int pcount);
void  interpret_csi_DL(vterm_t *vterm, int param[], int pcount);
void  interpret_csi_ECH(vterm_t *vterm, int param[], int pcount);
void  interpret_csi_CUx(vterm_t *vterm, char verb, int param[], int pcount);
void  interpret_csi_DECSTBM(vterm_t *vterm, int param[], int pcount);
void  interpret_csi_SAVECUR(vterm_t *vterm, int param[], int pcount);
void  interpret_csi_RESTORECUR(vterm_t *vterm, int param[], int pcount);
void  interpret_csi_RS1(vterm_t *vterm, char *data);

// trap for unknown escape sequences
void  interpret_csi_UNKNOWN(vterm_t *vterm,int param[],int pcount);

#endif

