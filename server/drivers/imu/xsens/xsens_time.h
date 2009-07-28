#ifndef _XSENS_TIME_2006_09_12
#define _XSENS_TIME_2006_09_12

#include <time.h>
#include <replace/replace.h>

namespace xsens {

//! The number of seconds in a normal day
#define XSENS_SEC_PER_DAY	(60*60*24)
//! The number of milliseconds in a normal day
#define XSENS_MS_PER_DAY	(XSENS_SEC_PER_DAY*1000)

//! A real-time timestamp (ms)
typedef unsigned long long TimeStamp;

/*! \brief A platform-independent clock.

	The function returns the time of day in ms since midnight. If the \c date parameter is
	non-NULL, corresponding the date is placed in the variable it points to.
*/
unsigned long getTimeOfDay(tm* date_ = NULL, time_t* secs_ = NULL);

/*! \brief A platform-independent sleep routine.

	Time is measured in ms. The function will not return until the specified
	number of ms have passed.
*/
void msleep(unsigned long ms);

TimeStamp timeStampNow(void);

}	// end of xsens namespace

#endif	// _XSENS_TIME_2006_09_12
