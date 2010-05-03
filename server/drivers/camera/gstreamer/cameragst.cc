/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey et al.
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
///////////////////////////////////////////////////////////////////////////
//
// Desc: GStreamer camera plugin
// Author: Piotr Trojanek
// $Date: 2008-12-09 16:47:18 -0500 (Tue, 09 Dec 2008) $
// $Id: gstreamplayer.cc 63 2008-12-09 21:47:18Z ptrojane $
//
///////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_gstreamer gstreamer
 * @brief Gstreamer driver

This driver takes image data from Gstreamer infrastructure and publish them
through provided camera interface.

@par Compile-time dependencies

- GStreamer

@par Provides

- This driver supports the @ref interface_camera interface.

@par Requires

- none

@par Configuration requests

- none

@par Configuration file options

- source (string)
  - Default: ""
  - A string describing GStreamer source.

- trace (bool)
  - Default: false
  - Trace gstreamer object allocation.

- jpeg (integer)
  - Default: 0
  - If set to 1, expect and publish JPEG images

@par Example

@verbatim
driver
(
  name "cameragst"
  provides ["camera:0"]

  # read/write image sequence
  # source "multifilesrc location=%05d.jpg ! jpegdec ! ffmpegcolorspace ! queue"
  # source "v4l2src ! video/x-raw-yuv,width=640,height=480 ! tee name=tee ! ffmpegcolorspace ! jpegenc ! multifilesink location=%05d.jpg tee. ! queue ! ffmpegcolorspace"

  # read/write movie file
  # source "filesrc location=movie.ogg ! decodebin ! ffmpegcolorspace"
  # source "v4l2src ! video/x-raw-yuv,width=640,height=480 ! tee name=tee tee. ! ffmpegcolorspace ! theoraenc ! queue ! oggmux name=mux mux. ! queue ! filesink location=movie.ogg tee. ! ffmpegcolorspace ! queue"

  # read data from V4L2 compatibile camera (UVC works too!)
  # source "v4l2src ! videoscale ! video/x-raw-yuv, width=640, height=480 ! ffmpegcolorspace"

  # test source
  source "videotestsrc ! timeoverlay ! ffmpegcolorspace"

  trace false
)
@endverbatim


@todo Add support for mono images

@author Piotr Trojanek

*/
/** @} */

#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <libplayercore/playercore.h>

#include <gst/gst.h>
#include "gstappsink.h"

////////////////////////////////////////////////////////////////////////////////
// The class for the driver
class GStreamerDriver : public ThreadedDriver
{
  public:

    // Constructor; need that
    GStreamerDriver(ConfigFile* cf, int section);

    ~GStreamerDriver();

    // Must implement the following methods.
    virtual int MainSetup ();
    virtual void MainQuit ();

  private:

    // Main function for device thread.
    virtual void Main();

    // GStreamer source description
    const char *source;

    // GStreamer elements
    GstElement *pipeline, *launchpipe, *sink;

    // Handle GStreamer bus messages
    void HandleMessage();
    int GrabFrame();
    int RetrieveFrame();

    // Data buffer
    GstBuffer *buffer;

    // Data to send to server
    player_camera_data_t data;

    // Retrieved image size
    size_t image_size;

    int jpeg;

    // Track object allocation
    gboolean trace;

    static int initialized;
};

int GStreamerDriver::initialized = 0;

// A factory creation function, declared outside of the class so that it
// can be invoked without any object context (alternatively, you can
// declare it static in the class).  In this function, we create and return
// (as a generic Driver*) a pointer to a new instance of this driver.
Driver*
GStreamerDriver_Init(ConfigFile* cf, int section)
{
  // Create and return a new instance of this driver
  return((Driver*)(new GStreamerDriver(cf, section)));
}

////////////////////////////////////////////////////////////////////////////////
// Registers the driver in the driver table. Called from the
// player_driver_init function that the loader looks for
void cameragst_Register (DriverTable* table)
{
  table->AddDriver ("cameragst", GStreamerDriver_Init);
}

////////////////////////////////////////////////////////////////////////////////
// Constructor.  Retrieve options from the configuration file and do any
// pre-Setup() setup.
GStreamerDriver::GStreamerDriver(ConfigFile* cf, int section)
    : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN, PLAYER_CAMERA_CODE)
{
  this->image_size = 0;
  this->data.image = NULL;
  // Read an option from the configuration file
  this->source = cf->ReadString(section, "source", NULL);
  if (!(this->source))
  {
    PLAYER_ERROR("Source not given.");
    this->SetError(-1);
    return;
  }
  if (!(strlen(this->source) > 0))
  {
    PLAYER_ERROR("Source not given.");
    this->SetError(-1);
    return;
  }
  this->jpeg = cf->ReadInt(section, "jpeg", 0);
  this->trace = cf->ReadBool(section, "trace", false);

  GStreamerDriver::initialized++;
  if ((GStreamerDriver::initialized) != 1)
  {
    PLAYER_ERROR("Due to the nature of gstream library API, this driver should be started one per Player server instance.");
    this->SetError(-1);
    return;
  } else gst_init(NULL, NULL);

  if (this->trace) {
    gst_alloc_trace_set_flags_all (GST_ALLOC_TRACE_LIVE);

    if (!gst_alloc_trace_available ()) {
      PLAYER_WARN("Trace not available (recompile with trace enabled).");
    }
    gst_alloc_trace_print_live ();
  }

#ifndef GST_PLUGIN_DEFINE_STATIC
  // according to the documentation this is the way to register a plugin now
  if (!gst_plugin_register_static(
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "opencv-appsink",		// a unique name of the plugin
    "Element application sink", // description of the plugin
    appsink_plugin_init,	// pointer to the init function of this plugin
    "0.1",			// version string of the plugin
    "LGPL",			// effective license of plugin
    "libplayerdrivers",			// source module plugin belongs to
    "player",			// shipped package plugin belongs to
    "http://playerstage.sourceforge.net" // URL to provider of plugin
  ))
  {
    PLAYER_ERROR("GStreamer plugin register failed\n");
    this->SetError(-1);
    return;
  }
#endif
}

GStreamerDriver::~GStreamerDriver()
{
  if (GStreamerDriver::initialized == 1) gst_deinit();
  GStreamerDriver::initialized--;

  if (this->trace) {
    gst_alloc_trace_print_live ();
  }
  if (this->data.image) free(this->data.image);
}

void GStreamerDriver::HandleMessage()
{
	GstBus* bus = gst_element_get_bus(this->pipeline);

	while(gst_bus_have_pending(bus)) {

		GstMessage* msg = gst_bus_pop(bus);

		//printf("Got %s message\n", GST_MESSAGE_TYPE_NAME(msg));

		switch (GST_MESSAGE_TYPE (msg)) {
		case GST_MESSAGE_STATE_CHANGED:
			GstState oldstate, newstate, pendstate;
			gst_message_parse_state_changed(msg, &oldstate, &newstate, &pendstate);

      /*
			printf("state changed from %s to %s (%s pending)\n",
					gst_element_state_get_name(oldstate),
					gst_element_state_get_name(newstate),
					gst_element_state_get_name(pendstate)
					);
      */

			break;
		case GST_MESSAGE_ERROR: {
			GError *err;
			gchar *debug;
			gst_message_parse_error(msg, &err, &debug);

			PLAYER_ERROR2("GStreamer Plugin: Embedded video playback halted; module %s reported: %s",
				  gst_element_get_name(GST_MESSAGE_SRC (msg)), err->message);

			g_error_free(err);
			g_free(debug);

			gst_element_set_state(this->pipeline, GST_STATE_NULL);

			break;
			}
		case GST_MESSAGE_EOS:
			PLAYER_WARN("NetStream has reached the end of the stream.");
			break;
		case GST_MESSAGE_ASYNC_DONE:
			// TODO: pipeline state changed, should be blocking?
			break;
		case GST_MESSAGE_NEW_CLOCK:
			// TODO: something?
			break;
		default:
			PLAYER_WARN1("unhandled message %s\n", GST_MESSAGE_TYPE_NAME(msg));
			break;
		}

		gst_message_unref(msg);
	}

	gst_object_unref(GST_OBJECT(bus));
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int GStreamerDriver::MainSetup()
{
  GError *err = NULL;
  launchpipe = gst_parse_launch(this->source, &err);

  if (launchpipe == NULL && err) {
    PLAYER_ERROR1("unable to parse GStreamer pipe: %s", err->message);
    g_error_free(err);
    return -1;
  }

  sink = gst_element_factory_make("player-appsink", NULL);
  if (!sink) {
    PLAYER_ERROR("unable to create player-appsink element");
    return -1;
  }

  GstCaps *caps = (this->jpeg) ? gst_caps_new_simple("image/jpeg", NULL) : gst_caps_new_simple("video/x-raw-rgb", NULL);
  gst_app_sink_set_caps(GST_APP_SINK(sink), caps);
  gst_caps_unref(caps);

  gst_base_sink_set_sync(GST_BASE_SINK(sink), TRUE);

  PLAYER_WARN1("GST_IS_PIPELINE(launchpipe) = %s", GST_IS_PIPELINE(launchpipe) ? "TRUE" : "FALSE");

  if(GST_IS_PIPELINE(launchpipe)) {
#ifdef GST_PLUGIN_DEFINE_STATIC
    // Old API
    GstPad *outpad = gst_bin_find_unconnected_pad(GST_BIN(launchpipe), GST_PAD_SRC);
#else
    // New API
    GstPad *outpad = gst_bin_find_unlinked_pad(GST_BIN(launchpipe), GST_PAD_SRC);
#endif
    g_assert(outpad);
    GstElement *outelement = gst_pad_get_parent_element(outpad);
    g_assert(outelement);
    gst_object_unref(outpad);

    pipeline = launchpipe;

    if(!gst_bin_add(GST_BIN(pipeline), sink)) {
      PLAYER_ERROR("gst_bin_add() failed"); // TODO: do some unref
      gst_object_unref(outelement);
      gst_object_unref(pipeline);
      return -1;
    }

    if(!gst_element_link(outelement, sink)) {
      PLAYER_ERROR1("GStreamer: cannot link outelement(\"%s\") -> sink", gst_element_get_name(outelement));
      gst_object_unref(outelement);
      gst_object_unref(pipeline);
      return -1;
    }

    gst_object_unref(outelement);
  } else {
    pipeline = gst_pipeline_new(NULL);
    g_assert(pipeline);

    gst_object_unparent(GST_OBJECT(launchpipe));

    gst_bin_add_many(GST_BIN(pipeline), launchpipe, sink, NULL);

    if(!gst_element_link(launchpipe, sink)) {
      PLAYER_ERROR("GStreamer: cannot link launchpipe -> sink");
      gst_object_unref(pipeline);
      return -1;
    }
  }

  if(gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
    PLAYER_ERROR("GStreamer: unable to set pipeline to paused");
    gst_object_unref(pipeline);
    return -1;
  }

  this->image_size = 0;
  this->buffer = NULL;

  this->HandleMessage();

  return(0);
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
void GStreamerDriver::MainQuit()
{
  // Here you would shut the device down by, for example, closing a
  // serial port.
  GstState state, pending;

  this->HandleMessage();

  PLAYER_WARN("Setting pipeline to PAUSED ...");
  gst_element_set_state (pipeline, GST_STATE_PAUSED);
  gst_element_get_state (pipeline, &state, &pending, GST_CLOCK_TIME_NONE);
  this->HandleMessage();

  PLAYER_WARN("Setting pipeline to READY ...");
  gst_element_set_state (pipeline, GST_STATE_READY);
  gst_element_get_state (pipeline, &state, &pending, GST_CLOCK_TIME_NONE);
  this->HandleMessage();

  PLAYER_WARN("Setting pipeline to NULL ...");
  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_element_get_state (pipeline, &state, &pending, GST_CLOCK_TIME_NONE);
  this->HandleMessage();

  if (this->pipeline) {
    gst_object_unref (GST_OBJECT (this->pipeline));
    pipeline = NULL;
  }

  if (this->buffer) {
    gst_buffer_unref(this->buffer);
    this->buffer = NULL;
  }

  this->image_size = 0;

  if (this->data.image) free(this->data.image);
  this->data.image = NULL;
}

int GStreamerDriver::GrabFrame()
{
  if (!this->pipeline) {
    return 0;
  }

  if (gst_app_sink_is_eos(GST_APP_SINK(this->sink))) {
    PLAYER_WARN("gst_app_sink_is_eos\n");
    return 0;
  }

  if (this->buffer) {
    gst_buffer_unref(this->buffer);
    this->buffer = NULL;
  }

  this->HandleMessage();

  if(!gst_app_sink_get_queue_length(GST_APP_SINK(this->sink))) {

    if(gst_element_set_state(GST_ELEMENT(this->pipeline), GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE) {
      this->HandleMessage();
      return 0;
    }

    this->buffer = gst_app_sink_pull_buffer(GST_APP_SINK(this->sink));

    if(gst_element_set_state(GST_ELEMENT(this->pipeline), GST_STATE_PAUSED) == GST_STATE_CHANGE_FAILURE) {
      this->HandleMessage();
      return 0;
    }
  } else {
    this->buffer = gst_app_sink_peek_buffer(GST_APP_SINK(this->sink));
  }

  if (!this->buffer) {
    return 0;
  }

  return 1;
}

int GStreamerDriver::RetrieveFrame()
{
  size_t desired_size;
  gint bpp;

  if(!this->buffer)
    return 0;

  //printf("getting buffercaps\n");

  GstCaps* caps = gst_buffer_get_caps(this->buffer);

  assert(gst_caps_get_size(caps) == 1);

  GstStructure* structure = gst_caps_get_structure(caps, 0);

  if (this->jpeg)
  {
    bpp = 24;
  } else
  {
    gint depth, endianness, redmask, greenmask, bluemask;
    if(!gst_structure_get_int(structure, "bpp", &bpp) ||
       !gst_structure_get_int(structure, "depth", &depth) ||
       !gst_structure_get_int(structure, "endianness", &endianness) ||
       !gst_structure_get_int(structure, "red_mask", &redmask) ||
       !gst_structure_get_int(structure, "green_mask", &greenmask) ||
       !gst_structure_get_int(structure, "blue_mask", &bluemask))
    {
      PLAYER_ERROR1("missing essential information in buffer caps, %s", gst_caps_to_string(caps));
      return 0;
    }
    if (!redmask || !greenmask || !bluemask || !depth) return 0;
    //printf("buffer has %d bpp, depth %d, endianness %d, rgb %x %x %x, %s\n", bpp, depth, endianness, redmask, greenmask, bluemask, gst_caps_to_string(caps));
  }

  gint height, width;

  if(!gst_structure_get_int(structure, "width", &width) ||
    !gst_structure_get_int(structure, "height", &height))
    return 0;

  desired_size = width * height * (bpp / 8);
  if (!(desired_size > 0)) return 0;
  if ((this->image_size) != desired_size)
  {
    PLAYER_WARN3("New size: width: %d, height: %d, bpp: %d", width, height, bpp);
    if (this->data.image) free(this->data.image);
    this->data.image = NULL;
    // PLAYER_WARN3("creating frame %dx%dx%d\n", width, height, bpp);

    this->data.width  = width;
    this->data.height = height;

    // TODO: fix this
    this->data.bpp    = bpp;
    this->data.fdiv   = 0;
    this->data.format = PLAYER_CAMERA_FORMAT_RGB888;
    this->data.compression = (this->jpeg) ? PLAYER_CAMERA_COMPRESS_JPEG : PLAYER_CAMERA_COMPRESS_RAW;
    this->image_size = desired_size;
    this->data.image_count = this->image_size;

    this->data.image = reinterpret_cast<uint8_t *>(malloc(this->data.image_count));
    if (!(this->data.image))
    {
      PLAYER_ERROR("Out of memory");
      this->image_size = 0;
      return 0;
    }
  }

  gst_caps_unref(caps);

  unsigned char *buffer_data = GST_BUFFER_DATA(this->buffer);

  if (!(this->data.image))
  {
    PLAYER_ERROR("NULL image pointer");
    return 0;
  }
  if (this->jpeg)
  {
    assert((this->buffer->size) <= (this->image_size));
    memcpy(this->data.image, buffer_data, this->buffer->size);
    this->data.image_count = this->buffer->size;
  } else
  {
    memcpy(this->data.image, buffer_data, this->image_size);
  }

  return !0;

  /*
  printf("generating shifts\n");

  IplImage *frame = cap->frame;
  unsigned nbyte = bpp >> 3;
  unsigned redshift, blueshift, greenshift;
  unsigned mask = redmask;
  for(redshift = 0, mask = redmask; (mask & 1) == 0; mask >>= 1, redshift++);
  for(greenshift = 0, mask = greenmask; (mask & 1) == 0; mask >>= 1, greenshift++);
  for(blueshift = 0, mask = bluemask; (mask & 1) == 0; mask >>= 1, blueshift++);

  printf("shifts: %u %u %u\n", redshift, greenshift, blueshift);

  for(int r = 0; r < frame->height; r++) {
    for(int c = 0; c < frame->width; c++, data += nbyte) {
      int at = r * frame->widthStep + c * 3;
      frame->imageData[at] = ((*((gint *)data)) & redmask) >> redshift;
      frame->imageData[at+1] = ((*((gint *)data)) & greenmask) >> greenshift;
      frame->imageData[at+2] = ((*((gint *)data)) & bluemask) >> blueshift;
    }
  }

  printf("converted buffer\n");

  gst_buffer_unref(cap->buffer);
  cap->buffer = 0;

  return cap->frame;
  */
}

////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void GStreamerDriver::Main()
{
  struct timespec tspec;

  while (true)
  {
    tspec.tv_sec = 0;
    tspec.tv_nsec = 1000000;
    nanosleep(&tspec, NULL);
    pthread_testcancel();
    if (this->GrabFrame() && this->RetrieveFrame()) this->Publish(this->device_addr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, &this->data);
    pthread_testcancel();
  }
}
