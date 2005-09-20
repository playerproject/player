/*
 *  Copyright (C) 2005
 *     Brad Kratochvil
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

 /*
 * $Id$
 *
 *   a collection of debugging macros
 */

#ifndef UTIL_DEBUG_H
#define UTIL_DEBUG_H

#include <iostream>

/** Debugging Macros
 *  \section debug Debug
 *  \brief macros used for debugging
 *
 * These macros can be turned on/off through defining of DEBUG_LEVEL as one
 * of the following:
 * - NONE
 * - LOW
 * - MEDIUM
 * - HIGH
 */

#define NONE   0
#define LOW    1
#define MEDIUM 2
#define HIGH   3

/** \def DEBUG(x)
 *  \brief output name and value of expression
 */
#if DEBUG_LEVEL < LOW
#define DEBUG(x)
#else
#define DEBUG(x) std::cout << #x"= " << (x) << std::endl
#endif

/** \def EVAL(x)
 *  \brief evaluate a variable
 */
#if DEBUG_LEVEL < HIGH
#define EVAL(x)
#else
#define EVAL(x) \
  std::cout << #x << ": " << x << std::endl;
#endif

/** \def CHECK(x, y)
 *  \brief check if condition x is true and if y!=0 then exit
 */
#if DEBUG_LEVEL < MEDIUM
#define CHECK(x, y)
#else
#define CHECK(x, y) \
  if (! (x)) \
  { \
    std::cerr << "CHECK " << #x << " failed" << std::endl; \
    std::cerr << " on line " << __LINE__  << std::endl; \
    std::cerr << " in file " << __FILE__ << std::endl;  \
    if (0!=y) exit(y);  \
  }
#endif

#endif
