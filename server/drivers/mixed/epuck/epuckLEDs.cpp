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

#include "epuckLEDs.hpp"

EpuckLEDs::EpuckLEDs(const SerialPort* const serialPort)
  :EpuckInterface(serialPort)
{
  this->ledState.front = false;
  this->ledState.body = false;

  for(unsigned led=0; led<RING_LEDS_NUM; ++led)
  {
    this->ledState.ring[led] = false;
  }
}

void
EpuckLEDs::SendLEDState() const
{
  // Each one of eight bits of ringLEDmsg represents one of eight ring LEDs
  // in e-puck, the rightmost bit meaning the LED 0. With a bit being true
  // the LED will be on, with a bit being false the LED will be off.
  char ringLEDmsg = 0;
  for(unsigned led=0; led<RING_LEDS_NUM; ++led)
  {
    if(this->ledState.ring[led]==true)
    {
      ringLEDmsg |= (0x1<<led);
    }
  }

  // Like above, the rightmost bit of frontBodyLEDmsg represents the front LED
  // state, and the immediately at left of that represents the body LED. The
  // other six LEDs are meaningless.
  char frontBodyLEDmsg = 0;
  if(this->ledState.front==true)
  {
    frontBodyLEDmsg = 0x1;
  }
  if(this->ledState.body==true)
  {
    frontBodyLEDmsg |= 0x2;
  }

  this->SendRequest(EpuckInterface::SET_LED_POWER);
  this->serialPort->sendChar(ringLEDmsg);
  this->serialPort->sendChar(frontBodyLEDmsg);

  this->serialPort->recvChar(); // Wait for e-puck to set the LEDs
}

void
EpuckLEDs::SetRingLED(bool ringLED[EpuckLEDs::RING_LEDS_NUM])
{
  for(unsigned led=0; led<RING_LEDS_NUM; ++led)
  {
    this->ledState.ring[led] = ringLED[led];
  }

  this->SendLEDState();
}

void
EpuckLEDs::SetRingLED(unsigned id, bool state)
{
  if(id >= RING_LEDS_NUM)
  {
    return;
  }

  this->ledState.ring[id] = state;
  this->SendLEDState();
}

void
EpuckLEDs::SetFrontLED(bool state)
{
  this->ledState.front = state;
  this->SendLEDState();
}

void
EpuckLEDs::SetBodyLED(bool state)
{
  this->ledState.body = state;
  this->SendLEDState();
}

void
EpuckLEDs::ClearInternal()
{
  this->ledState.front = false;
  this->ledState.body = false;

  for(unsigned led=0; led<RING_LEDS_NUM; ++led)
  {
    this->ledState.ring[led] = false;
  }
}
