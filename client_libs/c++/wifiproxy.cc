/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
 *     John Sweeney, UMASS, Amherst, Laboratory for Perceptual Robotics
 *     Brian Gerkey & Andrew Howard
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
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     John Sweeney, UMASS, Amherst, Laboratory for Perceptual Robotics
 *     Brian Gerkey & Andrew Howard
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
 * $Id$
 * implementation of WiFi proxy class
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <math.h>

/* fills in all the data....
 *
 * returns:
 */
void
WiFiProxy::FillData(player_msghdr_t hdr, const char *buffer)
{
  if (hdr.size != sizeof(player_wifi_data_t)) {
    fprintf(stderr, "WIFIPROXY: expected %d but got %d\n",
	    sizeof(player_wifi_data_t), hdr.size);
  }

  player_wifi_data_t * d = (player_wifi_data_t *)buffer;

  throughput = ntohl(d->throughput);
  op_mode = d->mode;
  bitrate = ntohl(d->bitrate);

  // get access point/cell addr
  strncpy(access_point, d->ap, sizeof(access_point));
  
  link_count = ntohl(d->link_count);
  
  for (int i = 0; i < link_count; i++) {
    links[i].qual_type = d->links[i].qual_type;
    links[i].qual = ntohs(d->links[i].qual);
    links[i].level = ntohs(d->links[i].level);
    links[i].noise = ntohs(d->links[i].noise);
    links[i].maxqual = ntohs(d->links[i].maxqual);
    links[i].maxlevel = ntohs(d->links[i].maxlevel);
    links[i].maxnoise = ntohs(d->links[i].maxnoise);

    //    links[i].bitrate = ntohl(d->links[i].bitrate);

    memcpy(links[i].ip, d->links[i].ip, sizeof(links[i].ip));
  }
  
}

/* print it
 *
 * returns:
 */
void
WiFiProxy::Print()
{
  char mode[16];

  switch(op_mode) {
  case PLAYER_WIFI_MODE_AUTO:
    strcpy(mode, "AUTO");
    break;
  case PLAYER_WIFI_MODE_ADHOC:
    strcpy(mode, "ADHOC");
    break;
  case PLAYER_WIFI_MODE_MASTER:
    strcpy(mode, "MASTER");
    break;
  case PLAYER_WIFI_MODE_INFRA:
    strcpy(mode, "INFRA");
    break;
  case PLAYER_WIFI_MODE_REPEAT:
    strcpy(mode, "REPEAT");
    break;
  case PLAYER_WIFI_MODE_SECOND:
    strcpy(mode, "SECOND");
    break;
  default:
    sprintf(mode, "OTHER (%d)", op_mode);
  }

  printf("#WiFi(%d:%d) - %c\n", m_device_id.code, 
         m_device_id.index, access);

  printf("\tMode: %s\t%s\n", mode, access_point);
  printf("\tBitrate: %d\tThroughput: %d\n", bitrate, throughput);

  if (!link_count) {
    printf("\tNo link information\n");
  } else {
    for (int i =0; i < link_count; i++) {
      printf("\tIP: %s", links[i].ip);

      switch(links[i].qual_type) {
      case PLAYER_WIFI_QUAL_DBM:
	printf("\tquality: %d/%d\tlevel: %d dBm\tnoise: %d dBm\n", 
	       links[i].qual, links[i].maxqual,
	       links[i].level - 0x100, links[i].noise - 0x100);
	break;
      case PLAYER_WIFI_QUAL_REL:
	printf("\tquality: %d/%d\tlevel: %d/%d\tnoise: %d/%d\n",
	       links[i].qual, links[i].maxqual,
	       links[i].level, links[i].maxlevel,
	       links[i].noise, links[i].maxnoise);
	break;
      case PLAYER_WIFI_QUAL_UNKNOWN:
      default:
	printf("\tquality: %d\tlevel: %d\tnoise: %d\n",
	       links[i].qual, links[i].level, links[i].noise);
	break;
      }
    }
  }
}

/* given the ip address, find the link quality.  if ip address
 * is NULL, return the first link entry
 *
 * returns: the link quality for given IP
 */
int
WiFiProxy::GetLinkQuality(char *ip)
{
  int idx = 0;

  if (ip) {
    idx = GetLinkIndex(ip);
    if (idx < 0) {
      return 0;
    }
  }

  return links[idx].qual;
}

/* given the IP, return the signal level, or first entry
 * if IP is NULL
 *
 * returns: signal level to IP
 */
int
WiFiProxy::GetLevel(char *ip)
{
  int idx = 0;
  
  if (ip) {
    idx = GetLinkIndex(ip);
    if (idx < 0) {
      return 0;
    }
  }

  return links[idx].level;
}

/* 
 *
 * returns: noise level for given IP
 */
int
WiFiProxy::GetNoise(char *ip)
{
  int idx =0;
  if (ip) {
    idx =  GetLinkIndex(ip);
    if (idx < 0) {
      return 0;
    }
  }

  return links[idx].level;
}

/* 
 *
 * returns: bitrate for given IP
 */
int
WiFiProxy::GetBitrate()
{
  return bitrate;
}

/* given the IP address, find the corresponding link entry
 *
 * returns: index of link entry with given IP, -1 if not found
 */
int
WiFiProxy::GetLinkIndex(char *ip)
{
  for (int i=0; i < link_count; i++) {
    if (!strcmp(links[i].ip, ip)) {
      return i;
    }
  }

  return -1;
}

char *
WiFiProxy::GetMAC(char *mac, int len)
{
  char buf[32];

  player_wifi_mac_req_t req;
  player_msghdr_t hdr;

  req.subtype = PLAYER_WIFI_MAC_REQ;

  if (client->Request(m_device_id, (const char *)&req, sizeof(req),
		      &hdr, buf, sizeof(buf)) < 0) {
    *mac = '\0';
  } else {
    strncpy(mac, buf, len);
  }

  return mac;
}
    
  
  
