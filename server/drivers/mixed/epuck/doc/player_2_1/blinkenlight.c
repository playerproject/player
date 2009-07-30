/** This is an example of how to manage the e-puck LEDs in Player version 2.1.X
 */

/*
Copyright (c) 2008 Renato Florentino Garcia <fgar.renato@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of the Player Project nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <libplayerc/playerc.h>
#include <unistd.h>

#define RING_LEDS_NUMBER 8
#define LED_ON 1

int main(int argc, const char **argv)
{
  playerc_client_t *client;
  playerc_blinkenlight_t *ringLED[RING_LEDS_NUMBER];
  playerc_blinkenlight_t *frontLED;
  playerc_blinkenlight_t *bodyLED;
  int led;

  client = playerc_client_create(NULL, "localhost", 6665);
  if(playerc_client_connect(client) != 0)
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    return -1;
  }
  for(led=0; led<RING_LEDS_NUMBER; led++)
  {
    ringLED[led] = playerc_blinkenlight_create(client, led);
    if(playerc_blinkenlight_subscribe(ringLED[led], PLAYERC_OPEN_MODE) != 0)
    {
      fprintf(stderr, "error: %s\n", playerc_error_str());
      return -1;
    }
  }
  frontLED = playerc_blinkenlight_create(client, 8);
  if(playerc_blinkenlight_subscribe(frontLED, PLAYERC_OPEN_MODE) != 0)
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    return -1;
  }
  bodyLED = playerc_blinkenlight_create(client, 9);
  if(playerc_blinkenlight_subscribe(bodyLED, PLAYERC_OPEN_MODE) != 0)
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    return -1;
  }

  if(playerc_client_datamode(client, PLAYERC_DATAMODE_PULL) != 0)
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    return -1;
  }
  if(playerc_client_set_replace_rule(client, -1, -1,
                                     PLAYER_MSGTYPE_DATA, -1, 1) != 0)
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    return -1;
  }

  /* Turn on the ring LEDs 2 and 6, and the front and body LEDs. */
  playerc_blinkenlight_enable(ringLED[2], LED_ON);
  playerc_blinkenlight_enable(ringLED[6], LED_ON);
  playerc_blinkenlight_enable(frontLED, LED_ON);
  playerc_blinkenlight_enable(bodyLED, LED_ON);

  /* Without this sleep there will not be enough time for process all above  *
   * messages. If the camera interface is not in provides section of Player  *
   * configuration file, the time for e-puck initialization will be smaller, *
   * and consequently this sleep time will can be smaller.                   */
  usleep(3e6);

  /* Shutdown and tidy up */
  for(led=0; led<RING_LEDS_NUMBER; led++)
  {
    playerc_blinkenlight_unsubscribe(ringLED[led]);
    playerc_blinkenlight_destroy(ringLED[led]);
  }
  playerc_blinkenlight_unsubscribe(frontLED);
  playerc_blinkenlight_destroy(frontLED);
  playerc_blinkenlight_unsubscribe(bodyLED);
  playerc_blinkenlight_destroy(bodyLED);
  playerc_client_disconnect(client);
  playerc_client_destroy(client);

  return 0;
}
