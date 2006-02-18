/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey et al.
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

#include <stdint.h>

#include <libplayercore/playercore.h>
#include <libplayercore/error.h>

#include "acpGarcia.h"

////////////////////////////////////////////////////////////////////////////////
// The class for the driver
class GarciaDriver : public Driver
{
  public:

    // Constructor; need that
    GarciaDriver(ConfigFile* cf, int section);

    // Must implement the following methods.
    int Setup();
    int Shutdown();
    // Main function for device thread.
    virtual void Main();

    // This method will be invoked on each incoming message
    virtual int ProcessMessage(MessageQueue* resp_queue,
                               player_msghdr * hdr,
                               void * data);
    //void ProcessConfig();
    void ProcessPos2dCommand(player_msghdr_t* hdr, player_position2d_cmd_t &data);
    void ProcessSpeechCommand(player_msghdr_t* hdr, player_speech_cmd_t &data);
    void ProcessDioCommand(player_msghdr_t* hdr, player_dio_cmd_t &data);
    void RefreshData();

 private:

    // position2d interface
    player_devaddr_t           mPos2dAddr;
    player_position2d_data_t   mPos2dData;
    player_position2d_cmd_t    mPos2dCmd;

    // ir interface
    player_devaddr_t       mIrAddr;
    player_ir_data_t       mIrData;
    player_ir_cmd_t        mIrCmd;

    // speech interface
    player_devaddr_t       mSpeechAddr;
    player_speech_data_t   mSpeechData;
    player_speech_cmd_t    mSpeechCmd;

    // dio interface
    player_devaddr_t       mDioAddr;
    player_dio_data_t      mDioData;
    player_dio_cmd_t       mDioCmd;

    int32_t mSleep;

    acpGarcia* mGarcia;

    const char *mConfigPath;
};
