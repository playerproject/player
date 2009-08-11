/*
Copyright (c) Geoffrey Biggs, 2009
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice, 
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of the Player Project nor the names of its contributors 
      may be used to endorse or promote products derived from this software 
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON 
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#if defined (WIN32)
#define _WIN32_WINNT 0x0500
#include <windows.h>

#include "replace.h"
#include "libplayercommon/error.h"

// Replacement for nanosleep on Windows.
// NOTES:
// The rem argument is never filled. You cannot rely on it.
// The return value will always be zero. There is no way to tell if the sleep was interrupted.
// The resolution of this is 100 nanoseconds, despite the argument being seconds and nanoseconds.
int nanosleep (const struct timespec *req, struct timespec *rem)
{
	HANDLE timer = NULL;
	LARGE_INTEGER sleepTime;

	sleepTime.QuadPart = req->tv_sec * 1000000000 + req->tv_nsec / 100;

	timer = CreateWaitableTimer (NULL, TRUE, NULL);
	if (timer == NULL)
	{
		LPVOID buffer = NULL;
		FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
						GetLastError(), 0, (LPTSTR) &buffer, 0, NULL);
		PLAYER_ERROR2 ("nanosleep: CreateWaitableTimer failed: (%d) %s\n",
						GetLastError (), (LPTSTR) buffer);
		LocalFree (buffer);
		return -1;
	}

	if (!SetWaitableTimer (timer, &sleepTime, 0, NULL, NULL, 0))
	{
		LPVOID buffer = NULL;
		FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
						GetLastError(), 0, (LPTSTR) &buffer, 0, NULL);
		PLAYER_ERROR2 ("nanosleep: SetWaitableTimer failed: (%d) %s\n",
						GetLastError (), (LPTSTR) buffer);
		LocalFree (buffer);
		return -1;
	}

	if (WaitForSingleObject (timer, INFINITE) != WAIT_OBJECT_0)
	{
		LPVOID buffer = NULL;
		FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
						GetLastError(), 0, (LPTSTR) &buffer, 0, NULL);
		PLAYER_ERROR2 ("nanosleep: WaitForSingleObject failed: (%d) %s\n",
						GetLastError (), (LPTSTR) buffer);
		LocalFree (buffer);
		return -1;
	}

	return 0;
}

#endif
