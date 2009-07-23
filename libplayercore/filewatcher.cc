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
 * FileWatcher.cc
 *
 *  Created on: 10/06/2008
 *      Author: tcollett
 */

#include <libplayercommon/playercommon.h>


#include <libplayercore/filewatcher.h>
#include <libplayercore/message.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#if defined (WIN32)
  #include <windows.h>
#else
  #include <sys/time.h>
#endif

FileWatcher::FileWatcher()
{
	WatchedFilesArraySize = INITIAL_WATCHED_FILES_ARRAY_SIZE;
	WatchedFilesArrayCount = 0;
	WatchedFiles = reinterpret_cast<struct fd_driver_pair *> (calloc(WatchedFilesArraySize,sizeof(WatchedFiles[0])));
	assert(WatchedFiles);
	pthread_mutex_init(&this->lock,NULL);

}

FileWatcher::~FileWatcher()
{
	free(WatchedFiles);
}

void FileWatcher::Lock()
{
  pthread_mutex_lock(&lock);
}

void FileWatcher::Unlock()
{
  pthread_mutex_unlock(&lock);
}


int FileWatcher::Wait(double Timeout)
{
	Lock();
	if (WatchedFilesArrayCount == 0)
	{
		PLAYER_ERROR("File watcher wait called with no files to watch");
		Unlock();
		return 0;
	}

	// intialise our FD sets for the select call
	fd_set ReadFds,WriteFds,ExceptFds;
	FD_ZERO(&ReadFds);
	FD_ZERO(&WriteFds);
	FD_ZERO(&ExceptFds);

	int maxfd = 0;

	for (unsigned int ii = 0; ii < WatchedFilesArrayCount; ++ii)
	{
		if (WatchedFiles[ii].fd >= 0)
		{
			if (WatchedFiles[ii].fd > maxfd)
				maxfd = WatchedFiles[ii].fd;
			if (WatchedFiles[ii].Read)
				FD_SET(WatchedFiles[ii].fd,&ReadFds);
			if (WatchedFiles[ii].Write)
				FD_SET(WatchedFiles[ii].fd,&WriteFds);
			if (WatchedFiles[ii].Except)
				FD_SET(WatchedFiles[ii].fd,&ExceptFds);
		}
	}

	struct timeval t;
	t.tv_sec = static_cast<int> (floor(Timeout));
	t.tv_usec = static_cast<int> ((Timeout - static_cast<int> (floor(Timeout))) * 1e6);

	// unlock for the select call
	// in the worst case if the list gets modified during the call we will either
	// not be able to match an event on a deleted fd, or will get spurious wake ups
	// on a newly added fd all of which are non fatal
	Unlock();
	int ret = select (maxfd+1,&ReadFds,&WriteFds,&ExceptFds,&t);

	if (ret < 0)
	{
		// dont print a warning if we are ctrl+c'd
		if (errno != EINTR)
			PLAYER_ERROR2("Select called failed in File Watcher: %d %s",errno,strerror(errno));
		return ret;
	}
	else if (ret == 0)
	{
		return 0;
	}

	Lock();

	int queueless_count = 0;
	int match_count = 0;

	for (unsigned int ii = 0; ii < WatchedFilesArrayCount && ret > match_count; ++ii)
	{
		int fd = WatchedFiles[ii].fd;
		QueuePointer &q = WatchedFiles[ii].queue;
		if (fd > 0 && fd <= maxfd)
		{
			if ((WatchedFiles[ii].Read && FD_ISSET(fd,&ReadFds)) ||
					(WatchedFiles[ii].Write && FD_ISSET(fd,&WriteFds)) ||
					(WatchedFiles[ii].Except && FD_ISSET(fd,&ExceptFds)))
			{
				match_count++;
				if (q != NULL)
				{
					q->DataAvailable();
				}
				else
				{
					queueless_count++;
				}

			}
		}
	}
	Unlock();

	if (ret != match_count)
	{
		PLAYER_ERROR1("Failed to match %d file descriptors in select results",ret - match_count);
	}

	return queueless_count;
}

int FileWatcher::AddFileWatch(int fd, bool WatchRead, bool WatchWrite, bool WatchExcept)
{
	QueuePointer q;
	return AddFileWatch(fd, q,WatchRead,WatchWrite,WatchExcept);
}


int FileWatcher::AddFileWatch(int fd, QueuePointer & queue, bool WatchRead, bool WatchWrite, bool WatchExcept)
{
	Lock();
	// find the first available file descriptor
	struct fd_driver_pair *next_entry = NULL;
		// first see if there is an empty spot in the list
	for (unsigned int ii = 0; ii < WatchedFilesArrayCount; ++ii)
	{
		if (WatchedFiles[ii].fd < 0)
		{
			next_entry = &WatchedFiles[ii];
			break;
		}
	}
	if (next_entry == NULL)
	{
		if (WatchedFilesArrayCount >= WatchedFilesArraySize)
		{
			// otherwise we allocate some more room for the array
			size_t orig_size = WatchedFilesArraySize;
			WatchedFilesArraySize*=2;
			WatchedFiles = reinterpret_cast<struct fd_driver_pair *> (realloc(WatchedFiles,sizeof(WatchedFiles[0])*WatchedFilesArraySize));
			memset(&WatchedFiles[orig_size],0,sizeof(WatchedFiles[0])*(WatchedFilesArraySize-orig_size));
		}
		next_entry = &WatchedFiles[WatchedFilesArrayCount];
		WatchedFilesArrayCount++;
	}

	next_entry->fd = fd;
	next_entry->queue = queue;
	next_entry->Read = WatchRead;
	next_entry->Write = WatchWrite;
	next_entry->Except = WatchExcept;

	Unlock();
	return 0;
}

int FileWatcher::RemoveFileWatch(int fd, bool WatchRead, bool WatchWrite, bool WatchExcept)
{
	QueuePointer q;
	return RemoveFileWatch(fd, q,WatchRead,WatchWrite,WatchExcept);
}


int FileWatcher::RemoveFileWatch(int fd, QueuePointer &queue, bool WatchRead, bool WatchWrite, bool WatchExcept)
{
	Lock();
	// this finds the first matching entry and removes it. It only removes one entry so call remove for every add
	for (unsigned int ii = 0; ii < WatchedFilesArrayCount; ++ii)
	{
		if (WatchedFiles[ii].fd == fd &&
				WatchedFiles[ii].queue == queue &&
				WatchedFiles[ii].Read == WatchRead &&
				WatchedFiles[ii].Write == WatchWrite &&
				WatchedFiles[ii].Except == WatchExcept)
		{
			WatchedFiles[ii].fd = -1;
			Unlock();
			return 0;
		}
	}
	Unlock();
	return -1;
}

