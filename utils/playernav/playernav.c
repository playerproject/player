
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


