/***************************************************************************
This is a test program intended to exercise the libvterm library by
reading a bunch of control codes from a file or pipe, e.g., could be captured
by 'script' on a unix/linux system, and playing them back into a charmap.
The output should be the final screen shown in the script capture.

The capture filename is hard-coded as /tmp/dump.script unless a
command-line argument is passed, in which case that's used.

Note that ncurses is not used here, so a flag is passed in to skip
initializing that.  The long term goal is to make it possible to compile
libvterm with ncurses.h but without libncurses.a, for use in smaller
systems or on systems where libncurses.so.5 is not installed.  Obviously,
we'd lose some functionality there, but that's acceptable.

Similarly, we don't create a pty or child process in this test -- we are
just rendering a byte stream into a buffer.

This program is a bit naive in that it assumes that the current terminal
dimensions are the same as the dimensions in the capture file.
***************************************************************************/
#include "vterm.h"
#include <malloc.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>

#define BUFLEN 500

int GetTerminalDimensions( int* rows, int* cols )
  {
  int err;
  struct winsize sz;

  err = ioctl(0, TIOCGWINSZ, &sz);
  
  if( err )
    {
    return err;
    }

  *rows = sz.ws_row;
  *cols = sz.ws_col;

  return 0;
  }

int windowResized = 0;

void ResizeEvent( int dummy )
  {
  windowResized = 1;
  }

void CheckForResize( vterm_t* vt )
  {
  if( windowResized )
    {
    windowResized = 0;
    int rows, cols;
    if( GetTerminalDimensions( &rows, &cols )==0 )
      {
      vterm_resize( vt, cols, rows );
      }
    }
  }

#define CSI "\033["

void ResetAttribute()
  {
  fputs( CSI, stdout );
  fputs( "0m", stdout );
  fputs( CSI, stdout );
  fputs( "37;40m", stdout );
  // fputs( CSI, stdout );
  // fprintf( stdout, "%dm", COLOR_WHITE+30 );
  // fputs( CSI, stdout );
  // fprintf( stdout, "%dm", COLOR_BLACK+40 );
  }

void ProcessAttribute( int attr )
  {
  if( attr & A_BOLD )
    {
    fputs( CSI, stdout );
    fputs( "1m", stdout );
    }

  if( attr & A_BLINK )
    {
    fputs( CSI, stdout );
    fputs( "5m", stdout );
    }

  if( attr & A_REVERSE )
    {
    fputs( CSI, stdout );
    fputs( "7m", stdout );
    }

  if( attr & A_INVIS )
    {
    fputs( CSI, stdout );
    fputs( "8m", stdout );
    }

  int attrCol = (attr & 0xff00)>>8;
  // int attrCol = (attr & 0x00ff);
  int fg = -1;
  int bg = -1;
  if( GetFGBGFromColorIndex( attrCol, &fg, &bg )==0 )
    {
    if( fg>=0 && fg<8 )
      {
      fputs( CSI, stdout );
      fprintf( stdout, "%dm", fg+30 );
      }
    else
      {
      printf("Invalid fg color: %d\n", fg );
      }
    if( bg>=0 && bg<8 )
      {
      fputs( CSI, stdout );
      fprintf( stdout, "%dm", bg+40 );
      }
    else
      {
      printf("Invalid bg color: %d\n", bg );
      }
    }
  else
    {
    printf("Invalid color index: %d\n", attrCol );
    }
  }

int main( int argc, char** argv )
  {
  int h = -1;
  int w = -1;
  unsigned char buf[BUFLEN];
  vterm_t* vt = NULL;

  signal( SIGWINCH, ResizeEvent );

  if( GetTerminalDimensions( &h, &w ) )
    {
    fprintf( stderr, "ERROR: Failed to get screen dimensions!\n" );
    exit(-1);
    }

  char* fileName = "/tmp/dump.script";
  if( argc==2 )
    {
    fileName = argv[1];
    }
  int fd = open( fileName, O_RDONLY);
  if( fd<0 )
    {
    fprintf( stderr, "ERROR: Cannot open %s!\n", fileName );
    exit(-1);
    }

  vt = vterm_create(w, h, VTERM_FLAG_NOPTY | VTERM_FLAG_NOCURSES );
  if( vt==NULL )
    {
    fprintf( stderr, "ERROR: Cannot init vterm!\n" );
    exit(-2);
    }

  int n = 0;
  int fileSize = 0;
  int result = 0;
  for(;;)
    {
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(fd, &readset);
    struct timeval to;
    to.tv_sec = 1;
    to.tv_usec = 0;
    result = select(fd + 1, &readset, NULL, NULL, &to);
    if( result!=1 )
      {
      // printf("Timeout.. Select returned %d\n",result);
      }
    if( result==-1 )
      {
      break;
      }
    if( FD_ISSET( fd, &readset ) )
      {
      // printf("Select ended and FD is set.\n");
      n = read(fd, buf, BUFLEN-1);
      if( n==EAGAIN )
        {
        // printf("n==EAGAIN\n");
        sleep(1);
        continue;
        }
      if( n>0 )
        {
        CheckForResize( vt );
        // printf("read %d chars\n", n);
        fileSize += n;
        vterm_render( vt, buf, n );
        }
      if( n==0 )
        {
        // printf("read nothing.  EOF.\n");
        break;
        }
      }
    else
      {
      // printf("Select ended but FD is not set!\n");
      }
    FD_CLR(fd, &readset);
    }

  close( fd );
  // printf("Final return from read was %d - %s\n", n, strerror(n));

  // printf("Pumped %d chars through.\n", fileSize);
  vterm_get_size( vt, &w, &h );

  vterm_cell_t** cells = vterm_get_buffer( vt );
  if( cells!=NULL )
    {
    int i=0, j=0, c=-1, a=-1;
    ResetAttribute();
    for( i=0; i<h; ++i )
      {
      vterm_cell_t* crow = cells[i], ptr=NULL;

      a=-1;
      // printf("%02d: ", i);
      for( ptr=crow+w-1; ptr>=crow; --ptr )
        { 
        if( ptr->ch==' ' )
          {
          ptr->ch = 0;
          }
        else
          {
          break;
          }
        }

      for( j=0; j<w; ++j )
        {
        c = crow->ch;
        if( c==0 )
          {
          break;
          }
        if( a != crow->attr )
          {
          ProcessAttribute( crow->attr );
          }
        a = crow->attr;
        fputc(c,stdout);
        ++crow;
        }

      fputc('\n',stdout);
      }
    }

  vterm_destroy( vt );
  ResetAttribute();

  return 0;
  }
