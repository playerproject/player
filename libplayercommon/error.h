/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ********************************************************************/

/***************************************************************************
 * Desc: Error handling macros
 * Author: Andrew Howard
 * Date: 13 May 2002
 * CVS: $Id: error.h 8003 2009-07-13 10:34:37Z thjc $
 **************************************************************************/

#ifndef ERROR_HH
#define ERROR_HH

#include <stdio.h>
#include <errno.h>

#include <playerconfig.h>

#if defined (WIN32)
  #if defined (PLAYER_STATIC)
    #define PLAYERERROR_EXPORT
  #elif defined (playererror_EXPORTS)
    #define PLAYERERROR_EXPORT    __declspec (dllexport)
  #else
    #define PLAYERERROR_EXPORT    __declspec (dllimport)
  #endif
#else
  #define PLAYERERROR_EXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

/// @internal Initialize error logging
PLAYERERROR_EXPORT void ErrorInit(int _msgLevel, FILE * logfile);

/// @internal Function for print and logging errors.  Do not call this
/// function directly; use the macros below.
PLAYERERROR_EXPORT void DefaultErrorPrint(int msgType, int level, const char *file, int line, const char *fmt, ...);

PLAYERERROR_EXPORT extern void (*ErrorPrint)(int msgType, int level, const char *file, int line, const char *fmt, ...);
PLAYERERROR_EXPORT extern int msgLevel;

// File for logging messages
PLAYERERROR_EXPORT extern FILE *msgFile;

#ifdef __cplusplus
}
#endif


/// @internal Message types (internal use only; code should use the macros)
#define PLAYER_ERR_ERR 0
#define PLAYER_ERR_WARN 1
#define PLAYER_ERR_MSG 2
#define PLAYER_ERR_DBG 2

/** @ingroup libplayercommon 
 @{ */
           
/// Error message macros
#define PLAYER_ERROR(msg)         ErrorPrint(PLAYER_ERR_ERR, 0, __FILE__, __LINE__, "error   : " msg "\n")
#define PLAYER_ERROR1(msg, a)     ErrorPrint(PLAYER_ERR_ERR, 0, __FILE__, __LINE__, "error   : " msg "\n", a)
#define PLAYER_ERROR2(msg, a, b)  ErrorPrint(PLAYER_ERR_ERR, 0, __FILE__, __LINE__, "error   : " msg "\n", a, b)
#define PLAYER_ERROR3(msg, a, b, c)  ErrorPrint(PLAYER_ERR_ERR, 0, __FILE__, __LINE__,  "error   : " msg "\n", a, b, c)
#define PLAYER_ERROR4(msg, a, b, c,d)  ErrorPrint(PLAYER_ERR_ERR, 0, __FILE__, __LINE__,  "error   : " msg "\n", a, b, c, d)
#define PLAYER_ERROR5(msg, a, b, c, d, e)  ErrorPrint(PLAYER_ERR_ERR, 0, __FILE__, __LINE__,  "error   : " msg "\n", a, b, c, d, e)

/// Warning message macros
#define PLAYER_WARN(msg)        ErrorPrint(PLAYER_ERR_WARN, 0, __FILE__, __LINE__, "warning : " msg "\n")
#define PLAYER_WARN1(msg, a)    ErrorPrint(PLAYER_ERR_WARN, 0, __FILE__, __LINE__, "warning : " msg "\n", a)
#define PLAYER_WARN2(msg, a, b) ErrorPrint(PLAYER_ERR_WARN, 0, __FILE__, __LINE__, "warning : " msg "\n", a, b)
#define PLAYER_WARN3(msg, a, b, c) ErrorPrint(PLAYER_ERR_WARN, 0, __FILE__, __LINE__, "warning : " msg "\n", a, b, c)
#define PLAYER_WARN4(msg, a, b, c, d) ErrorPrint(PLAYER_ERR_WARN, 0, __FILE__, __LINE__, "warning : " msg "\n", a, b, c, d)
#define PLAYER_WARN5(msg, a, b, c, d, e) ErrorPrint(PLAYER_ERR_WARN, 0, __FILE__, __LINE__, "warning : " msg "\n", a, b, c, d, e)
#define PLAYER_WARN6(msg, a, b, c, d, e, f) ErrorPrint(PLAYER_ERR_WARN, 0, __FILE__, __LINE__, "warning : " msg "\n", a, b, c, d, e, f)
#define PLAYER_WARN7(msg, a, b, c, d, e, f, g) ErrorPrint(PLAYER_ERR_WARN, 0, __FILE__, __LINE__, "warning : " msg "\n", a, b, c, d, e, f, g)

/// General messages.  Use level to indicate the message importance
///  - 0 : important
///  - 1 : informative
///  - 2+ : diagnostic
/// All messages are recorded in the log file, but only the more important
/// messages are printed on the console.  Use the command line option to
/// dictate which messages will be printed.
#define PLAYER_MSG0(level, msg) ErrorPrint(PLAYER_ERR_MSG, level, __FILE__, __LINE__, "" msg "\n") 
#define PLAYER_MSG1(level, msg, a) ErrorPrint(PLAYER_ERR_MSG, level, __FILE__, __LINE__, "" msg "\n", a) 
#define PLAYER_MSG2(level, msg, a, b)  ErrorPrint(PLAYER_ERR_MSG, level, __FILE__, __LINE__, "" msg "\n", a, b)
#define PLAYER_MSG3(level, msg, a, b, c) ErrorPrint(PLAYER_ERR_MSG, level, __FILE__, __LINE__, "" msg "\n", a, b, c)
#define PLAYER_MSG4(level, msg, a, b, c, d) ErrorPrint(PLAYER_ERR_MSG, level, __FILE__, __LINE__, "" msg "\n", a, b, c, d)
#define PLAYER_MSG5(level, msg, a, b, c, d, e) ErrorPrint(PLAYER_ERR_MSG, level, __FILE__, __LINE__, "" msg "\n", a, b, c, d, e)
#define PLAYER_MSG6(level, msg, a, b, c, d, e, f) ErrorPrint(PLAYER_ERR_MSG, level, __FILE__, __LINE__, "" msg "\n", a, b, c, d, e, f)
#define PLAYER_MSG7(level, msg, a, b, c, d, e, f, g) ErrorPrint(PLAYER_ERR_MSG, level, __FILE__, __LINE__, "" msg "\n", a, b, c, d, e, f, g)

/** @} */

#endif
