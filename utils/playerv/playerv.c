/*
 *  PlayerViewer
 *  Copyright (C) Andrew Howard 2002
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */
/**************************************************************************
 * Desc: PlayerView, program entry point
 * Author: Andrew Howard
 * Date: 28 Mar 2002
 * CVS: $Id$
 *************************************************************************/

#if HAVE_CONFIG_H
#  include <config.h>
#endif

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "playerv.h"


// Set flag to 1 to force program to quit
static int quit = 0;

// Handle quit signals
void sig_quit(int signum)
{
  quit = 1;
}


// Print the usage string
void print_usage()
{
  printf("\nPlayerViewer %s, ", VERSION);
  printf("a visualization tool for the Player robot device server.\n");
  printf("Usage  : playerv [-h <hostname>] [-p <port>]\n");
  printf("                 [--<device>:<index>] [--<device>:<index>] ... \n");
  printf("Example: playerv -p 6665 --position:0 --sonar:0\n");
  printf("\n");
}


// Main
int main(int argc, char **argv)
{
  playerc_client_t *client;
  rtk_app_t *app;  
  mainwnd_t *mainwnd;
  opt_t *opt;
  const char *host;
  int i;
  int port;
  int rate;
  char section[256];
  int device_count;
  device_t devices[PLAYER_MAX_DEVICES];
  device_t *device;
  void *proxy;

  printf("PlayerViewer %s\n", VERSION);

  // Initialise rtk lib (after we have read the program options we
  // want).
  rtk_init(&argc, &argv);

  // Register signal handlers
  signal(SIGINT, sig_quit);
  signal(SIGQUIT, sig_quit);

  // Load program options
  opt = opt_init(argc, argv, NULL);
  if (!opt)
  {
    print_usage();
    return -1;
  }

  // Pick out some important program options
  rate = opt_get_int(opt, "gui", "rate", 10);
  host = opt_get_string(opt, "", "host", NULL);
  if (!host)
    host = opt_get_string(opt, "", "h", "localhost");
  port = opt_get_int(opt, "", "port", -1);
  if (port < 0)
    port = opt_get_int(opt, "", "p", 6665);

  // Connect to the server
  printf("Connecting to [%s:%d]\n", host, port);
  client = playerc_client_create(NULL, host, port);
  if (playerc_client_connect(client) != 0)
  {
    PRINT_ERR1("%s", playerc_errorstr);
    print_usage();
    return -1;
  }

  // Get the available devices.
  if (playerc_client_get_devlist(client) != 0)
  {
    PRINT_ERR1("%s", playerc_errorstr);
    return -1;
  }

  // Create gui
  app = rtk_app_create();

  // Create a window for most of the sensor data
  mainwnd = mainwnd_create(app, host, port);
  if (!mainwnd)
    return -1;
  
  // Create a list of available devices, with their gui proxies.
  for (i = 0; i < client->id_count; i++)
  {
    device = devices + i;
    device->code = client->ids[i].code;
    device->index = client->ids[i].index;
    device->drivername = strdup(client->drivernames[i]);

    // See if the device should be subscribed immediately.
    snprintf(section, sizeof(section), "%s:%d", playerc_lookup_name(device->code), device->index);
    device->subscribe = opt_get_int(opt, section, "", 0);
    device->subscribe = opt_get_int(opt, section, "subscribe", device->subscribe);
    if (device->index == 0)
    {
      snprintf(section, sizeof(section), "%s", playerc_lookup_name(device->code));
      device->subscribe = opt_get_int(opt, section, "", device->subscribe);
      device->subscribe = opt_get_int(opt, section, "subscribe", device->subscribe);
    }

    // Create the GUI proxy for this device.
    create_proxy(device, opt, mainwnd, client);
  }
  device_count = client->id_count;
    
  // Print the list of available devices.
  printf("Available devices:\n", host, port);
  for (i = 0; i < device_count; i++)
  {
    device = devices + i;
    snprintf(section, sizeof(section), "%s:%d",
             playerc_lookup_name(device->code), device->index);
    printf("%-16s %-40s", section, device->drivername);
    if (device->proxy)
    {
      if (device->subscribe)
        printf("subscribed");
      else
        printf("ready");
    }
    else
      printf("unsupported");
    printf("\n");
  }
  
  // Print out a list of unused options.
  opt_warn_unused(opt);

  // Start the gui; dont run in a separate thread.
  rtk_app_refresh_rate(app, 0);
  rtk_app_main_init(app);
  
  while (!quit)
  {
    // Wait for some data.  We rely on getting the sync messages if no
    // devices are subscribed.
    proxy = playerc_client_read(client);

    // Update everything on the sync packet.
    if (proxy == client)
    {      
      // Update all the subscribed devices
      for (i = 0; i < device_count; i++)
      {
        device = devices + i;
        if (device->proxy)
          (*(device->fnupdate)) (device->proxy);
      }

      // Let gui process messages
      rtk_app_main_loop(app);

      // Update the main window
      if (mainwnd_update(mainwnd) != 0)
        break;
    }
  }
  
  // Stop the gui
  rtk_app_main_term(app);

  // Destroy devices
  for (i = 0; i < device_count; i++)
  {
    device = devices + i;
    if (device->proxy)
      (*(device->fndestroy)) (device->proxy);
    free(device->drivername);
  }

  // Disconnect from server
  if (playerc_client_disconnect(client) != 0)
  {
    PRINT_ERR1("%s", playerc_errorstr);
    return -1;
  }
  playerc_client_destroy(client);

  // Destroy the windows
  mainwnd_destroy(mainwnd);

  // Destroy the gui
  rtk_app_destroy(app);

  opt_term(opt);
  
  return 0;
}
