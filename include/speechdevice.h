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

/*
 * $Id$
 *
 *   The speech device.  This one interfaces to the Festival speech
 *   synthesis system (see http://www.cstr.ed.ac.uk/projects/festival/).
 *   It runs Festival in server mode and feeds it text strings to say.
 *
 *   Takes variable length commands which are just ASCII strings to say.
 *   Shouldn't return any data, but returns a single dummy byte right now
 *   Accepts no configuration (for now)
 */

#ifndef _SPEECHDEVICE_H
#define _SPEECHDEVICE_H

#include <pthread.h>

#include <lock.h>
#include <device.h>
#include <messages.h>

class CSpeechDevice:public CDevice 
{
  private:
    pthread_t thread;   // the thread that interacts with Festival
    int pid;      // Festival's pid so we can kill it later (if necessary)

    CLock lock;

  public:
    /* a queue to hold incoming speech strings */
    /*  what kind of replacement policy to use? */
    char queue[SPEECH_MAX_QUEUE_LEN][SPEECH_MAX_STRING_LEN];
    int queue_insert_idx;
    int queue_remove_idx;
    int queue_len;
    bool read_pending;
    int sock;               // socket to Festival
    int portnum;  // port number where Festival will run (default 1314)
    player_speech_data_t* data;
    player_speech_cmd_t* command;
    int command_size; // zeroed by thread (commands are consumed)

    // constructor 
    //
    CSpeechDevice(int argc, char** argv);

    ~CSpeechDevice();
    void KillFestival();

    virtual CLock* GetLock( void ){ return &lock; };
    
    int Setup();
    int Shutdown();

    size_t GetData(unsigned char *, size_t maxsize);
    void PutData(unsigned char *, size_t maxsize);

    void GetCommand(unsigned char *, size_t maxsize);
    void PutCommand(unsigned char *, size_t maxsize);

    size_t GetConfig(unsigned char *, size_t maxsize);
    void PutConfig(unsigned char *, size_t maxsize);
};

#endif

