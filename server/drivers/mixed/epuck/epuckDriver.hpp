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

#ifndef EPUCK_DRIVER2_1_HPP
#define EPUCK_DRIVER2_1_HPP

#include <libplayercore/playercore.h>
#include <string>
#include <memory>
#include "serialPort.hpp"
#include "epuckPosition2d.hpp"
#include "epuckIR.hpp"
#include "epuckCamera.hpp"
#include "epuckLEDs.hpp"

/** \file
 * Header file where is the EpuckDriver class, for Player version 2.1
 */

/** Plug-in driver for the e-puck robot, in conformity with Player version 2.1.
 *
 * This class implements the player driver interface, and perform
 * all comunication with the player-server. It must to have a pointer
 * for each class that controls a e-puck device for which there are
 * a player interface, and bind them with player-server.
 * \author Renato Florentino Garcia
 * \date November 2008
 */
class EpuckDriver : public ThreadedDriver
{
public:

  EpuckDriver(ConfigFile* cf, int section);

  ~EpuckDriver();

  virtual int MainSetup();

  virtual void MainQuit();

  virtual int ProcessMessage(QueuePointer &resp_queue,
                             player_msghdr* hdr, void* data);

  virtual int Subscribe(player_devaddr_t addr);

  virtual int Unsubscribe(player_devaddr_t addr);

  static Driver* EpuckDriver_Init(ConfigFile* cf, int section);

private:

  const unsigned EXPECTED_EPUCK_SIDE_VERSION;

  virtual void Main();

  player_devaddr_t position2dAddr;
  player_position2d_data_t posData;
  std::auto_ptr<EpuckPosition2d> epuckPosition2d;

  player_devaddr_t irAddr;
  player_ir_data_t irData;
  std::auto_ptr<EpuckIR> epuckIR;

  player_devaddr_t cameraAddr;
  player_camera_data_t cameraData;
  std::auto_ptr<EpuckCamera> epuckCamera;

  player_devaddr_t ringLEDAddr[EpuckLEDs::RING_LEDS_NUM];
  player_blinkenlight_data_t ringLEDsData[EpuckLEDs::RING_LEDS_NUM];
  player_devaddr_t frontLEDAddr;
  player_blinkenlight_data_t frontLEDData;
  player_devaddr_t bodyLEDAddr;
  player_blinkenlight_data_t bodyLEDData;
  std::auto_ptr<EpuckLEDs> epuckLEDs;

  SerialPort* serialPort;
};

#endif
