/***************************************************************************
 * Desc: Player client lib; stuff used internally.
 * Author: Andrew Howard
 * Date: 13 May 2002
 * CVS: $Id$
 **************************************************************************/

#ifndef PLAYERC_PRIVATE_H
#define PLAYERC_PRIVATE_H

#include <stdio.h>

// Useful error macros.
// These print out the error
/*
#define PLAYERC_ERR(msg)         printf("playerc error   : " msg "\n")
#define PLAYERC_ERR1(msg, a)     printf("playerc error   : " msg "\n", a)
#define PLAYERC_ERR2(msg, a, b)  printf("playerc error   : " msg "\n", a, b)
#define PLAYERC_ERR3(msg, a, b, c)  printf("playerc error   : " msg "\n", a, b, c)
#define PLAYERC_WARN(msg)        printf("playerc warning : " msg "\n")
#define PLAYERC_WARN1(msg, a)    printf("playerc warning : " msg "\n", a)
#define PLAYERC_WARN2(msg, a, b) printf("playerc warning : " msg "\n", a, b)
#define PLAYERC_MSG3(msg, a, b, c) printf("playerc message : " msg "\n", a, b, c)
*/

// Useful error macros.
// These ones store the error message.
#define PLAYERC_ERR(msg)         sprintf(playerc_errorstr, msg)
#define PLAYERC_ERR1(msg, a)     sprintf(playerc_errorstr, msg, a)
#define PLAYERC_ERR2(msg, a, b)  sprintf(playerc_errorstr, msg, a, b)
#define PLAYERC_ERR3(msg, a, b, c)  sprintf(playerc_errorstr, msg, a, b, c)
#define PLAYERC_WARN(msg)        sprintf(playerc_errorstr, "warning : " msg)
#define PLAYERC_WARN1(msg, a)    sprintf(playerc_errorstr, "warning : " msg, a)
#define PLAYERC_WARN2(msg, a, b) sprintf(playerc_errorstr, "warning : " msg, a, b)


// DEBUG macros
#ifdef DEBUG
#define PRINT_DEBUG(m)         printf("\rstage debug : %s %s\n  "m"\n", \
                                     __FILE__, __FUNCTION__)
#define PRINT_DEBUG1(m, a)     printf("\rstage debug : %s %s\n  "m"\n", \
                                     __FILE__, __FUNCTION__, a)
#define PRINT_DEBUG2(m, a, b)  printf("\rstage debug : %s %s\n  "m"\n", \
                                     __FILE__, __FUNCTION__, a, b)
#define PRINT_DEBUG3(m, a, b, c) printf("\rstage debug : %s %s\n  "m"\n", \
                                     __FILE__, __FUNCTION__, a, b, c)
#else
#define PRINT_DEBUG(m)
#define PRINT_DEBUG1(m, a)
#define PRINT_DEBUG2(m, a, b)
#define PRINT_DEBUG3(m, a, b, c)
#endif

#endif
