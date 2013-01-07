/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2004
 *     Brian P. Gerkey <gerkey@ai.stanford.edu>
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 ********************************************************************/

/*
 * A timer thread.  This thread has a main loop that sleeps, wakes up the
 * server thread, and repeats.
 *
 * $Id$
 */

#include <pthread.h>
#include <time.h>

#include "clientmanager.h"

class Timer
{
  private:
    // how long we'll sleep for each iteration
    struct timespec ts;

    // our thread
    pthread_t thread;

    // pointer to the clientmanager that we'll wake up
    ClientManager* manager;

    // function we'll pass to pthread_create
    static void* DummyMain(void* timer);

  public:
    Timer(ClientManager* tmp_manager, struct timespec tmp_ts);
    ~Timer() {}

    // Start the timer thread
    void Start(void);

    // Stop the timer thread
    void Stop(void);

    void Main(void);
};
