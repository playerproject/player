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
 * part of the P2OS parser.  methods for filling and parsing server
 * information packets (SIPs)
 */
#include <stdio.h>
#include <limits.h>
#include <math.h>  /* rint(3) */
#include <sys/types.h>
#include <netinet/in.h>
#include <sip.h>
#include <error.h>
#include <stdlib.h> /* for abs() */
#include <unistd.h>

void SIP::Fill(player_p2os_data_t* data)
{
  // initialize position to current offset
  data->position.xpos = this->x_offset;
  data->position.ypos = this->y_offset;
  // now transform current position by rotation if there is one
  // and add to offset
  if(this->angle_offset != 0) 
  {
    double rot = DTOR(this->angle_offset);    // convert rotation to radians
    data->position.xpos += (int32_t) (this->xpos * cos(rot) - 
                                      this->ypos * sin(rot));
    data->position.ypos += (int32_t) (this->xpos * sin(rot) + 
                                      this->ypos * cos(rot));
    data->position.yaw = (this->angle_offset + angle) % 360;
  }
  else 
  {
    data->position.xpos += this->xpos;
    data->position.ypos += this->ypos;
    data->position.yaw = this->angle;
  }
  // now byteswap fields
  data->position.xpos = htonl(data->position.xpos);
  data->position.ypos = htonl(data->position.ypos);
  data->position.yaw = htonl(data->position.yaw);
  data->position.xspeed = htonl((int32_t) (((this->lvel) + (this->rvel) ) / 2));
  data->position.yawspeed = htonl((int32_t)
                                  (180*((double)(this->rvel - this->lvel) /
                                        (2.0/PlayerRobotParams[param_idx].DiffConvFactor)) / 
                                   M_PI));
  data->position.stall = (unsigned char)(this->lwstall || this->rwstall);

  // compass
  memset(&(data->compass),0,sizeof(data->compass));
  data->compass.yaw = htonl(this->compass);

  // gyro
  memset(&(data->gyro),0,sizeof(data->gyro));
  data->gyro.yawspeed = htonl(this->gyro_rate);

  data->sonar.range_count = htons(PlayerRobotParams[param_idx].SonarNum);
  for(int i=0;i<min(PlayerRobotParams[param_idx].SonarNum,ARRAYSIZE(sonars));i++)
    data->sonar.ranges[i] = htons((unsigned short)this->sonars[i]);

  data->gripper.state = (unsigned char)(this->timer >> 8);
  data->gripper.beams = (unsigned char)this->digin;

  data->bumper.bumper_count = 10;
  int j = 0;
  for(int i=4;i>=0;i--)
    data->bumper.bumpers[j++] = 
      (unsigned char)((this->frontbumpers >> i) & 0x01);
  for(int i=4;i>=0;i--)
    data->bumper.bumpers[j++] = 
      (unsigned char)((this->rearbumpers >> i) & 0x01);
  data->power.charge = htons((unsigned short)this->battery);
  data->dio.count = (unsigned char)8;
  data->dio.digin = htonl((unsigned int)this->digin);
  //TODO: should do this smarter, based on which analog input is selected
  data->aio.count = (unsigned char)1;
  data->aio.anin[0] = (unsigned char)this->analog;

  /* CMUcam blob tracking interface.  The CMUcam only supports one blob
  ** (and therefore one channel too), so everything else is zero.  All
  ** data is storde in the blobfinder packet in Network byte order.  
  ** Note: In CMUcam terminology, X is horizontal and Y is vertical, with
  ** (0,0) being TOP-LEFT (from the camera's perspective).  Also,
  ** since CMUcam doesn't have range information, but does have a
  ** confidence value, I'm passing it back as range. */
  memset(data->blobfinder.blobs,0,
         sizeof(player_blobfinder_blob_t)*PLAYER_BLOBFINDER_MAX_BLOBS);
  data->blobfinder.width = htons(CMUCAM_IMAGE_WIDTH);
  data->blobfinder.height = htons(CMUCAM_IMAGE_HEIGHT);

  if (blobarea > 1)	// With filtering, definition of track is 2 pixels
  {
    data->blobfinder.blob_count = htons(1);
    data->blobfinder.blobs[0].color = htonl(this->blobcolor);
    data->blobfinder.blobs[0].x = htons(this->blobmx);
    data->blobfinder.blobs[0].y = htons(this->blobmy);
    data->blobfinder.blobs[0].left = htons(this->blobx1);
    data->blobfinder.blobs[0].right = htons(this->blobx2);
    data->blobfinder.blobs[0].top = htons(this->bloby1);
    data->blobfinder.blobs[0].bottom = htons(this->bloby2);
    data->blobfinder.blobs[0].area = htonl(this->blobarea);
    data->blobfinder.blobs[0].range = htons(this->blobconf);
  }
  else
    data->blobfinder.blob_count = htons(0);
}

int SIP::PositionChange( unsigned short from, unsigned short to ) 
{
  int diff1, diff2;

  /* find difference in two directions and pick shortest */
  if ( to > from ) {
    diff1 = to - from;
    diff2 = - ( from + 4096 - to );
  }
  else {
    diff1 = to - from;
    diff2 = 4096 - from + to;
  }

  if ( abs(diff1) < abs(diff2) ) 
    return(diff1);
  else
    return(diff2);

}

void SIP::Print() 
{
  int i;

  printf("lwstall:%d rwstall:%d\n", lwstall, rwstall);

  printf("Front bumpers: ");
  for(int i=0;i<5;i++) {
    printf("%d", (frontbumpers >> i) & 0x01 );
  }
  puts("");

  printf("Rear bumpers: ");
  for(int i=0;i<5;i++) {
    printf("%d", (rearbumpers >> i) & 0x01 );
  }
  puts("");

  printf("status: 0x%x analog: %d ", status, analog);
  printf("digin: ");
  for(i=0;i<8;i++) {
    printf("%d", (digin >> 7-i ) & 0x01);
  }
  printf(" digout: ");
  for(i=0;i<8;i++) {
    printf("%d", (digout >> 7-i ) & 0x01);
  }
  puts("");
  printf("battery: %d compass: %d sonarreadings: %d\n", battery, compass, sonarreadings);
  printf("xpos: %d ypos:%d ptu:%hu timer:%hu\n", xpos, ypos, ptu, timer);
  printf("angle: %d lvel: %d rvel: %d control: %d\n", angle, lvel, rvel, control);
  
  PrintSonars();
}

void SIP::PrintSonars() 
{
  printf("Sonars: ");
  for(int i = 0; i < 16; i++){
    printf("%hu ", sonars[i]);
  } 
  puts("");
}

void SIP::Parse( unsigned char *buffer ) 
{
  int cnt = 0, change;
  unsigned short newxpos, newypos;

  status = buffer[cnt];
  cnt += sizeof(unsigned char);
  /*
   * Remember P2OS uses little endian: 
   * for a 2 byte short (called integer on P2OS)
   * byte0 is low byte, byte1 is high byte
   * The following code is host-machine endian independant
   * Also we must or (|) bytes together instead of casting to a
   * short * since on ARM architectures short * must be even byte aligned!
   * You can get away with this on a i386 since shorts * can be
   * odd byte aligned. But on ARM, the last bit of the pointer will be ignored!
   * The or'ing will work on either arch.
   */
  newxpos = ((buffer[cnt] | (buffer[cnt+1] << 8))
	     & 0xEFFF) % 4096; /* 15 ls-bits */
  
  if (xpos!=INT_MAX) {
    change = (int) rint(PositionChange( rawxpos, newxpos ) * 
			PlayerRobotParams[param_idx].DistConvFactor);
    if (abs(change)>100)
      PLAYER_WARN1("invalid odometry change [%d]; odometry values are tainted", change);
    else
      xpos += change;
  }
  else 
    xpos = 0;
  rawxpos = newxpos;
  cnt += sizeof(short);

  newypos = ((buffer[cnt] | (buffer[cnt+1] << 8))
	     & 0xEFFF) % 4096; /* 15 ls-bits */

  if (ypos!=INT_MAX) {
    change = (int) rint(PositionChange( rawypos, newypos ) *
			PlayerRobotParams[param_idx].DistConvFactor);
    if (abs(change)>100)
      PLAYER_WARN1("invalid odometry change [%d]; odometry values are tainted", change);
    else
      ypos += change;
  }
  else
    ypos = 0;
  rawypos = newypos;
  cnt += sizeof(short);

  angle = (short)
    rint(((short)(buffer[cnt] | (buffer[cnt+1] << 8))) *
	 PlayerRobotParams[param_idx].AngleConvFactor * 180.0/M_PI);
  cnt += sizeof(short);

  lvel = (short)
    rint(((short)(buffer[cnt] | (buffer[cnt+1] << 8))) *
	 PlayerRobotParams[param_idx].VelConvFactor);
  cnt += sizeof(short);

  rvel = (short)
    rint(((short)(buffer[cnt] | (buffer[cnt+1] << 8))) *
	 PlayerRobotParams[param_idx].VelConvFactor);
  cnt += sizeof(short);

  battery = buffer[cnt];
  cnt += sizeof(unsigned char);
  
  lwstall = buffer[cnt] & 0x01;
  rearbumpers = buffer[cnt] >> 1;
  cnt += sizeof(unsigned char);
  
  rwstall = buffer[cnt] & 0x01;
  frontbumpers = buffer[cnt] >> 1;
  cnt += sizeof(unsigned char);

  control = (short)
    rint(((short)(buffer[cnt] | (buffer[cnt+1] << 8))) *
	 PlayerRobotParams[param_idx].AngleConvFactor);
  cnt += sizeof(short);

  ptu = (buffer[cnt] | (buffer[cnt+1] << 8));
  cnt += sizeof(short);

  //compass = buffer[cnt]*2;
  if(buffer[cnt] != 255 && buffer[cnt] != 0 && buffer[cnt] != 181)
    compass = (buffer[cnt]-1)*2;
  cnt += sizeof(unsigned char);

  sonarreadings = buffer[cnt];
  cnt += sizeof(unsigned char);

  //printf("%hu sonar readings:\n", sonarreadings);
  for(unsigned char i = 0;i < sonarreadings;i++) {
    sonars[buffer[cnt]]=   (unsigned short)
      rint((buffer[cnt+1] | (buffer[cnt+2] << 8)) *
	   PlayerRobotParams[param_idx].RangeConvFactor);
    //printf("%d %hu:",buffer[cnt],*((unsigned short *)&buffer[cnt+1]));
    //     
    //printf("%hu %hu %hu\n", buffer[cnt], buffer[cnt+1], buffer[cnt+2]);
    //printf("index %d value %hu\n", buffer[cnt], sonars[buffer[cnt]]);
    cnt += 3*sizeof(unsigned char);
  }
  //printf("\n");

  timer = (buffer[cnt] | (buffer[cnt+1] << 8));
  cnt += sizeof(short);

  analog = buffer[cnt];
  cnt += sizeof(unsigned char);

  digin = buffer[cnt];
  cnt += sizeof(unsigned char);

  digout = buffer[cnt];
  cnt += sizeof(unsigned char);
  // for debugging:
  //Print();
}


/** Parse a SERAUX SIP packet.  For a CMUcam, this will have blob
 **  tracking messages in the format (all one-byte values, no spaces): 
 **
 **      255 M mx my x1 y1 x2 y2 pixels confidence  (10-bytes)
 **
 ** Or color info messages of the format:
 **
 **      255 S Rval Gval Bval Rvar Gvar Bvar    (8-bytes)
 */
void SIP::ParseSERAUX( unsigned char *buffer ) 
{
  unsigned char type = buffer[1];
  if (type != SERAUX && type != SERAUX2)
  {
    // Really should never get here...
    printf("ERROR: Attempt to parse non SERAUX packet as serial data.\n");
    return;
  }

  int len  = (int)buffer[0]-3;		// Get the string length

  /* First thing is to find the beginning of last full packet (if possible).
  ** If there are less than CMUCAM_MESSAGE_LEN*2-1 bytes (19), we're not
  ** guaranteed to have a full packet.  If less than CMUCAM_MESSAGE_LEN
  ** bytes, it is impossible to have a full packet.
  ** To find the beginning of the last full packet, search between bytes
  ** len-17 and len-8 (inclusive) for the start flag (255).
  */
  int ix;
  for (ix = (len>18 ? len-17 : 2); ix<=len-8; ix++)
    if (buffer[ix] == 255)
      break;		// Got it!
  if (len < 10 || ix > len-8) {
    printf("ERROR: Failed to get a full blob tracking packet.\n");
    return;
  }

  // There is a special 'S' message containing the tracking color info
  if (buffer[ix+1] == 'S')
  {
     // Color information (track color)
     printf("Tracking color (RGB):  %d %d %d\n"
            "       with variance:  %d %d %d\n",
              buffer[ix+2], buffer[ix+3], buffer[ix+4],
              buffer[ix+5], buffer[ix+6], buffer[ix+7]);
     blobcolor = buffer[ix+2]<<16 | buffer[ix+3]<<8 | buffer[ix+4];
     return;
  }

  // Tracking packets with centroid info are designated with a 'M'
  if (buffer[ix+1] == 'M')
  {
     // Now it's easy.  Just parse the packet.
     blobmx	= buffer[ix+2];
     blobmy	= buffer[ix+3];
     blobx1	= buffer[ix+4];
     bloby1	= buffer[ix+5];
     blobx2	= buffer[ix+6];
     bloby2	= buffer[ix+7];
     blobconf	= buffer[ix+9];
     // Xiaoquan Fu's calculation for blob area (max 11297).
     blobarea	= (bloby2 - bloby1 +1)*(blobx2 - blobx1 + 1)*blobconf/255;
     return;
  }

  printf("ERROR: Unknown blob tracker packet type: %c\n", buffer[ix+1]); 
  return;
}

// Parse a set of gyro measurements.  The buffer is formatted thusly:
//     length (2 bytes), type (1 byte), count (1 byte)
// followed by <count> pairs of the form:
//     rate (2 bytes), temp (1 byte)
// <rate> falls in [0,1023]; less than 512 is CCW rotation and greater
// than 512 is CW
void
SIP::ParseGyro(unsigned char* buffer)
{
  // Get the message length (account for the type byte and the 2-byte
  // checksum)
  int len  = (int)buffer[0]-3;

  unsigned char type = buffer[1];
  if(type != GYROPAC)
  {
    // Really should never get here...
    PLAYER_ERROR("ERROR: Attempt to parse non GYRO packet as gyro data.\n");
    return;
  }

  if(len < 1)
  {
    PLAYER_WARN("Couldn't get gyro measurement count");
    return;
  }

  // get count
  int count = (int)buffer[2];

  // sanity check
  if((len-1) != (count*3))
  {
    PLAYER_WARN("Mismatch between gyro measurement count and packet length");
    return;
  }

  // Actually handle the rate values.  Any number of things could (and
  // probably should) be done here, like filtering, calibration, conversion
  // from the gyro's arbitrary units to something meaningful, etc.
  //
  // As a first cut, we'll just average all the rate measurements in this 
  // set, and ignore the temperatures.
  float ratesum = 0;
  int bufferpos = 3;
  unsigned short rate;
  unsigned char temp;
  for(int i=0; i<count; i++)
  {
    rate = (unsigned short)(buffer[bufferpos++] | (buffer[bufferpos++] << 8));
    temp = bufferpos++;

    ratesum += rate;
  }

  int32_t average_rate = (int32_t)rint(ratesum / (float)count);

  // store the result for sending
  gyro_rate = average_rate;
}

