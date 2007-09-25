/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2007
 *     Brian Gerkey
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
 *  Copyright (C) 2007
 *     Brian Gerkey
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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <errno.h>

//#include <sys/poll.h>

#include <dns_sd.h>

#include <libplayercore/error.h>
#include <libplayercore/interface_util.h>

#include "playersd.h"

#define PLAYER_SD_MDNS_DEVS_LEN_INITIAL 4
#define PLAYER_SD_MDNS_DEVS_LEN_MULTIPLIER 2

// Info for one registered device
typedef struct
{
  // Is this entry valid?
  uint8_t valid;
  // Did registration fail?
  uint8_t fail;
  // Identifying information, as provided by user when registering
  player_sd_device_t sdDev;
  // Index appended to sdDev.name to make it unique
  int nameIdx;
  // Session reference used for registration.  We deallocate to terminate
  DNSServiceRef regRef;
  // TXT reference
  TXTRecordRef txtRecord;
  // Local storage for TXT record
  uint8_t txtBuf[PLAYER_SD_TXT_MAXLEN];
} player_sd_mdns_dev_t;

// We'll use this as the opaque reference that the user is given
typedef struct
{
  // Session reference for browsing
  DNSServiceRef browseRef;
  // Is browse reference valid?
  uint8_t browseRef_valid;
  // List of registered devices
  player_sd_mdns_dev_t* mdnsDevs;
  // Length of list
  size_t mdnsDevs_len;
} player_sd_mdns_t;

static void registerCB(DNSServiceRef sdRef, 
                       DNSServiceFlags flags, 
                       DNSServiceErrorType errorCode, 
                       const char *name, 
                       const char *regtype, 
                       const char *domain, 
                       void *context);  


player_sd_t* 
player_sd_init(void)
{
  player_sd_t* sd;
  player_sd_mdns_t* mdns;

  sd = (player_sd_t*)malloc(sizeof(player_sd_t));
  assert(sd);
  mdns = (player_sd_mdns_t*)malloc(sizeof(player_sd_mdns_t));
  assert(mdns);
  mdns->browseRef_valid = 0;
  mdns->mdnsDevs = NULL;
  mdns->mdnsDevs_len = 0;
  sd->sdRef = mdns;

  return(sd);
}

void
player_sd_fini(player_sd_t* sd)
{
  int i;
  player_sd_mdns_t* mdns = (player_sd_mdns_t*)(sd->sdRef);

  if(mdns->browseRef_valid)
  {
    DNSServiceRefDeallocate(mdns->browseRef);
    mdns->browseRef_valid = 0;
  }

  for(i=0;i<mdns->mdnsDevs_len;i++)
  {
    if(mdns->mdnsDevs[i].valid)
    {
      DNSServiceRefDeallocate(mdns->mdnsDevs[i].regRef);
      mdns->mdnsDevs[i].valid = 0;
    }
  }
  
  if(mdns->mdnsDevs)
    free(mdns->mdnsDevs);
  free(mdns);
  free(sd);
}

int 
player_sd_register(player_sd_t* sd, 
                   const char* name, 
                   player_devaddr_t addr)
{
  DNSServiceErrorType sdErr;
  char recordval[PLAYER_SD_TXT_MAXLEN];
  int i,j;
  player_sd_mdns_t* mdns = (player_sd_mdns_t*)(sd->sdRef);
  player_sd_mdns_dev_t* dev;
  char nameBuf[PLAYER_SD_NAME_MAXLEN];

  // Find a spot for this device
  for(i=0;i<mdns->mdnsDevs_len;i++)
  {
    if(!mdns->mdnsDevs[i].valid)
      break;
  }
  if(i==mdns->mdnsDevs_len)
  {
    // Make the list bigger
    if(!mdns->mdnsDevs_len)
      mdns->mdnsDevs_len = PLAYER_SD_MDNS_DEVS_LEN_INITIAL;
    else
      mdns->mdnsDevs_len *= PLAYER_SD_MDNS_DEVS_LEN_MULTIPLIER;
    mdns->mdnsDevs = 
            (player_sd_mdns_dev_t*)realloc(mdns->mdnsDevs,
                                           (mdns->mdnsDevs_len * 
                                            sizeof(player_sd_mdns_dev_t)));
    assert(mdns->mdnsDevs);
    for(j=i;j<mdns->mdnsDevs_len;j++)
      mdns->mdnsDevs[j].valid = 0;
  }

  dev = mdns->mdnsDevs + i;
  dev->fail = 0;
  memset(dev->sdDev.name,0,sizeof(dev->sdDev.name));
  strncpy(dev->sdDev.name,name,sizeof(dev->sdDev.name)-1);
  dev->sdDev.addr = addr;
  dev->nameIdx = 1;

  TXTRecordCreate(&(dev->txtRecord),sizeof(dev->txtBuf),dev->txtBuf);
  memset(recordval,0,sizeof(recordval));
  snprintf(recordval, sizeof(recordval), "%s:%d",
           interf_to_str(addr.interf), addr.index);
  if((sdErr = TXTRecordSetValue(&(dev->txtRecord),
                                "device",
                                strlen(recordval),
                                recordval)))
  {
    PLAYER_ERROR1("TXTRecordSetValue returned error: %d", sdErr);
    return(-1);
  }

  memset(nameBuf,0,sizeof(nameBuf));
  strncpy(nameBuf,name,sizeof(nameBuf)-1);
  sdErr = kDNSServiceErr_NameConflict;

  // Avahi can return the kDNSServiceErr_NameConflict immediately.
  while(sdErr == kDNSServiceErr_NameConflict)
  {
    sdErr = DNSServiceRegister(&(dev->regRef), 
                               0,
                               0,
                               nameBuf,
                               PLAYER_SD_SERVICENAME,
                               NULL,
                               NULL,
                               addr.robot,
                               TXTRecordGetLength(&(dev->txtRecord)),
                               TXTRecordGetBytesPtr(&(dev->txtRecord)),
                               registerCB,
                               (void*)dev);

    if(sdErr == kDNSServiceErr_NameConflict)
    {
      // Pick a new name
      memset(nameBuf,0,sizeof(nameBuf));
      snprintf(nameBuf,sizeof(nameBuf),"%s (%d)",
               name,dev->nameIdx++);
    }
  }

  if(sdErr != kDNSServiceErr_NoError)
  {
    PLAYER_ERROR1("DNSServiceRegister returned error: %d", sdErr);
    return(-1);
  }
  else
  {
    dev->valid = 1;
    if(strcmp(nameBuf,name))
      PLAYER_WARN2("Changing service name of %s to %s\n",
                   name,nameBuf);
    PLAYER_MSG1(2,"Registration of %s successful", name);
    return(0);
  }
}

void 
registerCB(DNSServiceRef sdRef, 
           DNSServiceFlags flags, 
           DNSServiceErrorType errorCode, 
           const char *name, 
           const char *regtype, 
           const char *domain, 
           void *context)
{
  // Don't need to do anything here
}
