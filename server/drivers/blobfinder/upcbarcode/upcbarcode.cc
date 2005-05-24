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

/** @addtogroup drivers Drivers */
/** @{ */
/** @defgroup player_driver_upcbarcode upcbarcode

The upcbarcode driver searches for standard, single-digit UPC barcodes
in a camera image (a sample barcode is shown below)

@image html upc.jpg "Sample UPC barcode (one digit)"

@par Compile-time dependencies

- OpenCV

@par Provides

- @ref player_interface_blobfinder : outputs blob information on detected 
  barcodes
- @ref player_interface_camera : passes through image data from underlying 
  camera device (optional)

@par Requires

- @ref player_interface_camera

@par Configuration requests

- none

@par Configuration file options

- edgeThresh (float)
  - Default: 20
  - Edge threshold
- bit_width (length)
  - Default: 0.08 m
  - Width of a single bit
- bit_count (integer)
  - Default: 3
  - Number of bits per digit
- guardMin (integer)
  - Default: 3
  - Minimum height of bit (pixels)
- guardTol (length)
  - Default: 0.2 m
  - Height tolerance for bit (ratio)
- digit_errFirst (float)
  - Default: 0.5
  - Error threshold on the best bit
- digit_errSecond (float)
  - Default: 1.0
  - Error threshold on the second-best bit

@par Example

@verbatim
driver
(
  name "upcbarcode"
  provides ["blobfinder:0"]
  requires ["camera:0"]
)
@endverbatim

@par Authors

Andrew Howard

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

#include "error.h"
#include "driver.h"
#include "devicetable.h"
#include "drivertable.h"
#include "clientdata.h"
#include "clientmanager.h"

extern ClientManager* clientmanager;

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

  // Process incoming messages from clients 
  int ProcessMessage(ClientData * client, player_msghdr * hdr, uint8_t * data, uint8_t * resp_data, int * resp_len);

  // Main function for device thread.
  private: virtual void Main();

  // Process requests.  Returns 1 if the configuration has changed.
  //private: int HandleRequests();

  // Handle geometry requests.
  //private: void HandleGetGeom(void *client, void *req, int reqlen);
  
  // Process any new camera data.
  //private: int ReadCamera();

  // Look for barcodes in the image.  
  private: void ProcessImage();

  // Extract a bit string from the image.  
  private: int ExtractSymbols(int x, int symbol_max_count, int symbols[][2]);

  // Extract a code from a symbol string.
  private: int ExtractCode(int symbol_count, int symbols[][2], int *min, int *max);
  
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
  private:
  Driver *camera;
  player_device_id_t camera_id;
  struct timeval cameraTime;  
  player_camera_data_t cameraData;
  bool NewCamData;
	
  ClientDataInternal * BaseClient;
  
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
  if(this->AddInterface(this->blobfinder_id, PLAYER_READ_MODE) != 0)
  {
    this->SetError(-1);    
    return;
  }

  // Optionally have a camera interface
  if (cf->ReadDeviceId(&this->out_camera_id, section, "provides",
                       PLAYER_CAMERA_CODE, -1, NULL) == 0)
  {
    if(this->AddInterface(this->out_camera_id, PLAYER_READ_MODE) != 0)
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
  this->BaseClient = NULL;
  this->NewCamData = false;

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
  BaseClient = new ClientDataInternal(this);
  clientmanager->AddClient(BaseClient);

  // Subscribe to the camera.
  this->camera = deviceTable->GetDriver(this->camera_id);
  if (!this->camera)
  {
    PLAYER_ERROR("unable to locate suitable camera device");
    return(-1);
  }
  
  if (BaseClient->Subscribe(this->camera_id) != 0)
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
  BaseClient->Unsubscribe(this->camera_id);

  clientmanager->RemoveClient(BaseClient);
  delete BaseClient;
  BaseClient = NULL;

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
    Lock();
    if (NewCamData)
    {
      NewCamData = false;
      Unlock();
      this->ProcessImage();
      this->WriteBlobfinderData();
      this->WriteCameraData();      
    }
    else
    	Unlock();

    // Process any pending requests.
    this->ProcessMessages();
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Process an incoming message
int UPCBarcode::ProcessMessage(ClientData * client, player_msghdr * hdr, uint8_t * data, uint8_t * resp_data, int * resp_len)
{
  assert(hdr);
  assert(data);
  assert(resp_data);
  assert(resp_len);
  assert(*resp_len==PLAYER_MAX_MESSAGE_SIZE);
  
  *resp_len = 0;

  /* TODO
  if (MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_BLOBFINDER_GET_GEOM, device_id))
  {
  }
  */
 
  if (MatchMessage(hdr, PLAYER_MSGTYPE_DATA, 0, camera_id))
  {
  	assert(hdr->size == sizeof(this->cameraData));
  	player_camera_data_t * cam_data = reinterpret_cast<player_camera_data_t * > (data);
  	Lock();
    this->cameraData.width = ntohs(cam_data->width);
    this->cameraData.height = ntohs(cam_data->height); 
    this->cameraData.bpp = cam_data->bpp;
    this->cameraData.image_size = ntohl(cam_data->image_size);
    this->cameraData.format = cam_data->format;
    this->cameraData.compression = cam_data->compression; 
    this->NewCamData=true;
  	Unlock();
  	return 0;
  }
 
  return -1;
}


////////////////////////////////////////////////////////////////////////////////
// Process any new camera data.
/*int UPCBarcode::ReadCamera()
{
  size_t size;

  // Get the camera data.
  size = this->camera->GetData(this->camera_id, &this->cameraData,
                               sizeof(this->cameraData), &this->cameraTime);
  
  // Do some byte swapping
  this->cameraData.width = ntohs(this->cameraData.width);
  this->cameraData.height = ntohs(this->cameraData.height); 
  this->cameraData.bpp = this->cameraData.bpp;
  this->cameraData.image_size = ntohl(this->cameraData.image_size);
  this->cameraData.format = this->cameraData.format;
  this->cameraData.compression = this->cameraData.compression;
  
  return 1;
}
*/

////////////////////////////////////////////////////////////////////////////////
// Look for barcodes in the image.  This looks for verical barcodes,
// and assumes barcodes are not placed above each other.
void UPCBarcode::ProcessImage()
{
  int x, step_x;
  int id, min, max;
  int symbol_count;
  int symbols[PLAYER_CAMERA_IMAGE_HEIGHT][2];
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
    case PLAYER_CAMERA_FORMAT_MONO8:
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
    id = this->ExtractCode(symbol_count, symbols, &min, &max);

    if (id >= 0)
      printf("%d %d %d\n", x, min, id);

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
      blob->ay = min;
      blob->by = this->cameraData.height - 2;

      if (this->out_camera_id.port)
      {
        /*
        cvRectangle(this->outSubImages + 0, cvPoint(blob->ax, blob->ay),
                    cvPoint(blob->bx, blob->by), CV_RGB(0, 0, 0), 1);
        cvRectangle(this->outSubImages + 1, cvPoint(blob->ax, blob->ay),
                    cvPoint(blob->bx, blob->by), CV_RGB(255, 255, 255), 1);
        */
      }
    }

    // If we have an open blob, and got an id, continue the blob
    else if (blob != NULL && id >= 0)
    {
      blob->bx = x + 1;
    }
  }

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Extract a bit string from the image.  Takes a vertical column in
// the image and applies an edge detector
int UPCBarcode::ExtractSymbols(int x, int symbol_max_count, int symbols[][2])
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
    for (j = -1; j <= 1; j++)
    {
      fv += kernel[j + 2] * (*cvPtr2D(this->inpImage, i + j, x));
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
        symbols[symbol_count][0] = i;
        symbols[symbol_count++][1] = -(i - start);
        state = 1;
        start = i;
      }
    }
    else if (state == 1)
    {
      if (fv < -this->edgeThresh)
      {
        symbols[symbol_count][0] = i;
        symbols[symbol_count++][1] = +(i - start);
        state = 0;
        start = i;
      }
    }

    // TESTING
    *cvPtr2D(this->outSubImages + 1, i, x) = 127 + 127 * state;;
    
    //fprintf(file, "%d %d %f %f %d\n", i, this->cameraData.image[pix], fv, fn, state);
  }

  if (state == 0)
  {
    symbols[symbol_count][0] = i;
    symbols[symbol_count++][1] = -(i - start);
  }
  else if (state == 1)
  {
    symbols[symbol_count][0] = i;
    symbols[symbol_count++][1] = +(i - start);
  }

  //fprintf(file, "\n\n");
  //fclose(file);

  return symbol_count;
}


////////////////////////////////////////////////////////////////////////////////
// Extract a code from a symbol string.
int UPCBarcode::ExtractCode(int symbol_count, int symbols[][2], int *miny, int *maxy)
{
  int i, j, k;
  double a, b, c;
  double mean, min, max, wm, wo;
  int best_digit, best_miny;
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
  best_miny = INT_MAX;
      
  // Note that each code has seven symbols in it, not counting the
  // initial space.
  for (i = 0; i < symbol_count - 7; i++)
  {
    /*
    for (j = 0; j < 7; j++)
      printf("%+d", symbols[i + j]);
    printf("\n");
    */

    a = symbols[i][1];
    b = symbols[i + 1][1];
    c = symbols[i + 2][1];

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
      best_miny = INT_MAX;
      
      // Read the code digit (4 symbols) and compare against the known
      // digit patterns
      for (k = 0; k < (int) (sizeof(digits) / sizeof(digits[0])); k++)
      {
        err[k] = 0;        
        for (j = 0; j < 4; j++)
        {
          wm = digits[k][j];
          wo = symbols[i + 3 + j][1] / mean;
          err[k] += fabs(wo - wm);
          //printf("digit %d = %.3f %.3f\n", k, wm, wo);
        }
        //printf("digit %d = %.3f\n", k, err[k]);

        if (err[k] < best_err)
        {
          best_err = err[k];
          best_digit = k;
          best_miny = symbols[i][0];
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

  *miny = best_miny;
  
  return best_digit;
}


////////////////////////////////////////////////////////////////////////////////
// Update the device data (the data going back to the client).
void UPCBarcode::WriteBlobfinderData()
{
  int i;
  blob_t *blob;
  size_t size;
  player_blobfinder_data_t data;

  data.width = htons(this->cameraData.width);
  data.height = htons(this->cameraData.height);

  data.blob_count = htons(this->blobCount);
    
  for (i = 0; i < this->blobCount; i++)
  {
    blob = this->blobs + i;

    data.blobs[i].id = 0;  // TODO
    data.blobs[i].color = 0;  // TODO
    data.blobs[i].area = htonl((int) ((blob->bx - blob->ax) * (blob->by - blob->ay)));
    data.blobs[i].x = htons((int) ((blob->bx + blob->ax) / 2));
    data.blobs[i].y = htons((int) ((blob->by + blob->ay) / 2));
    data.blobs[i].left = htons((int) (blob->ax));
    data.blobs[i].right = htons((int) (blob->ay));
    data.blobs[i].top = htons((int) (blob->bx));
    data.blobs[i].bottom = htons((int) (blob->by));
    data.blobs[i].range = htons(0);
  }
    
  // Copy data to server
  size = sizeof(data) - sizeof(data.blobs) + this->blobCount * sizeof(data.blobs[0]);
  PutMsg(blobfinder_id, NULL, PLAYER_MSGTYPE_DATA, 0, (unsigned char*) &data, size, &this->cameraTime);		
  
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
  this->outCameraData.bpp = 8;
  this->outCameraData.format = PLAYER_CAMERA_FORMAT_MONO8;
  this->outCameraData.compression = PLAYER_CAMERA_COMPRESS_RAW;
  this->outCameraData.image_size = htonl(this->outImage->imageSize);

  // Copy in the pixels
  memcpy(this->outCameraData.image, this->outImage->imageData, this->outImage->imageSize);

  // Compute message size
  size = sizeof(this->outCameraData) - sizeof(this->outCameraData.image)
    + this->outImage->imageSize;

  // Copy data to server
  //this->PutData(this->out_camera_id, &this->outCameraData, size, &this->cameraTime);
  PutMsg(out_camera_id, NULL, PLAYER_MSGTYPE_DATA, 0, &this->outCameraData, size, &this->cameraTime);		
  
  return;
}
