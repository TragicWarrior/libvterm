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

#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include "vterm.h"
#include "vterm_private.h"
#include "vterm_colors.h"

struct _my_color_pair
  {
  short fg;
  short bg;
  };
typedef struct _my_color_pair MY_COLOR_PAIR;

#define MAX_COLOR_PAIRS 512

MY_COLOR_PAIR *colorPalette = NULL;
int paletteSize = 0;

void InitColorSpace()
  {
  int i,j,n;

  if( colorPalette==NULL )
    {
    colorPalette = calloc( MAX_COLOR_PAIRS, sizeof( MY_COLOR_PAIR ) );
    if( colorPalette==NULL )
      {
      fprintf(stderr, "ERROR: cannot allocate color palette!\n");
      exit(1);
      }
    }

//  for( i = 0; i < 8; i++ )
//    {
//    for( j = 0; j < 8; j++ )
//      {
//      if (i != 7 || j != 0)
//        {
//        n = j*8+7-i;
//        if( n>=MAX_COLOR_PAIRS )
//          {
//          fprintf(stderr, "ERROR: cannot set color pair %d!\n", n);
//          exit(1);
//          }
//        colorPalette[n].fg = i;
//        colorPalette[n].bg = j;
//        if( n+1>paletteSize )
//          {
//          paletteSize = n+1;
//          }
//        }
//      }
//    }

  for( i = 0; i < 8; i++ )
    {
    for( j = 0; j < 8; j++ )
      {
        {
        n = i*8+j;
        if( n>=MAX_COLOR_PAIRS )
          {
          fprintf(stderr, "ERROR: cannot set color pair %d!\n", n);
          exit(1);
          }
        colorPalette[n].fg = 7-i;
        colorPalette[n].bg = j;
        if( n+1>paletteSize )
          {
          paletteSize = n+1;
          }
        }
      }
    }
  }

void FreeColorSpace()
  {
  if( colorPalette == NULL )
      return;

  free( colorPalette );
  colorPalette = NULL;
  paletteSize = 0;
  }

int FindColorPair( int fg, int bg )
  {
  int i=0;

  if( colorPalette==NULL )
    {
    InitColorSpace();
    }

  for( i=0; i<paletteSize; ++i )
    {
    MY_COLOR_PAIR* cp = colorPalette + i;
    if( cp->fg==fg && cp->bg==bg )
      {
      return i;
      }
    }

  return -1;
  }

int GetFGBGFromColorIndex( int index, int* fg, int* bg )
  {
  if( colorPalette==NULL || index >= paletteSize || index<0 )
    {
    *fg = 0;
    *bg = 0;
    return -1;
    }

  *fg = colorPalette[index].fg;
  *bg = colorPalette[index].bg;

  return 0;
  }

int
vterm_set_colors(vterm_t *vterm,short fg,short bg)
{
    short   colors;

    if(vterm == NULL) return -1;
    if( vterm->flags & VTERM_FLAG_NOCURSES ) // no ncurses
    {
      colors = (short)FindColorPair( fg, bg );
      if(colors == -1) colors = 0;
      vterm->colors = colors;
    }
    else // ncurses
    {
#ifdef NOCURSES
       colors = FindColorPair( fg, bg );
#else
       if(has_colors() == FALSE)
           return -1;

       colors = find_color_pair(vterm, fg,bg);
#endif
       if(colors == -1) colors = 0;

       vterm->colors = colors;
    }

    return 0;
}

short
vterm_get_colors(vterm_t *vterm)
{
    if(vterm == NULL) return -1;
    if( vterm->flags & VTERM_FLAG_NOCURSES ) // no ncurses
    {
    }
    else // ncurses
    {
#ifndef NOCURSES
       if(has_colors() == FALSE) return -1;
#endif
    }

    return vterm->colors;
}

short
find_color_pair(vterm_t *vterm, short fg,short bg)
{
    int     i;

    if( vterm->flags & VTERM_FLAG_NOCURSES ) // no ncurses
    {
       return FindColorPair( fg, bg );
    }
    else // ncurses
    {
#ifdef NOCURSES
       return -1;
#else
       short   fg_color,bg_color;
       if(has_colors() == FALSE)
           return -1;

       for(i = 1;i < COLOR_PAIRS;i++)
       {
           pair_content(i,&fg_color,&bg_color);
           if(fg_color == fg && bg_color == bg) break;
       }

       if(i == COLOR_PAIRS)
           return -1;
#endif
    }

    return i;
}
