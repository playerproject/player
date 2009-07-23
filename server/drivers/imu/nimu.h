/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2006
 *     Toby Collett
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

#include <usb.h>
#include <stdint.h>
#include <stdio.h>
#include <arpa/inet.h>


#define NIMU_VENDORID 0x10c4
#define NIMU_PRODUCTID 0xEA61

#define NIMU_DATA_SIZE 38

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;



class nimu_data
{
	public:
		uint8_t DeviceID;
		uint8_t MessageID;
		uint16_t SampleTimer;
		short GyroX;
		short GyroY;
		short GyroZ;
		short AccelX;
		short AccelY;
		short AccelZ;
		short MagX;
		short MagY;
		short MagZ;
		short GyroTempX;
		short GyroTempY;
		short GyroTempZ;

		void Print() {
			printf("%04X %04X %04X,%04X %04X %04X\n",GyroX,GyroY,GyroZ,AccelX,AccelY,AccelZ);
		}
};

class nimu
{
	public:
		nimu();
		~nimu();

		int Open();
		int Close();

		nimu_data GetData();

	private:
		usb_dev_handle * nimu_dev;

};

