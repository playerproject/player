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
 * LinuxWiFi driver.  Reads the wireless info found in /proc/net/wireless.
 * sort of ad hoc right now, as I've only tested on our own orinoco
 * cards.  
 *
 * 
 * 
 */

#include <ctype.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <linux/wireless.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/if_ether.h>
#include <configfile.h>
#include <device.h>
#include <drivertable.h>
#include <player.h>
#include <playertime.h>

extern PlayerTime *GlobalTime;

#define WIFI_INFO_FILE "/proc/net/wireless"

#define WIFI_UPDATE_INTERVAL 1000 // in milliseconds
class LinuxWiFi : public CDevice
{
public:
  LinuxWiFi(char *interface, ConfigFile *cf, int section);

  //  virtual void Main();

  ~LinuxWiFi();

  virtual size_t GetData(void*,unsigned char *, size_t maxsize,
			 uint32_t *timestamp_sec,
			 uint32_t *timestamp_usec);

  virtual int PutConfig(player_device_id_t *device, void *client,
			   void *data, size_t len);

  virtual int Setup();
  virtual int Shutdown();

protected:
  char * PrintEther(char *buf, unsigned char *addr);
  char * GetMACAddress(char *buf, int len);
protected:
  FILE *info_fp; // pointer to wifi info file
  fpos_t start_pos; // position of relevant info in info file

  int sfd; //socket to kernel
  char interface_name[32]; // name of wireless device

  struct iwreq *req; // used for getting wireless info in ioctls
  struct iw_range *range; 
  struct iw_statistics *stats;

  bool has_range;

  struct timeval last_update;
  int update_interval;

  player_wifi_data_t data;
};

CDevice * LinuxWiFi_Init(char *interface, ConfigFile *cf, int section);
void LinuxWiFi_Register(DriverTable *table);

/* check for supported interfaces.
 *
 * returns: pointer to new LinuxWiFi driver if supported, NULL else
 */
CDevice *
LinuxWiFi_Init(char *interface, ConfigFile *cf, int section)
{ 
  if(strcmp(interface, PLAYER_WIFI_STRING)) {
    PLAYER_ERROR1("driver \"linuxwifi\" does not support interface \"%s\"\n",
		  interface);
    return NULL;
  } else {
    return ((CDevice*)(new LinuxWiFi(interface, cf, section)));
  }
}

/* register with drivertable
 *
 * returns: 
 */
void
LinuxWiFi_Register(DriverTable *table)
{
  table->AddDriver("linuxwifi", PLAYER_READ_MODE, LinuxWiFi_Init);
}

LinuxWiFi::LinuxWiFi(char *interface, ConfigFile *cf, int section) :
  CDevice(0,0,0,1) 
{
  info_fp = NULL;
  
  sfd = -1;
  req = new struct iwreq;
  range = new struct iw_range;
  stats = new struct iw_statistics;

  has_range = false;

  last_update.tv_sec =0;
  last_update.tv_usec = 0;
  
  update_interval = cf->ReadInt(section, "interval", WIFI_UPDATE_INTERVAL);
}

LinuxWiFi::~LinuxWiFi() 
{
  delete req;
  delete range;
  delete stats;
}

int
LinuxWiFi::Setup()
{
  //  printf("LinuxWiFi: Wireless extensions %d\n", WIRELESS_EXT);
  
  // get the wireless device from /proc/net/wireless
  if ((this->info_fp = fopen(WIFI_INFO_FILE, "r")) == NULL) {
    fprintf(stderr, "LinuxWiFi: couldn't open info file \"%s\"\n",
	    WIFI_INFO_FILE);
    return -1;
  } 
  GlobalTime->GetTime(&last_update);

  // lets read it to the point we are interested in
  char buf[128];
  // read 3 lines
  for (int i =0; i < 3; i++) {
    if (i == 2) {
      fgetpos(info_fp, &start_pos);
    }

    if (fgets(buf, sizeof(buf),this->info_fp) == NULL) {
      fprintf(stderr, "LinuxWiFi: couldn't read line from info file\n");
      fclose(this->info_fp);
      return -1;
    }
  }

  // now we are at line of interest
  int eth, status; 
  double link, level, noise;
  sscanf(buf, "  eth%d: %d %lf %lf %lf", &eth, &status,
	 &link, &level, &noise);
  
  // buf has info about wireless interface
  char *name = strchr(buf, ':');
  if (name) {
    *name = '\0';
  } else {
    // no wireless interface
    fprintf(stderr, "LinuxWiFi: no wireless interface\n");
    return -1;
  }

  char *p = buf;
  while (isspace(*p)) {
    p++;
  }
  strncpy(interface_name, p, sizeof(interface_name));

  // copy this name into the iwreq struct for use in ioctls
  strncpy(req->ifr_name, interface_name, IFNAMSIZ);

  // this is the socket to use for wireless info
  sfd = socket(AF_INET, SOCK_DGRAM, 0);

  if (sfd < 0) {
    // didn't get a socket
    return -1;
  }
  
  char *reqbuf = new char[sizeof(reqbuf) *2];
  memset(reqbuf, 0, sizeof(reqbuf));
  
  // set the data part of the request
  req->u.data.pointer = (caddr_t) reqbuf;
  req->u.data.length = sizeof(reqbuf);
  req->u.data.flags = 0;
  
  delete [] reqbuf;

  // get range info... get it here because we set a flag on 
  // how it returns, so we know how to update
  if (ioctl(sfd, SIOCGIWRANGE, req) >= 0) {
    has_range = true;
    memcpy((char *) range, reqbuf, sizeof(struct iw_range));
  }

  return 0;
}  

int
LinuxWiFi::Shutdown()
{
  fclose(this->info_fp);
  
  close(sfd);
  return 0;
}

/* main loop.  just read info from file, update data, then sleep
 *
 * returns: 
 */
size_t
LinuxWiFi::GetData(void* client,unsigned char *dest, size_t maxsize, 
                   uint32_t *timestamp_sec, uint32_t *timestamp_usec)
{
  int eth, status;
  int link, level, noise;
  uint16_t wqual=0, wlevel=0, wnoise=0;
  uint16_t  wmaxqual=0, wmaxlevel=0, wmaxnoise=0;
  uint8_t qual_type;
  uint32_t throughput=0;
  int32_t bitrate =0; 
  uint8_t mode = 0;
  
  struct timeval curr;

  GlobalTime->GetTime(&curr);

  // check whether we should update...
  if (((curr.tv_sec - last_update.tv_sec)*1000 +
       (curr.tv_usec - last_update.tv_usec)/1000) < update_interval) {
    // just copy old data
    assert(sizeof(data) < maxsize);
    memcpy(dest, &data, sizeof(data));
    *timestamp_sec = curr.tv_sec;
    *timestamp_usec = curr.tv_usec;
    
    return sizeof(data);
  }

  last_update.tv_sec = curr.tv_sec;
  last_update.tv_usec = curr.tv_usec;

  // get new data

  // get the stats using an ioctl

  req->u.data.pointer = (caddr_t) stats;
  req->u.data.length = 0;
  req->u.data.flags = 1;  // clear updated flag
  if (ioctl(sfd, SIOCGIWSTATS, req) < 0) {
    printf("LINUXWIFI: couldn't ioctl stats!\n");
    // failed to get stats, try from /proc/net/wireless
    // Dummy rewind; this is a hack to force the kernel/stdlib to
    // re-read the file.
    rewind(this->info_fp);
    
    // get the wifi info
    if (fsetpos(this->info_fp, &this->start_pos)) {
      fprintf(stderr, "LinuxWiFi: fsetpos returned error\n");
    }
    
    fscanf(this->info_fp, "  eth%d: %d %d. %d. %d.", &eth, &status,
	   &link, &level, &noise);
    
    wqual = (uint16_t) link;
    wlevel = (uint16_t) level;
    wnoise = (uint16_t) noise;

    qual_type = PLAYER_WIFI_QUAL_UNKNOWN;
  } else {
    struct iw_quality *qual = &stats->qual;
    if (has_range) {
      throughput = range->throughput;
      if (qual->level != 0) {
	
	if (qual->level > range->max_qual.level) {
	  // we have dbm info
	  qual_type = PLAYER_WIFI_QUAL_DBM;
	} else {
	  qual_type = PLAYER_WIFI_QUAL_REL;
	}
      } 
    }else {
      qual_type = PLAYER_WIFI_QUAL_UNKNOWN;
    }

    wqual = qual->qual;
    wmaxqual = range->max_qual.qual;
    wlevel = qual->level;
    wmaxlevel = range->max_qual.level;
    wnoise = qual->noise;
    wmaxnoise = range->max_qual.noise;
  }

  // get operation mode
  mode = PLAYER_WIFI_MODE_UNKNOWN;
  if (ioctl(sfd, SIOCGIWMODE, req) >= 0) {
    // change it into a LINUX WIFI independent value
    switch(req->u.mode) {
    case IW_MODE_AUTO:
      mode = PLAYER_WIFI_MODE_AUTO;
      break;
    case IW_MODE_ADHOC:
      mode = PLAYER_WIFI_MODE_ADHOC;
      break;
    case IW_MODE_INFRA:
      mode = PLAYER_WIFI_MODE_INFRA;
      break;
    case IW_MODE_MASTER:
      mode = PLAYER_WIFI_MODE_MASTER;
      break;
    case IW_MODE_REPEAT:
      mode = PLAYER_WIFI_MODE_REPEAT;
      break;
    case IW_MODE_SECOND:
      mode = PLAYER_WIFI_MODE_SECOND;
      break;
    default:
      mode = PLAYER_WIFI_MODE_UNKNOWN;
    }
  }

  // set interface data
  data.throughput = htonl(throughput);
  data.mode = mode;
  
  // get AP address
  if (ioctl(sfd, SIOCGIWAP, req) >= 0) {
    // got it...
    struct sockaddr sa;
    memcpy(&sa, &(req->u.ap_addr), sizeof(sa));
    
    PrintEther((char *)data.ap, (unsigned char *)sa.sa_data);
  } else {
    strncpy(data.ap, "00:00:00:00:00:00", sizeof(data.ap));
  }
  
  // get bitrate
  bitrate = 0;
  if (ioctl(sfd, SIOCGIWRATE, req) >= 0) {
    bitrate = req->u.bitrate.value;
  }

  data.bitrate = htonl(bitrate);
    
  data.link_count = htonl(1);
  strncpy(data.links[0].ip, "0.0.0.0", sizeof(data.links[0].ip));
  data.links[0].qual = htons(wqual);
  data.links[0].maxqual = htons(wmaxqual);
  data.links[0].level = htons(wlevel);
  data.links[0].maxlevel = htons(wmaxlevel);
  data.links[0].noise = htons(wnoise);
  data.links[0].maxnoise = htons(wmaxnoise);
 
  data.links[0].qual_type = qual_type;
  
  assert(sizeof(data) < maxsize);
  memcpy(dest, &data, sizeof(data));

  GlobalTime->GetTime(&curr);
  *timestamp_sec = curr.tv_sec;
  *timestamp_usec = curr.tv_usec;
    
  return (sizeof(data));
}

int
LinuxWiFi::PutConfig(player_device_id_t *device, void *client,
		     void *data, size_t len)
{
  char * config = (char *)data;
  uint8_t which = config[0];
  char buf[32];

  switch(which) {
  case PLAYER_WIFI_MAC_REQ:
    GetMACAddress(buf, sizeof(buf));
    break;
  default:
    printf("LINUXWIFI: got other REQ\n");
  }

  if (PutReply(client, PLAYER_MSGTYPE_RESP_ACK, NULL, 
	       buf, sizeof(buf))) {
    PLAYER_ERROR("LinuxWiFi: failed to put reply");
    return -1;
  }

  return 0;
}

/* Taken from iwlib.c in wireless tools which in turn claims to be a
 * cut & paste from net-tools-1.2.0 */
char *
LinuxWiFi::GetMACAddress(char *buf, int len)
{
  struct ifreq ifr;
  
  /* Get the type of hardware address */
  strncpy(ifr.ifr_name, interface_name, IFNAMSIZ);
  if ((ioctl(sfd, SIOCGIFHWADDR, &ifr) < 0) ||
      (ifr.ifr_hwaddr.sa_family != ARPHRD_ETHER)) {
    /* Deep trouble... */
    fprintf(stderr, "LinuxWiFi: Interface %s doesn't support MAC addresses\n", 
	    interface_name);
    *buf='\0';
    return buf;
  }
  PrintEther(buf, (unsigned char *)ifr.ifr_hwaddr.sa_data);

  return buf;
}

/* write the ethernet address in standard hex format
 * writes it into given buffer.
 * taken from iwlib.c (wireless_tools)
 *
 * returns: pointer to buffer given as arg
 */
char *
LinuxWiFi::PrintEther(char *buf, unsigned char *data)
{
  struct ether_addr * p = (struct ether_addr *)data;
  sprintf(buf, "%02X:%02X:%02X:%02X:%02X:%02X",
	  p->ether_addr_octet[0], p->ether_addr_octet[1],
	  p->ether_addr_octet[2], p->ether_addr_octet[3],
	  p->ether_addr_octet[4], p->ether_addr_octet[5]);

  return buf;
}

  
