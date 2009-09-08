/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/********************************************************************
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
 *
 ********************************************************************/

/*
 * $Id: driver.cc 6501 2008-06-10 05:16:08Z thjc $
 *
 *  the base class from which all device classes inherit.  here
 *  we implement some generic methods that most devices will not need
 *  to override
 */
#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <stdlib.h>
#include <libplayercore/device.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <assert.h>
#include <stddef.h>
#if defined WIN32
  // For Sleep()
  #include <windows.h>
#else
  #include <unistd.h>
  #include <time.h>
  #include <netinet/in.h>
#endif

#include <libplayerinterface/interface_util.h>
#include <libplayercommon/playercommon.h>

#include <libplayercore/playertime.h>
#include <libplayercore/driver.h>
#include <libplayercore/devicetable.h>
#include <libplayercore/configfile.h>
#include <libplayercore/globals.h>
#include <libplayercore/filewatcher.h>
#include <libplayercore/property.h>

// Default constructor for single-interface drivers.  Specify the
// interface code and buffer sizes.
ThreadedDriver::ThreadedDriver(ConfigFile *cf, int section, bool overwrite_cmds, size_t queue_maxlen, int interf) :
	Driver(cf, section, overwrite_cmds, queue_maxlen, interf),
	ThreadState(PLAYER_THREAD_STATE_STOPPED)
{
	memset (&driverthread, 0, sizeof (driverthread));
}

// this is the other constructor, used by multi-interface drivers.
ThreadedDriver::ThreadedDriver(ConfigFile *cf, int section, bool overwrite_cmds, size_t queue_maxlen) :
	Driver(cf, section, overwrite_cmds, queue_maxlen),
	ThreadState(PLAYER_THREAD_STATE_STOPPED)
{
	memset (&driverthread, 0, sizeof (driverthread));
}

// destructor, to free up allocated queue.
ThreadedDriver::~ThreadedDriver()
{
	// if our thread is still running request it to be stopped
	if (ThreadState == PLAYER_THREAD_STATE_RUNNING || ThreadState == PLAYER_THREAD_STATE_RESTARTING)
	{
		StopThread();
	}
	// wait for the thread to actually stop
	while (ThreadState != PLAYER_THREAD_STATE_STOPPED)
	{
#if defined WIN32
		Sleep (100);
#else
		struct timespec ts;
		ts.tv_sec = 0;
		ts.tv_nsec = 100000000;
		nanosleep(&ts, NULL);
#endif
	}

}

void ThreadedDriver::TestCancel()
{
	int oldstate;
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&oldstate);
	pthread_testcancel();
	pthread_setcancelstate(oldstate,NULL);
}

/* start a thread that will invoke Main() */
void
ThreadedDriver::StartThread(void)
{
  if (ThreadState == PLAYER_THREAD_STATE_STOPPED)
  {
    SetupBarrier.SetValue(2);
    pthread_create(&driverthread, NULL, &DummyMain, this);

    // sync with dummy main
    SetupBarrier.Wait();
    ThreadState = PLAYER_THREAD_STATE_RUNNING;
  }
  else if (ThreadState == PLAYER_THREAD_STATE_STOPPING)
  {
    ThreadState = PLAYER_THREAD_STATE_RESTARTING;
  }
  else
  {
    PLAYER_ERROR1("StartThread called when state != stopped or stopping (%d)", ThreadState);

  }
}

/* cancel (and wait for termination) of the thread */
void
ThreadedDriver::StopThread(void)
{
  if (ThreadState == PLAYER_THREAD_STATE_RUNNING)
  {
	PLAYER_MSG2(5,"Cancelling thread %p belonging to driver %p",driverthread,this);
    pthread_cancel(driverthread);
    if(pthread_detach(driverthread))
      perror("ThreadedDriver::StopThread:pthread_detach()");
    ThreadState = PLAYER_THREAD_STATE_STOPPING;
  }
  else if (ThreadState == PLAYER_THREAD_STATE_RESTARTING)
  {
    ThreadState = PLAYER_THREAD_STATE_STOPPING;
  }
  else
  {
    PLAYER_ERROR1("StopThread called when state != running or restarting (%d)", ThreadState);

  }
}

/* Dummy main (just calls real main) */
void*
ThreadedDriver::DummyMain(void *devicep)
{
  // should never be called without a driver pointer
  assert(devicep);

  // Defer initalisation
  pthread_setcancelstate(PTHREAD_CANCEL_DISABLE,NULL);
  ThreadedDriver &tdriver = *reinterpret_cast<ThreadedDriver*>(devicep);
  tdriver.SetupSuccessful = false;
  // sync with start thread
  tdriver.SetupBarrier.Wait();

  pthread_cleanup_push(&DummyMainQuit, devicep);
  int ret = tdriver.MainSetup();
  // Run the overloaded Main() in the subclassed device.
  if (ret == 0)
  {
    tdriver.SetupSuccessful = true;
    tdriver.Main();
  }
  else
  {
    // TODO: publish an exception message here, have to create exception message type and system first
    PLAYER_ERROR1("Driver failed to Setup (%d)", ret);
  }
  pthread_cleanup_pop(1);

  return NULL;
}

/* Dummy main cleanup (just calls real main cleanup) */
void
ThreadedDriver::DummyMainQuit(void *devicep)
{
  // should never be called without a driver pointer
  assert(devicep);

  // Run the overloaded MainCleanup() in the subclassed device.
  ThreadedDriver* driver = reinterpret_cast<ThreadedDriver*> (devicep);
  if (driver->SetupSuccessful)
    driver->MainQuit();
  driver->Lock();
  memset (&driver->driverthread, 0, sizeof (driver->driverthread));
  if (driver->ThreadState == PLAYER_THREAD_STATE_RESTARTING)
  {
    driver->ThreadState = PLAYER_THREAD_STATE_STOPPED;
    driver->StartThread();
  }
  else
  {
    driver->ThreadState = PLAYER_THREAD_STATE_STOPPED;
  }
  driver->Unlock();
}

int
ThreadedDriver::Shutdown()
{
	Lock();
	StopThread();
	Unlock();
	// Release the driver thread, in case it's waiting on the message queue
	// or the driver mutex.
	this->InQueue->DataAvailable();

	return 0;
}

int
ThreadedDriver::Setup()
{
	Lock();
	StartThread();
	Unlock();

	return 0;
}

int ThreadedDriver::Terminate()
{
	int ret = Driver::Terminate();
	// wait for the thread to actually stop
	while (ret == 0 && ThreadState != PLAYER_THREAD_STATE_STOPPED)
	{
#if defined (WIN32)
		Sleep (100);
#else
		struct timespec ts;
		ts.tv_sec = 0;
		ts.tv_nsec = 100000000;
		nanosleep(&ts, NULL);
#endif
	}
	return ret;
}

bool ThreadedDriver::Wait(double TimeOut)
{
	int oldstate, ret;
	pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,&oldstate);
	pthread_testcancel();
	ret = this->InQueue->Wait(TimeOut);
	pthread_testcancel();
	pthread_setcancelstate(oldstate,NULL);
	return ret ? true : false;
}
