/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
 *
 *  Audiodevice attempted written by Esben Ostergaard
 *
 * This program is free software; you can redistribute it and/or modify
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

#if HAVE_CONFIG_H
  #include <config.h>
#endif

#include <sys/soundcard.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>

#include "playercommon.h"
#include "drivertable.h"
#include "player.h"

#define DEFAULT_DEVICE "/dev/mixer"

class Mixer : public CDevice
{
  public:
    // Constructor
    Mixer(char* interface, ConfigFile* cf, int section);

    int Setup();
    int Shutdown();

    size_t GetCommand(void* dest, size_t maxsize);

  private:

    // Open or close the device
    int OpenDevice( int flag );
    int CloseDevice();

    // The main loop
    virtual void Main();

    int Write( int dev, int num );
    int Read( int dev, int& num );

    int mixerFD; // File descriptor for the device
    const char* deviceName; // Name of the device( ex: "/dev/mixer" )
};

Mixer::Mixer(char* interface, ConfigFile* cf, int section)
  : CDevice(0,sizeof(player_audiomixer_cmd_t),1,1)
{
  deviceName = cf->ReadString(section,"device",DEFAULT_DEVICE);
}

CDevice* Mixer_Init(char* interface, ConfigFile* cf, int section)
{
  if(strcmp(interface, PLAYER_AUDIOMIXER_STRING))
  {
    PLAYER_ERROR1("driver \"mixer\" does not support interface \"%s\"\n", 
                  interface);
    return(NULL);
  }
  else
    return((CDevice*)(new Mixer(interface, cf, section)));
}

void Mixer_Register(DriverTable* table)
{
  table->AddDriver("mixer", PLAYER_ALL_MODE, Mixer_Init );
}

int Mixer::Setup()
{
  mixerFD = open(deviceName,O_RDWR);

  StartThread();

  puts("mixer ready");
  return 0;
}

int Mixer::Shutdown()
{
  StopThread();
  close(mixerFD);

  puts("Mixer has been shutdown");
}

size_t Mixer::GetCommand(void* dest, size_t maxsize)
{
  int retval = device_used_commandsize;

  if(device_used_commandsize)
  {
    memcpy(dest,device_command,device_used_commandsize);
    device_used_commandsize = 0;
  }
  return(retval);
}

void Mixer::Main()
{
  void* client;
  unsigned char configBuffer[PLAYER_MAX_REQREP_SIZE];
  unsigned char cmdBuffer[sizeof(player_audiodsp_cmd)];
  int len;
  int vol;

  player_audiomixer_cmd_t mixerCmd;

  while(true)
  {
    // test if we are suppose to cancel
    pthread_testcancel();

    // Set/Get the configuration
    while((len = GetConfig(&client, &configBuffer, sizeof(configBuffer))) > 0)
    {
      player_audiomixer_config_t config;

      if( len != 1 )
      {
        PLAYER_ERROR2("config request len is invalid (%d != %d)",len,1);
        if(PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
          PLAYER_ERROR("PutReply() failed");
        continue;
      }

      this->Read(SOUND_MIXER_VOLUME,vol);
      config.masterLeft = htons( vol & 0xFF );
      config.masterRight = htons( (vol>>8) & 0xFF );

      this->Read(SOUND_MIXER_PCM,vol);
      config.pcmLeft = htons( vol & 0xFF );
      config.pcmRight = htons( (vol>>8) & 0xFF );

      this->Read(SOUND_MIXER_LINE,vol);
      config.lineLeft = htons( vol & 0xFF );
      config.lineRight = htons( (vol>>8) & 0xFF );

      this->Read(SOUND_MIXER_MIC,vol);
      config.micLeft = htons( vol & 0xFF );
      config.micRight = htons( (vol>>8) & 0xFF );

      this->Read(SOUND_MIXER_IGAIN,vol);
      config.iGain = htons( vol );

      this->Read(SOUND_MIXER_OGAIN,vol);
      config.oGain= htons( vol );

      if( PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, &config,
            sizeof(config)) != 0)
        PLAYER_ERROR("PutReply() failed");
    }

    // Get the next command
    memset(&cmdBuffer,0,sizeof(cmdBuffer));
    len = this->GetCommand(&cmdBuffer,sizeof(cmdBuffer));

    // Process the command
    switch(cmdBuffer[0])
    {
      case PLAYER_AUDIOMIXER_SET_MASTER:
        memcpy(&mixerCmd,cmdBuffer,sizeof(mixerCmd));
        vol = ( ntohs(mixerCmd.left)&0xFF) | ( ntohs(mixerCmd.right) << 8);
        this->Write( SOUND_MIXER_VOLUME, vol );
        break;

      case PLAYER_AUDIOMIXER_SET_PCM:
        memcpy(&mixerCmd,cmdBuffer,sizeof(mixerCmd));
        vol = ( ntohs(mixerCmd.left)&0xFF) | ( ntohs(mixerCmd.right) << 8);
        this->Write( SOUND_MIXER_PCM, vol );
        break;

      case PLAYER_AUDIOMIXER_SET_LINE:
        memcpy(&mixerCmd,cmdBuffer,sizeof(mixerCmd));
        vol = ( ntohs(mixerCmd.left)&0xFF) | ( ntohs(mixerCmd.right) << 8);
        this->Write( SOUND_MIXER_LINE, vol );
        break;

      case PLAYER_AUDIOMIXER_SET_MIC:
        memcpy(&mixerCmd,cmdBuffer,sizeof(mixerCmd));
        vol = ( ntohs(mixerCmd.left)&0xFF) | ( ntohs(mixerCmd.right) << 8);
        this->Write( SOUND_MIXER_MIC, vol );
        break;

      case PLAYER_AUDIOMIXER_SET_IGAIN:
        memcpy(&mixerCmd,cmdBuffer,sizeof(mixerCmd));
        this->Write( SOUND_MIXER_IGAIN, ntohs(mixerCmd.left) );
        break;

      case PLAYER_AUDIOMIXER_SET_OGAIN:
        memcpy(&mixerCmd,cmdBuffer,sizeof(mixerCmd));
        this->Write( SOUND_MIXER_OGAIN, ntohs(mixerCmd.left) );
        break;

      default:
        break;
    }
  }
}

int Mixer::Write(int dev, int num)
{
  int result;

  if( (result = ioctl(mixerFD,MIXER_WRITE(dev), &num)) == -1 )
    PLAYER_ERROR("write failed");

  return result;
}

int Mixer::Read(int dev, int& num)
{
  int result;

  if( (result = ioctl(mixerFD,MIXER_READ(dev), &num)) == -1 )
    PLAYER_ERROR("read failed");

  return result;
}
