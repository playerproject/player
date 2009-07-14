#if defined (WIN32)
#include <windows.h>

#include "replace.h"

// Replacement for usleep on Windows.
// NOTES:
// The return value will always be zero. There is no way to tell if the sleep was interrupted.
// Although the argument is in microseconds, the resolution of this is 100 nanoseconds.
int usleep (int usec)
{
	HANDLE timer = NULL;
	LARGE_INTEGER sleepTime;

	sleepTime.QuadPart = usec * 10000;

	timer = CreateWaitableTimer (NULL, TRUE, NULL);
	if (timer == NULL)
	{
		LPVOID buffer = NULL;
		FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
						GetLastError(), 0, reinterpret_cast<LPTSTR> (&buffer), 0, NULL);
		PLAYER_ERROR2 ("usleep: CreateWaitableTimer failed: (%d) %s\n",
						GetLastError (), reinterpret_cast<LPTSTR> (buffer));
		LocalFree (buffer);
		return -1;
	}

	if (!SetWaitableTimer (timer, &sleepTime, 0, NULL, NULL, 0))
	{
		LPVOID buffer = NULL;
		FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
						GetLastError(), 0, reinterpret_cast<LPTSTR> (&buffer), 0, NULL);
		PLAYER_ERROR2 ("usleep: SetWaitableTimer failed: (%d) %s\n",
						GetLastError (), reinterpret_cast<LPTSTR> (buffer));
		LocalFree (buffer);
		return -1;
	}

	if (WaitForSingleObject (timer, INFINITE) != WAIT_OBJECT_0)
	{
		LPVOID buffer = NULL;
		FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL,
						GetLastError(), 0, reinterpret_cast<LPTSTR> (&buffer), 0, NULL);
		PLAYER_ERROR2 ("usleep: WaitForSingleObject failed: (%d) %s\n",
						GetLastError (), reinterpret_cast<LPTSTR> (buffer));
		LocalFree (buffer);
		return -1;
	}
	Sleep (usec / 1000);
	return 0;
}

#endif
