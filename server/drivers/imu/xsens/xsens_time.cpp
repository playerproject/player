// Note, this function requires compiler option "-lrt" to be set when compiling with gcc

#include "xsens_time.h"
#include <sys/timeb.h>

#ifdef _WIN32
#	include <windows.h>
#else
#	include <unistd.h>
#   include <sys/time.h>
#endif

namespace xsens {

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////// Other  functions ////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
// A platform-independent clock.
unsigned long getTimeOfDay(tm* date_, time_t* secs_)
{
#ifdef _WIN32
	__timeb32 tp;

	_ftime32_s(&tp);

	if (date_ != NULL)
	{
		__time32_t tin = tp.time;
		_localtime32_s(date_,&tin);
	}
	if (secs_ != NULL)
		secs_[0] = tp.time;

	// 86400 = 24*60*60 = secs in a day, this gives us the seconds since midnight
	return (1000 * ((unsigned long) tp.time % XSENS_SEC_PER_DAY)) + tp.millitm;
#else
	timespec tp;
	clock_gettime(CLOCK_REALTIME, &tp); // compile with -lrt

	if (date_ != NULL)
		localtime_r(&tp.tv_sec,date_);

	if (secs_ != NULL)
		secs_[0] = tp.tv_sec;

	// 86400 = 24*60*60 = secs in a day, this gives us the seconds since midnight
	return (1000 * (tp.tv_sec % XSENS_SEC_PER_DAY)) + (tp.tv_nsec/1000000);
#endif
}

//////////////////////////////////////////////////////////////////////////////////////////
// A platform-independent sleep routine.
void msleep(unsigned long ms)
{
#ifdef _WIN32
	Sleep(ms);
#else
	clock_t end = clock() + (CLOCKS_PER_SEC/1000) * ms;
	clock_t diff;

	while ((diff = end - clock()) > 0)
	{
		diff = (1000 * diff) / CLOCKS_PER_SEC;
		if (diff > 1000)
			sleep(diff / 1000);
		else
			usleep(diff * 1000);
	}
#endif
}

TimeStamp timeStampNow(void)
{
	TimeStamp ms;
	time_t s;
	ms = (TimeStamp) getTimeOfDay(NULL,&s);
	ms = (ms % 1000) + (((TimeStamp)s)*1000);

	return ms;
}

}	// end of xsens namespace
