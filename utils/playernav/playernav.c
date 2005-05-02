// TODO:
//
//   * Speed up screenshot dumps.
//
//   * Make initial window size / zoom fit the whole map.
//
//   * Make the zoom work better / look nicer

/** @addtogroup utils Utilities */
/** @{ */
/** @defgroup player_util_playernav playernav

@par Synopsis

playernav is a GUI client that provides control over @ref
player_interface_localize and @ref player_interface_planner devices.
It allows you to set your robots' localization hypotheses by dragging
and dropping them in the map.  You can set global goals the same way,
and see the planned paths and the robots' progress toward the goals.
playernav can also display (a subset of) the localization system's current
particle set, which may help in debugging localization.  You can think
of playernav as an Operator Control Unit (OCU).  playernav can also be
used just to view a map.

playernav requires libgnomecanvas.

@par Usage

playernav is installed alongside player in $prefix/bin, so if player is
in your PATH, then playernav should also be.  Command-line usage is:
@verbatim
$ playernav [-fps <dumprate>] [-zoom <zoom>] [-aa {0|1}] [-map <map_idx>] <host:port> [<host:port>...]
@endverbatim
Where the options are:
- -fps &lt;dumprate&gt; : when requested, dump screenshots at this rate in Hz (default: 5Hz)
- -zoom &lt;zoom&gt; : initial level of zoom in the display (default: 1)
- -aa {0|1} : whether to use anti-aliased canvas for display; the anti-aliased canvas looks nicer but may require more processor cycles (default: 1)
- -map  &lt;map_idx&gt; : the index of the map to be requested and displayed (default: 0)

playernav will connect to Player at each host:port combination given on
the command line.  For each one, playernav will attempt to subscribe to
the following devices:
- @ref player_interface_localize :0
- @ref player_interface_planner :0

Additionally, playernav will attempt to subscribe to the following device
on the first server:
- @ref player_interface_map :&lt;map_idx&gt; 

If the subscription to the map fails, playernav will exit.  If other
subscriptions fail, you'll be warned, but playernav will continue.  So,
for example, if you just want to view a map, point playernav at a server
with @ref player_interface_map :&lt;map_idx&gt; defined.  Of course, if subscription
to @ref player_interface_localize :0 or @ref player_interface_planner
:0 fails for a robot, then you will not be able to localize or command,
respectively, that robot.

When playernav starts, a window will pop up, displaying the map.
Use the scrollbars on the right and bottom to pan the window.  Use the
scrollbar on the left to zoom the window.  An icon representing each
robot will be shown.  Pass the pointer over a robot to see its
host:port address.  

Use the "File:Capture stills" menu item to enable / disable dumping
screenshots; they will be saved at the rate set on the command line
(5Hz default).  Beware that dumping screenshots requires significant
processing and will seriously degrade performance of other jobs running
on the same machine.  In particular, if one or more of the player servers
is running on the same machine, their localization systems may fall behind
the incoming data, which will have deleterious consequences.

@par Localizing and commanding robots

To set the localization hypothesis for a robot, drag and drop the robot
with the left mouse button to the desired position.  Then click the
left button again to set the desired orientation.  You should see the
robot's icon move to assume a similar pose (it won't be exactly the same
pose, but rather whatever estimate the localization system provides).
As the robot moves, its estimated pose will be updated in playernav.

To set a goal for a robot, drag and drop the robot with the right mouse
button to the desired position.  Then click the right button again to set
the desired orientation.  You should see a path from the robot to the
goal, with waypoints shown (if you do not see a path, then the planner
probably failed to find one).  You will then see the robot follow the
path to the goal, with subsegments disappearing as waypoints are reached.

You can set new localization hypotheses and target at any time, even
while the robot is moving.

@par Stopping robots

To disable (i.e., stop) a robot, click it with the middle button.  To
enable a robot, click it again with the middle button.  To enable/disable
ALL robots, use the "Stop/Go..." menu.  The latter option is good in an
emergency, or when it's too difficult to click on a misbehaving robot.
When enabled, a robot will (or at least should) continue executing its
previous plan, if any.

@par Screenshots

@image html playernav-example.jpg "Screenshot of playernav with three robots moving toward goals"

*/

/** @} */

#include <assert.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include "playernav.h"

#define USAGE "USAGE: playernav [-fps <dumprate>] [-zoom <zoom>] [-aa {0|1}] [-map <map_idx>] <host:port> [<host:port>...]"

// flag and index for robot currently being moved by user (if any)
int robot_moving_p;
int robot_moving_idx;

double dumpfreq;
int dumpp;
int showparticlesp;

static void
_interrupt_callback(int signum)
{
  gtk_main_quit();
}

// Function to read from player; will be called when GTK is idle
gboolean
player_read_func(gpointer* arg)
{
  int i;
  static int count=0;
  pose_t robot_pose;
  gui_data_t* gui_data = (gui_data_t*)arg;
  static struct timeval last = {0, 0};
  struct timeval curr;
  double diff;
  gboolean onmap;

  // read new data
  if(playerc_mclient_read(gui_data->mclient,10) < 0)
  {
    fprintf(stderr, "Error on read\n");
    gtk_main_quit();
  }
  for(i=0;i<gui_data->num_robots;i++)
  {
    if(gui_data->localizes[i] && gui_data->localizes[i]->info.fresh)
    {
      assert(gui_data->localizes[i]->hypoth_count > 0);
      robot_pose.px = gui_data->localizes[i]->hypoths[0].mean[0];
      robot_pose.py = gui_data->localizes[i]->hypoths[0].mean[1];
      robot_pose.pa = gui_data->localizes[i]->hypoths[0].mean[2];

      // is the robot localized within the map?
      onmap = ((fabs(robot_pose.px) <
                (gui_data->mapdev->width * 
                 gui_data->mapdev->resolution / 2.0)) ||
               (fabs(robot_pose.py) <
                (gui_data->mapdev->height * 
                 gui_data->mapdev->resolution / 2.0)));
      // if it's off the map, put it in the middle
      if(!onmap)
        robot_pose.px = robot_pose.py = 0.0;

      // don't draw the new pose if the user is in the middle of moving the
      // robot
      if(!robot_moving_p || (robot_moving_idx != i))
      {
        // also don't draw it if the pose hasn't changed since last time
        if((gui_data->robot_poses[i].px != robot_pose.px) ||
           (gui_data->robot_poses[i].py != robot_pose.py) ||
           (gui_data->robot_poses[i].pa != robot_pose.pa))
        {
          //printf("moving robot %d\n", i);
          move_item(gui_data->robot_items[i],robot_pose,1);

          if(onmap && showparticlesp)
          {
            playerc_localize_get_particles(gui_data->localizes[i]);
            draw_particles(gui_data,i);
          }
        }
      }

      // regardless, store this pose for comparison on next iteration
      gui_data->robot_poses[i] = robot_pose;

      gui_data->localizes[i]->info.fresh = 0;
    }

    // every once in a while, get the latest path from each robot
    if(!(count % (DATA_FREQ * 10 * gui_data->num_robots)))
    {
      if(gui_data->planners[i])
      {
        if(playerc_planner_get_waypoints(gui_data->planners[i]) < 0)
        {
          fprintf(stderr, "error while getting waypoints for robot %d\n", i);
          gtk_main_quit();
          break;
        }
        //puts("drawing waypoints");
        draw_waypoints(gui_data,i);
      }
    }

    // raise the robot's canvas item, so that the user can select it
    gnome_canvas_item_raise_to_top(gui_data->robot_items[i]);
  }

  // dump screenshot
  if(dumpp)
  {
    gettimeofday(&curr,NULL);
    diff = (curr.tv_sec + curr.tv_usec/1e6) - (last.tv_sec + last.tv_usec/1e6);
    if(diff >= 1.0/dumpfreq)
    {
      dump_screenshot(gui_data);
      last = curr;
    }
  }

  count++;
  return(TRUE);
}

int
main(int argc, char** argv)
{
  int i;
  int map_idx;

  pose_t robot_pose;

  gui_data_t gui_data;

  memset(&gui_data, 0, sizeof(gui_data));

  dumpfreq = 5.0;

  gui_data.initial_zoom = 1.0;
  gui_data.aa = 1;
  map_idx = 0;

  if(parse_args(argc-1, argv+1, &(gui_data.num_robots), gui_data.hostnames, 
                gui_data.ports, &(gui_data.initial_zoom), &(gui_data.aa),
                &map_idx) < 0)
  {
    puts(USAGE);
    exit(-1);
  }

  assert(signal(SIGINT, _interrupt_callback) != SIG_ERR);

  assert(gui_data.mclient = init_player(gui_data.clients, 
                                        gui_data.maps, 
                                        gui_data.localizes, 
                                        gui_data.planners, 
                                        gui_data.num_robots, 
                                        gui_data.hostnames, 
                                        gui_data.ports, 
                                        DATA_FREQ,
                                        map_idx));

  // assume the robots all start enabled (should really get the current
  // enable/disable state for each robot from the server).
  for(i=0;i<gui_data.num_robots;i++)
    gui_data.robot_enable_states[i] = 1;

  // we've read the map
  gui_data.mapdev = gui_data.maps[0];

  init_gui(&gui_data, argc, argv);

  create_map_image(&gui_data);

  for(i=0;i<gui_data.num_robots;i++)
  {
    robot_pose.px = 0.0;
    robot_pose.py = 0.0;
    robot_pose.pa = 0.0;
    create_robot(&gui_data, i, robot_pose);
  }

  gtk_widget_show((GtkWidget*)(gui_data.main_window));

  // setup read function to be called when idle
  g_idle_add((GSourceFunc)player_read_func,
             (gpointer*)&gui_data);

  gtk_main();

  fini_player(gui_data.mclient,
              gui_data.clients,
              gui_data.maps,
              gui_data.localizes,
              gui_data.planners,
              gui_data.num_robots);
  fini_gui(&gui_data);

  return(0);
}


