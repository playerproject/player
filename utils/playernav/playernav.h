#ifndef PLAYERNAV_GUI_H
#define PLAYERNAV_GUI_H

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnomecanvas/libgnomecanvas.h>

#include <playerc.h>

#define DEFAULT_DISPLAY_WIDTH 800
#define MIN_DISPLAY_WIDTH 10

#define MAX_HOSTNAME_LEN 256

#define DATA_FREQ 10
#define MAX_NUM_ROBOTS 8

typedef struct
{
  GtkWindow* main_window;
  GtkBox* hbox;
  GtkScrolledWindow* map_window;
  GnomeCanvas* map_canvas;
  // aspect ratio of map (width / height)
  double aspect;
  // proxy for map
  playerc_map_t* mapdev;
  // scrollbar for zooming
  GtkVScrollbar* zoom_scrollbar;
  GtkAdjustment* zoom_adjustment;
} gui_data_t;

void create_map_image(gui_data_t* gui_data);
void init_gui(gui_data_t* gui_data, 
              int argc, char** argv);

playerc_mclient_t* init_player(playerc_client_t** clients,
                               playerc_map_t** maps,
                               playerc_localize_t** localizes,
                               int num_bots,
                               char** hostnames,
                               int* ports,
                               int data_freq);
void fini_player(playerc_mclient_t* mclient,
                 playerc_client_t** clients,
                 playerc_map_t** maps,
                 int num_bots);

/* Parse command line arguments, of the form host:port */
int parse_args(int argc, char** argv, int* num_bots, 
               char** hostnames, int* ports);

#endif
