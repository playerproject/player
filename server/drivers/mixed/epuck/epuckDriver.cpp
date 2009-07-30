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

#include "epuckDriver.hpp"
#include <string>
#include <cstddef>

EpuckDriver::EpuckDriver(ConfigFile* cf, int section)
  :ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
  ,EXPECTED_EPUCK_SIDE_VERSION(300) // Version 3.0
{
  std::string strSerialPort(cf->ReadString(section, "port", "/dev/rfcomm0"));
  this->serialPort = new SerialPort(strSerialPort);

  //-------------------- POSITION2D
  if(cf->ReadDeviceAddr(&this->position2dAddr, section, "provides",
                        PLAYER_POSITION2D_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->position2dAddr) != 0)
    {
      this->SetError(-1);
      return;
    }
    this->epuckPosition2d.reset(new EpuckPosition2d(this->serialPort));
  }

  //-------------------- IR
  if(cf->ReadDeviceAddr(&this->irAddr, section, "provides",
                        PLAYER_IR_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->irAddr) != 0)
    {
      this->SetError(-1);
      return;
    }
    this->epuckIR.reset(new EpuckIR(this->serialPort));
  }

  //-------------------- CAMERA
  if(cf->ReadDeviceAddr(&this->cameraAddr, section, "provides",
                        PLAYER_CAMERA_CODE , -1, NULL) == 0)
  {
    if(this->AddInterface(this->cameraAddr) != 0)
    {
      this->SetError(-1);
      return;
    }

    int sensor_x1 = cf->ReadInt(section, "sensor_x1", 240);
    int sensor_y1 = cf->ReadInt(section, "sensor_y1", 160);
    int sensor_width = cf->ReadInt(section, "sensor_width", 160);
    int sensor_height = cf->ReadInt(section, "sensor_height", 160);
    int zoom_fact_width = cf->ReadInt(section, "zoom_fact_width", 4);
    int zoom_fact_height = cf->ReadInt(section, "zoom_fact_height", 4);
    EpuckCamera::ColorModes color_mode;
    std::string strColorMode(cf->ReadString(section,
                                            "color_mode",
                                            "GREY_SCALE_MODE"));

    if(strColorMode == "GREY_SCALE_MODE")
    {
      color_mode = EpuckCamera::GREY_SCALE_MODE;
    }
    else if(strColorMode == "RGB_565_MODE")
    {
      color_mode = EpuckCamera::RGB_565_MODE;
    }
    else if(strColorMode == "YUV_MODE")
    {
      color_mode = EpuckCamera::YUV_MODE;
    }
    else
    {
      PLAYER_WARN("Invalid camera color mode, using default grey scale mode.");
      color_mode = EpuckCamera::GREY_SCALE_MODE;
    }

    this->epuckCamera.reset(new EpuckCamera(this->serialPort,
                                            sensor_x1, sensor_y1,
                                            sensor_width, sensor_height,
                                            zoom_fact_width, zoom_fact_height,
                                            color_mode));
  }

  //-------------------- RING LEDS
  const char* key[]={"ring_led0", "ring_led1", "ring_led2", "ring_led3",
                     "ring_led4", "ring_led5", "ring_led6", "ring_led7"};
  for(unsigned led = 0; led < EpuckLEDs::RING_LEDS_NUM; ++led)
  {
    if(cf->ReadDeviceAddr(&this->ringLEDAddr[led], section, "provides",
                          PLAYER_BLINKENLIGHT_CODE, -1, key[led]) == 0)
    {
      if(this->AddInterface(this->ringLEDAddr[led]) != 0)
      {
        this->SetError(-1);
        return;
      }

      if(this->epuckLEDs.get() == NULL)
        this->epuckLEDs.reset(new EpuckLEDs(this->serialPort));
    }
  }
  //-------------------- FRONT LED
  if(cf->ReadDeviceAddr(&this->frontLEDAddr, section, "provides",
                        PLAYER_BLINKENLIGHT_CODE, -1, "front_led") == 0)
  {
    if(this->AddInterface(this->frontLEDAddr) != 0)
    {
      this->SetError(-1);
      return;
    }

    if(this->epuckLEDs.get() == NULL)
      this->epuckLEDs.reset(new EpuckLEDs(this->serialPort));
  }

  //-------------------- BODY LED
  if(cf->ReadDeviceAddr(&this->bodyLEDAddr, section, "provides",
                        PLAYER_BLINKENLIGHT_CODE, -1, "body_led") == 0)
  {
    if(this->AddInterface(this->bodyLEDAddr) != 0)
    {
      this->SetError(-1);
      return;
    }

    if(this->epuckLEDs.get() == NULL)
      this->epuckLEDs.reset(new EpuckLEDs(this->serialPort));
  }
}

EpuckDriver::~EpuckDriver()
{
  delete serialPort;
}

int
EpuckDriver::MainSetup()
{
  if(this->serialPort->initialize() == -1)
  {
    PLAYER_ERROR1("%s",this->serialPort->getError().c_str());
    return -1;
  }

  // Request e-puck side program version
  this->serialPort->sendInt(0x01);
  if(this->serialPort->recvUnsigned() != this->EXPECTED_EPUCK_SIDE_VERSION)
  {
    PLAYER_ERROR("The e-puck side program version isn't the expected");
    return -1;
  }

  if(epuckCamera.get() != NULL)
  {
    try
    {
      this->epuckCamera->Initialize();
      std::string version = this->epuckCamera->GetCameraVersion();
      PLAYER_MSG1(1,"E-puck camera initialized. Camera Version: %s", version.c_str());
    }
    catch(std::exception &e)
    {
      PLAYER_ERROR1("%s", e.what());
      return -1;
    }
  }

  return 0;
}

void
EpuckDriver::MainQuit()
{

}

int
EpuckDriver::Subscribe(player_devaddr_t addr)
{
  if(addr.interf == PLAYER_POSITION2D_CODE)
  {
    this->epuckPosition2d->ResetOdometry();
  }

  return Driver::Subscribe(addr);
}

int
EpuckDriver::Unsubscribe(player_devaddr_t addr)
{
  if(addr.interf == PLAYER_POSITION2D_CODE)
  {
    this->epuckPosition2d->StopMotors();
  }
  else if(addr.interf == PLAYER_BLINKENLIGHT_CODE)
  {
    this->epuckLEDs->ClearInternal();
  }

  return Driver::Unsubscribe(addr);
}


int
EpuckDriver::ProcessMessage(QueuePointer &resp_queue,
                            player_msghdr* hdr, void* data)
{
  //-------------------- POSITION2D
  if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD,
                           PLAYER_POSITION2D_CMD_VEL,
                           this->position2dAddr))
  {
    player_position2d_cmd_vel_t position_cmd_vel =
      *(player_position2d_cmd_vel_t *)data;

    this->epuckPosition2d->SetVel(position_cmd_vel.vel.px,
                                  position_cmd_vel.vel.pa);

    if(position_cmd_vel.vel.py != 0)
    {
      PLAYER_WARN("Ignored invalid sideways volocity command");
    }

    return 0;

  }else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD,
                           PLAYER_POSITION2D_CMD_CAR,
                           this->position2dAddr))
  {
    player_position2d_cmd_car_t position_cmd_car =
      *(player_position2d_cmd_car_t *)data;

    this->epuckPosition2d->SetVel(position_cmd_car.velocity,
                                  position_cmd_car.angle);

    return 0;

  }else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                                 PLAYER_POSITION2D_REQ_GET_GEOM,
                                 this->position2dAddr))
  {
    EpuckPosition2d::BodyGeometry epuckGeom = this->epuckPosition2d->GetGeometry();

    player_position2d_geom_t playerGeom;
    playerGeom.pose.px   = 0;
    playerGeom.pose.py   = 0;
    playerGeom.pose.pyaw = 0;
    playerGeom.size.sw   = epuckGeom.width;
    playerGeom.size.sl   = epuckGeom.height;

    this->Publish(this->position2dAddr,
                  resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_POSITION2D_REQ_GET_GEOM,
                  (void*)&playerGeom,
                  sizeof(playerGeom),
                  NULL);

    return 0;

  }else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                                 PLAYER_POSITION2D_REQ_SET_ODOM,
                                 this->position2dAddr))
  {
    player_position2d_set_odom_req_t* set_odom_req =
      (player_position2d_set_odom_req_t*)data;

    EpuckInterface::Triple epuckOdom;
    epuckOdom.x     = set_odom_req->pose.px;
    epuckOdom.y     = set_odom_req->pose.py;
    epuckOdom.theta = set_odom_req->pose.pa;

    this->epuckPosition2d->SetOdometry(epuckOdom);
    this->Publish(this->position2dAddr, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_SET_ODOM);

    return 0;

  }else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                                 PLAYER_POSITION2D_REQ_RESET_ODOM,
                                 this->position2dAddr))
  {
    this->epuckPosition2d->ResetOdometry();
    this->Publish(this->position2dAddr, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_POSITION2D_REQ_RESET_ODOM);

    return 0;

  }
  //-------------------- IR
  else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ,
                                PLAYER_IR_REQ_POSE,
                                this->irAddr))
  {
    std::vector<EpuckInterface::Triple> epuckGeom = this->epuckIR->GetGeometry();

    player_ir_pose_t playerGeom;
    playerGeom.poses_count = epuckGeom.size();
    playerGeom.poses = new player_pose3d_t[playerGeom.poses_count];

    std::vector<EpuckInterface::Triple>::iterator it = epuckGeom.begin();
    for(int count = 0;
        it < epuckGeom.end();
        it++, count++)
    {
      playerGeom.poses[count].px   = it->x;
      playerGeom.poses[count].py   = it->y;
      playerGeom.poses[count].pyaw = it->theta;
    }

    this->Publish(this->irAddr,
                  resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_IR_REQ_POSE,
                  (void*)&playerGeom,
                  sizeof(playerGeom),
                  NULL);

    delete[] playerGeom.poses;

    return 0;
  }
  //-------------------- ALL LEDs
  else if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD,
                                PLAYER_BLINKENLIGHT_CMD_POWER))
  {
    //-------------------- RING LEDs
    for(unsigned led = 0; led < EpuckLEDs::RING_LEDS_NUM; ++led)
    {
      if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD,
                               PLAYER_BLINKENLIGHT_CMD_POWER,
                               this->ringLEDAddr[led]))
      {
        player_blinkenlight_cmd_power_t* blinkenlight_data =
          (player_blinkenlight_cmd_power_t*)data;

        this->epuckLEDs->SetRingLED(led, blinkenlight_data->enable);
      }
    }
    //-------------------- FRONT LED
    if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD,
                             PLAYER_BLINKENLIGHT_CMD_POWER,
                             this->frontLEDAddr))
    {
      player_blinkenlight_cmd_power_t* blinkenlight_data =
        (player_blinkenlight_cmd_power_t*)data;

      this->epuckLEDs->SetFrontLED(blinkenlight_data->enable);
    }
    //-------------------- BODY LED
    if(Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD,
                             PLAYER_BLINKENLIGHT_CMD_POWER,
                             this->bodyLEDAddr))
    {
      player_blinkenlight_cmd_power_t* blinkenlight_data =
        (player_blinkenlight_cmd_power_t*)data;

      this->epuckLEDs->SetBodyLED(blinkenlight_data->enable);
    }
    return 0;
  }

  else
  {
    PLAYER_ERROR("Epuck:: Unhandled message");
    return -1;
  }
}

void
EpuckDriver::Main()
{
  EpuckPosition2d::DynamicConfiguration epuckOdometry;

  EpuckIR::IRData epuckIRData;
  if(this->epuckIR.get() != NULL)
  {
    epuckIRData = this->epuckIR->GetIRData();
    this->irData.voltages_count = epuckIRData.voltages.size();
    this->irData.voltages = new float[this->irData.voltages_count];

    this->irData.ranges_count = epuckIRData.ranges.size();
    this->irData.ranges = new float[this->irData.ranges_count];
  }

  // If there are a camera, initialize the cameraData struct
  if(this->epuckCamera.get() != NULL)
  {
    unsigned imageWidth, imageHeight;
    EpuckCamera::ColorModes colorMode;
    this->epuckCamera->GetCameraData(imageWidth, imageHeight, colorMode);

    this->cameraData.width  = imageWidth;
    this->cameraData.height = imageHeight;

    switch (colorMode)
    {
    case EpuckCamera::GREY_SCALE_MODE:
      this->cameraData.bpp = 8;
      this->cameraData.format = PLAYER_CAMERA_FORMAT_MONO8;
      break;
    case EpuckCamera::RGB_565_MODE:
      this->cameraData.bpp = 16;
      this->cameraData.format = PLAYER_CAMERA_FORMAT_RGB565;
      break;
    case EpuckCamera::YUV_MODE:
      this->cameraData.bpp = 16;
      this->cameraData.format = PLAYER_CAMERA_FORMAT_MONO16;
      break;
    }

    this->cameraData.fdiv        = 1;
    this->cameraData.compression = PLAYER_CAMERA_COMPRESS_RAW;
    this->cameraData.image_count = cameraData.width*cameraData.height*cameraData.bpp/8;
    this->cameraData.image = new uint8_t[this->cameraData.image_count];
  }

  while(true)
  {
    pthread_testcancel();

    if(this->InQueue->Empty() == false)
    {
      ProcessMessages();
    }

    if(this->epuckPosition2d.get() != NULL)
    {
      epuckOdometry = this->epuckPosition2d->UpdateOdometry();
      this->posData.pos.px = epuckOdometry.pose.x;
      this->posData.pos.py = epuckOdometry.pose.y;
      this->posData.pos.pa = epuckOdometry.pose.theta;
      this->posData.vel.px = epuckOdometry.velocity.x;
      this->posData.vel.py = epuckOdometry.velocity.y;
      this->posData.vel.pa = epuckOdometry.velocity.theta;

      this->Publish(this->position2dAddr,
                    PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE,
                    (void*)&this->posData,sizeof(this->posData), NULL);
    }

    if(this->epuckIR.get() != NULL)
    {
      epuckIRData = this->epuckIR->GetIRData();

      std::vector<float>::iterator itVoltages, itRanges;
      itVoltages = epuckIRData.voltages.begin();
      itRanges   = epuckIRData.ranges.begin();
      for(int count = 0;
          itVoltages < epuckIRData.voltages.end();
          itVoltages++, itRanges++, count++)
      {
        this->irData.voltages[count] = *itVoltages;
        this->irData.ranges[count] = *itRanges;
      }

      this->Publish(this->irAddr,
                    PLAYER_MSGTYPE_DATA, PLAYER_IR_DATA_RANGES,
                    (void*)&this->irData,sizeof(this->irData), NULL);
    }

    if(this->epuckCamera.get() != NULL)
    {
      this->epuckCamera->GetImage(this->cameraData.image);
      this->Publish(this->cameraAddr,
                    PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE,
                    (void*)&this->cameraData,sizeof(this->cameraData), NULL);
    }

  }

  if(this->epuckIR.get() != NULL)
  {
    delete[] this->irData.voltages;
    delete[] this->irData.ranges;
  }
  if(epuckCamera.get() != NULL)
  {
    delete[] this->cameraData.image;
  }
}

Driver*
EpuckDriver::EpuckDriver_Init(ConfigFile* cf, int section)
{
  return((Driver*)(new EpuckDriver(cf, section)));
}

// Registers the driver in the driver table. Called from the
// player_driver_init function that the loader looks for
void
  epuck_Register (DriverTable* table)
{
  table->AddDriver ("epuck", EpuckDriver::EpuckDriver_Init);
}
