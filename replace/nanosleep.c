#if defined (WIN32)
#include <windows.h>

#include "replace.h"

// Replacement for nanosleep on Windows.
// NOTES:
// The rem argument is never filled. You cannot rely on it.
// The return value will always be zero. There is no way to tell if the sleep was interrupted.
// The resolution of this is milliseconds. TODO: fix that by using the high performance timer.
int nanosleep (const struct timespec *req, struct timespec *rem)
{
	Sleep (req->tv_sec * 1000 + req->tv_nsec / 1000000);
	return 0;
}

#endif
