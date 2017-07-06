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

#define DISPLAY_INTERVAL 1
#define MAX_CHARS_IN_BUF 20

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
  fputs( "37m", stdout ); // white foreground
  fputs( CSI, stdout );
  fputs( "40m", stdout ); // black background
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

void CursorTopLeft()
  {
  fputs( CSI, stdout );
  fputs( "1;1H", stdout );
  }

void PrintTermBuffer( vterm_t* vt )
  {
  int h = -1;
  int w = -1;

  vterm_get_size( vt, &w, &h );

  CursorTopLeft();

  vterm_cell_t** cells = vterm_get_buffer( vt );
  if( cells!=NULL )
    {
    int i=0, j=0, c=-1, a=-1;
    ResetAttribute();
    for( i=0; i<h; ++i )
      {
      vterm_cell_t* crow = cells[i];

      for( j=0; j<w; ++j )
        {
        if( i==h-1 && j==w-1 )
          { // don't print the last char on the screen - it causes a scroll.
          CursorTopLeft();
          }
        else
          {
          c = crow->ch;
          if( a != crow->attr )
            {
            ProcessAttribute( crow->attr );
            }
          a = crow->attr;
          fputc(c,stdout);
          ++crow;
          }
        }

      fputc('\n',stdout);
      }
    }

  ResetAttribute();
  }

void ReadFileIntoTerminal( vterm_t* vt, char* fileName )
  {
  int n = 0;
  int charsSincePrint = 0;
  int fileSize = 0;
  int result = 0;
  time_t tLast = -1, tNow = -1;
  unsigned char buf[BUFLEN];

  int fd = open( fileName, O_RDONLY);
  if( fd<0 )
    {
    fprintf( stderr, "ERROR: Cannot open %s!\n", fileName );
    exit(-1);
    }

  for(;;)
    {
    fd_set readset;
    FD_ZERO(&readset);
    FD_SET(fd, &readset);
    struct timeval to;
    to.tv_sec = 1;
    to.tv_usec = 0;
    result = select(fd + 1, &readset, NULL, NULL, &to);

    tNow = time( NULL );
    if( (tNow - tLast) > DISPLAY_INTERVAL || charsSincePrint > MAX_CHARS_IN_BUF )
      {
      if( charsSincePrint>0 )
        {
        PrintTermBuffer( vt );
        charsSincePrint = 0;
        }
      tLast = tNow;
      }

    if( result!=1 )
      {
      // printf("Timeout.. Select returned %d\n",result);
      }
    if( result==-1 )
      {
      // EOF or other error.
      break;
      }
    if( FD_ISSET( fd, &readset ) )
      {
      // got something
      n = read(fd, buf, BUFLEN-1);
      if( n==EAGAIN )
        {
        // nope - not really.
        sleep(1);
        continue;
        }
      if( n>0 )
        {
        // got n chars.
        CheckForResize( vt );
        fileSize += n;
        charsSincePrint += n;
        vterm_render( vt, buf, n );
        }
      if( n==0 )
        {
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

  // printf("Read %d chars from %s.\n", fileSize, fileName );
  }

int main( int argc, char** argv )
  {
  vterm_t* vt = NULL;

  signal( SIGWINCH, ResizeEvent );

  char* fileName = "/tmp/dump.script";
  if( argc==2 )
    {
    fileName = argv[1];
    }

  int h=-1, w=-1;
  if( GetTerminalDimensions( &h, &w ) )
    {
    fprintf( stderr, "ERROR: Failed to get screen dimensions!\n" );
    exit(-1);
    }

  vt = vterm_create(w, h, VTERM_FLAG_NOPTY | VTERM_FLAG_NOCURSES );
  if( vt==NULL )
    {
    fprintf( stderr, "ERROR: Cannot init vterm!\n" );
    exit(-2);
    }

  ReadFileIntoTerminal( vt, fileName );
  PrintTermBuffer( vt );

  vterm_destroy( vt );

  return 0;
  }
