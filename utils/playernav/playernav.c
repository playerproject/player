
#include <assert.h>
#include <signal.h>
#include <string.h>
#include <math.h>

#include "playernav.h"

#define USAGE "USAGE: playernav <host:port> [<host:port>...]"

// global quit flag
char quit;

static void
_interrupt_callback(int signum)
{
  quit=1;
}

int
main(int argc, char** argv)
{
  int i;
  int count;
  double epsilon=1e-3;

  pose_t robot_pose;

  gui_data_t gui_data;

  memset(&gui_data, 0, sizeof(gui_data));

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
                                        gui_data.positions, 
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

  count=0;
  while(!quit)
  {
    // non-blocking GTK event servicing
    while(gtk_events_pending())
      gtk_main_iteration_do(0);

    // read new data
    if(playerc_mclient_read(gui_data.mclient,10) < 0)
    {
      fprintf(stderr, "Error on read\n");
      quit=1;
    }
    for(i=0;i<gui_data.num_robots;i++)
    {
      if(gui_data.localizes[i]->info.fresh)
      {
        assert(gui_data.localizes[i]->hypoth_count > 0);
        robot_pose.px = gui_data.localizes[i]->hypoths[0].mean[0];
        robot_pose.py = gui_data.localizes[i]->hypoths[0].mean[1];
        robot_pose.pa = gui_data.localizes[i]->hypoths[0].mean[2];
        move_robot(gui_data.robot_items[i],robot_pose);
        gui_data.localizes[i]->info.fresh = 0;
      }

      // every once in a while, get the latest path from each robot
      if(!(count % (DATA_FREQ * 10 * gui_data.num_robots)))
      {
        if(playerc_position_get_waypoints(gui_data.positions[i]) < 0)
        {
          fprintf(stderr, "error while getting waypoints for robot %d\n", i);
          quit=1;
          break;
        }
        draw_waypoints(&gui_data,i);
      }
    }
    count++;
  }

  fini_player(gui_data.mclient,
              gui_data.clients,
              gui_data.maps,
              gui_data.num_robots);
  fini_gui(&gui_data);

  return(0);
}


