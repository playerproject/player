/***************************************************************************
 * Desc: Error handling
 * Author: Andrew Howard
 * Date: 13 May 2002
 * CVS: $Id$
 **************************************************************************/

#include "playerc.h"
#include "error.h"


// A place to put error strings
char playerc_errorstr[1024];


// Use this function to read the error string
const char *playerc_error_str()
{
  return playerc_errorstr;
}
