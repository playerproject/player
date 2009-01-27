#if defined (WIN32)
#include <windows.h>

#include "replace.h"

// Replacement for usleep on Windows.
// NOTES:
// The return value will always be zero. There is no way to tell if the sleep was interrupted.
// The resolution of this is milliseconds. TODO: fix that by using the high performance timer.
int usleep (int usec)
{
	Sleep (usec / 1000);
	return 0;
}

#endif
