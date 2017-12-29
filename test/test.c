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
#include <time.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>

#define BUFLEN 500

#define DISPLAY_INTERVAL 1
#define MAX_CHARS_IN_BUF 20

#define DEBUG

#ifdef DEBUG
#  define DEBUG_fputs(A) fputs(A,debug)
#else
#  define DEBUG_fputs(A) 
#endif

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

#ifdef DEBUG
  FILE *debug = NULL;
  int randCol = 0;
#endif

int lastFG = -1;
int lastBG = -1;

void ProcessAttribute( int attr, int row, int col )
  {
#ifdef DEBUG
  if( debug==NULL )
    {
    debug = fopen("/tmp/debug.log","w");
    }
#endif

  if( attr & A_BOLD )
    {
    fputs( CSI, stdout );
    fputs( "1m", stdout );
    DEBUG_fputs( "(bold)\n" );
    }

  if( attr & A_BLINK )
    {
    fputs( CSI, stdout );
    fputs( "5m", stdout );
    DEBUG_fputs("(blink)\n" );
    }

  if( attr & A_REVERSE )
    {
    fputs( CSI, stdout );
    fputs( "7m", stdout );
    DEBUG_fputs("(reverse)\n" );
    }

  if( attr & A_INVIS )
    {
    fputs( CSI, stdout );
    fputs( "8m", stdout );
    DEBUG_fputs("(invis)\n" );
    }

  int attrCol = (attr & 0xff00)>>8;

  int fg = -1;
  int bg = -1;
  if( GetFGBGFromColorIndex( attrCol, &fg, &bg )==0 )
    {
#ifdef DEBUG
    if( (bg==0 && fg==0)
        || (bg!=lastBG && fg!=lastBG) )
      {
      fprintf(debug,"(fg=%d, bg=%d @ %d,%d )\n", fg, bg, row, col);
      }
#endif
    if( fg!=lastFG )
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
      lastFG = fg;
      }

    if( bg!=lastBG )
      {
      if( bg>=0 && bg<8 )
        {
        fputs( CSI, stdout );
        fprintf( stdout, "%dm", bg+40 );
        }
      else
        {
        printf("Invalid bg color: %d\n", bg );
        }
      lastBG = bg;
      }
    }
  else
    {
    DEBUG_fputs("(invalid color)\n" );
    printf("Invalid color index: %d\n", attrCol );
    }
  }

void CursorTopLeft()
  {
  fputs( CSI, stdout );
  fputs( "1;1H", stdout );
  }

int lastW = -1;
int lastH = -1;
vterm_cell_t** lastBuffer = NULL;
FILE* changeLog = NULL;
time_t tStart = 0;

void FatalError( char* msg )
  {
  fputs( "FATAL ERROR: ", stderr );
  fputs( msg, stderr );
  fputs( "\n", stderr );
  exit(-99);
  }

void InitPrivateBuffer( int w, int h )
  {
  int row, col;
  lastBuffer = (vterm_cell_t**)calloc(h, sizeof(vterm_cell_t*));
  if( lastBuffer==NULL )
    {
    FatalError("Allocating private buffer");
    }
  for( row=0; row<h; row++ )
    {
    vterm_cell_t *ptr
      = lastBuffer[row]
      = (vterm_cell_t*)calloc(w, sizeof(vterm_cell_t));
    if( lastBuffer[row]==NULL )
      {
      FatalError("Allocating private buffer row");
      }
    for( col=0; col<w; col++ )
      {
      ptr->ch = ' ';
      ++ptr;
      }
    }
  lastW = w;
  lastH = h;

  if( changeLog==NULL )
    {
    changeLog = fopen("/tmp/session-capture.log","w");
    if( changeLog==NULL )
      {
      FatalError("Opening change log file");
      }
    }

  tStart = time(NULL);
  }

void ResizePrivateBuffer( int w, int h )
  {
  int row;
  if( lastBuffer != NULL )
    {
    for( row=0; row<lastH; row++ )
      {
      if( lastBuffer[row]!=NULL )
        {
        free( lastBuffer[row] );
        lastBuffer[row] = NULL;
        }
      }
    free( lastBuffer );
    lastBuffer = NULL;
    lastW = -1;
    lastH = -1;
    }
  InitPrivateBuffer( w, h );
  }

void StartDiff( int row, int col )
  {
  fprintf( changeLog, "@%03d,%03d\n", row, col );
  }

void PrintChar( int c )
  {
  if( isalnum( c ) || c==' ' )
    {
    fputc( c, changeLog );
    }
  else
    {
    fprintf( changeLog, "%%%02x", c );
    }
  }

void PrintAttr( int attr )
  {
  fprintf( changeLog, "\\%04x", attr );
  }

void PrintBufferDiffs( int w, int h, vterm_cell_t** cells )
  {
  int row=0, col=0, inDiff=0, diffC=0, diffA=0, diff=0;
  vterm_cell_t *lastRow=NULL, *currentRow=NULL;
  if( cells==NULL || lastBuffer == NULL || w!=lastW || h!=lastH || w<0 || h<0 )
    {
    FatalError("Cannot process diffs across mismatched buffers");
    }

  // print a heartbeat
  time_t tNow = time(NULL);
  fprintf( changeLog, ":%ld\n", (unsigned long)(tNow - tStart));

  for( row=0; row<h; ++row )
    {
    lastRow = lastBuffer[row];
    currentRow = cells[row];
    inDiff = 0;
    for( col=0; col<w; ++col )
      {
      diffC = (lastRow->ch == currentRow->ch) ? 0 : 1;
      diffA = (lastRow->attr == currentRow->attr) ? 0 : 1;
      diff = diffC || diffA;

      if( inDiff )
        { // in the middle of printing a diff
        if( diff )
          {
          if( diffC )
            {
            PrintChar( currentRow->ch );
            lastRow->ch = currentRow->ch;
            }
          if( diffA )
            {
            PrintAttr( currentRow->attr );
            lastRow->attr = currentRow->attr;
            }
          }
        else
          {
          fputc( '\n', changeLog );
          inDiff = 0;
          }
        }
      else
        { // not currently printing a diff
        if( diff )
          {
          inDiff = 1;
          StartDiff( row, col );
          if( diffC )
            {
            PrintChar( currentRow->ch );
            lastRow->ch = currentRow->ch;
            }
          if( diffA )
            {
            PrintAttr( currentRow->attr );
            lastRow->attr = currentRow->attr;
            }
          }
        else
          {
          // do nothing.
          }
        }
      ++lastRow;
      ++currentRow;
      } // end of row

    if( inDiff )
      {
      fputc( '\n', changeLog );
      inDiff = 0;
      }
    } // rows
  }

void PrintTermBuffer( vterm_t* vt )
  {
  int h = -1;
  int w = -1;

#ifdef DEBUG
  if( debug==NULL )
    {
    debug = fopen("/tmp/debug.log","w");
    }
#endif

  DEBUG_fputs("New TermBuffer\n" );
  vterm_get_size( vt, &w, &h );

  CursorTopLeft();

  vterm_cell_t** cells = vterm_get_buffer( vt );
  if( cells!=NULL )
    {

    if( lastW==-1 )
      {
      InitPrivateBuffer( w, h );
      }
    else if( lastW!=w || lastH!=h )
      {
      ResizePrivateBuffer( w, h );
      }

    PrintBufferDiffs( w, h, cells );

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
            ProcessAttribute( crow->attr, i, j );
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
#ifdef DEBUG
        if( debug==NULL )
          {
          debug = fopen("/tmp/debug.log","w");
          }
        int i;
        fputs("DATA:\n", debug );
        for( i=0; i<n; ++i )
          {
          int c = buf[i];
          fputc( c, debug );
          }
        fputs("===\n", debug );
#endif
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
