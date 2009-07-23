/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2006
 *     Raymond Sheh, Luke Gumbley
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

#include <linux/types.h>
#include <linux/videodev2.h>
#include <string.h>

class UvcInterface;

#ifndef UVCINTERFACE_H_
#define UVCINTERFACE_H_

class UvcInterface
{
	public:
		UvcInterface(char const *sDevice,int aWidth=320,int aHeight=240):device(sDevice),frame(0),frameSize(0),fd(-1),width(aWidth),height(aHeight){buffer[0]=0;buffer[1]=0;}
		~UvcInterface(void) {device=0;Close();}
		
		int Open(void);
		int Close(void);
		int Read(void);
		
		int GetWidth(void) const;
		int GetHeight(void) const;
		
		int GetFrameSize(void) const {return frameSize;}
		void CopyFrame(unsigned char *dest) const {memcpy(dest,frame,frameSize);}

		bool IsOpen(void) const {return fd!=-1;}
		
	private:
		char const *device;

		unsigned char *frame;
		int frameSize;
		
		unsigned char *buffer[2];
		int length[2];
		
		int fd;
		
		v4l2_capability cap;
		v4l2_format fmt;
		
		static const int dht_size;
		static const unsigned char dht_data[];
		
		int width;
		int height;
};

#endif /*UVCINTERFACE_H_*/
