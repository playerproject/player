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
// Desc: Driver for detecting simple shapes in a camera image
// Author: Andrew Howard
// Date: 15 Feb 2004
// CVS: $Id$
//
///////////////////////////////////////////////////////////////////////////

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_simpleshape simpleshape

The simpleshape driver looks for simple geometric shapes in a camera
image.  The user must supply a @e model of the target shape, in the
form of a binary image (such as the one shown below).

@image html simpleshape_h.gif "Sample model for the simpleshape detector"


@par Inputs

- This driver acquires image data from a @ref player_interface_camera
  interface.

@par Outputs

- This driver outputs detected shapes through a @ref
  player_interface_blobfinder interface.

- This driver also supplies processed image data through a @ref
  player_interface_camera interface (this data is intended mostly for
  debugging).  Note that the dimensions of the output image are twice
  that of the input image: the output image is divided into four
  parts, each showing one step in the detection process.  From
  top-to-bottom and left-to-right, these are: original image
  (monochrome), edge image, contour image, detected shapes.

@image html simpleshape_output.gif "Output image (debugging)"

@par Supported configuration requests

- None.

@par Configuration file options

- camera 0
  - Index of the input camera device.

- model NULL
  - Filename of the model image file.  This should by a binary,
    grayscale image.

- canny_thresh [40 20]
  - Thresholds for the Canny edge detector.

- match_thresh [1.0 1.0 1.0]
  - Threshold for shape matching.  Smaller values will produce fewer
 false positive.


@par Example 

@verbatim
driver
(
  name "simpleshape"
  devices ["blobfinder:1" "camera:1"]
  camera 0
  model "simpleshape_h.pgm"
)
@endverbatim
*/
/** @} */

#include "player.h"

#include <errno.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>       // for atoi(3)
#include <netinet/in.h>   // for htons(3)
#include <unistd.h>

#include <opencv/cv.h>
#include <opencv/highgui.h>

#include "driver.h"
#include "devicetable.h"
#include "drivertable.h"


// Invariant feature set for a contour
class FeatureSet
{
  // Contour moments
  public: CvMoments moments;

  // Compactness
  public: double compact;

  // Elliptical variance
  public: double variance;
};



// Info on potential shapes.
class Shape 
{
  // Id (-1) if undetermined.
  public: int id;

  // Shape bounding coords
  public: int ax, ay, bx, by;
};



// Driver for detecting laser retro-reflectors.
class SimpleShape : public Driver
{
  // Constructor
  public: SimpleShape( ConfigFile* cf, int section);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Main function for device thread.
  private: virtual void Main();

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests();
  
  // Grab new camera data.
  private: bool UpdateCamera();

  // Load a shape model
  private: int LoadModel();

  // Process the image
  private: void ProcessImage();

  // Having pre-processed the image, find some shapes
  private: void FindShapes();

  // Extract a feature set for the given contour
  private: void ExtractFeatureSet(CvContour *contour, FeatureSet *feature);

  // Write the outgoing blobfinder data
  private: void WriteBlobfinderData();

  // Write the outgoing camera data
  private: void WriteCameraData();

  // Output devices
  private: player_device_id_t blobfinder_id;
  private: player_device_id_t out_camera_id;

  // Input camera stuff
  private: int cameraIndex;
  private: player_device_id_t camera_id;
  private: Driver *camera;
  private: struct timeval cameraTime;
  private: player_camera_data_t cameraData;

  // Output camera stuff
  private: player_camera_data_t outCameraData;

  // Model data (this is the shape to search for)
  private: const char *modelFilename;
  private: CvMemStorage *modelStorage;
  private: CvSeq *modelContour;
  private: FeatureSet modelFeatureSet;

  // Images
  private: IplImage *inpImage;
  private: IplImage *outImage;
  private: CvMat outSubImages[4];
  private: IplImage *workImage;

  // Parameters
  private: double cannyThresh1, cannyThresh2;
  private: double matchThresh[3];

  // List of potential shapes
  private: Shape shapes[256];
  private: unsigned int shapeCount;
};


// Initialization function
Driver* SimpleShape_Init( ConfigFile* cf, int section)
{
  return ((Driver*) (new SimpleShape( cf, section)));
}


// a driver registration function
void SimpleShape_Register(DriverTable* table)
{
  table->AddDriver("simpleshape", SimpleShape_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
SimpleShape::SimpleShape( ConfigFile* cf, int section)
    : Driver(cf, section)
{
  player_device_id_t* ids;
  int num_ids;

  memset(&this->blobfinder_id.code, 0, sizeof(player_device_id_t));
  memset(&this->out_camera_id.code, 0, sizeof(player_device_id_t));

  // Parse devices section
  if((num_ids = cf->ParseDeviceIds(section,&ids)) < 0)
  {
    this->SetError(-1);    
    return;
  }

  // Must have a blobfinder interface
  if (cf->ReadDeviceId(&(this->blobfinder_id), ids, num_ids, PLAYER_BLOBFINDER_CODE, 0) != 0)
  {
    this->SetError(-1);    
    return;
  }
  if (this->AddInterface(this->blobfinder_id, PLAYER_READ_MODE,
                         sizeof(player_blobfinder_data_t), 0, 1, 1) != 0)
  {
    this->SetError(-1);    
    return;
  }

  // Optionally have a camera interface
  if (cf->ReadDeviceId(&(this->out_camera_id), ids, num_ids, PLAYER_CAMERA_CODE, 0) == 0)
  {
    if (this->AddInterface(this->out_camera_id, PLAYER_READ_MODE,
                           sizeof(player_camera_data_t), 0, 1, 1) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }

  this->cameraIndex = cf->ReadInt(section, "camera", 0);
  this->camera = NULL;

  this->inpImage = NULL;
  this->outImage = NULL;
  this->workImage = NULL;

  // Filename for the target shape image
  this->modelFilename = cf->ReadFilename(section, "model", NULL);

  // Parameters
  this->cannyThresh1 = cf->ReadTupleFloat(section, "canny_thresh", 0, 40);
  this->cannyThresh2 = cf->ReadTupleFloat(section, "canny_thresh", 1, 20);

  this->matchThresh[0] = cf->ReadTupleFloat(section, "match_thresh", 0, 0.50);
  this->matchThresh[1] = cf->ReadTupleFloat(section, "match_thresh", 1, 20.0);
  this->matchThresh[2] = cf->ReadTupleFloat(section, "match_thresh", 2, 0.20);  

  this->shapeCount = 0;
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int SimpleShape::Setup()
{
  // Subscribe to the camera.
  this->camera_id.code = PLAYER_CAMERA_CODE;
  this->camera_id.index = this->cameraIndex;
  this->camera_id.port = this->device_id.port;
  this->camera = deviceTable->GetDriver(this->camera_id);

  if (!this->camera)
  {
    PLAYER_ERROR("unable to locate suitable camera device");
    return(-1);
  }
  if (this->camera->Subscribe(this->camera_id) != 0)
  {
    PLAYER_ERROR("unable to subscribe to camera device");
    return(-1);
  }

  // Load the shape model
  if (this->LoadModel() != 0)
    return -1;

  // Start the driver thread.
  this->StartThread();

  //cvNamedWindow(winName,1);
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int SimpleShape::Shutdown()
{
  // Stop the driver thread.
  StopThread();
  
  // Unsubscribe from devices.
  this->camera->Unsubscribe(this->camera_id);

  // Free images
  if (this->inpImage)
    cvReleaseImage(&(this->inpImage));
  if (this->outImage)
    cvReleaseImage(&(this->outImage));

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void SimpleShape::Main() 
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
      this->WriteBlobfinderData();
      this->WriteCameraData();
    }

    // Process any pending requests.
    this->HandleRequests();
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process requests.  Returns 1 if the configuration has changed.
int SimpleShape::HandleRequests()
{
  void *client;
  char request[PLAYER_MAX_REQREP_SIZE];
  int len;
  while ((len = GetConfig(&client, &request, sizeof(request),NULL)) > 0)
  {
    switch (request[0])
    {
      default:
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
    }
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Grag new camera data; returns true if there is new data to process.
bool SimpleShape::UpdateCamera()
{
  size_t size;

  // Get the camera data.
  size = this->camera->GetData(this->camera_id, &this->cameraData,
                               sizeof(this->cameraData), &this->cameraTime);
  
  // Do some byte swapping
  this->cameraData.width = ntohs(this->cameraData.width);
  this->cameraData.height = ntohs(this->cameraData.height);
  this->cameraData.depth = this->cameraData.depth;
  this->cameraData.format = this->cameraData.format;
  this->cameraData.compression = this->cameraData.compression;
  this->cameraData.image_size = ntohl(this->cameraData.image_size);

  if (this->cameraData.compression != PLAYER_CAMERA_COMPRESS_RAW)
  {
    PLAYER_WARN("camera data is compressed");
    return false;
  }

  return true;
}


////////////////////////////////////////////////////////////////////////////////
// Load a shape model
int SimpleShape::LoadModel()
{
  IplImage *img, *work;
  CvSize size;
  CvSeq *contour, *maxContour;
  double area, maxArea;

  // Load the image
  img = cvLoadImage(this->modelFilename, 0);
  if (img == NULL)
  {
    PLAYER_ERROR("failed to load model file");
    return -1;
  }

  // Create work image
  size = cvSize(img->width, img->height);
  work = cvCreateImage(size, IPL_DEPTH_8U, 1);

  // Find edges
  cvCanny(img, work, this->cannyThresh1, this->cannyThresh2);

  // Extract contours
  this->modelStorage = cvCreateMemStorage(0);
  cvFindContours(work, this->modelStorage, &contour, sizeof(CvContour), 
                 CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
  
  // Find the contour with the largest area (we will use the outer
  // contour only)
  maxArea = 0;
  maxContour = NULL;
  for (; contour != NULL; contour = contour->h_next)
  {
    area = fabs(cvContourArea(contour));
    if (area > maxArea)
    {
      maxArea = area;
      maxContour = contour;
    }
  }
  if (maxContour == NULL)
  {
    PLAYER_ERROR("no usable contours in model image");
    return -1;
  }
  this->modelContour = maxContour;

  // Record some features of the contour; we will use these to
  // recognise it later.
  this->ExtractFeatureSet((CvContour*) this->modelContour, &this->modelFeatureSet);

  // Free the image
  cvReleaseImage(&work);
  cvReleaseImage(&img);

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Look for stuff in the image.
void SimpleShape::ProcessImage()
{
  CvSize size;
  int width, height;

  width = this->cameraData.width;
  height = this->cameraData.height;
  
  // Create input and output image if it doesnt exist
  size = cvSize(width, height);
  if (this->inpImage == NULL)
    this->inpImage = cvCreateImage(size, IPL_DEPTH_8U, 1);
  size = cvSize(2 * width, 2 * height);
  if (this->outImage == NULL)
  {
    this->outImage = cvCreateImage(size, IPL_DEPTH_8U, 1);
    cvGetSubRect(this->outImage, this->outSubImages + 0, cvRect(0, 0, width, height));
    cvGetSubRect(this->outImage, this->outSubImages + 1, cvRect(width, 0, width, height));
    cvGetSubRect(this->outImage, this->outSubImages + 2, cvRect(0, height, width, height));
    cvGetSubRect(this->outImage, this->outSubImages + 3, cvRect(width, height, width, height));
  }

  // Create a main image and copy in the pixels
  switch (this->cameraData.format)
  {
    case PLAYER_CAMERA_FORMAT_GREY8:
    {
      // Copy pixels to input image (grayscale)
      assert(this->inpImage->imageSize >= (int) this->cameraData.image_size);
      memcpy(this->inpImage->imageData, this->cameraData.image, this->inpImage->imageSize);
      break;
    }
    default:
      PLAYER_WARN1("image format [%d] is not supported", this->cameraData.format);
  }

  // Copy original image to output
  if (this->out_camera_id.port)
  {
    cvSetZero(this->outImage);
    cvCopy(this->inpImage, this->outSubImages + 0);
  }

  // Clone the input image to our workspace
  this->workImage = cvCloneImage(this->inpImage);

  // Find all the shapes in the working image
  this->FindShapes();

  // Free temp storage
  cvReleaseImage(&this->workImage);
  this->workImage = NULL;
    
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Having pre-processed the image, find some shapes
void SimpleShape::FindShapes()
{
  int i;
  double sim[3];
  double area;
  FeatureSet featureSet;
  CvMemStorage *storage;
  CvSeq *contour;
  CvRect rect;
  Shape *shape;

  // Reset the shape count
  this->shapeCount = 0;

  // Find edges
  cvCanny(this->workImage, this->workImage, this->cannyThresh1, this->cannyThresh2);

  // Copy edges to output image
  if (this->out_camera_id.port)
    cvCopy(this->workImage, this->outSubImages + 1);

  // Find contours on a binary image
  storage = cvCreateMemStorage(0);
  cvFindContours(this->workImage, storage, &contour, sizeof(CvContour), 
                 CV_RETR_LIST, CV_CHAIN_APPROX_NONE);
    
  printf("\n");
  
  for(; contour != NULL; contour = contour->h_next)
  {
    area = fabs(cvContourArea(contour));
    rect = cvBoundingRect(contour);
        
    // Discard very small contours
    if (area < 5 * 5)
      continue;

    // Discard the countour generated from the image border
    if (rect.x < 5 || rect.y < 5)
      continue;
    if (rect.x + rect.width >= this->workImage->width - 5)
      continue;
    if (rect.y + rect.height >= this->workImage->height - 5)
      continue;

    // Draw eligable contour on the output image; useful for debugging
    if (this->out_camera_id.port)
      cvDrawContours(this->outSubImages + 2, contour, CV_RGB(255, 255, 255),
                     CV_RGB(255, 255, 255), 0, 1, 8);

    // Compute the contour features
    this->ExtractFeatureSet((CvContour*) contour, &featureSet);
            
    // Compute similarity based on Hu moments (smaller is better)
    sim[0] = cvMatchShapes(contour, this->modelContour, CV_CONTOURS_MATCH_I2);

    // Compute similarity based on compactness (smaller is better)
    sim[1] = fabs(featureSet.compact - this->modelFeatureSet.compact);

    // Compute similarity based on elliptical variance (smaller is better)
    sim[2] = fabs(featureSet.variance - this->modelFeatureSet.variance);

    printf("features %f %.4f/%.4f %.4f/%.4f\n",
           sim[0],
           featureSet.compact, this->modelFeatureSet.compact,
           featureSet.variance, this->modelFeatureSet.variance);
    
    // Check if the contour matches the model
    for (i = 0; i < 3; i++)
      if (sim[i] > this->matchThresh[i])
        break;
    if (i < 3)
      continue;

    // Draw contour on the main image; useful for debugging
    if (this->out_camera_id.port)
    {
      cvDrawContours(this->outSubImages + 3, contour, CV_RGB(128, 128, 128),
                     CV_RGB(128, 128, 128), 0, 1, 8);
      /*
        cvRectangle(this->outSubImages + 3, cvPoint(rect.x, rect.y),
        cvPoint(rect.x + rect.width, rect.y + rect.height), CV_RGB(255, 255, 255), 1);
      */
    }

    // Check for overrun
    if (this->shapeCount >= sizeof(this->shapes) / sizeof(this->shapes[0]))
    {
      PLAYER_WARN("image contains too many shapes");
      break;
    }
    
    // Add the shape to our internal list
    shape = this->shapes + this->shapeCount++;
    shape->id = -1;
    shape->ax = rect.x;
    shape->ay = rect.y;
    shape->bx = rect.x + rect.width;
    shape->by = rect.y + rect.height;
  }

  cvReleaseMemStorage(&storage);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Extract a feature set for the given contour
void SimpleShape::ExtractFeatureSet(CvContour *contour, FeatureSet *feature)
{
  int i;
  CvBox2D box;
  CvPoint *p;
  double aa, bb;
  double dx, dy;
  double var;
  
  // Get the moments (we will use the Hu invariants)
  cvMoments(contour, &feature->moments);

  // Compute the compactness measure: perimeter squared divided by area
  feature->compact = (contour->total * contour->total) / fabs(cvContourArea(contour));

  // Compute elliptical variance
  box = cvFitEllipse2(contour);

  //printf("%f %f %f %f\n", box.center.x, box.center.y, box.size.width / 2, box.size.height / 2);

  aa = box.size.width * box.size.width / 4;
  bb = box.size.height * box.size.height / 4;

  var = 0.0;
  for (i = 0; i < contour->total; i++)
  {
    p = CV_GET_SEQ_ELEM(CvPoint, contour, i);
    dx = (p->x - box.center.x);
    dy = (p->y - box.center.y);
    var += dx * dx / aa + dy * dy / bb;
  }
  var /= contour->total;

  feature->variance = var;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Write blobfinder data
void SimpleShape::WriteBlobfinderData()
{
  unsigned int i;
  int shapeCount, channelCount, channel;
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
   
  // Copy data to server.
  this->PutData(this->blobfinder_id, &data, sizeof(data), &this->cameraTime);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Write camera data; this is a little bit naughty: we re-use the
// input camera data, but modify the pixels
void SimpleShape::WriteCameraData()
{
  size_t size;
  
  if (this->camera_id.port == 0)
    return;
  if (this->outImage == NULL)
    return;

  // Do some byte swapping
  this->outCameraData.width = htons(this->outImage->width);
  this->outCameraData.height = htons(this->outImage->height);
  this->outCameraData.depth = 8;
  this->outCameraData.format = PLAYER_CAMERA_FORMAT_GREY8;
  this->outCameraData.compression = PLAYER_CAMERA_COMPRESS_RAW;
  this->outCameraData.image_size = htonl(this->outImage->imageSize);

  // Copy in the pixels
  memcpy(this->outCameraData.image, this->outImage->imageData, this->outImage->imageSize);

  // Compute message size
  size = sizeof(this->outCameraData) - sizeof(this->outCameraData.image)
    + this->outImage->imageSize;

  // Copy data to server
  this->PutData(this->out_camera_id, &this->outCameraData, size, &this->cameraTime);
  
  return;
}


