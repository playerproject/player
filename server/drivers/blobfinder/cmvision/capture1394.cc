/* $Id$
 *
 * Derived class implements 1394 data feed to CMVision
 */

#if HAVE_CONFIG_H
  #include <config.h>
#endif

#if HAVE_1394

#include "capture1394.h"

//#define QPRINT(x) {printf(x);fflush(stdout);}
#define QPRINT(x)

bool capture1394::initialize(int nwidth,int nheight)
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
  printf("init called nwidth=%d,nheight=%d,width=%d,height=%d,DoBayer=%d,Pattern=%d\n",
	 nwidth,nheight,width,height,DoBayerConversion,BayerPattern);

  // set video mode and format
  int videoMode = MODE_320x240_YUV422; // defaults 
  int videoFormat = FORMAT_VGA_NONCOMPRESSED;
  if(DoBayerConversion)
       {    // colour image is really 8bpp mono
	    // there is no mono 320x240 mode
	    if (nwidth==640&nheight==480)
		 videoMode = MODE_640x480_MONO; 
	    else if (nwidth==800&nheight==600)
		 videoMode = MODE_800x600_MONO; 
	    else if (nwidth==1024&nheight==768){
		 videoMode = MODE_1024x768_MONO;
		 videoFormat = FORMAT_SVGA_NONCOMPRESSED_1;
	    }
       }
  else
       {
	    if (nwidth==640&nheight==480)
		 videoMode = MODE_640x480_YUV422;     
	    else if (nwidth==800&nheight==600)
		 videoMode = MODE_800x600_YUV422; 
	    else if (nwidth==1024&nheight==768){
		 videoMode = MODE_1024x768_YUV422; 
		 videoFormat = FORMAT_SVGA_NONCOMPRESSED_1;
	    }
       }   

  if (dc1394_setup_capture(handle,camera_nodes[0],
                           0, 
			   videoFormat,
			   videoMode,
                           SPEED_400,
			   FRAMERATE_15,
                           //FRAMERATE_30,
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

  /*----------------------------------------------
   * initialize storage for bayer conversion
   *---------------------------------------------*/
  if (DoBayerConversion){
       current_rgb = (unsigned char *) malloc(width*height*3);
       current_YUV = (unsigned char *) malloc(width*height*2);
  }

  return true;
}

void capture1394::close()
{
  current=NULL;
  if (camera_nodes==NULL) return; //we did not initialize
  //this is my guess at what needs to be done to release everything
  dc1394_release_camera(handle,&camera);
  raw1394_destroy_handle(handle);
  camera_nodes=NULL;

  if (DoBayerConversion){
       free(current_rgb);
       free(current_YUV);
  }
}


/*
unsigned char *capture1394::captureFrame(int &index,int &field)
{
  field = 0;
  index = 0;

  captured_frame = true; 
  return captureFrame();
}
*/

/*
void capture1394::releaseFrame(unsigned char* frame, int index)
{
  captured_frame = false;
}
*/

unsigned char *capture1394::captureFrame()
{
  if (dc1394_single_capture(handle,&camera)!=DC1394_SUCCESS) 
    {
      fprintf( stderr, "unable to capture a frame\n");
      close();
    }
  timeval  t;
  gettimeofday(&t,NULL);
  timestamp = (stamp_t)((t.tv_sec + t.tv_usec/1.0E6) * 1.0E9);

  if (DoBayerConversion){
       // do bayer conversion for cameras that require it.
       // e.g. Pt Grey Dragonfly
       // put do_bayer_conversion "????" (insert pattern e.g. BGGR)
       // in the blobfinder:? section of player cfg file to turn it on.

       // use latest conversions.h from coriander 
       BayerEdgeSense(current, current_rgb, width, height,(bayer_pattern_t)BayerPattern);
       // bayer2rgb(NEAREST_NEIGHBOR, current, current_rgb, width, height);
       // bayer2rgb(BILERP, current, current_rgb, width, height);
       
       //FILE *ftmp = fopen("frame_tmp.raw","w");
       //fwrite(current_rgb,width * height*3,1,ftmp);
       //fclose(ftmp);
       
       // convert to yuv422
       // use 2001 version of conversions.h from Dan Dennedy  <dan@dennedy.org>
       // rgb2yuy2(current_rgb, current_YUV, width * height);
       // use latest conversions.h from coriander 
       rgb2uyvy(current_rgb, current_YUV, width * height);
       
       //ftmp = fopen("frame_tmp.yuv","w");
       //fwrite(current_YUV,width * height*2,1,ftmp);
       //fclose(ftmp);
       
       return current_YUV;
  }else
       return current;
}

#endif // HAVE_1394
