
#include <assert.h>
#include <signal.h>
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

void fini_gui();

int
main(int argc, char** argv)
{
  int num_bots=0;
  char* hostnames[MAX_NUM_ROBOTS];
  int ports[MAX_NUM_ROBOTS];
  playerc_mclient_t* mclient;
  playerc_client_t* clients[MAX_NUM_ROBOTS];
  playerc_map_t* maps[MAX_NUM_ROBOTS];
  playerc_localize_t* localizes[MAX_NUM_ROBOTS];

  int i;
  double dx, dy, da;
  GnomeCanvasItem *item;
  GnomeCanvasPoints *points;

  gui_data_t gui_data;

  if(parse_args(argc-1, argv+1, &num_bots, hostnames, ports) < 0)
  {
    puts(USAGE);
    exit(-1);
  }

  assert(signal(SIGINT, _interrupt_callback) != SIG_ERR);

  assert(mclient = init_player(clients, maps, localizes, num_bots, 
                               hostnames, ports, DATA_FREQ));

  // we've read the map, so fill in the aspect ratio
  gui_data.mapdev = maps[0];
  gui_data.aspect = gui_data.mapdev->width / (double)(gui_data.mapdev->height);

  init_gui(&gui_data, argc, argv);

  create_map_image(&gui_data);


  points = gnome_canvas_points_new(4);
  for (i = 0; i < 4; i++)
  {
    da = i * M_PI / 2 + M_PI / 4;
    dx = cos(da) * sqrt(2) * 10.0 / 2;
    dy = sin(da) * sqrt(2) * 10.0 / 2;
    points->coords[2 * i + 0] = 0.0 + dx * cos(0.0) - dy * sin(0.0);
    points->coords[2 * i + 1] = 0.0 + dx * sin(0.0) + dy * cos(0.0);
  }
  item = gnome_canvas_item_new(gnome_canvas_root(gui_data.map_canvas),
                               gnome_canvas_polygon_get_type(),
                               "points", points,
                               "outline_color_rgba",
                               GNOME_CANVAS_COLOR_A(255,0,0,255),
                               "fill_color_rgba", 
                               GNOME_CANVAS_COLOR_A(0,255,0,127),
                               "width_pixels", 1,
                               NULL);
  gnome_canvas_points_unref(points);

  gtk_widget_show((GtkWidget*)(gui_data.main_window));

  while(!quit)
  {
    // non-blocking GTK event servicing
    gtk_main_iteration_do(0);

    // read new data
    if(playerc_mclient_read(mclient,10) < 0)
    {
      fprintf(stderr, "Error on read\n");
      return(-1);
    }
  }

  fini_player(mclient,clients,maps,num_bots);
  fini_gui(&gui_data);

  return(0);
}


