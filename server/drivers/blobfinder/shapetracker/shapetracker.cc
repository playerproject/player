/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey et al
 *                      gerkey@usc.edu    
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
// Desc: Driver for detecting UPC barcodes from camera
// Author: Andrew Howard
// Date: 15 Feb 2004
// CVS: $Id$
//
// Theory of operation:
//   TODO
//
// Requires:
//   Camera device.
//
///////////////////////////////////////////////////////////////////////////

#include "player.h"

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <netinet/in.h>   // for htons(3)
#include <unistd.h>

#include <opencv/cv.h>

#include "device.h"
#include "devicetable.h"
#include "drivertable.h"

// Info on potential shapes.
class Shape 
{
  public: Shape() : id(-1), ax(0),ay(0),bx(0),by(0) {}

  // Id (-1) if undetermined.
  public: int id;

  // Shape bounding coords
  public: int ax, ay, bx, by;
};



// Driver for detecting laser retro-reflectors.
class ShapeTracker : public CDevice
{
  // Constructor
  public: ShapeTracker(char* interface, ConfigFile* cf, int section);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Main function for device thread.
  private: virtual void Main();

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests();
  
  // Process any new camera data.
  private: int UpdateCamera();

  // Look for barcodes in the image.  
  private: void ProcessImage();

  private: void FindShapes();
  private: void KalmanFilter();

  private: void CreateBinaryImage(IplImage *src, IplImage *dest);

  // Write the device data (the data going back to the client).
  private: void WriteData();

  // Calculate angle between three points
  private: double CalcAngle( CvPoint *pt1, CvPoint *pt2, CvPoint *pt0);

  // Camera stuff
  private: int cameraIndex;
  private: CDevice *camera;
  private: double cameraTime;
  private: player_camera_data_t cameraData;


  private: IplImage *mainImage;
  private: IplImage *workImage;

  private: double threshold;
  private: int vertices;

  private: Shape shapes[256];
  private: unsigned int shapeCount;
};


// Initialization function
CDevice* ShapeTracker_Init(char* interface, ConfigFile* cf, int section)
{
  if (strcmp(interface, PLAYER_BLOBFINDER_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"shapetracker\" does not support interface \"%s\"\n",
                  interface);
    return (NULL);
  }

  return ((CDevice*) (new ShapeTracker(interface, cf, section)));
}


// a driver registration function
void ShapeTracker_Register(DriverTable* table)
{
  table->AddDriver("shapetracker", PLAYER_READ_MODE, ShapeTracker_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
ShapeTracker::ShapeTracker(char* interface, ConfigFile* cf, int section)
    : CDevice(sizeof(player_blobfinder_data_t), 0, 10, 10)
{
  this->cameraIndex = cf->ReadInt(section, "camera", 0);
  this->camera = NULL;
  this->cameraTime = 0;

  this->mainImage = NULL;
  this->workImage = NULL;

  this->threshold = 80;
  this->vertices = 8;

  this->shapeCount = 0;
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int ShapeTracker::Setup()
{
  player_device_id_t id;


  // Subscribe to the camera.
  id.code = PLAYER_CAMERA_CODE;
  id.index = this->cameraIndex;
  id.port = this->device_id.port;
  this->camera = deviceTable->GetDevice(id);

  if (!this->camera)
  {
    PLAYER_ERROR("unable to locate suitable camera device");
    return(-1);
  }
  if (this->camera->Subscribe(this) != 0)
  {
    PLAYER_ERROR("unable to subscribe to camera device");
    return(-1);
  }


  // Start the driver thread.
  this->StartThread();
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int ShapeTracker::Shutdown()
{
  // Stop the driver thread.
  StopThread();
  
  // Unsubscribe from devices.
  this->camera->Unsubscribe(this);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void ShapeTracker::Main() 
{

  while (true)
  {
    // Let the camera drive update rate
    this->camera->Wait();

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    // Process any new camera data.
    if (this->UpdateCamera())
    {
      // Find all the shapes in the image
      this->ProcessImage();

      // Write the results to the client
      this->WriteData();
    }

    // Process any pending requests.
    this->HandleRequests();
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process requests.  Returns 1 if the configuration has changed.
int ShapeTracker::HandleRequests()
{
  void *client;
  char request[PLAYER_MAX_REQREP_SIZE];
  int len;
  while ((len = GetConfig(&client, &request, sizeof(request))) > 0)
  {
    switch (request[0])
    {
      default:
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
    }
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Process any new camera data.
int ShapeTracker::UpdateCamera()
{
  size_t size;
  uint32_t timesec, timeusec;
  double time;

  // Get the camera data.
  size = this->camera->GetData(this, (unsigned char*) &this->cameraData,
                               sizeof(this->cameraData), &timesec, &timeusec);
  time = (double) timesec + ((double) timeusec) * 1e-6;

  // Dont do anything if this is old data.
  if (fabs(time - this->cameraTime) < 0.001)
    return 0;
  this->cameraTime = time;
  
  // Do some byte swapping
  this->cameraData.width = ntohs(this->cameraData.width);
  this->cameraData.height = ntohs(this->cameraData.height); 
  this->cameraData.depth = ntohs(this->cameraData.depth);
  this->cameraData.image_size = ntohl(this->cameraData.image_size); 

  return 1;
}


void ShapeTracker::FindShapes()
{
  double s, t1;
  CvRect rect;
  int i;

  CvMemStorage *storage = cvCreateMemStorage(0);
  CvSeq *contour = 0, *result = 0;

  // Find contours on a binary image
  cvFindContours(this->workImage, storage, &contour, sizeof(CvContour), 
      CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE);

  for(; contour != 0; contour = contour->h_next)
  {

    // Approximates polygonal curves with desired precision
    result = cvApproxPoly(contour, sizeof(CvContour), storage, 
        CV_POLY_APPROX_DP, cvContourPerimeter(contour)*0.02, 0);

    
    if ( result->total > 0 &&
        (result->total/2.0 == (int)(result->total/2)) &&
        fabs(cvContourArea(result, CV_WHOLE_SEQ)) > 50)
    {
      s = 0;
      for(i = 0; i < result->total +1; i++)
      {
        if(i >= 2)
        {
          t1 = fabs(this->CalcAngle(
                (CvPoint*)cvGetSeqElem(result, i , 0),
                (CvPoint*)cvGetSeqElem(result, i-2 , 0),
                (CvPoint*)cvGetSeqElem(result, i-1 , 0)));
          s = s > t1 ? s : t1;
          
        }
      }
      if(s < 0.5)
      {
        rect = cvBoundingRect(result,0);

        this->shapes[this->shapeCount].id = result->total;
        this->shapes[this->shapeCount].ax = rect.x;
        this->shapes[this->shapeCount].ay = rect.y;
        this->shapes[this->shapeCount].bx = rect.x + rect.width;
        this->shapes[this->shapeCount].by = rect.y + rect.height;

        this->shapeCount = (this->shapeCount+1) % 255;

        //cvDrawContours(this->workImage, result, 255, 0, 0, 5, 8);
        //cvDrawContours(this->mainImage, result, 255, 0, 0, 5, 8);

        //cvSaveImage("main2.jpg", this->mainImage);
        //orientpoint = getcentralpoint(init_image, result);
      }
    }
  }

  cvReleaseMemStorage(&storage);
}

void ShapeTracker::KalmanFilter()
{
/*  // TODO: image processing
  CvMat *corrx, *corry;
  CvMat *predictionx, *predictiony;
  float xmeanp, ymeanp, xmeanvp, ymeanvp;
  int xmean, ymean;
  //float orientation = 0.0;
  CvMoments moments;
  float vx, vy;
  CvPoint pt3, pt4;
  CvPoint tmp_orient;
  double heading;

  cvMoments(tmp_image2, &moments, 1);
  xmean = (int)(moments.m10/moments.m00);
  ymean = (int)(moments.m01/moments.m00);
  //orientation = 0.0f;

  if((xmean != 0) && (ymean != 0)){
    float cm02 = cvGetCentralMoment(&moments, 0, 2);
    float cm20 = cvGetCentralMoment(&moments, 2, 0);
    float cm11 = cvGetCentralMoment(&moments, 1, 1);
    tmp_orient.x = xmean;
    tmp_orient.y = ymean;
    heading = getorientation(tmp_orient, orientpoint);
    //fprintf(stderr, " heading %f\n", heading);
    //orientation = atan2(cm20 -cm02, 2*cm11)*180.0/(2*M_PI);
    if(first == 1){
      fprintf(stderr, " x %f %f\n", (float)xmean, (float)ymean);
      kalmanx->state_post->data.fl[0] = (float) xmean;
      kalmanx->state_post->data.fl[1] = (float) 0.0;
      kalmany->state_post->data.fl[0] = (float) ymean;
      kalmany->state_post->data.fl[1] = (float) 0.0;
      first = 0;
    }
  }
  if(xmean == 0 && ymean == 0){
    xmean = (width/2);
    ymean = (height/2);
  }
  if(first == 0){
    predictionx = cvKalmanPredict(kalmanx, 0);
    predictiony = cvKalmanPredict(kalmany, 0);

    xmeanp = predictionx->data.fl[0];
    ymeanp = predictiony->data.fl[0];
    xmeanvp = predictionx->data.fl[1];
    ymeanvp = predictiony->data.fl[1];

    if(debug > 1){
      fprintf(stderr, "predict %f %f %f %f\n",xmeanp,xmeanvp,ymeanp, ymeanvp);
      fprintf(stderr, "meas %f %f\n",(float)xmean, (float)ymean);
    }
    if(xmean == (width/2) && ymean == (height/2)){
      pt3.x=(int)predictionx->data.fl[0];
      pt4.x=(int)predictionx->data.fl[1];
      pt3.y = (int)predictiony->data.fl[0];
      pt4.y=(int)predictiony->data.fl[1];

      //used for stealthy
      tracked_values[0].heading = tracked_values[1].heading;
      tracked_values[0].point.x = tracked_values[1].point.x;
      tracked_values[0].point.y = tracked_values[1].point.y;

      tracked_values[1].heading = heading;
      tracked_values[1].point.x = pt3.x;
      tracked_values[1].point.y = pt3.y;

      kalmanx->state_post->data.fl[0]=predictionx->data.fl[0];
      kalmanx->state_post->data.fl[1]=predictionx->data.fl[1];
      kalmany->state_post->data.fl[0]=predictiony->data.fl[0];
      kalmany->state_post->data.fl[1]=predictiony->data.fl[1];
    }else{
      measurementx->data.fl[0] = (float)xmean;
      measurementy->data.fl[0] = (float)ymean;

      corrx = cvKalmanCorrect(kalmanx, measurementx);
      corry = cvKalmanCorrect(kalmany, measurementy);

      pt3.x=(int)(corrx->data.fl[0]);
      pt4.x=(int)(corrx->data.fl[1]);
      pt3.y = (int)(corry->data.fl[0]);
      pt4.x=(int)(corry->data.fl[1]);

    }
    if(debug == 3){
      fprintf(stderr, "post %d %d %d %d\n", pt3.x, pt3.y, pt4.x, pt4.y);
      fprintf(stderr, "pre after %f %f %f %f\n",
          kalmanx->state_post->data.fl[0],
          kalmany->state_post->data.fl[0],
          kalmanx->state_post->data.fl[1],
          kalmany->state_post->data.fl[1]);
    }

    cvCircle(init_image, pt3, 15.0, CV_RGB(255,0,0), 2);

    vx = (float)(pt3.x - (width/2))/(width/2);
    vy = (float)(-pt3.y + (height/2))/(height/2);
    //cvReleaseMat(&predictionx);
    //cvReleaseMat(&predictiony);
    //cvReleaseMat(&corrx);
    //cvReleaseMat(&corry);

  }    

  if(reset_kalman == 1){
    if(debug == 2){
      fprintf(stderr, " xmean  %d, ymean %d\n",xmean, ymean);
    }
    vx = (float)(xmean - (width/2))/(width/2);
    vy = (float)(-ymean + (height/2))/(height/2);
    first =1; // reset the kalman filter
  }
  if(vx > 1.0)
    vx = 1.0;
  if(vx < -1.0)
    vx = -1.0;
  if(vy > 1.0)
    vy = 1.0;
  if(vy < -1.0)
    vy = -1.0;

  visual_servo_command.vel_right = vx;
  visual_servo_command.vel_forward = vy;
  visual_servo_command.heading_offset = (float)heading;
  visual_servo_command.tracking_state = HV_OBJECT_TRACK_STATE;

  if(debug > 1 ){
    fprintf(stderr, "Sent x %f, y %f, z %f, state %d\n",
        visual_servo_command.vel_right,
        visual_servo_command.vel_forward,
        visual_servo_command.heading_offset,
        visual_servo_command.tracking_state);
  }



  put_data_on_image(width, height, vx, vy, heading, font);
  draw_lines(width, height, heading, pt3);
  deploymote(width, height, xmean, ymean, &deploy_mote, debug);
  */

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Look for stuff in the image.
void ShapeTracker::ProcessImage()
{

  // Reset the shape count
  this->shapeCount = 0;

  // Create a new mainImage if it doesn't already exist
  if (this->mainImage == NULL)
    this->mainImage = cvCreateImage( cvSize(this->cameraData.width,
          this->cameraData.height), IPL_DEPTH_8U, 3);

  // Create a work image if it doesn't already exist
  if (this->workImage == NULL)
    workImage = cvCreateImage( cvSize(this->cameraData.width,
          this->cameraData.height), IPL_DEPTH_8U, 1);

  // Initialize the main image
  memcpy(this->mainImage->imageData, this->cameraData.image, 
         this->mainImage->imageSize);

  // Make dest a gray scale image
  cvCvtColor(this->mainImage, this->workImage, CV_BGR2GRAY);

  // Create a binary image
  cvThreshold(this->workImage, this->workImage, this->threshold, 255, 
      CV_THRESH_BINARY);

  // Find all the shapes in the image
  this->FindShapes();

  //cvSaveImage("work.jpg", this->workImage);

  //this->KalmanFilter();
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Update the device data (the data going back to the client).
void ShapeTracker::WriteData()
{
  unsigned int i;
  int shapeCount, channelCount, channel;
  uint32_t timesec, timeusec;
  Shape *shape;
  player_blobfinder_data_t data;

  // Se the image dimensions
  data.width = htons(this->cameraData.width);
  data.height = htons(this->cameraData.height);

  // Reset the header data
  for (i = 0; i < PLAYER_BLOBFINDER_MAX_CHANNELS; i++)
  {
    data.header[i].index = 0;
    data.header[i].num = 0;
  }
  shapeCount = 0;

  // Go through the blobs
  for (channel = 0; channel < PLAYER_BLOBFINDER_MAX_CHANNELS; channel++)
  {
    // Set the offest of the first shape for this channel
    data.header[channel].index = htons(shapeCount);
    channelCount = 0;
    
    for (i = 0; i < this->shapeCount; i++)
    {
      shape = this->shapes + i;

      // Make sure this shape belong to this channel
      if (shape->id != channel)
        continue;

      // Set the data to pass back
      data.blobs[shapeCount].color = 0;  // TODO
      data.blobs[shapeCount].area = htonl((int) ((shape->bx - shape->ax) * (shape->by - shape->ay)));
      data.blobs[shapeCount].x = htons((int) ((shape->bx + shape->ax) / 2));
      data.blobs[shapeCount].y = htons((int) ((shape->by + shape->ay) / 2));
      data.blobs[shapeCount].left = htons((int) (shape->ax));
      data.blobs[shapeCount].top = htons((int) (shape->ay));
      data.blobs[shapeCount].right = htons((int) (shape->bx));
      data.blobs[shapeCount].bottom = htons((int) (shape->by));
      data.blobs[shapeCount].range = htons(0);
      channelCount++;
      shapeCount++;
    }

    data.header[channel].num = htons(channelCount);
  }
  
  // Compute the data timestamp (from camera).
  timesec = (uint32_t) this->cameraTime;
  timeusec = (uint32_t) (fmod(this->cameraTime, 1.0) * 1e6);
 
  // Copy data to server.
  PutData((unsigned char*) &data, sizeof(data), timesec, timeusec);
  
  return;
}

////////////////////////////////////////////////////////////////////////////////
// Calculate the angle between three points
double ShapeTracker::CalcAngle( CvPoint *pt1, CvPoint *pt2, CvPoint *pt0)
{
  double dx1 = pt1->x - pt0->x;
  double dy1 = pt1->y - pt0->y;
  double dx2 = pt2->x - pt0->x;
  double dy2 = pt2->y - pt0->y;
  return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
}
