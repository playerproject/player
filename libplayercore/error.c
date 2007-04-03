
/***************************************************************************
 * Desc: Error handling macros
 * Author: Andrew Howard
 * Date: 13 May 2002
 * CVS: $Id$
 **************************************************************************/

#include <stdarg.h>

#include <libplayercore/error.h>

// User-selected msg level: 0 for the most important messages (always
// printed); 9 for the least important.
static int msgLevel;

// File for logging messages
static FILE *msgFile;


// Initialize error logging
void 
ErrorInit(int _msgLevel)
{
  msgLevel = _msgLevel;
  msgFile = fopen(".player", "a+");
}

#define MSG_MAX 1024

// Function for printing and logging errors.
void ErrorPrint(int msgType, int level, const char *file, int line, const char *fmt, ...)
{
  va_list ap;

  if (level <= msgLevel)
  {
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
  }
  if (msgFile)
  {
    char msgBuf[MSG_MAX];
    va_start(ap, fmt);
    vsnprintf(msgBuf, MSG_MAX, fmt, ap);
    va_end(ap);
    fprintf(msgFile, "%s:%d %s", file, line, msgBuf);
  }
  
  return;
}
