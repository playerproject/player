/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2008
 *     Toby Collett
 *
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
 */

/*
 * filewatcher.h
 *
 *  Created on: 10/06/2008
 *      Author: tcollett
 */

#ifndef FILEWATCHER_H_
#define FILEWATCHER_H_

#if defined (WIN32)
  #if defined (PLAYER_STATIC)
    #define PLAYERCORE_EXPORT
  #elif defined (playercore_EXPORTS)
    #define PLAYERCORE_EXPORT    __declspec (dllexport)
  #else
    #define PLAYERCORE_EXPORT    __declspec (dllimport)
  #endif
#else
  #define PLAYERCORE_EXPORT
#endif

#if !defined WIN32
  #include <sys/select.h>
#endif
#include <sys/types.h>
#include <stdlib.h>
#include <pthread.h>
#include <libplayercore/message.h>

class MessageQueue;

struct fd_driver_pair
{
	int fd;
	QueuePointer queue;
	bool Read;
	bool Write;
	bool Except;
};

const size_t INITIAL_WATCHED_FILES_ARRAY_SIZE = 32;

class PLAYERCORE_EXPORT FileWatcher
{
public:
	FileWatcher();
	virtual ~FileWatcher();

	int Wait(double Timeout = 0);
	int AddFileWatch(int fd, QueuePointer & queue, bool WatchRead = true, bool WatchWrite = false, bool WatchExcept = true);
	int RemoveFileWatch(int fd, QueuePointer & queue, bool WatchRead = true, bool WatchWrite = false, bool WatchExcept = true);
	int AddFileWatch(int fd, bool WatchRead = true, bool WatchWrite = false, bool WatchExcept = true);
	int RemoveFileWatch(int fd, bool WatchRead = true, bool WatchWrite = false, bool WatchExcept = true);

private:
	struct fd_driver_pair * WatchedFiles;
	size_t WatchedFilesArraySize;
	size_t WatchedFilesArrayCount;

    /** @brief Lock access to watcher internals. */
    virtual void Lock(void);
    /** @brief Unlock access to watcher internals. */
    virtual void Unlock(void);
    /// Used to lock access to Data.
    pthread_mutex_t lock;

};

#endif /* FILEWATCHER_H_ */
