#include <math.h>

#include "playernav.h"

// global quit flag
extern char quit;

// flag and index for robot currently being moved by user (if any)
extern int robot_moving_p;
extern int robot_moving_idx;

#define ROBOT_ALPHA 128
guint32 robot_colors[] = { GNOME_CANVAS_COLOR_A(255,0,0,ROBOT_ALPHA),
                           GNOME_CANVAS_COLOR_A(0,255,0,ROBOT_ALPHA),
                           GNOME_CANVAS_COLOR_A(0,0,255,ROBOT_ALPHA),
                           GNOME_CANVAS_COLOR_A(255,0,255,ROBOT_ALPHA),
                           GNOME_CANVAS_COLOR_A(255,255,0,ROBOT_ALPHA),
                           GNOME_CANVAS_COLOR_A(0,255,255,ROBOT_ALPHA) };
size_t num_robot_colors = sizeof(robot_colors) / sizeof(robot_colors[0]);


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
_resize_window_callback(GtkWidget *widget,
               GtkAllocation* allocation,
               gpointer data)
{
  gui_data_t* gui_data = (gui_data_t*)data;

  gui_data->zoom_adjustment->lower = 
          allocation->width / 
          (gui_data->mapdev->width * gui_data->mapdev->resolution);
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

static gboolean
_robot_button_callback(GnomeCanvasItem *item,
                       GdkEvent *event,
                       gpointer data)
{
  static int idx;
  gboolean onrobot=FALSE;
  double theta;
  static gboolean dragging=FALSE;
  static gboolean setting_theta=FALSE;
  static gboolean setting_goal=FALSE;
  static GnomeCanvasPoints* points = NULL;
  static GnomeCanvasItem* setting_theta_line = NULL;
  pose_t pose;
  double mean[3];
  static double cov[3][3] = {{0.5*0.5, 0.0, 0.0},
                             {0.0, 0.5*0.5, 0.0},
                             {0.0, 0.0, (M_PI/6.0)*(M_PI/6.0)}};

  gui_data_t* gui_data = (gui_data_t*)data;

  pose.pa = 0.0;
  pose.px = event->button.x;
  pose.py = -event->button.y;
  
  // lookup (and store) which robot was clicked
  if(item != (GnomeCanvasItem*)gnome_canvas_root(gui_data->map_canvas))
  {
    for(idx=0;idx<gui_data->num_robots;idx++)
    {
      if(item == gui_data->robot_items[idx])
        break;
    }
    assert(idx < gui_data->num_robots);
    gnome_canvas_item_hide(gui_data->robot_labels[idx]);
    onrobot=TRUE;
  }

  switch(event->type)
  {
    case GDK_BUTTON_PRESS:
      switch(event->button.button)
      {
        case 3:
          if(onrobot && !setting_theta)
          {
            setting_goal=TRUE;
            move_robot(gui_data->robot_goals[idx],pose);
            gnome_canvas_item_show(gui_data->robot_goals[idx]);
          }
        case 1:
          if(item == (GnomeCanvasItem*)gnome_canvas_root(gui_data->map_canvas))
          {
            if(setting_theta)
            {
              theta = atan2(-points->coords[3] + points->coords[1],
                            points->coords[2] - points->coords[0]);

              mean[0] = points->coords[0];
              mean[1] = -points->coords[1];
              mean[2] = theta;

              if(!setting_goal)
              {
                if(gui_data->localizes[idx])
                {
                  printf("setting pose for robot %d to (%.3f, %.3f, %.3f)\n",
                         idx, mean[0], mean[1], mean[2]);

                  if(playerc_localize_set_pose(gui_data->localizes[idx], 
                                               mean, cov) < 0)
                  {
                    fprintf(stderr, "error while setting pose on robot %d\n", 
                            idx);
                    quit=1;
                    return(TRUE);
                  }
                }
                else
                {
                  puts("WARNING: NOT setting pose; couldn't connect to localize\n");
                }
              }
              else
              {
                if(gui_data->planners[idx])
                {
                  printf("setting goal for robot %d to (%.3f, %.3f, %.3f)\n",
                         idx, mean[0], mean[1], mean[2]);
                  if(playerc_planner_set_cmd_pose(gui_data->planners[idx],
                                                   mean[0], mean[1], 
                                                   mean[2], 1) < 0)
                  {
                    fprintf(stderr, "error while setting goal on robot %d\n", 
                            idx);
                    quit=1;
                    return(TRUE);
                  }
                  gui_data->goals[idx][0] = mean[0];
                  gui_data->goals[idx][1] = mean[1];
                  gui_data->goals[idx][2] = mean[2];
                  gui_data->planners[idx]->waypoint_count = -1;
                }
                else
                {
                  puts("WARNING: NOT setting goal; couldn't connect to planner\n");
                }
              }

              //move_robot(gui_data->robot_items[idx],pose);
              gnome_canvas_item_hide(setting_theta_line);
              setting_theta = FALSE;
              setting_goal = FALSE;

              robot_moving_p = 0;
            }
          }
          else
          {
            gnome_canvas_item_grab(item,
                                   GDK_POINTER_MOTION_MASK | 
                                   GDK_BUTTON_RELEASE_MASK,
                                   NULL, event->button.time);
            dragging = TRUE;

            // set these so that the robot's pose won't be updated in
            // the GUI while we're dragging  the robot (that happens
            // in playernav.c)
            if(event->button.button == 1)
            {
              robot_moving_p = 1;
              robot_moving_idx = idx;
            }
          }
          break;
        default:
          break;
      }
      break;
    case GDK_MOTION_NOTIFY:
      if(onrobot)
        gnome_canvas_item_show(gui_data->robot_labels[idx]);
      if(dragging)
      {
        if(!setting_goal)
          move_robot(item,pose);
        else
          move_robot(gui_data->robot_goals[idx],pose);
      }
      else if(setting_theta)
      {
        points->coords[2] = pose.px;
        points->coords[3] = -pose.py;
        gnome_canvas_item_set(setting_theta_line,
                              "points",points,
                              NULL);
      }
      break;
    case GDK_BUTTON_RELEASE:
      if(dragging)
      {
        dragging = FALSE;
        setting_theta = TRUE;

        if(!points)
          g_assert((points = gnome_canvas_points_new(2)));
        points->coords[0] = pose.px;
        points->coords[1] = -pose.py;
        points->coords[2] = pose.px;
        points->coords[3] = -pose.py;
        if(!setting_theta_line)
        {
          g_assert((setting_theta_line = 
                  gnome_canvas_item_new(gnome_canvas_root(gui_data->map_canvas),
                                        gnome_canvas_line_get_type(),
                                        "points", points,
                                        "width_pixels", 1,
                                        "fill-color-rgba", COLOR_BLACK,
                                        NULL)));
        }
        else
        {
          gnome_canvas_item_set(setting_theta_line,
                                "points",points,
                                NULL);
          gnome_canvas_item_show(setting_theta_line);
        }

        gnome_canvas_item_ungrab(item, event->button.time);
      }
      break;
    default:
      break;
  }

  return(TRUE);
}

void
canvas_to_meters(gui_data_t* gui_data, double* dx, double* dy, int cx, int cy)
{
  gnome_canvas_c2w(gui_data->map_canvas,cx,cy,dx,dy);
  *dy = -*dy;
}

void
item_to_meters(GnomeCanvasItem* item,
               double* dx, double* dy, 
               double ix, double iy)
{
  *dx=ix;
  *dy=iy;
  gnome_canvas_item_i2w(item, dx, dy);
  *dy = -*dy;
}

void
meters_to_canvas(gui_data_t* gui_data, int* cx, int* cy, double dx, double dy)
{
  double x,y;
  x=dx;
  y=-dy;
  gnome_canvas_w2c(gui_data->map_canvas,x,y,cx,cy);
}

void
make_menu(gui_data_t* gui_data)
{
  GtkMenuBar* menu_bar;
  GtkMenu* file_menu;
  //GtkMenuItem* open_item;
  //GtkMenuItem* save_item;
  GtkMenuItem* quit_item;
  GtkMenuItem* file_item;

  file_menu = (GtkMenu*)gtk_menu_new();    /* Don't need to show menus */

  /* Create the menu items */
  //open_item = (GtkMenuItem*)gtk_menu_item_new_with_label ("Open");
  //save_item = (GtkMenuItem*)gtk_menu_item_new_with_label ("Save");
  quit_item = (GtkMenuItem*)gtk_menu_item_new_with_label ("Quit");

  /* Add them to the menu */
  //gtk_menu_shell_append (GTK_MENU_SHELL(file_menu), (GtkWidget*)open_item);
  //gtk_menu_shell_append (GTK_MENU_SHELL(file_menu), (GtkWidget*)save_item);
  gtk_menu_shell_append (GTK_MENU_SHELL(file_menu), (GtkWidget*)quit_item);

  /* Attach the callback functions to the
   * activate signal */
  /*
  g_signal_connect_swapped (G_OBJECT (open_item), "activate",
                            G_CALLBACK (menuitem_response),
                            (gpointer) "file.open");
  g_signal_connect_swapped (G_OBJECT (save_item), "activate",
                            G_CALLBACK (menuitem_response),
                            (gpointer) "file.save");
                            */

  /* We can attach the Quit menu item to
   * our exit function */
  g_signal_connect_swapped (G_OBJECT (quit_item), "activate",
                            G_CALLBACK(_quit_callback),
                            (gpointer) "file.quit");

  /* We do need to show menu items */
  //gtk_widget_show((GtkWidget*)open_item);
  //gtk_widget_show((GtkWidget*)save_item);
  gtk_widget_show((GtkWidget*)quit_item);

  menu_bar = (GtkMenuBar*)gtk_menu_bar_new ();
  gtk_box_pack_start(gui_data->vbox,
                     (GtkWidget*)(menu_bar),
                     FALSE, FALSE, 0);
  gtk_widget_show((GtkWidget*)menu_bar);

  file_item = (GtkMenuItem*)gtk_menu_item_new_with_label ("File");
  gtk_widget_show((GtkWidget*)file_item);

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), (GtkWidget*)file_menu);
  gtk_menu_bar_append(GTK_MENU_BAR (menu_bar), (GtkWidget*)file_item);
}

void
init_gui(gui_data_t* gui_data, int argc, char** argv)
{
  //double t[6];
  double initial_zoom, max_zoom;
  GtkAdjustment* adjust;

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
  g_assert((gui_data->vbox = (GtkBox*)gtk_vbox_new(FALSE, 5)));

  /* a box to hold everything else */
  g_assert((gui_data->hbox = (GtkBox*)gtk_hbox_new(FALSE, 5)));

  g_assert((gui_data->map_window = 
            (GtkScrolledWindow*)gtk_scrolled_window_new(NULL, NULL)));
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(gui_data->map_window),
                                 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  // Initialize horizontal scroll bars
  adjust = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(gui_data->map_window));
  adjust->step_increment = 5;
  gtk_adjustment_changed(adjust);
  gtk_adjustment_set_value(adjust, adjust->value - GTK_WIDGET(gui_data->map_window)->allocation.width / 2);

  // Initialize vertical scroll bars
  adjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(gui_data->map_window));
  adjust->step_increment = 5;
  gtk_adjustment_changed(adjust);
  gtk_adjustment_set_value(adjust, adjust->value - GTK_WIDGET(gui_data->map_window)->allocation.height / 2);

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
                                 -(gui_data->mapdev->width * 
                                   gui_data->mapdev->resolution)/2.0,
                                 -(gui_data->mapdev->height *
                                   gui_data->mapdev->resolution)/2.0,
                                 (gui_data->mapdev->width *
                                   gui_data->mapdev->resolution)/2.0,
                                 (gui_data->mapdev->width *
                                   gui_data->mapdev->resolution)/2.0);

  // the zoom scrollbar

  // set canvas zoom to make the map fill the window
  initial_zoom = DEFAULT_DISPLAY_WIDTH / 
          (gui_data->mapdev->width * gui_data->mapdev->resolution);
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
                    (GtkWidget*)(gui_data->vbox));
  make_menu(gui_data);

  gtk_box_pack_start(gui_data->vbox,
                     (GtkWidget*)(gui_data->hbox), 
                     TRUE, TRUE, 0);
  gtk_box_pack_start(gui_data->hbox,
                     (GtkWidget*)(gui_data->zoom_scrollbar), 
                     FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(gui_data->map_window),
                    (GtkWidget*)(gui_data->map_canvas));
  gtk_box_pack_start(gui_data->hbox,
                     (GtkWidget*)(gui_data->map_window), 
                     TRUE, TRUE, 0);

  gtk_widget_show((GtkWidget*)(gui_data->vbox));
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

  gtk_signal_connect(GTK_OBJECT(gnome_canvas_root(gui_data->map_canvas)),
                     "event",
                     (GtkSignalFunc)(_robot_button_callback),(void*)gui_data);

  gtk_adjustment_set_value(gui_data->zoom_adjustment,initial_zoom);
  gtk_adjustment_value_changed(gui_data->zoom_adjustment);
  g_signal_connect(G_OBJECT(gui_data->main_window),"size-allocate",
                   G_CALLBACK(_resize_window_callback),(void*)gui_data);
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
                                  "width", gui_data->mapdev->width *
                                  gui_data->mapdev->resolution,
                                  "height", gui_data->mapdev->height *
                                  gui_data->mapdev->resolution,
                                  "x", -(gui_data->mapdev->width *
                                         gui_data->mapdev->resolution)/2.0,
                                  "y", -(gui_data->mapdev->height *
                                         gui_data->mapdev->resolution)/2.0,
                                  "pixbuf", pixbuf,
                                  NULL)));

  g_object_unref((GObject*)pixbuf);
}

void
create_robot(gui_data_t* gui_data, int idx, pose_t pose)
{
  GnomeCanvasGroup* robot;
  GnomeCanvasItem* robot_circle;
  GnomeCanvasItem* robot_v;
  GnomeCanvasItem* robot_text;
  GnomeCanvasItem* robot_goal;
  GnomeCanvasPoints* points;
  char robotname[256];

  assert(idx < gui_data->num_robots);

  g_assert((robot = (GnomeCanvasGroup*)
            gnome_canvas_item_new(gnome_canvas_root(gui_data->map_canvas),
                                  gnome_canvas_group_get_type(),
                                  "x", 0.0, "y", 0.0,
                                  NULL)));

  g_assert((robot_circle = 
            gnome_canvas_item_new(robot,
                                  gnome_canvas_ellipse_get_type(),
                                  "x1", -ROBOT_RADIUS,
                                  "y1", -ROBOT_RADIUS,
                                  "x2",  ROBOT_RADIUS,
                                  "y2",  ROBOT_RADIUS,
                                  "outline_color_rgba", COLOR_BLACK,
                                  "fill_color_rgba", 
                                  robot_colors[idx % num_robot_colors],
                                  "width_pixels", 1,
                                  NULL)));
  g_assert((points = gnome_canvas_points_new(3)));
  points->coords[0] = ROBOT_RADIUS * cos(ROBOT_V_ANGLE);
  points->coords[1] = ROBOT_RADIUS * sin(ROBOT_V_ANGLE);
  points->coords[2] = 0.0;
  points->coords[3] = 0.0;
  points->coords[4] = ROBOT_RADIUS * cos(ROBOT_V_ANGLE);
  points->coords[5] = ROBOT_RADIUS * sin(-ROBOT_V_ANGLE);

  g_assert((robot_v = 
            gnome_canvas_item_new(robot,
                                  gnome_canvas_line_get_type(),
                                  "points", points,
                                  "fill_color_rgba", COLOR_BLACK,
                                  "width_pixels", 1,
                                  NULL)));

  // a triangle to mark the goal
  points->coords[0] = ROBOT_RADIUS * cos(M_PI/2.0);
  points->coords[1] = ROBOT_RADIUS * sin(M_PI/2.0);
  points->coords[2] = ROBOT_RADIUS * cos(7*M_PI/6.0);
  points->coords[3] = ROBOT_RADIUS * sin(7*M_PI/6.0);
  points->coords[4] = ROBOT_RADIUS * cos(11*M_PI/6.0);
  points->coords[5] = ROBOT_RADIUS * sin(11*M_PI/6.0);

  g_assert((robot_goal = 
            gnome_canvas_item_new(gnome_canvas_root(gui_data->map_canvas),
                                  gnome_canvas_polygon_get_type(),
                                  "points", points,
                                  "outline_color_rgba", COLOR_BLACK,
                                  "fill_color_rgba", 
                                  robot_colors[idx % num_robot_colors],
                                  "width_pixels", 1,
                                  NULL)));
  gnome_canvas_item_hide(robot_goal);


  gnome_canvas_points_unref(points);


  sprintf(robotname, "%s:%d", 
          gui_data->hostnames[idx], gui_data->ports[idx]);
  g_assert((robot_text =
            gnome_canvas_item_new(robot,
                                  gnome_canvas_text_get_type(),
                                  "text", robotname,
                                  "x", 0.0,
                                  "y", 0.0,
                                  "x-offset", 2.0*ROBOT_RADIUS,
                                  "y-offset", -2.0*ROBOT_RADIUS,
                                  "fill-color-rgba", COLOR_BLACK,
                                  NULL)));
  gnome_canvas_item_hide(robot_text);

  move_robot((GnomeCanvasItem*)robot,pose);

  gui_data->robot_items[idx] = (GnomeCanvasItem*)robot;
  gui_data->robot_labels[idx] = robot_text;
  gui_data->robot_goals[idx] = robot_goal;

  gtk_signal_connect(GTK_OBJECT(robot), "event",
                     (GtkSignalFunc)_robot_button_callback, (void*)gui_data);
}

void
move_robot(GnomeCanvasItem* item, pose_t pose)
{
  double t[6];

  t[0] = cos(pose.pa);
  t[1] = -sin(pose.pa);
  t[2] = sin(pose.pa);
  t[3] = cos(pose.pa);
  t[4] = pose.px;
  t[5] = -pose.py;
  gnome_canvas_item_affine_absolute(item, t);
  gnome_canvas_item_raise_to_top(item);
}

void
draw_waypoints(gui_data_t* gui_data, int idx)
{
  int i;
  GnomeCanvasPoints* points;
  GnomeCanvasPoints* linepoints;
  GnomeCanvasItem* waypoint;
  GnomeCanvasItem* line;
  pose_t pose;

  if(gui_data->robot_paths[idx])
  {
    gtk_object_destroy(GTK_OBJECT(gui_data->robot_paths[idx]));
    gui_data->robot_paths[idx] = NULL;
  }

  if(gui_data->planners[idx]->path_valid && 
     !gui_data->planners[idx]->path_done)
  {
    g_assert((gui_data->robot_paths[idx] = 
              gnome_canvas_item_new(gnome_canvas_root(gui_data->map_canvas),
                                    gnome_canvas_group_get_type(),
                                    "x", 0.0, "y", 0.0,
                                    NULL)));

    g_assert((points = gnome_canvas_points_new(3)));
    g_assert((linepoints = gnome_canvas_points_new(2)));

    // a small triangle to mark each waypoint
    points->coords[0] = 0.5 * ROBOT_RADIUS * cos(M_PI/2.0);
    points->coords[1] = 0.5 * ROBOT_RADIUS * sin(M_PI/2.0);
    points->coords[2] = 0.5 * ROBOT_RADIUS * cos(7*M_PI/6.0);
    points->coords[3] = 0.5 * ROBOT_RADIUS * sin(7*M_PI/6.0);
    points->coords[4] = 0.5 * ROBOT_RADIUS * cos(11*M_PI/6.0);
    points->coords[5] = 0.5 * ROBOT_RADIUS * sin(11*M_PI/6.0);

    for(i=MAX(gui_data->planners[idx]->curr_waypoint-1,0);
        i<gui_data->planners[idx]->waypoint_count;
        i++)
    {
      g_assert((waypoint = 
                gnome_canvas_item_new((GnomeCanvasGroup*)gui_data->robot_paths[idx],
                                      gnome_canvas_polygon_get_type(),
                                      "points", points,
                                      "outline_color_rgba", COLOR_BLACK,
                                      "fill_color_rgba", 
                                      robot_colors[idx % num_robot_colors],
                                      "width_pixels", 1,
                                      NULL)));

      pose.px =  gui_data->planners[idx]->waypoints[i][0];
      pose.py =  gui_data->planners[idx]->waypoints[i][1];
      pose.pa = 0.0;
      move_robot(waypoint, pose);

      if(i>0)
      {
        linepoints->coords[0] = gui_data->planners[idx]->waypoints[i-1][0];
        linepoints->coords[1] = -gui_data->planners[idx]->waypoints[i-1][1];
        linepoints->coords[2] = gui_data->planners[idx]->waypoints[i][0];
        linepoints->coords[3] = -gui_data->planners[idx]->waypoints[i][1];

        g_assert((line = 
                  gnome_canvas_item_new((GnomeCanvasGroup*)gui_data->robot_paths[idx],
                                        gnome_canvas_line_get_type(),
                                        "points", linepoints,
                                        "width_pixels", 3,
                                        "fill-color-rgba",
                                        robot_colors[idx % num_robot_colors],
                                        NULL)));
      }
    }

    gnome_canvas_points_unref(points);
    gnome_canvas_points_unref(linepoints);
  }
}

