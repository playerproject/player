/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */
/********************************************************************
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 ********************************************************************/

/***************************************************************************
 * Desc: Cooperating Object proxy
 * Author: Adrian Jimenez Gonzalez
 * Date: 15 Aug 2011
 **************************************************************************/

#include "playerc++.h"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <climits>

using namespace PlayerCc;

CoopObjectProxy::CoopObjectProxy( PlayerClient *aPc, uint32_t aIndex )
	: ClientProxy(aPc, aIndex),
  mDevice(NULL)
{
    Subscribe(aIndex);
  // how can I get this into the clientproxy.cc?
  // right now, we're dependent on knowing its device type
    mInfo = &(mDevice->info);
	id = aIndex;
	
}

CoopObjectProxy::~CoopObjectProxy()
{
    Unsubscribe();
}

void
CoopObjectProxy::Subscribe(uint32_t aIndex)
{
    scoped_lock_t lock(mPc->mMutex);
    mDevice = playerc_coopobject_create(mClient, aIndex);
    if (NULL==mDevice)
	throw PlayerError("CoopObjectProxy::CoopObjectProxy()", "could not create");

    if (0 != playerc_coopobject_subscribe(mDevice, PLAYER_OPEN_MODE))
	throw PlayerError("CoopObjectProxy::CoopObjectProxy()", "could not subscribe");
}

void
CoopObjectProxy::Unsubscribe()
{
    assert(NULL!=mDevice);
    scoped_lock_t lock(mPc->mMutex);
    playerc_coopobject_unsubscribe(mDevice);
    playerc_coopobject_destroy(mDevice);
    mDevice = NULL;
}

std::ostream& std::operator << (std::ostream &os, const PlayerCc::CoopObjectProxy &c)
{
	os << "#CoopObject(" << c.GetInterface() << ":" << c.GetIndex() << ")" << std::endl;
	switch ( c.MessageType() ) {
 		case PLAYER_COOPOBJECT_MSG_NONE:
 			os << "No new message available" << std::endl;	 
 			break;
		case PLAYER_COOPOBJECT_MSG_HEALTH:						
		{
			os << "HEALTH message:" << std::endl;
			os << " - CoopObject ID: " << c.GetID() << std::endl;
			os << " - Parent ID: " << c.GetParentID() << std::endl;
			switch ( c.GetOrigin() ){
				case 0:
					os << " - Origin: Gateway"<< std::endl;
					break;
				case 1:
					os << " - Origin: Gateway (Mobile CoopObject)"<< std::endl;
					break;
				case 2:
					os << " - Origin: Static CoopObject"<< std::endl;
					break;
				case 3:
					os << " - Origin: Mobile CoopObject"<< std::endl;
					break;
				default:
					os << " - Origin: Unknown"<< std::endl;
					break;
			}

			break;
		}
		case PLAYER_COOPOBJECT_MSG_RSSI:
		{								
			os << "RSSI DATA message:" << std::endl;
			os << " - CoopObject ID: " << c.GetID() << std::endl;
			switch ( c.GetOrigin() ){
				case 0:
					os << " - Origin: Gateway"<< std::endl;
					break;
				case 1:
					os << " - Origin: Gateway (Mobile CoopObject)"<< std::endl;
					break;
				case 2:
					os << " - Origin: Static CoopObject"<< std::endl;
					break;
				case 3:
					os << " - Origin: Mobile CoopObject"<< std::endl;
					break;
				default:
					os << " - Origin: Unknown"<< std::endl;
					break;
			}
			os << " - RSSI data:" << std::endl;
			os << "\t· Sender ID: " << c.GetRSSIsenderId() << std::endl;
			os << "\t· RSSI: " << c.GetRSSIvalue() << std::endl;
			os << "\t· Stamp: " << c.GetRSSIstamp() << std::endl;
			os << "\t· Node time: " << c.GetRSSInodeTime() << std::endl;
			break;
		}
		case PLAYER_COOPOBJECT_MSG_POSITION:
		{								
			os << "POSITION DATA message:" << std::endl;
			os << " - CoopObject ID: " << c.GetID() << std::endl;
			os << " - Parent ID: " << c.GetParentID() << std::endl;
			switch ( c.GetOrigin() ){
				case 0:
					os << " - Origin: Gateway"<< std::endl;
					break;
				case 1:
					os << " - Origin: Gateway (Mobile CoopObject)"<< std::endl;
					break;
				case 2:
					os << " - Origin: Static CoopObject"<< std::endl;
					break;
				case 3:
					os << " - Origin: Mobile CoopObject"<< std::endl;
					break;
				default:
					os << " - Origin: Unknown"<< std::endl;
					break;
			}
			os << " - Position (x,y,z): (" << c.GetX() << "," << c.GetY() << "," << c.GetZ() << ")" << std::endl;
			os << " - State: 0x" << hex <<static_cast<uint32_t>(c.GetStatus()) << std::endl;
			os.setf(ios::dec, ios::basefield);
			break;
		}
		case PLAYER_COOPOBJECT_MSG_SENSOR:
		{
			os << "SENSOR DATA message:" << std::endl;
			os << " - CoopObject ID: " << c.GetID() << std::endl;
			os << " - Parent ID: " << c.GetParentID() << std::endl;
			switch ( c.GetOrigin() ){
				case 0:
					os << " - Origin: Gateway"<< std::endl;
					break;
				case 1:
					os << " - Origin: Gateway (Mobile CoopObject)"<< std::endl;
					break;
				case 2:
					os << " - Origin: Static CoopObject"<< std::endl;
					break;
				case 3:
					os << " - Origin: Mobile CoopObject"<< std::endl;
					break;
				default:
					os << " - Origin: Unknown"<< std::endl;
					break;
			}
			uint32_t sensorNumber = c.GetSensorNumber();
			if (sensorNumber > 0) {
				os << " - Node SENSOR data:" << std::endl;
				for (uint32_t i = 0; i < sensorNumber; i++){
						os << "\t· sensor[" << i << "] of type " << static_cast<uint32_t>(c.GetSensorType(i)) << ": " << c.GetSensorData(i) << std::endl;
				}	 
			} else os << "No sensor data available." << std::endl;
			break;
		}
		case PLAYER_COOPOBJECT_MSG_ALARM:
		{
			os << "ALARM message:" << std::endl;
			os << " - CoopObject ID: " << c.GetID() << std::endl;
			os << " - Parent ID: " << c.GetParentID() << std::endl;
			switch ( c.GetOrigin() ){
				case 0:
					os << " - Origin: Gateway"<< std::endl;
					break;
				case 1:
					os << " - Origin: Gateway (Mobile CoopObject)"<< std::endl;
					break;
				case 2:
					os << " - Origin: Static CoopObject"<< std::endl;
					break;
				case 3:
					os << " - Origin: Mobile CoopObject"<< std::endl;
					break;
				default:
					os << " - Origin: Unknown"<< std::endl;
					break;
			}
			uint32_t alarmNumber = c.GetAlarmNumber();
			if (alarmNumber >0) {
				os << " - Node ALARM data:" << std::endl;
				for (uint32_t i = 0; i < alarmNumber; i++){
						os << "\t· alarm[" << i << "] of type " << static_cast<uint32_t>(c.GetAlarmType(i)) << ": " << c.GetAlarmData(i) << std::endl;
				}
			} else os << "No alarm data available." << std::endl;
			break;
		}
		case PLAYER_COOPOBJECT_MSG_REQUEST:
		{
			os << "REQUEST message:" << std::endl;
			os << " - CoopObject ID: " << c.GetID() << std::endl;
			os << " - Parent ID: " << c.GetParentID() << std::endl;
			switch ( c.GetOrigin() ){
				case 0:
					os << " - Origin: Gateway"<< std::endl;
					break;
				case 1:
					os << " - Origin: Gateway (Mobile CoopObject)"<< std::endl;
					break;
				case 2:
					os << " - Origin: Static CoopObject"<< std::endl;
					break;
				case 3:
					os << " - Origin: Mobile CoopObject"<< std::endl;
					break;
				default:
					os << " - Origin: Unknown"<< std::endl;
					break;
			}
			os << " - REQUEST:  " << c.GetRequest() << std::endl;
			int parametersSize = c.GetParametersSize();
			uint8_t *parameters = c.GetAllParameters();

			if (parameters != NULL && parametersSize > 0) {
				os << "\t· Parameters: [";
				for (int i = 0; i < parametersSize; i++)
					os << " 0x" << hex << static_cast<uint32_t>(parameters[i]);
				os << " ]" << std::endl;
				os.setf(ios::dec, ios::basefield);
			}
			break;
		}
		case PLAYER_COOPOBJECT_MSG_COMMAND:
		{
			os << "COMMAND message:" << std::endl;
			os << " - CoopObject ID: " << c.GetID() << std::endl;
			os << " - Parent ID: " << c.GetParentID() << std::endl;
			switch ( c.GetOrigin() ){
				case 0:
					os << " - Origin: Gateway"<< std::endl;
					break;
				case 1:
					os << " - Origin: Gateway (Mobile CoopObject)"<< std::endl;
					break;
				case 2:
					os << " - Origin: Static CoopObject"<< std::endl;
					break;
				case 3:
					os << " - Origin: Mobile CoopObject"<< std::endl;
					break;
				default:
					os << " - Origin: Unknown"<< std::endl;
					break;
			}
			os << " - COMMAND : " << c.GetCommand() << std::endl;
			int parametersSize = c.GetParametersSize();
			uint8_t *parameters = c.GetAllParameters();

			if (parameters != NULL && parametersSize > 0) {
				os << "\t· Parameters: [";
				for (int i = 0; i < parametersSize; i++)
					os << " 0x" << hex << static_cast<uint32_t>(parameters[i]);
				os << " ]" << std::endl;
				os.setf(ios::dec, ios::basefield);
			}
			break;
		}
		default:
		{
			os << "USER DATA message: " << std::endl;
			os << " - CoopObject ID: " << c.GetID() << std::endl;
			os << " - Parent ID: " << c.GetParentID() << std::endl;
			switch ( c.GetOrigin() ){
				case 0:
					os << " - Origin: Gateway"<< std::endl;
					break;
				case 1:
					os << " - Origin: Gateway (Mobile CoopObject)"<< std::endl;
					break;
				case 2:
					os << " - Origin: Static CoopObject"<< std::endl;
					break;
				case 3:
					os << " - Origin: Mobile CoopObject"<< std::endl;
					break;
				default:
					os << " - Origin: Unknown"<< std::endl;
					break;
			}
			uint32_t userDataNumber = c.GetUserDataNumber();
			uint8_t *userData = c.GetAllUserData();
			if (userData != NULL && userDataNumber > 0) {
				os << " - USER data:" <<  std::endl;
				os << "\t· Type: " << c.MessageType() << std::endl;
				os << "\t· Data: [";
				for (uint32_t i = 0; i < userDataNumber; i++)
					os << " 0x" << hex << static_cast<uint32_t>(userData[i]);
				os << " ]" << std::endl;
				os.setf(ios::dec, ios::basefield);
			} else os << "No user data available." << std::endl;
			break;
		}
	}
    return os;
}

void
CoopObjectProxy::SendData(int nodeID, int sourceID, player_pose2d_t pos, int status)
{
 //  scoped_lock_t lock(mPc->mMutex);
  playerc_coopobject_send_position(mDevice, nodeID, GetProxyID(), pos, status);
}

void
CoopObjectProxy::SendData(int nodeID, int sourceID, int extradata_type, uint32_t extradata_size, unsigned char *extradata)
{
//  scoped_lock_t lock(mPc->mMutex);
  playerc_coopobject_send_data(mDevice, nodeID, GetProxyID(), extradata_type, extradata_size, extradata);

}

void
CoopObjectProxy::SendCommand(int nodeID, int sourceID, int command, uint32_t cmd_parameters_size, unsigned char *cmd_parameters)
{
//  scoped_lock_t lock(mPc->mMutex);
  playerc_coopobject_send_cmd(mDevice, nodeID, GetProxyID(), command, cmd_parameters_size, cmd_parameters);
}

void
CoopObjectProxy::SendRequest(int nodeID, int sourceID, int request, uint32_t req_parameters_size, unsigned char *req_parameters)
{
//  scoped_lock_t lock(mPc->mMutex);
  playerc_coopobject_send_req(mDevice, nodeID, GetProxyID(), request, req_parameters_size, req_parameters);
}
