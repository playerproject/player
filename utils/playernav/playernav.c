
#include <math.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <playerc.h>

#define DEFAULT_DISPLAY_WIDTH 800
#define MIN_DISPLAY_WIDTH 10

#define DATA_FREQ 10
#define MAX_NUM_ROBOTS 8
#define MAX_HOSTNAME_LEN 256

#define USAGE "USAGE: playernav <host:port> [<host:port>...]"

GtkWindow* map_window;
GtkImage* map_image;
// aspect ratio (width / height)
double aspect;
int map_display_width, map_display_height;

// global quit flag
char quit;

// forward declarations
static gboolean _quit_callback(GtkWidget *widget,
                               GdkEvent *event,
                               gpointer data);
static void _resize_window(GtkWidget *widget,
                           GtkAllocation* allocation,
                           gpointer data);
int display_map(playerc_map_t* map_device);
void init_gui(int argc, char** argv, playerc_map_t* mapdev);
void fini_gui();
playerc_mclient_t* connect_to_player(playerc_client_t** clients,
                                     playerc_map_t** maps,
                                     int num_bots,
                                     char** hostnames,
                                     int* ports,
                                     int data_freq);
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

  if(parse_args(argc-1, argv+1, &num_bots, hostnames, ports) < 0)
  {
    puts(USAGE);
    exit(-1);
  }

  assert(mclient = connect_to_player(clients, maps, num_bots, 
                                     hostnames, ports, DATA_FREQ));

  // we've read the map, so fill in the aspect ratio
  aspect = maps[0]->width / (double)(maps[0]->height);

  init_gui(argc, argv, maps[0]);

  display_map(maps[0]);

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
connect_to_player(playerc_client_t** clients,
                  playerc_map_t** maps,
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
init_gui(int argc, char** argv, playerc_map_t* mapdev)
{
  g_type_init();
  gtk_init(&argc, &argv);

  map_display_width = DEFAULT_DISPLAY_WIDTH;
  map_display_height = (int)rint(DEFAULT_DISPLAY_WIDTH / aspect);

  g_assert((map_window = (GtkWindow*)gtk_window_new(GTK_WINDOW_TOPLEVEL)));
  gtk_widget_set_size_request((GtkWidget*)map_window,
                              MIN_DISPLAY_WIDTH,MIN_DISPLAY_WIDTH);
  gtk_window_resize(map_window,
                    map_display_width,
                    map_display_height);

  g_assert((map_image = (GtkImage*)gtk_image_new()));
  gtk_container_add(GTK_CONTAINER(map_window),(GtkWidget*)map_image);

  g_signal_connect(G_OBJECT(map_window),"delete-event",
                   G_CALLBACK(_quit_callback),NULL);
  g_signal_connect(G_OBJECT(map_window),"destroy-event",
                   G_CALLBACK(_quit_callback),NULL);
  g_signal_connect(G_OBJECT(map_window),"size-allocate",
                   G_CALLBACK(_resize_window),(void*)mapdev);

}

void
fini_gui()
{
  assert(map_window);
  gtk_widget_destroy((GtkWidget*)map_window);
}

int
display_map(playerc_map_t* map_device)
{
  static char firsttime=1;
  GdkPixbuf* pixbuf;
  GdkPixbuf* scaled_pixbuf;
  static guchar* pixels = NULL;
  int i,j;

  if(firsttime)
  {
    gtk_widget_show((GtkWidget*)map_image);
    gtk_widget_show((GtkWidget*)map_window);
    firsttime=0;
  }

  if(pixels)
    free(pixels);
  assert(pixels = (guchar*)malloc(sizeof(unsigned char) * 3 *
                                  map_device->width *
                                  map_device->height));

  for(j=0; j < map_device->height; j++)
  {
    for(i=0; i < map_device->width; i++)
    {
      if(map_device->cells[PLAYERC_MAP_INDEX(map_device,i,j)] == -1)
      {
        pixels[(map_device->width * (map_device->height - j-1) + i)*3] = 255;
        pixels[(map_device->width * (map_device->height - j-1) + i)*3+1] = 255;
        pixels[(map_device->width * (map_device->height - j-1) + i)*3+2] = 255;
      }
      else if(map_device->cells[PLAYERC_MAP_INDEX(map_device,i,j)] == 0)
      {
        pixels[(map_device->width * (map_device->height - j-1) + i)*3] = 100;
        pixels[(map_device->width * (map_device->height - j-1) + i)*3+1] = 100;
        pixels[(map_device->width * (map_device->height - j-1) + i)*3+2] = 100;
      }
      else
      {
        pixels[(map_device->width * (map_device->height - j-1) + i)*3] = 0;
        pixels[(map_device->width * (map_device->height - j-1) + i)*3+1] = 0;
        pixels[(map_device->width * (map_device->height - j-1) + i)*3+2] = 0;
      }
    }
  }
  
  // create the pixbuf
  if(!(pixbuf = gdk_pixbuf_new_from_data(pixels,
                                         GDK_COLORSPACE_RGB,
                                         FALSE,
                                         8,
                                         map_device->width, 
                                         map_device->height, 
                                         3*map_device->width,
                                         NULL,
                                         NULL)))
  {
    fprintf(stderr, "Failed to create image\n");
    return(-1);
  }

  // scale the pixbuf
  g_assert((scaled_pixbuf = 
            gdk_pixbuf_scale_simple(pixbuf,
                                    map_display_width,
                                    map_display_height,
                                    GDK_INTERP_NEAREST)));


  g_object_unref((GObject*)pixbuf);

  // render to the image
  gtk_image_set_from_pixbuf(map_image,scaled_pixbuf);
  gtk_window_resize(map_window,
                    map_display_width,
                    map_display_height);

  g_object_unref((GObject*)scaled_pixbuf);

  return(0);
}

static gboolean 
_quit_callback(GtkWidget *widget,
               GdkEvent *event,
               gpointer data)
{
  quit = 1;
  return(TRUE);
}

/*
 * Handle window resize events.  Scale and redraw the image(s)
 */
static void 
_resize_window(GtkWidget *widget,
               GtkAllocation* allocation,
               gpointer data)
{
  playerc_map_t* mapdev = (playerc_map_t*)data;

  // which window was resized?
  if(widget == (GtkWidget*)map_window)
  {
    if((map_display_width != allocation->width) ||
       (map_display_height != allocation->height))
    {
      if(map_display_width != allocation->width)
      {
        map_display_width = allocation->width;
        map_display_height = (int)rint(allocation->width / aspect);
      }
      else
      {
        map_display_height = allocation->height;
        map_display_width = (int)rint(allocation->height * aspect);
      }
      display_map(mapdev);
    }
  }
  else
  {
    fprintf(stderr, "Got resize event for unknown widget!\n");
  }
}

