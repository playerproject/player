/*
 *  PlayerCam
 *  Copyright (C) Brad Kratochvil 2005
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
 * Desc: PlayerCam
 * Author: Brad Kratochvil
 * Date: 20050902
 * CVS: $Id$
 *************************************************************************/

/** @addtogroup utils Utilities */
/** @{ */
/** @defgroup player_util_playecam playercam

@par Synopsis

Playercam is a gui client that displays images captured from a player camera
device.

@par Usage

playercam is installed alongside player in $prefix/bin, so if player is
in your PATH, then playerv should also be.  Command-line usage is:
@verbatim
$ playercam [options]
@endverbatim

Where [options] can be:\n"
  -help : print this message
  -h <hostname> : host that is running player
  -p <port> : the port number of the host
  -i <index> : the index of the camera
  -r <rate> : the refresh rate of the video

For example, to connect to Player on localhost at the default port
(6665), and subscribe to the 1st camera device:
@verbatim
$ playercam --i=1
@endverbatim


@par Features

playercam can visualize data from devices that support the following
colorspaces:
- @ref PLAYER_CAMERA_FORMAT_MONO8
- @ref PLAYER_CAMERA_FORMAT_RGB888

Any time a user clicks on the image display, the pixel location and color
value at that place will be written to standard out.

@todo todo
- add better vision feedback abilities w/ opencv (directional histogram)
- add blobfinder overlay using the alpha channel
*/

/** @} */

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <gtk/gtk.h>
#include "playerc.h"

char g_hostname[255] = "localhost";
int32_t g_port       = 6665;
int16_t g_index      = 0;
int16_t g_rate       = 30;

playerc_client_t *g_client = NULL;
playerc_camera_t *g_camera = NULL;

uint16_t    g_width = 0;
uint16_t   g_height = 0;
GdkPixbuf *g_pixbuf = NULL;
guchar g_img[PLAYER_CAMERA_IMAGE_SIZE];

int player_init(int argc, char *argv[]);
int player_update();
int player_quit();

int
get_options(int argc, char **argv)
{
  int ch=0, errflg=0;
  const char* optflags = "i:h:p:r:";

  while((ch=getopt(argc, argv, optflags))!=-1)
  {
    switch(ch)
    {
      /* case values must match long_options */
      case 'i':
          g_index = atoi(optarg);
          break;
      case 'h':
          strcpy(g_hostname,optarg);
          break;
      case 'p':
          g_port = atoi(optarg);
          break;
      case 'r':
          g_rate = atoi(optarg);
          break;
      case '?':
      case ':':
      default:
        return (-1);
    }
  }

  return (0);
}

void print_usage()
{
  printf("\n"
         " playercam - camera test utility for a player camera\n\n"
         "USAGE:  playercam [options] \n\n"
         "Where [options] can be:\n"
         "  -help          : print this message.\n"
         "  -h <hostname>  : host that is running player\n"
         "  -p <port>      : the port number of the host\n"
         "  -i <index>     : the index of the camera\n"
         "  -r <rate>      : the refresh rate of the video\n\n"
         "Currently supports RGB888 and 8-bit grey scale images.\n\n");
}


gint
render_camera(gpointer data)
{
  GdkPixbuf *pixbuf = NULL;
  GdkGC         *gc = NULL;
  GtkWidget *drawing_area = GTK_WIDGET(data);
  gc = GTK_WIDGET(drawing_area)->style->fg_gc[GTK_WIDGET_STATE(GTK_WIDGET(drawing_area))];

  player_update();

  if ((g_width==g_camera->width)&&(g_height==g_camera->height))
  { // we don't need to worry about scaling
    gdk_draw_pixbuf(GTK_WIDGET(drawing_area)->window, gc,
                    g_pixbuf, 0, 0, 0, 0, g_width, g_height,
                    GDK_RGB_DITHER_NONE, 0, 0);
  }
  else
  {
    pixbuf = gdk_pixbuf_scale_simple(g_pixbuf, g_width, g_height,
                                     GDK_INTERP_BILINEAR);
    gdk_draw_pixbuf(GTK_WIDGET(drawing_area)->window, gc,
                    pixbuf, 0, 0, 0, 0, g_width, g_height,
                    GDK_RGB_DITHER_NONE, 0, 0);
    gdk_pixbuf_unref(pixbuf);
  }

  return TRUE;
}

gint
resize(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    GtkAllocation *size = (GtkAllocation*)event;
    g_width = size->width;
    g_height = size->height;
    return TRUE;
}

gint
click(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  GdkEventButton *bevent = (GdkEventButton *)event;
  int x,y,o;

  switch ((gint)event->type)
  {
    case GDK_BUTTON_PRESS:
      x = (int)rint(bevent->x/g_width*g_camera->width);
      y = (int)rint(bevent->y/g_height*g_camera->height);
      g_print("[%i, %i] = ", x, y);

      switch (g_camera->format)
      {
        case PLAYER_CAMERA_FORMAT_MONO8:
          /// @todo
          // I'm not 100% sure this works
          // need to check when I get time
          g_print("[%i]\n", g_camera->image[x + g_camera->width*y]);
          break;
        case PLAYER_CAMERA_FORMAT_RGB888:
          /// @todo
          // I'm not 100% sure this works
          // need to check when I get time
          o = x + 3*g_camera->width*y;
          g_print("[%i %i %i]\n",
                  g_camera->image[o],
                  g_camera->image[o+1],
                  g_camera->image[o+2]);
          break;
      }
      return TRUE;
  }
  /* Event not handled; try parent item */
  return FALSE;
}

static
gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  gdk_pixbuf_unref(g_pixbuf);
  return FALSE;
}

static
void destroy(GtkWidget *widget, gpointer data)
{
  gtk_main_quit();
}

int
main(int argc, char *argv[])
{
  GtkWidget *window = NULL;
  GtkWidget *vbox;
  GtkWidget *drawing_area = NULL;

  player_init(argc, argv);
  gtk_init(&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "PlayerCam");

  vbox = gtk_vbox_new(FALSE, 0);
  gtk_container_add (GTK_CONTAINER(window), vbox);
  gtk_widget_show(vbox);

  drawing_area = gtk_drawing_area_new();
  gtk_widget_set_size_request(GTK_WIDGET (drawing_area), g_width, g_height);
  gtk_box_pack_start(GTK_BOX (vbox), drawing_area, TRUE, TRUE, 0);
  gtk_widget_show(drawing_area);

  gtk_widget_show_all(window);

  gtk_widget_add_events(GTK_WIDGET(drawing_area), GDK_BUTTON_PRESS_MASK);

  g_signal_connect(G_OBJECT (window), "delete_event",
                    G_CALLBACK (delete_event), NULL);
  g_signal_connect(G_OBJECT (window), "destroy",
                    G_CALLBACK (destroy), NULL);
  g_signal_connect(GTK_OBJECT(drawing_area), "size-allocate",
                    G_CALLBACK(resize), NULL);
  g_signal_connect(GTK_OBJECT(drawing_area), "event",
                    G_CALLBACK(click), NULL);

  g_pixbuf = gdk_pixbuf_new_from_data(g_img, GDK_COLORSPACE_RGB,
                                      FALSE, 8, g_width, g_height,
                                      g_width * 3, NULL, NULL);

  gtk_idle_add(render_camera, drawing_area);

  gtk_main();

  player_quit();
  return 0;
}

int
player_init(int argc, char *argv[])
{
  int csize, usize;

  if(get_options(argc, argv) < 0)
  {
    print_usage();
    exit(-1);
  }

  // Create a g_client object and connect to the server; the server must
  // be running on "localhost" at port 6665
  g_client = playerc_client_create(NULL, g_hostname, g_port);
  if (0 != playerc_client_connect(g_client))
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    exit(-1);
  }

/*  if (0 != playerc_client_datafreq(g_client, 20))
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    return -1;
  }
*/

  // Create a g_camera proxy (device id "camera:0") and susbscribe
  // in read/write mode
  g_camera = playerc_camera_create(g_client, g_index);
  if (0 != playerc_camera_subscribe(g_camera, PLAYER_OPEN_MODE))
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    exit(-1);
  }

  // Get 1 image for the image
  if (NULL != playerc_client_read(g_client))
  {
    // Decompress the image
    csize = g_camera->image_count;
    playerc_camera_decompress(g_camera);
    usize = g_camera->image_count;

    g_print("camera: [w %d h %d d %d] [%d/%d bytes]\n",
            g_camera->width, g_camera->height, g_camera->bpp, csize, usize);
  }

  g_width  = g_camera->width;
  g_height = g_camera->height;
}

int
player_update()
{
  int i;
  if (NULL != playerc_client_read(g_client))
  {
    // Decompress the image if necessary
    playerc_camera_decompress(g_camera);

    // figure out the colorspace
    switch (g_camera->format)
    {
      case PLAYER_CAMERA_FORMAT_MONO8:
        // we should try to use the alpha layer,
        // but for now we need to change
        // the image data
        for (i=0;i<g_camera->image_count;++i)
        {
          memcpy(g_img+i*3, g_camera->image+i, 3);
        }
        break;
      case PLAYER_CAMERA_FORMAT_RGB888:
        // do nothing
        memcpy(g_img, g_camera->image, g_camera->image_count);
        break;
      default:
        g_print("Unknown camera format: %i\n", g_camera->format);
        exit(-1);
    }
  }
  else
  {
    g_print("ERROR reading player g_client\n");
    exit(-1);
  }
}

int
player_quit()
{
  playerc_camera_unsubscribe(g_camera);
  playerc_camera_destroy(g_camera);
  playerc_client_disconnect(g_client);
  playerc_client_destroy(g_client);
}
