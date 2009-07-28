/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2009
 *     Geoff Biggs
 *
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
 */

#if defined (WIN32)
  #define ErrNo WSAGetLastError()
#else
  #define ErrNo errno
#endif

// Joy of joys, even strerror isn't the same for Windows sockets
#if defined (WIN32)
  const int ERRNO_EAGAIN = WSAEWOULDBLOCK;
  const int ERRNO_EWOULDBLOCK = WSAEWOULDBLOCK;
  #define STRERROR(errMacro,text) {LPVOID errBuffer = NULL; \
    FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, \
    ErrNo, 0, (LPTSTR) &errBuffer, 0, NULL); \
    errMacro (text, (LPTSTR) errBuffer); \
    LocalFree (errBuffer);}
#else
  #define STRERROR(errMacro,text) errMacro (text, strerror (ErrNo));
  const int ERRNO_EAGAIN = EAGAIN;
  const int ERRNO_EWOULDBLOCK = EWOULDBLOCK;
#endif
