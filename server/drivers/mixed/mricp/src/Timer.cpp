#include "Timer.h"
#include <ctime>
#include <cstddef>

Timer::Timer()
{
	gettimeofday(&start_time,NULL);
}
double Timer::TimeElapsed() // return in usec
{
	gettimeofday(&end_time,NULL);
	time_diff = ((double) end_time.tv_sec*1000000 + (double)end_time.tv_usec) -
	            ((double) start_time.tv_sec*1000000 + (double)start_time.tv_usec);
	return  time_diff;
}
Timer::~Timer()
{
}
void Timer::Reset()
{
	gettimeofday(&start_time,NULL);
}
void Timer::Synch(double period)
{
	struct timespec ts;
	int us;

	double time_elapsed = this->TimeElapsed();
	if( time_elapsed < period*1000)
		usleep((int)(period*1000 -time_elapsed));
	if (time_elapsed < (period*1000))
	{
		us = static_cast<int>(period*1000-time_elapsed);
		ts.tv_sec = us/1000000;
		ts.tv_nsec = (us%1000000)*1000;
		nanosleep(&ts, NULL);
	}
}
