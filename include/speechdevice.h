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


#include <device.h>
#include <messages.h>

class CSpeechDevice:public CDevice 
{
  private:
    int pid;      // Festival's pid so we can kill it later (if necessary)

    int portnum;  // port number where Festival will run (default 1314)
    char festival_libdir_value[MAX_FILENAME_SIZE]; // the libdir

    /* a queue to hold incoming speech strings */
    PlayerQueue* queue;

    bool read_pending;


  public:
    int sock;               // socket to Festival
    void KillFestival();

    static CDevice* Init(int argc, char** argv)
    {
      return((CDevice*)(new CSpeechDevice(argc,argv)));
    }
    
    // constructor 
    CSpeechDevice(int argc, char** argv);

    ~CSpeechDevice();
    virtual void Main();

    int Setup();
    int Shutdown();

    size_t GetCommand(unsigned char *, size_t maxsize);
    void PutCommand(unsigned char *, size_t maxsize);
};

#endif

