#include <math.h>

#include "playernav.h"

// global quit flag
extern char quit;

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

static void
_zoom_callback(GtkAdjustment* adjustment,
               gpointer data)
{
  double newzoom;
  gui_data_t* gui_data = (gui_data_t*)data;

  newzoom = gtk_adjustment_get_value(adjustment);

  gnome_canvas_set_pixels_per_unit(gui_data->map_canvas, newzoom);
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

  gui_data->zoom_adjustment->lower = 
          allocation->width / (double)(gui_data->mapdev->width);
  gui_data->zoom_adjustment->upper = 10.0 * gui_data->zoom_adjustment->lower;
  gui_data->zoom_adjustment->step_increment = 
          (gui_data->zoom_adjustment->upper - 
           gui_data->zoom_adjustment->lower) / 1e3;
  gui_data->zoom_adjustment->page_increment = 
          (gui_data->zoom_adjustment->upper -
           gui_data->zoom_adjustment->lower) / 1e2;
  gui_data->zoom_adjustment->page_size = 
          gui_data->zoom_adjustment->page_increment;

  if(gtk_adjustment_get_value(gui_data->zoom_adjustment) < 
     gui_data->zoom_adjustment->lower)
    gtk_adjustment_set_value(gui_data->zoom_adjustment,
                                 gui_data->zoom_adjustment->lower);
  else if(gtk_adjustment_get_value(gui_data->zoom_adjustment) > 
          gui_data->zoom_adjustment->upper)
    gtk_adjustment_set_value(gui_data->zoom_adjustment,
                             gui_data->zoom_adjustment->upper);
  gtk_adjustment_value_changed(gui_data->zoom_adjustment);
}

void
init_gui(gui_data_t* gui_data, int argc, char** argv)
{
  double initial_zoom, max_zoom;

  g_type_init();
  gtk_init(&argc, &argv);

  g_assert((gui_data->main_window = 
            (GtkWindow*)gtk_window_new(GTK_WINDOW_TOPLEVEL)));
  gtk_widget_set_size_request((GtkWidget*)(gui_data->main_window),
                              MIN_DISPLAY_WIDTH,MIN_DISPLAY_WIDTH);
  gtk_window_resize(gui_data->main_window,
                    DEFAULT_DISPLAY_WIDTH,
                    (int)rint(DEFAULT_DISPLAY_WIDTH / gui_data->aspect));

  /* a box to hold everything else */
  g_assert((gui_data->hbox = (GtkBox*)gtk_hbox_new(FALSE, 5)));

  g_assert((gui_data->map_window = 
            (GtkScrolledWindow*)gtk_scrolled_window_new(NULL, NULL)));
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gui_data->map_window),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  gtk_widget_push_visual(gdk_rgb_get_visual());
  gtk_widget_push_colormap(gdk_rgb_get_cmap());
  g_assert((gui_data->map_canvas = (GnomeCanvas*)gnome_canvas_new_aa()));
  gtk_widget_pop_colormap();
  gtk_widget_pop_visual();

  // TODO: figure out how to use the transformations
  //
  // Re-orient everything from graphics to right-handed coords
  // (i.e., rotate by pi, then scale x by -1)
  /*
  t[0] = 1.0;
  t[1] = 0.0;
  t[2] = 0.0;
  t[3] = -1.0;
  t[4] = 0.0;
  t[5] = 0.0;
  gnome_canvas_item_affine_absolute((GnomeCanvasItem*)gnome_canvas_root(gui_data->map_canvas), t);
  */

  gnome_canvas_set_center_scroll_region(gui_data->map_canvas, TRUE);
  gnome_canvas_set_scroll_region(gui_data->map_canvas,
                                 -gui_data->mapdev->width/2.0,
                                 -gui_data->mapdev->height/2.0,
                                 gui_data->mapdev->width/2.0,
                                 gui_data->mapdev->height/2.0);

  // the zoom scrollbar

  // set canvas zoom to make the map fill the window
  initial_zoom = DEFAULT_DISPLAY_WIDTH / (double)(gui_data->mapdev->width);
  max_zoom = 10.0 * initial_zoom;

  g_assert((gui_data->zoom_adjustment = 
            (GtkAdjustment*)gtk_adjustment_new(initial_zoom, 
                                               initial_zoom,
                                               max_zoom,
                                               (max_zoom-initial_zoom)/1e3,
                                               (max_zoom-initial_zoom)/1e2,
                                               (max_zoom-initial_zoom)/1e2)));
  g_assert((gui_data->zoom_scrollbar = 
            (GtkVScrollbar*)gtk_vscrollbar_new(gui_data->zoom_adjustment)));

  gtk_container_add(GTK_CONTAINER(gui_data->main_window),
                    (GtkWidget*)(gui_data->hbox));
  gtk_box_pack_start(gui_data->hbox,
                     (GtkWidget*)(gui_data->zoom_scrollbar), 
                     FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(gui_data->map_window),
                    (GtkWidget*)(gui_data->map_canvas));
  gtk_box_pack_start(gui_data->hbox,
                     (GtkWidget*)(gui_data->map_window), 
                     TRUE, TRUE, 0);

  gtk_widget_show((GtkWidget*)(gui_data->hbox));
  gtk_widget_show((GtkWidget*)(gui_data->zoom_scrollbar));
  gtk_widget_show((GtkWidget*)(gui_data->map_window));
  gtk_widget_show((GtkWidget*)(gui_data->map_canvas));

  g_signal_connect(G_OBJECT(gui_data->main_window),"delete-event",
                   G_CALLBACK(_quit_callback),NULL);
  g_signal_connect(G_OBJECT(gui_data->main_window),"destroy-event",
                   G_CALLBACK(_quit_callback),NULL);
  g_signal_connect(G_OBJECT(gui_data->zoom_adjustment),"value-changed",
                   G_CALLBACK(_zoom_callback),(void*)gui_data);


  gtk_adjustment_set_value(gui_data->zoom_adjustment,initial_zoom);
  gtk_adjustment_value_changed(gui_data->zoom_adjustment);
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
        /*
        pixels[(gui_data->mapdev->width * j + i)*3] = 255;
        pixels[(gui_data->mapdev->width * j + i)*3+1] = 255;
        pixels[(gui_data->mapdev->width * j + i)*3+2] = 255;
        */
      }
      else if(gui_data->mapdev->cells[PLAYERC_MAP_INDEX(gui_data->mapdev,i,j)] == 0)
      {
        pixels[(gui_data->mapdev->width * 
                (gui_data->mapdev->height - j-1) + i)*3] = 100;
        pixels[(gui_data->mapdev->width * 
                (gui_data->mapdev->height - j-1) + i)*3+1] = 100;
        pixels[(gui_data->mapdev->width * 
                (gui_data->mapdev->height - j-1) + i)*3+2] = 100;
        /*
        pixels[(gui_data->mapdev->width * j + i)*3] = 100;
        pixels[(gui_data->mapdev->width * j + i)*3+1] = 100;
        pixels[(gui_data->mapdev->width * j + i)*3+2] = 100;
                */
      }
      else
      {
        pixels[(gui_data->mapdev->width * 
                (gui_data->mapdev->height - j-1) + i)*3] = 0;
        pixels[(gui_data->mapdev->width * 
                (gui_data->mapdev->height - j-1) + i)*3+1] = 0;
        pixels[(gui_data->mapdev->width * 
                (gui_data->mapdev->height - j-1) + i)*3+2] = 0;
        /*
        pixels[(gui_data->mapdev->width * j + i)*3] = 0;
        pixels[(gui_data->mapdev->width * j + i)*3+1] = 0;
        pixels[(gui_data->mapdev->width * j + i)*3+2] = 0;
                */
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
                                  "width", (double)gui_data->mapdev->width,
                                  "height", (double)gui_data->mapdev->height,
                                  "x", -gui_data->mapdev->width/2.0,
                                  "y", -gui_data->mapdev->height/2.0,
                                  "pixbuf", pixbuf,
                                  NULL)));

  g_object_unref((GObject*)pixbuf);
}

