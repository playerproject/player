
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <signal.h>

#include <gtk/gtk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <libgnomecanvas/libgnomecanvas.h>

#include <playerc.h>

#define DEFAULT_DISPLAY_WIDTH 800
#define MIN_DISPLAY_WIDTH 10

#define DATA_FREQ 10
#define MAX_NUM_ROBOTS 8
#define MAX_HOSTNAME_LEN 256

#define USAGE "USAGE: playernav <host:port> [<host:port>...]"

typedef struct
{
  GtkWindow* main_window;
  GtkWindow* map_window;
  GnomeCanvas* map_canvas;
  // aspect ratio (width / height)
  double aspect;
  playerc_map_t* mapdev;
  // pixels per canvas unit
  double canvas_zoom;
} gui_data_t;

#define METERS_TO_CANVAS_X(gui_data,x) \
  ((((x) / (gui_data->mapdev->resolution)) + ((gui_data->mapdev->width) / 2.0)) / (gui_data->canvas_zoom))
#define CANVAS_TO_METERS_X(gui_data,x) \
  ((((x) * (gui_data->canvas_zoom)) - ((gui_data->mapdev->width) / 2.0)) * (gui_data->mapdev->resolution))

// global quit flag
char quit;

/*
 * handle quit events, by setting a flag that will make the main loop exit
 */
static gboolean 
_quit_callback(GtkWidget *widget,
               GdkEvent *event,
               gpointer data)
{
  quit = 1;
  return(TRUE);
}

void
_interrupt_callback(int signum)
{
  quit=1;
}

/*
 * Handle window resize events.
 */
static void 
_resize_window(GtkWidget *widget,
               GtkAllocation* allocation,
               gpointer data)
{
  gui_data_t* gui_data = (gui_data_t*)data;

  gui_data->canvas_zoom = ((allocation->width - 20) / 
                           (gui_data->mapdev->width * 
                            gui_data->mapdev->resolution));


  // set canvas units to meters, scaled by window size
  gnome_canvas_set_pixels_per_unit(gui_data->map_canvas, 
                                   gui_data->canvas_zoom);
}

void create_map_image(gui_data_t* gui_data);
void init_gui(gui_data_t* gui_data, 
              int argc, char** argv);
void fini_gui();
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

/* Parse command line arguments, of the form host:port */
int
parse_args(int argc, char** argv,
           int* num_bots, char** hostnames, int* ports)
{
  char* idx;
  int port;
  int hostlen;
  int i;

  if(argc < 1)
    return(-1);

  *num_bots=0;
  for(i=0; i < argc; i++)
  {
    // Look for ':' (colon), and extract the trailing port number.  If not
    // given, then use the default Player port (6665)
    if((idx = strchr(argv[i],':')) && (strlen(idx) > 1))
    {
      port = atoi(idx+1);
      hostlen = idx - argv[i];
    }
    else
    {
      port = PLAYER_PORTNUM;
      hostlen = strlen(argv[i]);
    }

    
    // Store the hostnames and port numbers
    assert((hostlen > 0) && (hostlen < (MAX_HOSTNAME_LEN - 1)));
    argv[i][hostlen] = '\0';
    hostnames[*num_bots] = strdup(argv[i]);
    ports[*num_bots] = port;
    (*num_bots)++;
  }

  return(0);
}

/*
 * Connect to player at each host:port pair, as specified by the global
 * vars 'hostnames' and 'ports'.  Also subscribes to each device, and adds 
 * each client into a new multiclient (which is returned)
 */
playerc_mclient_t*
init_player(playerc_client_t** clients,
            playerc_map_t** maps,
            playerc_localize_t** localizes,
            int num_bots,
            char** hostnames,
            int* ports,
            int data_freq)
{
  int i;
  playerc_mclient_t* mclient;

  /* Connect to Player */
  assert(mclient = playerc_mclient_create());
  for(i=0; i<num_bots; i++)
  {
    assert(clients[i] = 
           playerc_client_create(mclient, hostnames[i], ports[i]));
    if(playerc_client_connect(clients[i]) < 0)
    {
      fprintf(stderr, "Failed to connect to %s:%d\n", 
              hostnames[i], ports[i]);
      return(NULL);
    }
    if(playerc_client_datafreq(clients[i],data_freq) < 0)
    {
      fprintf(stderr, "Failed to set data frequency\n");
      return(NULL);
    }
    assert(maps[i] = playerc_map_create(clients[i], 0));
    if(playerc_map_subscribe(maps[i],PLAYER_READ_MODE) < 0)
    {
      fprintf(stderr, "Failed to subscribe to map\n");
      return(NULL);
    }
    assert(localizes[i] = playerc_localize_create(clients[i], 0));
    if(playerc_localize_subscribe(localizes[i],PLAYER_READ_MODE) < 0)
    {
      fprintf(stderr, "Failed to subscribe to localize\n");
      //return(NULL);
    }
    // hostnames were strdup'd in parse_args()
    free(hostnames[i]);
  }

  /* Get the map from the first robot */
  if(playerc_map_get_map(maps[0]) < 0)
  {
    fprintf(stderr, "Failed to get map\n");
    return(NULL);
  }

#if 0
  /* Get at least one round of data from each robot */
  for(;;)
  {
    if(playerc_mclient_read(mclient,-1) < 0)
    {
      fprintf(stderr, "Error on read\n");
      return(NULL);
    }

    for(i=0; i<num_bots; i++)
    {
      if(!truths[i]->info.fresh || 
         !lasers[i]->info.fresh || 
         !positions[i]->info.fresh)
        break;
    }
    if(i==num_bots)
      break;
  }
#endif

  return(mclient);
}

void
fini_player(playerc_mclient_t* mclient,
            playerc_client_t** clients,
            playerc_map_t** maps,
            int num_bots)
{
  int i;

  for(i=0;i<num_bots;i++)
  {
    playerc_map_destroy(maps[i]);
    playerc_client_destroy(clients[i]);
  }
  playerc_mclient_destroy(mclient);
}

void
init_gui(gui_data_t* gui_data, int argc, char** argv)
{
  double t[6];

  g_type_init();
  gtk_init(&argc, &argv);

  g_assert((gui_data->main_window = 
            (GtkWindow*)gtk_window_new(GTK_WINDOW_TOPLEVEL)));
  gtk_widget_set_size_request((GtkWidget*)(gui_data->main_window),
                              MIN_DISPLAY_WIDTH,MIN_DISPLAY_WIDTH);
  gtk_window_resize(gui_data->main_window,
                    DEFAULT_DISPLAY_WIDTH,
                    (int)rint(DEFAULT_DISPLAY_WIDTH / gui_data->aspect));

  g_assert((gui_data->map_window = 
            (GtkWindow*)gtk_scrolled_window_new(NULL, NULL)));
      
  /* the policy is one of GTK_POLICY AUTOMATIC, or GTK_POLICY_ALWAYS.
   * GTK_POLICY_AUTOMATIC will automatically decide whether you
   * need scrollbars, whereas GTK_POLICY_ALWAYS will always
   * leave the scrollbars there.  The first one is the horizontal
   * scrollbar, the second, the vertical. 
   */
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gui_data->map_window),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(gui_data->main_window),
                    (GtkWidget*)gui_data->map_window);
  gtk_widget_show((GtkWidget*)(gui_data->map_window));

  gtk_widget_push_visual(gdk_rgb_get_visual());
  gtk_widget_push_colormap(gdk_rgb_get_cmap());
  g_assert((gui_data->map_canvas = (GnomeCanvas*)gnome_canvas_new_aa()));
  gtk_widget_pop_colormap();
  gtk_widget_pop_visual();

  // Re-orient everything so it is the right way up
  t[0] = +1; t[2] = 0; t[4] = 0;
  t[1] = 0; t[3] = -1; t[5] = 0;
  gnome_canvas_item_affine_absolute((GnomeCanvasItem*)gnome_canvas_root(gui_data->map_canvas), t);

  // set canvas units to meters, scaled by window size
  gui_data->canvas_zoom = ((DEFAULT_DISPLAY_WIDTH - 20) / 
                           (gui_data->mapdev->width * 
                            gui_data->mapdev->resolution));

  gnome_canvas_set_pixels_per_unit(gui_data->map_canvas, 
                                   gui_data->canvas_zoom);
                                   
  //gnome_canvas_set_center_scroll_region(gui_data->map_canvas, TRUE);
  gnome_canvas_set_scroll_region(gui_data->map_canvas,0,0,
                                 gui_data->mapdev->width * 
                                 gui_data->mapdev->resolution,
                                 gui_data->mapdev->height * 
                                 gui_data->mapdev->resolution);

  gtk_container_add(GTK_CONTAINER(gui_data->map_window),
                    (GtkWidget*)(gui_data->map_canvas));
  gtk_widget_show((GtkWidget*)(gui_data->map_canvas));

  g_signal_connect(G_OBJECT(gui_data->main_window),"delete-event",
                   G_CALLBACK(_quit_callback),NULL);
  g_signal_connect(G_OBJECT(gui_data->main_window),"destroy-event",
                   G_CALLBACK(_quit_callback),NULL);
  g_signal_connect(G_OBJECT(gui_data->main_window),"size-allocate",
                   G_CALLBACK(_resize_window),(void*)gui_data);
}

void
fini_gui(gui_data_t* gui_data)
{
  assert(gui_data->main_window);
  gtk_widget_destroy((GtkWidget*)(gui_data->main_window));
}

/*
 * create the background map image and put it on the canvas
 */
void
create_map_image(gui_data_t* gui_data)
{
  GdkPixbuf* pixbuf;
  static guchar* pixels = NULL;
  int i,j;
  GnomeCanvasItem* imageitem;

  if(pixels)
    free(pixels);
  assert(pixels = (guchar*)malloc(sizeof(unsigned char) * 3 *
                                  gui_data->mapdev->width *
                                  gui_data->mapdev->height));

  for(j=0; j < gui_data->mapdev->height; j++)
  {
    for(i=0; i < gui_data->mapdev->width; i++)
    {
      if(gui_data->mapdev->cells[PLAYERC_MAP_INDEX(gui_data->mapdev,i,j)] == -1)
      {
        pixels[(gui_data->mapdev->width * 
                (gui_data->mapdev->height - j-1) + i)*3] = 255;
        pixels[(gui_data->mapdev->width * 
                (gui_data->mapdev->height - j-1) + i)*3+1] = 255;
        pixels[(gui_data->mapdev->width * 
                (gui_data->mapdev->height - j-1) + i)*3+2] = 255;
      }
      else if(gui_data->mapdev->cells[PLAYERC_MAP_INDEX(gui_data->mapdev,i,j)] == 0)
      {
        pixels[(gui_data->mapdev->width * 
                (gui_data->mapdev->height - j-1) + i)*3] = 100;
        pixels[(gui_data->mapdev->width * 
                (gui_data->mapdev->height - j-1) + i)*3+1] = 100;
        pixels[(gui_data->mapdev->width * 
                (gui_data->mapdev->height - j-1) + i)*3+2] = 100;
      }
      else
      {
        pixels[(gui_data->mapdev->width * 
                (gui_data->mapdev->height - j-1) + i)*3] = 0;
        pixels[(gui_data->mapdev->width * 
                (gui_data->mapdev->height - j-1) + i)*3+1] = 0;
        pixels[(gui_data->mapdev->width * 
                (gui_data->mapdev->height - j-1) + i)*3+2] = 0;
      }
    }
  }
  
  // create the pixbuf
  g_assert((pixbuf = gdk_pixbuf_new_from_data(pixels,
                                              GDK_COLORSPACE_RGB,
                                              FALSE,
                                              8,
                                              gui_data->mapdev->width, 
                                              gui_data->mapdev->height, 
                                              3*gui_data->mapdev->width,
                                              NULL,
                                              NULL)));

  g_assert((imageitem = 
            gnome_canvas_item_new(gnome_canvas_root(gui_data->map_canvas), 
                                  gnome_canvas_pixbuf_get_type(),
                                  "width-set", TRUE,
                                  "height-set", TRUE,
                                  "width", gui_data->mapdev->width *
                                  gui_data->mapdev->resolution,
                                  "height", gui_data->mapdev->height *
                                  gui_data->mapdev->resolution,
                                  "pixbuf", pixbuf,
                                  NULL)));

  g_object_unref((GObject*)pixbuf);
}

