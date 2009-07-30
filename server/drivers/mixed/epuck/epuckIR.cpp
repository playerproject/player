/* Copyright 2008 Renato Florentino Garcia <fgar.renato@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "epuckIR.hpp"

EpuckIR::EpuckIR(const SerialPort* const serialPort)
  :EpuckInterface(serialPort)
{
  Triple sensorPose;
  sensorPose.x     =  0.033436777;
  sensorPose.y     = -0.010343207;
  sensorPose.theta = -0.29999999996640769;
  this->geometry.push_back(sensorPose);

  sensorPose.x     =  0.022530689693073834;
  sensorPose.y     = -0.026783726812271973;
  sensorPose.theta = -0.87142857131499862;
  this->geometry.push_back(sensorPose);

  sensorPose.x     =  0;
  sensorPose.y     = -0.035;
  sensorPose.theta = -1.5707963267948966;
  this->geometry.push_back(sensorPose);

  sensorPose.x     = -0.029706926397973173;
  sensorPose.y     = -0.018506715645554311;
  sensorPose.theta = -2.5844497965195483;
  this->geometry.push_back(sensorPose);

  sensorPose.x     = -0.029706926397973173;
  sensorPose.y     = 0.018506715645554311;
  sensorPose.theta = 2.5844497965195483;
  this->geometry.push_back(sensorPose);

  sensorPose.x     = 0;
  sensorPose.y     = 0.035;
  sensorPose.theta = 1.5707963267948966;
  this->geometry.push_back(sensorPose);

  sensorPose.x     = 0.022530689693073834;
  sensorPose.y     = 0.026783726812271973;
  sensorPose.theta = 0.87142857131499862;
  this->geometry.push_back(sensorPose);

  sensorPose.x     = 0.033436777;
  sensorPose.y     = 0.010343207;
  sensorPose.theta = 0.29999999996640769;
  this->geometry.push_back(sensorPose);
}

EpuckIR::IRData
EpuckIR::GetIRData() const
{
  int reading;
  IRData irData;
  this->SendRequest(EpuckInterface::GET_IR_PROX);

  for(unsigned sensor = 0;  sensor < EpuckIR::SENSOR_QUANTITY; ++sensor)
  {
    reading = this->serialPort->recvInt();

    irData.voltages.push_back(reading);

    // The range response curve was approximated by linear equations in
    // three intervals

    // A reading larger than 941, represents a range between 0 and 2cm roughly
    if(reading > 941)
    {
      irData.ranges.push_back(-4.2260e-06 * reading + 2.3378e-02);
    }
    // A reading between 403 and 941, represents a range between 2cm and 3cm
    // roughly
    else if(reading > 403)
    {
      irData.ranges.push_back(-1.8174e-05 * reading + 3.6798e-02);
    }
    // A reading between 0 and 403, represents a range larger than 3cm
    else
    {
      irData.ranges.push_back(-1.2936e-04 * reading + 7.6357e-02);
    }
  }

  return irData;
}
