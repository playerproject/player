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
 * the C++ client
 */

#include <string.h>
#include <playerclient.h>
#include <unistd.h> // for close(2)
#include <netinet/in.h> // for ntohs(3)
#include <sys/time.h> // for ntohs(3)


PlayerClient::PlayerClient()
{
  DeviceDataEntry* thisentry;
  
  // make sure we don't use an uninitialized socket
  conn.sock = -1;
  
  // defaults
  port = PLAYER_PORTNUM;
  strcpy(host,"localhost");

  devicedatatable = new DeviceDataTable();

  // initialize the processed data to reasonable values
  minfrontsonar = 5000;
  minbacksonar = 5000;
  minlaser = 8000;
  minlaser_index = 181;

  // set up the 'normal' devices
  
  // the zeroth position device
  devicedatatable->AddDevice(PLAYER_POSITION_CODE, 0, 'c',
                             POSITION_DATA_BUFFER_SIZE,
                             POSITION_COMMAND_BUFFER_SIZE);
  thisentry = devicedatatable->GetDeviceEntry(PLAYER_POSITION_CODE,0);
  position = (player_position_data_t*)(thisentry->data);
  newspeed = &(((player_position_cmd_t*)(thisentry->command))->speed);
  newturnrate = &(((player_position_cmd_t*)(thisentry->command))->turnrate);
  
  // the zeroth laser device
  devicedatatable->AddDevice(PLAYER_LASER_CODE, 0, 'c',
                             LASER_DATA_BUFFER_SIZE,
                             LASER_COMMAND_BUFFER_SIZE);
  thisentry = devicedatatable->GetDeviceEntry(PLAYER_LASER_CODE,0);
  laser = ((player_laser_data_t*)(thisentry->data))->ranges;

  // the zeroth sonar device
  devicedatatable->AddDevice(PLAYER_SONAR_CODE, 0, 'c',
                             SONAR_DATA_BUFFER_SIZE,
                             SONAR_COMMAND_BUFFER_SIZE);
  thisentry = devicedatatable->GetDeviceEntry(PLAYER_SONAR_CODE,0);
  sonar = ((player_sonar_data_t*)(thisentry->data))->ranges;
  
  // the zeroth PTZ device
  devicedatatable->AddDevice(PLAYER_PTZ_CODE, 0, 'c',
                             PTZ_DATA_BUFFER_SIZE,
                             PTZ_COMMAND_BUFFER_SIZE);
  thisentry = devicedatatable->GetDeviceEntry(PLAYER_PTZ_CODE,0);
  ptz = (player_ptz_data_t*)(thisentry->data);
  newpan = &(((player_ptz_cmd_t*)(thisentry->command))->pan);
  newtilt = &(((player_ptz_cmd_t*)(thisentry->command))->tilt);
  newzoom = &(((player_ptz_cmd_t*)(thisentry->command))->zoom);
 
  // the zeroth misc device
  devicedatatable->AddDevice(PLAYER_MISC_CODE, 0, 'c',
                             MISC_DATA_BUFFER_SIZE,
                             MISC_COMMAND_BUFFER_SIZE);
  thisentry = devicedatatable->GetDeviceEntry(PLAYER_MISC_CODE,0);
  misc = (player_misc_data_t*)(thisentry->data);
  
  // the zeroth vision device
  devicedatatable->AddDevice(PLAYER_VISION_CODE, 0, 'c',
                             ACTS_DATA_BUFFER_SIZE,
                             ACTS_COMMAND_BUFFER_SIZE);
  thisentry = devicedatatable->GetDeviceEntry(PLAYER_VISION_CODE,0);
  vision = (vision_data*)(thisentry->data);

  // the zeroth laserbeacon device
  devicedatatable->AddDevice(PLAYER_LASERBEACON_CODE, 0, 'c',
                             LASERBEACON_DATA_BUFFER_SIZE,
                             LASERBEACON_COMMAND_BUFFER_SIZE);
  thisentry = devicedatatable->GetDeviceEntry(PLAYER_LASERBEACON_CODE,0);
  laserbeacon = (player_laserbeacon_data_t*)(thisentry->data);

  // the zeroth broadcast device
  devicedatatable->AddDevice(PLAYER_BROADCAST_CODE, 0, 'c',
                             BROADCAST_DATA_BUFFER_SIZE,
                             BROADCAST_COMMAND_BUFFER_SIZE);
  thisentry = devicedatatable->GetDeviceEntry(PLAYER_BROADCAST_CODE,0);
  broadcast_data = (player_broadcast_data_t*)(thisentry->data);
  broadcast_cmd = (player_broadcast_cmd_t*)(thisentry->command);
  broadcast_msg_count = 0;
}

PlayerClient::~PlayerClient()
{
  if(conn.sock != -1)
    close(conn.sock);
  delete devicedatatable;
}


/* Connect to Player server */
int PlayerClient::Connect(const char* host, int port)
{
  if(conn.sock != -1)
    close(conn.sock);
  return(player_connect(&conn, host, port));
}

/* Connect using 'this.port' */
int PlayerClient::Connect(const char* host)
{
  return(Connect(host, port));
}

/* Connect using 'this.host' */
int PlayerClient::Connect()
{
  return(Connect(host, port));
}

/* Disconnect from Player server */
int PlayerClient::Disconnect()
{
  return(player_disconnect(&conn));
}

/* request access to a single device */
int PlayerClient::RequestDeviceAccess(uint16_t device, uint16_t index, 
                                       uint8_t access)
{
  if(player_request_device_access(&conn, device, index, access) == -1)
    return(-1);

  return(devicedatatable->UpdateAccess(device, index, access));

  return(0);
}

/* query the current device access */
uint8_t PlayerClient::QueryDeviceAccess(uint16_t device, uint16_t index)
{
  return devicedatatable->GetDeviceAccess(device, index);  
}


// byte-swap incoming data as necessary
void PlayerClient::ByteSwapData(void* data, player_msghdr_t hdr)
{
  // see if we know how to byte-swap it
  switch(hdr.device)
  {
    case PLAYER_LASER_CODE:
      player_laser_data_t* templaser;
      templaser = (player_laser_data_t*)data;
      minlaser = 8000;
      for(int j=0;j<PLAYER_NUM_LASER_SAMPLES;j++)
      {
        templaser->ranges[j] = ntohs(templaser->ranges[j]);
        if(templaser->ranges[j] < minlaser)
        {
          minlaser = templaser->ranges[j];
          minlaser_index = j;
        }
      }
      break;
    case PLAYER_SONAR_CODE:
      player_sonar_data_t* tempsonar;
      tempsonar = (player_sonar_data_t*)data;
      minfrontsonar = 5000;
      minbacksonar = 5000;
      for(int j=0;j<PLAYER_NUM_SONAR_SAMPLES;j++)
      {
        tempsonar->ranges[j] = ntohs(tempsonar->ranges[j]);
        if((j > 1) && (j < 6) && (tempsonar->ranges[j] < minfrontsonar))
          minfrontsonar = tempsonar->ranges[j];
        else if((j > 9) && (j < 14) && (tempsonar->ranges[j] < minbacksonar))
          minbacksonar = tempsonar->ranges[j];
      }
      break;
    case PLAYER_POSITION_CODE:
      player_position_data_t* tempposition;
      tempposition = (player_position_data_t*)data;
      tempposition->xpos = ntohl(tempposition->xpos);
      tempposition->ypos = ntohl(tempposition->ypos);
      tempposition->theta = ntohs(tempposition->theta);
      tempposition->speed = ntohs(tempposition->speed);
      tempposition->turnrate = ntohs(tempposition->turnrate);
      tempposition->compass = ntohs(tempposition->compass);
      break;
    case PLAYER_PTZ_CODE:
      player_ptz_data_t* tempptz;
      tempptz = (player_ptz_data_t*)data;
      tempptz->pan = ntohs(tempptz->pan);
      tempptz->tilt = ntohs(tempptz->tilt);
      tempptz->zoom = ntohs(tempptz->zoom);
      break;
    case PLAYER_MISC_CODE:
      // nothing to swap here
      break;
    case PLAYER_LASERBEACON_CODE:
    {
        player_laserbeacon_data_t* temp;
        temp = (player_laserbeacon_data_t*) data;
        temp->count = ntohs(temp->count);
        for (int i = 0; i < temp->count; i++)
        {
            temp->beacon[i].range = ntohs(temp->beacon[i].range);
            temp->beacon[i].bearing = ntohs(temp->beacon[i].bearing);
            temp->beacon[i].orient = ntohs(temp->beacon[i].orient);
        }
        break;
    }
    default:
      //don't know it.  oh well.
      break;
  }
}

// byte-swap outgoing commands as necessary
void PlayerClient::ByteSwapCommands(void* cmd, uint16_t device)
{
  switch(device)
  {
    case PLAYER_POSITION_CODE:
      player_position_cmd_t* tempposition;
      tempposition = (player_position_cmd_t*)cmd;
      tempposition->speed = htons(tempposition->speed);
      tempposition->turnrate = htons(tempposition->turnrate);
      break;
    case PLAYER_PTZ_CODE:
      player_ptz_cmd_t* tempptz;
      tempptz = (player_ptz_cmd_t*)cmd;
      tempptz->pan = htons(tempptz->pan);
      tempptz->tilt = htons(tempptz->tilt);
      tempptz->zoom = htons(tempptz->zoom);
      break;
    default:
      // no byte-swapping to be done.  oh well.
      break;
  }
}

/* read from all currently open devices */
int PlayerClient::Read()
{
  DeviceDataEntry* thisentry;
  int numtoread = 0;
  player_msghdr_t hdr;
  char buffer[PLAYER_MAX_MESSAGE_SIZE];
  struct timeval curr;

  // count up how many devices from which we should expect data
  for(thisentry = devicedatatable->head; thisentry; thisentry = thisentry->next)
  {
    if((thisentry->access == PLAYER_READ_MODE) || 
       (thisentry->access == PLAYER_ALL_MODE))
      numtoread++;
  }

  // read from each device
  for(int i = 0; i<numtoread; i++)
  {
    if(player_read(&conn, &hdr, buffer, sizeof(buffer)))
      return(-1);
    gettimeofday(&curr,NULL);

    if(!(thisentry = devicedatatable->GetDeviceEntry(hdr.device, 
                                                     hdr.device_index)))
    {
      // couldn't find a place to put the data.  skip it.
      continue;
    }

    // put the data in its place
    FillData(thisentry->data,buffer, hdr);
    
    // fill in the timestamp
    thisentry->timestamp = hdr.timestamp;
    thisentry->senttime = hdr.time;
    thisentry->rectime = (uint64_t)((((uint64_t)curr.tv_sec) * 1000000) + 
                                      (uint64_t)curr.tv_usec);

    // byte-swap it
    ByteSwapData(thisentry->data,hdr);
  }
  return(0);
}


/* print current data (for debug) */
void PlayerClient::Print()
{
  DeviceDataEntry* thisentry;

  // see which devices are open for reading
  for(thisentry = devicedatatable->head; thisentry; thisentry = thisentry->next)
  {
    if((thisentry->access == PLAYER_READ_MODE) || 
       (thisentry->access == PLAYER_ALL_MODE))
    {
      switch(thisentry->device)
      {
        case PLAYER_LASER_CODE:
          printf("Laser %d data (timestamp:%Lu):\n", 
                          thisentry->index,thisentry->timestamp);
          for(int i=0;i<PLAYER_NUM_LASER_SAMPLES;i++)
            printf("  laser(%d):%d\n", i, 
                         ((player_laser_data_t*)(thisentry->data))->ranges[i]);
          break;
        case PLAYER_SONAR_CODE:
          printf("Sonar %d data (timestamp:%Lu):\n", 
                          thisentry->index, thisentry->timestamp);
          for(int i=0;i<PLAYER_NUM_SONAR_SAMPLES;i++)
            printf("  sonar(%d):%d\n", i, 
                         ((player_sonar_data_t*)(thisentry->data))->ranges[i]);
          break;
        case PLAYER_POSITION_CODE:
          printf("Position %d data (timestamp:%Lu:%Lu:%Lu):\n", 
                          thisentry->index,thisentry->timestamp,
                          thisentry->senttime, thisentry->rectime);
          printf("  (x,y,theta) : (%d,%d,%d)\n",
                    ((player_position_data_t*)(thisentry->data))->xpos,
                    ((player_position_data_t*)(thisentry->data))->ypos,
                    ((player_position_data_t*)(thisentry->data))->theta);
          printf("  speed:%d\tturnrate:%d\n",
                    ((player_position_data_t*)(thisentry->data))->speed,
                    ((player_position_data_t*)(thisentry->data))->turnrate);
          printf("  compass:%d\tstalls:%d\n",
                    ((player_position_data_t*)(thisentry->data))->compass,
                    ((player_position_data_t*)(thisentry->data))->stalls);
          break;
        case PLAYER_PTZ_CODE:
          printf("PTZ %d data (timestamp:%Lu):\n", 
                          thisentry->index, thisentry->timestamp);
          printf("  pan:%d\ttilt:%d\tzoom:%d\n",
                    ((player_ptz_data_t*)(thisentry->data))->pan,
                    ((player_ptz_data_t*)(thisentry->data))->tilt,
                    ((player_ptz_data_t*)(thisentry->data))->zoom);
          break;
        case PLAYER_MISC_CODE:
          printf("Misc %d data (timestamp:%Lu):\n", 
                          thisentry->index, thisentry->timestamp);
          printf("  frontbumpers:%d\trearbumpers:%d\tvoltage:%d\n",
                    ((player_misc_data_t*)(thisentry->data))->frontbumpers,
                    ((player_misc_data_t*)(thisentry->data))->rearbumpers,
                    ((player_misc_data_t*)(thisentry->data))->voltage);
          break;
        case PLAYER_VISION_CODE:
          printf("Vision %d data (timestamp:%Lu):\n", 
                          thisentry->index, thisentry->timestamp);
          for(int i=0;i<ACTS_NUM_CHANNELS;i++)
          {
            if(((vision_data*)(thisentry->data))->NumBlobs[i])
            {
              printf("Channel %d:\n", i);
              for(int j=0;j<((vision_data*)(thisentry->data))->NumBlobs[i];j++)
              {
                printf("  blob %d:\n"
                       "             area: %d\n"
                       "                X: %d\n"
                       "                Y: %d\n"
                       "             Left: %d\n"
                       "            Right: %d\n"
                       "              Top: %d\n"
                       "           Bottom: %d\n",
                       j+1,
                       ((vision_data*)(thisentry->data))->Blobs[i][j].area,
                       ((vision_data*)(thisentry->data))->Blobs[i][j].x,
                       ((vision_data*)(thisentry->data))->Blobs[i][j].y,
                       ((vision_data*)(thisentry->data))->Blobs[i][j].left,
                       ((vision_data*)(thisentry->data))->Blobs[i][j].right,
                       ((vision_data*)(thisentry->data))->Blobs[i][j].top,
                       ((vision_data*)(thisentry->data))->Blobs[i][j].bottom);
              }
            }
          }
          break;
        default:
          printf("don't know how print data from device %x\n", 
                          thisentry->device);
      }
    }
  }
}

// this method is used to possible transform incoming data
void PlayerClient::FillData(void* dest, void* src, player_msghdr_t hdr)
{
  switch(hdr.device)
  {
    case PLAYER_VISION_CODE:
      // fill the special vision buffer.
      vision_data* tempvision;
      int i,j,k;
      int area;
      int bufptr;
      char* buf;
      
      buf = (char*)src;

      tempvision = (vision_data*)dest;
      bufptr = ACTS_HEADER_SIZE;
      for(i=0;i<ACTS_NUM_CHANNELS;i++)
      {
        //printf("%d blobs starting at %d on %d\n", buf[2*i+1]-1,buf[2*i]-1,i+1);
        if(!(buf[2*i+1]-1))
        {
          tempvision->NumBlobs[i] = 0;
        }
        else
        {
          /* check to see if we need more room */
          if(buf[2*i+1]-1 > tempvision->NumBlobs[i])
          {
            //assert(tempvision != NULL);
            delete tempvision->Blobs[i];
            tempvision->Blobs[i] = new blob_data[buf[2*i+1]-1];
          }
          for(j=0;j<buf[2*i+1]-1;j++)
          {
            /* first compute the area */
            area=0;
            for(k=0;k<4;k++)
            {
              area = area << 6;
              area |= buf[bufptr + k] - 1;
            }
            tempvision->Blobs[i][j].area = area;
            tempvision->Blobs[i][j].x = buf[bufptr + 4] - 1;
            tempvision->Blobs[i][j].y = buf[bufptr + 5] - 1;
            tempvision->Blobs[i][j].left = buf[bufptr + 6] - 1;
            tempvision->Blobs[i][j].right = buf[bufptr + 7] - 1;
            tempvision->Blobs[i][j].top = buf[bufptr + 8] - 1;
            tempvision->Blobs[i][j].bottom = buf[bufptr + 9] - 1;

            bufptr += ACTS_BLOB_SIZE;
          }
          tempvision->NumBlobs[i] = buf[2*i+1]-1;
        }
      }
      break;
    case PLAYER_BROADCAST_CODE:
    {
        // Copy the raw packet
        memcpy(dest,src,hdr.size);
        // Extract separate messages from packet
        player_broadcast_data_t* data = (player_broadcast_data_t*) dest;
        broadcast_msg_count = 0;
        int offset = 0;
        while (true)
        {
            uint8_t *msg = data->buffer + offset;
            int len = ntohs(*(uint16_t*) msg);
            if (len == 0)
                break;
            offset += 2 + len;
            broadcast_msg[broadcast_msg_count++] = (player_broadcast_cmd_t*) msg;
            if (broadcast_msg_count >= ARRAYSIZE(broadcast_msg))
                break;
        }
        break;
    }
    default:
      // no transformations to be done; just copy
      ASSERT(hdr.size < PLAYER_MAX_MESSAGE_SIZE);
      memcpy(dest,src,hdr.size);
      break;
  }
}

// this method is used to possible transform outgoing commands
int PlayerClient::FillCommand(void* dest, void* src, uint16_t device)
{
  int datasize,commandsize;

  devicedatatable->GetDeviceSizes(device, &datasize, &commandsize);
  ASSERT(commandsize <= PLAYER_MAX_MESSAGE_SIZE);
  
  switch(device)
  {
    case PLAYER_BROADCAST_CODE:
    {
      // Broadcast commands have a variable length
      player_broadcast_cmd_t* tempsrc = (player_broadcast_cmd_t*) src;
      commandsize = tempsrc->len + 2;
      memcpy(dest, src, commandsize);
      // Byte swap
      player_broadcast_cmd_t* tempdest = (player_broadcast_cmd_t*) dest;
      tempdest->len = htons(tempdest->len);
      // Now reset the source message so it doesnt get sent again
      memset(src, 0, commandsize);
      break;
    }
    default:
      // no transformations to be done; just copy
      memcpy(dest, src, commandsize);
      break;
  }
  return commandsize;
}

/* write one device's commands to server */
int PlayerClient::Write(uint16_t device, uint16_t index)
{
  char buffer[PLAYER_MAX_MESSAGE_SIZE];

  DeviceDataEntry* thisentry;

  if(!(thisentry = devicedatatable->GetDeviceEntry(device,index)))
    return(-1);

  int commandsize = FillCommand(buffer, thisentry->command, device);
  ByteSwapCommands(buffer, device);

  return(player_write(&conn, device, index, buffer, commandsize));
}


/* write ALL commands to server */
int PlayerClient::Write()
{
  DeviceDataEntry* thisentry;
  
  // see which devices we should write
  for(thisentry = devicedatatable->head; thisentry; thisentry = thisentry->next)
  {
    if((thisentry->access == PLAYER_WRITE_MODE) || 
       (thisentry->access == PLAYER_ALL_MODE))
    {
      if(Write(thisentry->device,thisentry->index) == -1)
        return(-1);
    }
  }
  return(0);
}


/* change velocity control mode.
 *   'mode' should be either DIRECTWHEELVELOCITY or SEPARATETRANSROT
 *
 *   default is DIRECTWHEELVELOCITY, which should be faster, if a bit
 *      jerky
 */
int PlayerClient::ChangeVelocityControl(velocity_mode_t mode)
{
  char payload[2]; // space for 'v' and single-byte arg
  player_msghdr_t replyhdr;
  char replybuffer[PLAYER_MAX_MESSAGE_SIZE];

  payload[0] = 'v';
  if(mode == DIRECTWHEELVELOCITY)
    payload[1] = 0;
  else if(mode == SEPARATETRANSROT)
    payload[1] = 1;
  else
  {
    puts("PlayerClient::ChangeVelocityControl(): received unknown velocity"
                    "mode as argument");
    return(-1);
  }
  return((player_request(&conn, PLAYER_POSITION_CODE, 0,
                          payload, sizeof(payload),
                          &replyhdr, replybuffer, sizeof(replybuffer))));
}

/*
 * Set the laser configuration
 * Use min_ and max_segment to specify a restricted scan.
 * Set intensity to true to get intensity data in the top three bits
 * of the range scan data.
 *
 * Returns 0 on success; non-zero otherwise
 */
int PlayerClient::SetLaserConfig(int min_segment, int max_segment, 
                bool intensity)
{
  // space for two indices, and a byte for intensity
  char payload[2*sizeof(uint16_t)+sizeof(uint8_t)];

  uint16_t min_segment_swap = htons(min_segment);
  uint16_t max_segment_swap = htons(max_segment);

  player_msghdr_t replyhdr;
  char replybuffer[PLAYER_MAX_MESSAGE_SIZE];

  memcpy(payload, &min_segment_swap, sizeof(uint16_t));
  memcpy(payload+sizeof(uint16_t), &max_segment_swap, sizeof(uint16_t));
  memcpy(payload+2*sizeof(uint16_t), &intensity, sizeof(uint8_t));

  return(player_request(&conn, PLAYER_LASER_CODE, 0,
                          payload, sizeof(payload),
                          &replyhdr, replybuffer, sizeof(replybuffer)));
}

/*
 * Enable/disable the motors
 *   if 'state' is non-zero, then enable motors
 *   else disable motors
 *
 * Returns 0 on success; non-zero otherwise
 */
int PlayerClient::ChangeMotorState(unsigned char state)
{
  // space for 'm' and a single-byte arg
  char payload[2];

  player_msghdr_t replyhdr;
  char replybuffer[PLAYER_MAX_MESSAGE_SIZE];

  payload[0] = 'm';
  payload[1] = state;
  return(player_request(&conn, PLAYER_POSITION_CODE, 0,
                          payload, sizeof(payload),
                          &replyhdr, replybuffer, sizeof(replybuffer)));

}

/*
 * Change the update frequency at which this client receives data
 *
 * Returns 0 on success; non-zero otherwise
 */
int PlayerClient::SetFrequency(unsigned short freq)
{
  player_device_ioctl_t hdr;
  player_device_datafreq_req_t payload;
  char buffer[sizeof(hdr)+sizeof(payload)];

  player_msghdr_t replyhdr;
  char replybuffer[PLAYER_MAX_MESSAGE_SIZE];

  hdr.subtype = htons(PLAYER_PLAYER_DATAFREQ_REQ);
  payload.frequency = htons(freq);
  memcpy(buffer,&hdr,sizeof(hdr));
  memcpy(buffer+sizeof(hdr),&payload,sizeof(payload));
  return(player_request(&conn, PLAYER_PLAYER_CODE, 0,
                          buffer, sizeof(buffer),
                          &replyhdr, replybuffer, sizeof(replybuffer)));

}


// Set the broadcast message for this player
//
size_t PlayerClient::SetBroadcastMsg(const void *msg, size_t len)
{
    if (len > sizeof(broadcast_cmd->msg))
        return 0;
    
    broadcast_cmd->len = len;
    memcpy(broadcast_cmd->msg, msg, len);
    return len;
}


// Get the n'th broadcast message that was received by the last read
//
size_t PlayerClient::GetBroadcastMsg(int n, void *msg, size_t maxlen)
{
    if (n < 0 || n >= broadcast_msg_count)
        return 0;
    size_t len = min(broadcast_msg[n]->len, maxlen);
    memcpy(msg, broadcast_msg[n]->msg, len);
    return len;
}
