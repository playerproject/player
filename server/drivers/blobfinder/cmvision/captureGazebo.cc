#if HAVE_CONFIG_H
  #include <config.h>
#endif

#if INCLUDE_GAZEBO_CAMERA

#include "conversions.h"
#include "captureGazebo.h"

#include <unistd.h> // for debugging file writes
#include <fcntl.h>  // for debugging file writes 

bool captureGazebo::initialize(int nwidth,int nheight)
{
  // Set defaults if not given
  if(!nwidth || !nheight){
    nwidth  = DEFAULT_IMAGE_WIDTH;
    nheight = DEFAULT_IMAGE_HEIGHT;
  }
  
  //  current = camera->iface->data->image;//.capture_buffer;
   width = nwidth;
  height = nheight;
  current_rgb=(unsigned char *) malloc(width*height*3);
  YUV=(unsigned char *)malloc(width*height*2);
  return true;
}

void captureGazebo::close()
{
  free(current_rgb);
  free(YUV);
  current=NULL;
}


 unsigned char *captureGazebo::captureFrame()
{
    captured_frame = true; 
    uint32_t tSec,tUSec;
    //    unsigned char * temp=camera->iface->data->image;
    //    unsigned char * temp=NULL;

    camera->GetData(GzClient::client,current_rgb,4,&tSec,&tUSec);
    // printf("Got Image\n");
    // if(temp==NULL) 
    //  return NULL;
    // else
      return convertImageRgb2Yuv422(current_rgb,width * height);
    // has to be gz_camera getting the image!
}


unsigned char * captureGazebo::convertImageRgb2Yuv422(unsigned char *RGB,int NumPixels)
{

     // use 2001 version of conversions.h from Dan Dennedy  <dan@dennedy.org>
     //rgb2yuy2(RGB, YUV, NumPixels);
     // use latest conversions.h from coriander 
     rgb2uyvy(RGB, YUV, NumPixels);


     return YUV;
 }

/*
image_pixel * captureGazebo::uchar2impixel(unsigned char *image,int NumPixels)
{ 
   int pixel;
   image_pixel * impixel=NULL;
   for(pixel=0;pixel<NumPixels;pixel++)
   { 
    impixel[pixel]->u=image[pixel*4];
    impixel[pixel]->y1=image[pixel*4+1];
    impixel[pixel]->v=image[pixel*4+2];
    impixel[pixel]->y2=image[pixel*4+3];
   }
   return impixel;
}
*/

#endif
