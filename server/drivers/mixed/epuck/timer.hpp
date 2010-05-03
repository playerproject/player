/* Copyright 2008 Renato Florentino Garcia <fgar.renato@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TIMER_HPP
#define TIMER_HPP

#include <sys/timeb.h>
#include <time.h>

/** \file
 * Header file of EpuckTimer class.
 */

#define TIMEB timeb
#define FTIME ftime

/** A timer class.
 *
 * Provides a timer with resolution of milliseconds.
 *  \author Renato Florentino Garcia
 *  \date August 2008
 */
class EpuckTimer
{
public:

  EpuckTimer()
    :intervalRunning(false)
  {}

  /** Set the start time
   *
   * @param startTime Start time offset in seconds.
   *
   * @return The start time, i.e. the time since 00:00:00 UTC in seconds.
   */
  double initialize(double startTime=0.0)
  {
    this->offset = startTime;

    FTIME(&t);
    t0 = (double)(t.time + t.millitm/1000.0);
    return t0;
  }

  /** Get the time elapsed since the initialization.
   *
   * Get the time elapsed since the initialization, plus the offset that was
   * given.
   * @return The elapsed time, in seconds.
   */
  double elapsedTime()
  {
    currentTime();
    return(t2-t0 + offset);
  }

  /** Start a new interval.
   *
   * Start a new interval which will be intervalDelay.
   */
  void resetInterval()
  {
    intervalRunning = true;
    FTIME(&t);
    t1 = (double)(t.time + t.millitm/1000.0);
  }

  /** Get the time elapsed since the last call at resetInterval.
   *
   * Return the elapsed time since resetInterval was called at last time.
   * Case it wasn't yet called the delta time will be 0.
   * @return The elapsed time in seconds.
   */
  double intervalDelay()
  {
    if(intervalRunning == false)
    {
      return 0;
    }
    FTIME(&t);
    t2 = (double)(t.time + t.millitm/1000.0);
    return t2-t1;
  }

private:

  // Offset that will be added at current delta time from start instant.
  double offset;
  struct TIMEB t;
  double t0, t1, t2;
  bool intervalRunning;

  // return The elapsed time since 00:00:00 UTC, in seconds.
  double currentTime()
  {
    FTIME(&t);
    t2 = (double)(t.time + t.millitm/1000.0);
    return t2;
  }

};


#endif

