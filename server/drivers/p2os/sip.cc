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
#include <values.h>
#include <math.h>  /* rint(3) */
#include <sys/types.h>
#include <netinet/in.h>
#include <sip.h>
#include <stdlib.h> /* for abs() */
#include <unistd.h>

void SIP::Fill(player_p2os_data_t* data,  struct timeval timeBegan_tv) 
{
  //struct timeval timeNow_tv;
  //unsigned int timeNow;
  
  //gettimeofday(&timeNow_tv, NULL );
  //timeNow = 
    //(unsigned int) ((timeNow_tv.tv_sec - timeBegan_tv.tv_sec ) * 1000 + 
		    //( timeNow_tv.tv_usec - timeBegan_tv.tv_usec ) / 1000.0);
  
  /* time and position */
  //data->position.time = htonl((unsigned int)timeNow);
  data->position.xpos = htonl((int32_t)xpos); 
  data->position.ypos = htonl((int32_t)ypos); 
  data->position.yaw = htonl((int32_t)angle);
  data->position.xspeed = htonl((int32_t) (((lvel) + (rvel) ) / 2));
  data->position.yawspeed = htonl((int32_t)
     (180*((double)(rvel - lvel) /
           (2.0/PlayerRobotParams[param_idx].DiffConvFactor)) / 
           M_PI));
  // TODO: where does compass go?
  //data->position.compass = htons(compass);
  data->position.stall = (unsigned char)(lwstall || rwstall);

  data->sonar.range_count = htons(PlayerRobotParams[param_idx].SonarNum);
  for(int i=0;i<min(PlayerRobotParams[param_idx].SonarNum,ARRAYSIZE(sonars));i++)
    data->sonar.ranges[i] = htons((unsigned short)sonars[i]);

  data->gripper.state = (unsigned char)(timer >> 8);
  data->gripper.beams = (unsigned char)digin;

  data->bumper.bumper_count = 10;
  int j = 0;
  for(int i=4;i>=0;i--)
    data->bumper.bumpers[j++] = (unsigned char)((frontbumpers >> i) & 0x01);
  for(int i=4;i>=0;i--)
    data->bumper.bumpers[j++] = (unsigned char)((rearbumpers >> i) & 0x01);
  data->power.charge = htons((unsigned short)battery);
  data->dio.count = (unsigned char)8;
  data->dio.digin = htonl((unsigned int)digin);
  //TODO: should do this smarter, based on which analog input is selected
  data->aio.count = (unsigned char)1;
  data->aio.anin[0] = (unsigned char)analog;
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
  
  if (xpos!=MAXINT) {
    change = (int) rint(PositionChange( rawxpos, newxpos ) * 
			PlayerRobotParams[param_idx].DistConvFactor);
    if (abs(change)>100)
      puts("Change is not valid");
    else
      xpos += change;
  }
  else 
    xpos = 0;
  rawxpos = newxpos;
  cnt += sizeof(short);

  newypos = ((buffer[cnt] | (buffer[cnt+1] << 8))
	     & 0xEFFF) % 4096; /* 15 ls-bits */

  if (ypos!=MAXINT) {
    change = (int) rint(PositionChange( rawypos, newypos ) *
			PlayerRobotParams[param_idx].DistConvFactor);
    if (abs(change)>100)
      puts("Change is not valid");
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

  //printf("%hu:", sonarreadings);
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
