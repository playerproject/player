// TODO:
//
//   * Debug constant redraw problem.  It seems that, on some machines,
//     the app gets a continuous streams of GTK events, which makes X suck
//     up the whole CPU.
//
//     UPDATE: this happens whether or not direct rendering support is on.
//
//     UPDATE: changing to a non-antialiased canvas helps, but not much
//
//     UPDATE: restarting X on my laptop fixes it (!)
//
//     UPDATE: I may have just fixed that by adding the player read func as
//             a GTK idle callback
//
//   * Make initial window size / zoom fit the whole map.
//
//   * Make the zoom work better / look nicer

#include <assert.h>
#include <signal.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#include "playernav.h"

#define USAGE "USAGE: playernav [-fps <dumprate>] <host:port> [<host:port>...]"

// flag and index for robot currently being moved by user (if any)
int robot_moving_p;
int robot_moving_idx;

double dumpfreq;
int dumpp;

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

      // if it's off the map, put it in the middle
      if((fabs(robot_pose.px) >=
          (gui_data->mapdev->width * gui_data->mapdev->resolution / 2.0)) ||
         (fabs(robot_pose.py) >=
          (gui_data->mapdev->height * gui_data->mapdev->resolution / 2.0)))
      {
        robot_pose.px = robot_pose.py = 0.0;
      }

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
          move_robot(gui_data->robot_items[i],robot_pose);
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
  }

  // dump screenshot
  if(dumpp)
  {
    gettimeofday(&curr,NULL);
    diff = (curr.tv_sec + curr.tv_usec/1e6) - (last.tv_sec + last.tv_usec/1e6);
    if(diff > 1.0/dumpfreq)
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

  pose_t robot_pose;

  gui_data_t gui_data;

  memset(&gui_data, 0, sizeof(gui_data));

  dumpfreq = 5.0;

  if(parse_args(argc-1, argv+1, &(gui_data.num_robots), 
                gui_data.hostnames, gui_data.ports) < 0)
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
                                        DATA_FREQ));

  // we've read the map, so fill in the aspect ratio
  gui_data.mapdev = gui_data.maps[0];
  gui_data.aspect = gui_data.mapdev->width / (double)(gui_data.mapdev->height);

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
  g_idle_add((GSourceFunc)player_read_func,(gpointer*)&gui_data);

  gtk_main();

  fini_player(gui_data.mclient,
              gui_data.clients,
              gui_data.maps,
              gui_data.num_robots);
  fini_gui(&gui_data);

  return(0);
}


