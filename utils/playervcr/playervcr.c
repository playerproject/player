/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2004
 *     Brian Gerkey <gerkey@stanford.edu>
 *                      
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * $Id$
 *
 * A simple GUI for controlling start/stop of data logging/playback
 */

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include <gtk/gtk.h>

#include <playerc.h>

#define USAGE "USAGE: playervcr [-h <host>] [-p <port>] [-i <index>]"
#define MAX_HOSTNAME_LEN 256

char quit;

typedef struct
{
  char hostname[MAX_HOSTNAME_LEN];
  int port;
  int index;

  GtkWindow* main_window;
  GtkBox* vbox;
  GtkBox* hbox;
  GtkFrame* label_frame;
  GtkLabel* label;
  GtkButton* rewindbutton;
  GtkButton* playbutton;
  GtkButton* stopbutton;
  GtkButton* quitbutton;

  playerc_client_t* client;
  playerc_log_t* log;
  int isreadlog;
} gui_data_t;

int parse_args(gui_data_t* gui_data, int argc, char** argv);
void init_gui(gui_data_t* gui_data, int argc, char** argv);
int init_player(gui_data_t* gui_data);
void fini_player(gui_data_t* gui_data);
void button_callback(GtkWidget *widget, gpointer data);
void update_status_label(gui_data_t* gui_data);

int
main(int argc, char** argv)
{
  int peek_result;
  gui_data_t gui_data;

  if(parse_args(&gui_data, argc, argv) < 0)
  {
    puts(USAGE);
    exit(-1);
  }


  if(init_player(&gui_data) < 0)
    exit(-1);

  init_gui(&gui_data, argc, argv);

  gtk_widget_show_all((GtkWidget*)(gui_data.main_window));

  while(!quit)
  {
    // non-blocking GTK event servicing
    while(gtk_events_pending())
      gtk_main_iteration_do(0);

    if((peek_result = playerc_client_peek(gui_data.client,10)) < 0)
    {
      fprintf(stderr, "Error: Failed to peek at Player socket\n");
      exit(-1);
    }
    if(peek_result && (playerc_client_read(gui_data.client) < 0))
    {
      fprintf(stderr, "Error: Failed to read from Player\n");
      exit(-1);
    }
  }

  fini_player(&gui_data);

  return(0);
}

int
parse_args(gui_data_t* gui_data, int argc, char** argv)
{
  int i;

  strcpy(gui_data->hostname,"localhost");
  gui_data->port = PLAYER_PORTNUM;
  gui_data->index = 0;

  for(i=1; i<argc; i++)
  {
    if(!strcmp(argv[i],"-h"))
    {
      if(++i < argc)
        strcpy(gui_data->hostname, argv[i]);
      else
        return(-1);
    }
    else if(!strcmp(argv[i],"-p"))
    {
      if(++i < argc)
        gui_data->port = atoi(argv[i]);
      else
        return(-1);
    }
    else if(!strcmp(argv[i],"-i"))
    {
      if(++i < argc)
        gui_data->index = atoi(argv[i]);
      else
        return(-1);
    }
    else
      return(-1);
  }

  return(0);
}

void
init_gui(gui_data_t* gui_data, int argc, char** argv)
{
  char titlebuf[2*MAX_HOSTNAME_LEN];

  g_type_init();
  gtk_init(&argc, &argv);

  g_assert((gui_data->main_window = 
            (GtkWindow*)gtk_window_new(GTK_WINDOW_TOPLEVEL)));
  sprintf(titlebuf,"playervcr -- %s:%d", 
          gui_data->hostname, gui_data->port);
  gtk_window_set_title(gui_data->main_window, titlebuf);

  /* boxes to hold everything else */
  g_assert((gui_data->vbox = (GtkBox*)gtk_vbox_new(FALSE, 15)));
  g_assert((gui_data->hbox = (GtkBox*)gtk_hbox_new(FALSE, 10)));

  /* a status label */
  g_assert((gui_data->label_frame = (GtkFrame*)gtk_frame_new(NULL)));
  g_assert((gui_data->label = (GtkLabel*)gtk_label_new("")));

  update_status_label(gui_data);

  /* create the buttons */
  if(gui_data->log->type == PLAYER_LOG_TYPE_READ)
  {
    g_assert((gui_data->rewindbutton = 
              (GtkButton*)gtk_button_new_with_label("gtk-go-back")));
    g_assert((gui_data->playbutton = 
              (GtkButton*)gtk_button_new_with_label("gtk-execute")));
  }
  else
  {
    gui_data->rewindbutton = NULL;
    g_assert((gui_data->playbutton = 
              (GtkButton*)gtk_button_new_with_label("gtk-save")));
  }
  g_assert((gui_data->stopbutton = 
            (GtkButton*)gtk_button_new_with_label("gtk-stop")));
  g_assert((gui_data->quitbutton = 
            (GtkButton*)gtk_button_new_with_label("gtk-quit")));

  if(gui_data->log->type == PLAYER_LOG_TYPE_READ)
    gtk_button_set_use_stock(gui_data->rewindbutton,TRUE);
  gtk_button_set_use_stock(gui_data->playbutton,TRUE);
  gtk_button_set_use_stock(gui_data->stopbutton,TRUE);
  gtk_button_set_use_stock(gui_data->quitbutton,TRUE);

  /* hook them up to callbacks */
  if(gui_data->log->type == PLAYER_LOG_TYPE_READ)
    gtk_signal_connect(GTK_OBJECT(gui_data->rewindbutton), "clicked",
                       (GtkSignalFunc)(button_callback),(void*)gui_data);
  gtk_signal_connect(GTK_OBJECT(gui_data->playbutton), "clicked",
                     (GtkSignalFunc)(button_callback),(void*)gui_data);
  gtk_signal_connect(GTK_OBJECT(gui_data->stopbutton), "clicked",
                     (GtkSignalFunc)(button_callback),(void*)gui_data);
  gtk_signal_connect(GTK_OBJECT(gui_data->quitbutton), "clicked",
                     (GtkSignalFunc)(button_callback),(void*)gui_data);

  /* pack them */
  if(gui_data->log->type == PLAYER_LOG_TYPE_READ)
    gtk_box_pack_start(gui_data->hbox, (GtkWidget*)gui_data->rewindbutton, 
                       FALSE, FALSE, 0);
  gtk_box_pack_start(gui_data->hbox, (GtkWidget*)gui_data->playbutton, 
                     FALSE, FALSE, 0);
  gtk_box_pack_start(gui_data->hbox, (GtkWidget*)gui_data->stopbutton, 
                     FALSE, FALSE, 0);
  gtk_box_pack_start(gui_data->hbox, (GtkWidget*)gui_data->quitbutton, 
                     FALSE, FALSE, 0);
  gtk_box_pack_start(gui_data->vbox, (GtkWidget*)gui_data->hbox, 
                     FALSE, FALSE, 0);
  gtk_container_add(GTK_CONTAINER(gui_data->label_frame),
                    (GtkWidget*)(gui_data->label));
  gtk_box_pack_start(gui_data->vbox, (GtkWidget*)gui_data->label_frame, 
                     TRUE, TRUE, 0);
  gtk_container_add(GTK_CONTAINER(gui_data->main_window),
                    (GtkWidget*)(gui_data->vbox));
}

int
init_player(gui_data_t* gui_data)
{
  assert(gui_data->client = 
         playerc_client_create(NULL, gui_data->hostname, gui_data->port));

  if(playerc_client_connect(gui_data->client) < 0)
  {
    fprintf(stderr, "Error: Failed to connect to %s:%d\n", 
            gui_data->hostname, gui_data->port);
    return(-1);
  }

  assert(gui_data->log = 
         playerc_log_create(gui_data->client, gui_data->index));
  if(playerc_log_subscribe(gui_data->log, PLAYER_READ_MODE) < 0)
  {
    fprintf(stderr, "Error: Failed to subscribe to log device\n");
    return(-1);
  }

  return(0);
}

void
fini_player(gui_data_t* gui_data)
{
  playerc_log_unsubscribe(gui_data->log);
  playerc_log_destroy(gui_data->log);
  playerc_client_disconnect(gui_data->client);
  playerc_client_destroy(gui_data->client);
}

void 
button_callback(GtkWidget *widget, gpointer data)
{
  gui_data_t* gui_data;

  gui_data = (gui_data_t*)data;

  if((GtkButton*)widget == gui_data->playbutton)
  {
    if(gui_data->log->type == PLAYER_LOG_TYPE_READ)
    {
      if(playerc_log_set_read_state(gui_data->log,1) < 0)
      {
        fprintf(stderr, "Error: Failed to start playback\n");
        quit = 1;
      }
    }
    else
    {
      if(playerc_log_set_write_state(gui_data->log,1) < 0)
      {
        fprintf(stderr, "Error: Failed to start logging\n");
        quit = 1;
      }
    }
  }
  else if((GtkButton*)widget == gui_data->rewindbutton)
  {
    if(gui_data->log->type == PLAYER_LOG_TYPE_READ)
    {
      if(playerc_log_set_read_rewind(gui_data->log) < 0)
      {
        fprintf(stderr, "Error: Failed to rewind playback\n");
        quit = 1;
      }
    }
    else
    {
      puts("Warning: Can't rewind while writing");
    }
  }
  else if((GtkButton*)widget == gui_data->stopbutton)
  {
    if(gui_data->log->type == PLAYER_LOG_TYPE_WRITE)
    {
      if(playerc_log_set_write_state(gui_data->log,0) < 0)
      {
        fprintf(stderr, "Error: Failed to stop logging\n");
        quit = 1;
      }
    }
    else
    {
      if(playerc_log_set_read_state(gui_data->log,0) < 0)
      {
        fprintf(stderr, "Error: Failed to stop playback\n");
        quit = 1;
      }
    }
  }
  else if((GtkButton*)widget == gui_data->quitbutton)
    quit=1;
  else
    puts("MOO!");

  update_status_label(gui_data);
}

void
update_status_label(gui_data_t* gui_data)
{
  // Get the state
  if(playerc_log_get_state(gui_data->log) < 0)
  {
    fprintf(stderr, "Error: Failed to get log type/state\n");
    quit = 1;
    return;
  }

  if(gui_data->log->type == PLAYER_LOG_TYPE_READ)
  {
    if(gui_data->log->state)
      gtk_label_set_text(gui_data->label, "Playback: started");
    else
      gtk_label_set_text(gui_data->label, "Playback: stopped");
  }
  else
  {
    if(gui_data->log->state)
      gtk_label_set_text(gui_data->label, "Logging: started");
    else
      gtk_label_set_text(gui_data->label, "Logging: stopped");
  }
}
