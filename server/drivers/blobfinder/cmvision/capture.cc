#include "capture.h"

//#define QPRINT(x) {printf(x);fflush(stdout);}
#define QPRINT(x)

bool capture::initialize(int nwidth,int nheight)
{
  // Set defaults if not given
  if(!nwidth || !nheight){
    nwidth  = DEFAULT_IMAGE_WIDTH;
    nheight = DEFAULT_IMAGE_HEIGHT;
  }

  QPRINT("C");

  /*-----------------------------------------------------------------------
   *  Open ohci and assign handle to it
   *-----------------------------------------------------------------------*/
  handle = dc1394_create_handle(0);
  if (handle==NULL)
  {
    fprintf( stderr, "Unable to aquire a raw1394 handle\n"
             "did you insmod the drivers?\n");
    return false;
  }
  
  QPRINT("a");

  /*-----------------------------------------------------------------------
   *  get the camera nodes and describe them as we find them
   *-----------------------------------------------------------------------*/
  numNodes = raw1394_get_nodecount(handle);
  camera_nodes = dc1394_get_camera_nodes(handle,&numCameras,0);
  
  QPRINT("m");

  if (numCameras<1)
  {
    fprintf( stderr, "no cameras found :(\n");
    raw1394_destroy_handle(handle);
    camera_nodes=NULL;
    return false;
  }

  QPRINT("e");
  
  /*-----------------------------------------------------------------------
   *  to prevent the iso-transfer bug from raw1394 system, check if
   *  camera is highest node. For details see 
   *  http://linux1394.sourceforge.net/faq.html#DCbusmgmt
   *  and
   *  http://sourceforge.net/tracker/index.php?func=detail&aid=435107&group_id=8157&atid=108157
   *-----------------------------------------------------------------------*/
  if( camera_nodes[0] == numNodes-1)
  {
    fprintf( stderr, "\n"
             "Sorry, your camera is the highest numbered node\n"
             "of the bus, and has therefore become the root node.\n"
             "The root node is responsible for maintaining \n"
             "the timing of isochronous transactions on the IEEE \n"
             "1394 bus.  However, if the root node is not cycle master \n"
             "capable (it doesn't have to be), then isochronous \n"
             "transactions will not work.  The host controller card is \n"
             "cycle master capable, however, most cameras are not.\n"
             "\n"
             "The quick solution is to add the parameter \n"
             "attempt_root=1 when loading the OHCI driver as a \n"
             "module.  So please do (as root):\n"
             "\n"
             "   rmmod ohci1394\n"
             "   insmod ohci1394 attempt_root=1\n"
             "\n"
             "for more information see the FAQ at \n"
             "http://linux1394.sourceforge.net/faq.html#DCbusmgmt\n"
             "\n");
    return false;
  }
  
  QPRINT("r");

  /*-----------------------------------------------------------------------
   *  setup capture
   *-----------------------------------------------------------------------*/

  if (dc1394_setup_capture(handle,camera_nodes[0],
                           0, 
                           FORMAT_VGA_NONCOMPRESSED,
			   //MODE_160x120_YUV444,
                           MODE_320x240_YUV422,
			   //MODE_640x480_RGB,
                           SPEED_400,
                           FRAMERATE_3_75,
                           &camera)!=DC1394_SUCCESS) 
  {
    fprintf( stderr,"unable to setup camera-\n"
             "check line %d of %s to make sure\n"
             "that the video mode,framerate and format are\n"
             "supported by your camera\n",
             __LINE__,__FILE__);
    close();
    return false;
  }

  QPRINT("a");

  /*-----------------------------------------------------------------------
   *  have the camera start sending us data
   *-----------------------------------------------------------------------*/
  if (dc1394_start_iso_transmission(handle,camera.node)
      !=DC1394_SUCCESS) 
  {
    fprintf( stderr, "unable to start camera iso transmission\n");
    close();
    return false;
  }

  QPRINT(" I");

 /*-------------------------------------------------------------------------
  * grabbing first garbage image
  *------------------------------------------------------------------------*/

  if (dc1394_single_capture(handle,&camera)!=DC1394_SUCCESS) 
  {
    fprintf( stderr, "unable to capture a frame\n");
    close();
    return false;
  }

  QPRINT("nit complete.\n");
  
  current = (unsigned char *)camera.capture_buffer;
  width = camera.frame_width;
  height = camera.frame_height;

  return true;
}

void capture::close()
{
  current=NULL;
  if (camera_nodes==NULL) return; //we did not initialize
  //this is my guess at what needs to be done to release everything
  dc1394_release_camera(handle,&camera);
  raw1394_destroy_handle(handle);
  camera_nodes=NULL;
}


unsigned char *capture::captureFrame(int &index,int &field)
{
  field = 0;
  index = 0;

  captured_frame = true; 
  return captureFrame();
}

void capture::releaseFrame(unsigned char* frame, int index)
{
  captured_frame = false;
}

unsigned char *capture::captureFrame()
{
  if (dc1394_single_capture(handle,&camera)!=DC1394_SUCCESS) 
    {
      fprintf( stderr, "unable to capture a frame\n");
      close();
    }
  timeval  t;
  gettimeofday(&t,NULL);
  timestamp = (stamp_t)((t.tv_sec + t.tv_usec/1.0E6) * 1.0E9);

  return current;
}
