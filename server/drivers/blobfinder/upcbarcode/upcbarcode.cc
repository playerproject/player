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
#include <opencv/highgui.h>

#include "driver.h"
#include "devicetable.h"
#include "drivertable.h"


// Info on potential blobs.
struct blob_t
{
  // Id (-1) if undetermined.
  int id;

  // Blob bounding coords
  int ax, ay, bx, by;
};


// Driver for detecting laser retro-reflectors.
class UPCBarcode : public Driver
{
  // Constructor
  public: UPCBarcode( ConfigFile* cf, int section);

  // Setup/shutdown routines.
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Main function for device thread.
  private: virtual void Main();

  // Process requests.  Returns 1 if the configuration has changed.
  private: int HandleRequests();

  // Handle geometry requests.
  private: void HandleGetGeom(void *client, void *req, int reqlen);
  
  // Process any new camera data.
  private: int ReadCamera();

  // Look for barcodes in the image.  
  private: void ProcessImage();

  // Extract a bit string from the image.  
  private: int ExtractSymbols(int x, int symbol_max_count, int symbols[]);

  // Extract a code from a symbol string.
  private: int ExtractCode(int symbol_count, int symbols[]);
  
  // Write the device data (the data going back to the client).
  private: void WriteBlobfinderData();

  // Write the device data (the data going back to the client).
  private: void WriteCameraData();

  // Output devices
  private: player_device_id_t blobfinder_id;
  private: player_device_id_t out_camera_id;

  // Image processing
  private: double edgeThresh;
  
  // Barcode tolerances
  private: int barcount;
  private: double barwidth;
  private: double guardMin, guardTol;
  private: double errFirst, errSecond;

  // Input camera stuff
  private: player_device_id_t camera_id;
  private: Driver *camera;
  private: struct timeval cameraTime;
  private: player_camera_data_t cameraData;

  // Images
  private: IplImage *inpImage;
  private: IplImage *outImage;
  private: CvMat outSubImages[4];
  
  // Output camera stuff
  private: player_camera_data_t outCameraData;

  // List of currently tracked blobs.
  private: int blobCount;
  private: blob_t blobs[256];
};


// Initialization function
Driver* UPCBarcode_Init( ConfigFile* cf, int section)
{
  return ((Driver*) (new UPCBarcode( cf, section)));
}


// a driver registration function
void UPCBarcode_Register(DriverTable* table)
{
  table->AddDriver("upcbarcode", UPCBarcode_Init);
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
UPCBarcode::UPCBarcode( ConfigFile* cf, int section)
    : Driver(cf, section)
{
  
  // Must have a blobfinder interface
  if (cf->ReadDeviceId(&this->blobfinder_id, section, "provides",
                       PLAYER_BLOBFINDER_CODE, -1, NULL) != 0)
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
  if (cf->ReadDeviceId(&this->out_camera_id, section, "provides",
                       PLAYER_CAMERA_CODE, -1, NULL) == 0)
  {
    if (this->AddInterface(this->out_camera_id, PLAYER_READ_MODE,
                           sizeof(player_camera_data_t), 0, 1, 1) != 0)
    {
      this->SetError(-1);    
      return;
    }
  }
  else
    memset(&this->out_camera_id.code, 0, sizeof(player_device_id_t));

  // Must have an input camera
  if (cf->ReadDeviceId(&this->camera_id, section, "requires",
                       PLAYER_CAMERA_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);    
    return;
  }

  // Other camera settings
  this->camera = NULL;

  // Image workspace
  this->inpImage = NULL;
  this->outImage = NULL;

  // Image processing
  this->edgeThresh = cf->ReadFloat(section, "edgeThresh", 20);
  
  // Default blobfinder properties.
  this->barwidth = cf->ReadLength(section, "bit_width", 0.08);
  this->barcount = cf->ReadInt(section, "bit_count", 3);

  // Barcode properties: minimum height (pixels), height tolerance (ratio).
  this->guardMin = cf->ReadInt(section, "guardMin", 3);
  this->guardTol = cf->ReadLength(section, "guardTol", 0.20);

  // Error threshold on the first and second best digits
  this->errFirst = cf->ReadFloat(section, "digit_errFirst", 0.5);
  this->errSecond = cf->ReadFloat(section, "digit_errSecond", 1.0);

  // Reset blob list.
  this->blobCount = 0;

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Set up the device (called by server thread).
int UPCBarcode::Setup()
{
  // Subscribe to the camera.
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

  // Start the driver thread.
  this->StartThread();
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Shutdown the device (called by server thread).
int UPCBarcode::Shutdown()
{
  // Stop the driver thread.
  StopThread();
  
  // Unsubscribe from devices.
  this->camera->Unsubscribe(this->camera_id);

  if (this->inpImage)
    cvReleaseImage(&(this->inpImage));
  if (this->outImage)
    cvReleaseImage(&(this->outImage));

  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void UPCBarcode::Main() 
{
  while (true)
  {
    // Let the camera drive update rate
    this->camera->Wait();

    // Test if we are supposed to cancel this thread.
    pthread_testcancel();

    // Process any new camera data.
    if (this->ReadCamera())
    {
      this->ProcessImage();
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
int UPCBarcode::HandleRequests()
{
  void *client;
  char request[PLAYER_MAX_REQREP_SIZE];
  int len;
  
  while ((len = GetConfig(&client, &request, sizeof(request),NULL)) > 0)
  {
    switch (request[0])
    {
      /* TODO
      case PLAYER_BLOBFINDER_GET_GEOM:
        HandleGetGeom(client, request, len);
        break;
      */
      default:
        if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
          PLAYER_ERROR("PutReply() failed");
        break;
    }
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Handle foo requests.
// TODO
void UPCBarcode::HandleGetGeom(void *client, void *request, int len)
{
  if (PutReply(client, PLAYER_MSGTYPE_RESP_NACK,NULL) != 0)
    PLAYER_ERROR("PutReply() failed");

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process any new camera data.
int UPCBarcode::ReadCamera()
{
  size_t size;

  // Get the camera data.
  size = this->camera->GetData(this->camera_id, &this->cameraData,
                               sizeof(this->cameraData), &this->cameraTime);
  
  // Do some byte swapping
  this->cameraData.width = ntohs(this->cameraData.width);
  this->cameraData.height = ntohs(this->cameraData.height); 
  this->cameraData.depth = this->cameraData.depth;
  this->cameraData.image_size = ntohl(this->cameraData.image_size);
  this->cameraData.format = this->cameraData.format;
  this->cameraData.compression = this->cameraData.compression;
  
  return 1;
}


////////////////////////////////////////////////////////////////////////////////
// Look for barcodes in the image.  This looks for verical barcodes,
// and assumes barcodes are not placed above each other.
void UPCBarcode::ProcessImage()
{
  int x, step_x;
  int id;
  int symbol_count;
  int symbols[PLAYER_CAMERA_IMAGE_HEIGHT];
  blob_t *blob;

  int width, height;

  width = this->cameraData.width;
  height = this->cameraData.height;

  // Create input image if it doesnt exist
  if (this->inpImage == NULL)
    this->inpImage = cvCreateImage(cvSize(width, height), IPL_DEPTH_8U, 1);

  // Copy pixels into input image
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
    {
      PLAYER_WARN1("image format [%d] is not supported", this->cameraData.format);
      return;
    }
  }

  // Create an output image if we dont have one already.
  if (this->outImage == NULL)
  {
    this->outImage = cvCreateImage(cvSize(2 * width, height), IPL_DEPTH_8U, 1);
    cvGetSubRect(this->outImage, this->outSubImages + 0, cvRect(0, 0, width, height));
    cvGetSubRect(this->outImage, this->outSubImages + 1, cvRect(width, 0, width, height));
  }

  // Copy original image to output
  if (this->out_camera_id.port)
  {
    cvSetZero(this->outImage);
    cvCopy(this->inpImage, this->outSubImages + 0);

    // TESTING
    cvSaveImage("out.pgm", this->outImage);
  }

  step_x = 16;
  
  this->blobCount = 0;
  blob = NULL;

  // Process image columns
  for (x = 0; x < width; x += step_x)
  {
    // Extract raw symbols
    symbol_count = this->ExtractSymbols(x, sizeof(symbols) / sizeof(symbols[0]), symbols);

    // Identify barcode
    id = this->ExtractCode(symbol_count, symbols);

    if (id >= 0)
      printf("%d %d\n", x, id);

    // If we have an open blob, and didnt get the same id, close the blob
    if (blob != NULL && id != blob->id)
    {
      this->blobCount++;
      blob = NULL;
    }

    // If we dont have a blob, and we got an id, open a blob
    if (blob == NULL && id >= 0)
    {
      assert(this->blobCount < (int) (sizeof(this->blobs) / sizeof(this->blobs[0])));
      blob = this->blobs + this->blobCount;
      blob->id = id;
      blob->ax = x;
      blob->bx = x + 1;
      blob->ay = 0;
      blob->by = this->cameraData.height;
    }

    // If we have an open blob, and got an id, continue the blob
    else if (blob != NULL && id >= 0)
    {
      blob->bx = x + 1;
    }
  }

  printf("\n");

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Extract a bit string from the image.  Takes a vertical column in
// the image and applies an edge detector
int UPCBarcode::ExtractSymbols(int x, int symbol_max_count, int symbols[])
{
  int i, j, off, inc, pix;
  double fn, fv;
  int state, start, symbol_count;
  double kernel[] = {+1, +2, 0, -2, -1};

  off = x;
  inc = this->inpImage->width;

  assert(symbol_max_count >= this->inpImage->height);

  state = -1;
  start = -1;
  symbol_count = 0;

  for (i = 2, pix = off + 2 * inc; i < this->inpImage->height - 2; i++, pix += inc)
  {
   // Run an edge detector
    fn = fv = 0.0;
    for (j = -2; j <= 2; j++)
    {
      fv += kernel[j + 2] * this->inpImage->imageData[pix + j * inc];
      fn += fabs(kernel[j + 2]);
    }
    fv /= fn;
    
    // Pick the transitions
    if (state == -1)
    {
      if (fv > +this->edgeThresh)
      {
        state = 1;
        start = i;
      }
      else if (fv < -this->edgeThresh)
      {
        state = 0;
        start = i;
      }
    }
    else if (state == 0)
    {
      if (fv > +this->edgeThresh)
      {
        symbols[symbol_count++] = -(i - start);
        state = 1;
        start = i;
      }
    }
    else if (state == 1)
    {
      if (fv < -this->edgeThresh)
      {
        symbols[symbol_count++] = +(i - start);
        state = 0;
        start = i;
      }
    }

    //fprintf(file, "%d %d %f %f %d\n", i, this->cameraData.image[pix], fv, fn, state);
  }

  if (state == 0)
    symbols[symbol_count++] = -(i - start);
  else if (state == 1)
    symbols[symbol_count++] = +(i - start);

  //fprintf(file, "\n\n");
  //fclose(file);

  return symbol_count;
}


////////////////////////////////////////////////////////////////////////////////
// Extract a code from a symbol string.
int UPCBarcode::ExtractCode(int symbol_count, int symbols[])
{
  int i, j, k;
  double a, b, c;
  double mean, min, max, wm, wo;
  int best_digit;
  double best_err;
  double err[10];

  // These are UPC the mark-space patterns for digits.  From:
  // http://www.ee.washington.edu/conselec/Sp96/projects/ajohnson/proposal/project.htm
  double digits[][4] =
    {
      {-3,+2,-1,+1}, // 0
      {-2,+2,-2,+1}, // 1
      {-2,+1,-2,+2}, // 2
      {-1,+4,-1,+1}, // 3
      {-1,+1,-3,+2}, // 4
      {-1,+2,-3,+1}, // 5
      {-1,+1,-1,+4}, // 6
      {-1,+3,-1,+2}, // 7
      {-1,+2,-1,+3}, // 8
      {-3,+1,-1,+2}, // 9
    };

  best_digit = -1;
  
  // Note that each code has seven symbols in it, not counting the
  // initial space.
  for (i = 0; i < symbol_count - 7; i++)
  {
    /*
    for (j = 0; j < 7; j++)
      printf("%+d", symbols[i + j]);
    printf("\n");
    */

    a = symbols[i];
    b = symbols[i + 1];
    c = symbols[i + 2];

    // Look for a start guard: +N-N+N
    if (a > this->guardMin && b < -this->guardMin && c > this->guardMin)
    {
      mean = (a - b + c) / 3.0;
      min = MIN(a, MIN(-b, c));
      max = MAX(a, MAX(-b, c));
      assert(mean > 0);

      if ((mean - min) / mean > this->guardTol)
        continue;
      if ((max - mean) / mean > this->guardTol)
        continue;

      //printf("guard %d %.2f\n", i, mean);

      best_err = this->errFirst;
      best_digit = -1;
      
      // Read the code digit (4 symbols) and compare against the known
      // digit patterns
      for (k = 0; k < (int) (sizeof(digits) / sizeof(digits[0])); k++)
      {
        err[k] = 0;        
        for (j = 0; j < 4; j++)
        {
          wm = digits[k][j];
          wo = symbols[i + 3 + j] / mean;
          err[k] += fabs(wo - wm);
          //printf("digit %d = %.3f %.3f\n", k, wm, wo);
        }
        //printf("digit %d = %.3f\n", k, err[k]);

        if (err[k] < best_err)
        {
          best_err = err[k];
          best_digit = k;
        }
      }

      // Id is good if it fits one and *only* one pattern.  So find the
      // second best digit and make sure it has a much higher error.
      for (k = 0; k < (int) (sizeof(digits) / sizeof(digits[0])); k++)
      {
        if (k == best_digit)
          continue;
        if (err[k] < this->errSecond)
        {
          best_digit = -1;
          break;
        }
      }

      // Stop if we found a valid digit
      if (best_digit >= 0)
        break;
    }    
  }

  //if (best_digit >= 0)
  //  printf("best = %d\n", best_digit);

  return best_digit;
}


////////////////////////////////////////////////////////////////////////////////
// Update the device data (the data going back to the client).
void UPCBarcode::WriteBlobfinderData()
{
  int i;
  int blobCount, channel_count, channel;
  blob_t *blob;
  player_blobfinder_data_t data;

  data.width = htons(this->cameraData.width);
  data.height = htons(this->cameraData.height);

  // Reset the header data
  for (i = 0; i < PLAYER_BLOBFINDER_MAX_CHANNELS; i++)
  {
    data.header[i].index = 0;
    data.header[i].num = 0;
  }
  blobCount = 0;

  // Go through the blobs
  for (channel = 0; channel < PLAYER_BLOBFINDER_MAX_CHANNELS; channel++)
  {
    data.header[channel].index = htons(blobCount);
    channel_count = 0;
    
    for (i = 0; i < this->blobCount; i++)
    {
      blob = this->blobs + i;
      if (blob->id != channel)
        continue;

      data.blobs[blobCount].color = 0;  // TODO
      data.blobs[blobCount].area = htonl((int) ((blob->bx - blob->ax) * (blob->by - blob->ay)));
      data.blobs[blobCount].x = htons((int) ((blob->bx + blob->ax) / 2));
      data.blobs[blobCount].y = htons((int) ((blob->by + blob->ay) / 2));
      data.blobs[blobCount].left = htons((int) (blob->ax));
      data.blobs[blobCount].right = htons((int) (blob->ay));
      data.blobs[blobCount].top = htons((int) (blob->bx));
      data.blobs[blobCount].bottom = htons((int) (blob->by));
      data.blobs[blobCount].range = htons(0);
      channel_count++;
      blobCount++;
    }

    data.header[channel].num = htons(channel_count);
  }
    
  // Copy data to server.
  PutData((unsigned char*) &data, sizeof(data), &this->cameraTime);
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Write camera data; this is a little bit naughty: we re-use the
// input camera data, but modify the pixels
void UPCBarcode::WriteCameraData()
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
