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

/* Copyright (C) 2002
 *   John Sweeney, UMASS, Amherst, Laboratory for Perceptual Robotics
 *
 * $Id$
 *
 * WiFi driver.  Reads the wireless info found in /proc/net/wireless.
 * sort of ad hoc right now, as I've only tested on our own orinoco
 * cards.  
 *
 * This will only update every WIFI_UPDATE_PERIOD milliseconds
 * 
 */

#include <stdio.h>
#include <pthread.h>
#include <netinet/in.h>
#include <string.h>
#include <device.h>
#include <configfile.h>
#include <playertime.h>
#include <drivertable.h>
#include <player.h>

extern PlayerTime *GlobalTime;

#define WIFI_UPDATE_PERIOD 2000
#define WIFI_INFO_FILE "/proc/net/wireless"

class WiFi : public CDevice
{
public:
  WiFi(char *interface, ConfigFile *cf, int section) :
    CDevice(0, 0, 0, 1) {}

  //  virtual void Main();

  virtual size_t GetData(void*,unsigned char *, size_t maxsize,
			 uint32_t *timestamp_sec,
			 uint32_t *timestamp_usec);

  virtual int Setup();
  virtual int Shutdown();

protected:
  FILE *info_fp; // pointer to wifi info file
  fpos_t start_pos; // position of relevant info in info file

};

CDevice * WiFi_Init(char *interface, ConfigFile *cf, int section);
void WiFi_Register(DriverTable *table);

/* check for supported interfaces.
 *
 * returns: pointer to new WiFi driver if supported, NULL else
 */
CDevice *
WiFi_Init(char *interface, ConfigFile *cf, int section)
{ 
  if(strcmp(interface, PLAYER_WIFI_STRING)) {
    PLAYER_ERROR1("driver \"wifi\" does not support interface \"%s\"\n",
		  interface);
    return NULL;
  } else {
    return ((CDevice*)(new WiFi(interface, cf, section)));
  }
}

/* register with drivertable
 *
 * returns: 
 */
void
WiFi_Register(DriverTable *table)
{
  table->AddDriver("wifi", PLAYER_READ_MODE, WiFi_Init);
}

int
WiFi::Setup()
{
  if ((this->info_fp = fopen(WIFI_INFO_FILE, "r")) == NULL) {
    fprintf(stderr, "WiFi: couldn't open info file \"%s\"\n",
	    WIFI_INFO_FILE);
    return -1;
  } 
  
  // lets read it to the point we are interested in
  char buf[128];
  // read 3 lines
  for (int i =0; i < 3; i++) {
    if (i == 2) {
      fgetpos(this->info_fp, &this->start_pos);
    }
    
    if (fgets(buf, sizeof(buf),this->info_fp) == NULL) {
      fprintf(stderr, "WiFi: couldn't read line from info file\n");
      fclose(this->info_fp);
      return -1;
    }
  }
  
  // now we are at line of interest
  int eth, status, link, level, noise;
  sscanf(buf, "  eth%d: %d %d %d %d", &eth, &status,
	 &link, &level, &noise);
  
  return 0;
}

int
WiFi::Shutdown()
{
  fclose(this->info_fp);

  return 0;
}

/* main loop.  just read info from file, update data, then sleep
 *
 * returns: 
 */
size_t
WiFi::GetData(void* client,unsigned char *dest, size_t maxsize, 
              uint32_t *timestamp_sec,
	      uint32_t *timestamp_usec)
{
  player_wifi_data_t data;
  int eth, status, link, level, noise;
  unsigned short wlink, wlevel, wnoise;

  struct timeval curr;
  
  // get the wifi info
  if (fsetpos(this->info_fp, &this->start_pos)) {
    fprintf(stderr, "WiFi: fsetpos returned error\n");
  }
  
  fscanf(this->info_fp, "  eth%d: %d %d %d %d", &eth, &status,
	 &link, &level, &noise);
  
  wlink = (unsigned short)link;
  wlevel = (unsigned short) level;
  wnoise = (unsigned short) noise;
  data.link = htons(wlink);
  data.level = htons(wlevel);
  data.noise = htons(wnoise);

  assert(sizeof(data) < maxsize);
  memcpy(dest, &data, sizeof(data));

  GlobalTime->GetTime(&curr);
  *timestamp_sec = curr.tv_sec;
  *timestamp_usec = curr.tv_usec;

  return (sizeof(data));
}
