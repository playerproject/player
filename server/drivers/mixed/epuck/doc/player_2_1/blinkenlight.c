/** This is an example of how to manage the e-puck LEDs in Player version 2.1.X
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
