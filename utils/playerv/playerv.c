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
  printf("A visualization tool for the Player robot device server.\n");
  printf("Usage  : playerv [-h <hostname>] [-p <port>]\n");
  printf("                 [--<device>:<index>] [--<device>:<index>] ... \n");
  printf("Example: playerv -p 6665 --position:0 --laser:0\n");
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
  int port;
  int rate;

  // Devices
  position_t *position;
  laser_t *laser[2];
  lbd_t *lbd[2];
  ptz_t *ptz;
  sonar_t *sonar;
  vision_t *vision;

  printf("PlayerViewer %s\n", VERSION);

  // Initialise rtk lib
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
    PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
    return -1;
  }
    
  // Create gui
  app = rtk_app_create();

  // Create a window for most of the sensor data
  mainwnd = mainwnd_create(app, host, port);
  if (!mainwnd)
    return -1;

  // Create (but dont subscribe) devices
  position = position_create(mainwnd, opt, client, 0);
  laser[0] = laser_create(mainwnd, opt, client, 0);
  laser[1] = laser_create(mainwnd, opt, client, 1);
  lbd[0] = lbd_create(mainwnd, opt, client, 0);
  lbd[1] = lbd_create(mainwnd, opt, client, 1);
  ptz = ptz_create(mainwnd, opt, client, 0);
  sonar = sonar_create(mainwnd, opt, client, 0);
  vision = vision_create(mainwnd, opt, client, 0);
  
  // Print out a list of unused options.
  opt_warn_unused(opt);

  // Start the gui
  rtk_app_refresh_rate(app, rate);
  rtk_app_start(app);
  
  while (!quit)
  {
    // Wait for some data.  We rely on getting the sync messages if no
    // devices are subscribed.
    playerc_client_read(client);

    // Update the main window
    if (mainwnd_update(mainwnd) != 0)
      break;

    // Update devices
    position_update(position);
    laser_update(laser[0]);
    laser_update(laser[1]);
    lbd_update(lbd[0]);
    lbd_update(lbd[1]);
    ptz_update(ptz);
    sonar_update(sonar);
    vision_update(vision);
  }
  
  // Stop the gui
  rtk_app_stop(app);

  // Destroy devices
  vision_destroy(vision);
  sonar_destroy(sonar);
  ptz_destroy(ptz);
  lbd_destroy(lbd[1]);
  lbd_destroy(lbd[0]);
  laser_destroy(laser[1]);
  laser_destroy(laser[0]);
  position_destroy(position);

  // Destroy the windows
  mainwnd_destroy(mainwnd);

  // Destroy the gui
  rtk_app_destroy(app);

  // Disconnect from server
  if (playerc_client_disconnect(client) != 0)
  {
    PRINT_ERR1("libplayerc error: %s", playerc_errorstr);
    return -1;
  }
  playerc_client_destroy(client);

  opt_term(opt);
  
  return 0;
}
