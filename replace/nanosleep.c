#if defined (WIN32)
#include <windows.h>

#include "replace.h"

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
						GetLastError(), 0, reinterpret_cast<LPTSTR> (&buffer), 0, NULL);
		PLAYER_ERROR2 ("nanosleep: CreateWaitableTimer failed: (%d) %s\n",
						GetLastError (), reinterpret_cast<LPTSTR> (buffer));
		LocalFree (buffer);
		return -1;
	}

	if (!SetWaitableTimer (timer, &sleepTime, 0, NULL, NULL, 0))
	{
		LPVOID buffer = NULL;
		FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
						GetLastError(), 0, reinterpret_cast<LPTSTR> (&buffer), 0, NULL);
		PLAYER_ERROR2 ("nanosleep: SetWaitableTimer failed: (%d) %s\n",
						GetLastError (), reinterpret_cast<LPTSTR> (buffer));
		LocalFree (buffer);
		return -1;
	}

	if (WaitForSingleObject (timer, INFINITE) != WAIT_OBJECT_0)
	{
		LPVOID buffer = NULL;
		FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
						GetLastError(), 0, reinterpret_cast<LPTSTR> (&buffer), 0, NULL);
		PLAYER_ERROR2 ("nanosleep: WaitForSingleObject failed: (%d) %s\n",
						GetLastError (), reinterpret_cast<LPTSTR> (buffer));
		LocalFree (buffer);
		return -1;
	}

	return 0;
}

#endif
