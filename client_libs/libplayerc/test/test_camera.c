/***************************************************************************
 * Desc: Tests for the camera device
 * Author: Andrew Howard
 * Date: 26 May 2002
 # CVS: $Id$
 **************************************************************************/

#include "test.h"
#include "playerc.h"


static void test_camera_save(playerc_camera_t *device, const char *filename);


// Basic test for camera device.
int test_camera(playerc_client_t *client, int index)
{
  int t;
  void *rdevice;
  playerc_camera_t *device;
  char filename[128];
  int csize, usize;

  printf("device [camera] index [%d]\n", index);

  device = playerc_camera_create(client, index);

  TEST("subscribing (read)");
  if (playerc_camera_subscribe(device, PLAYER_READ_MODE) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();

  for (t = 0; t < 10; t++)
  {
    TEST1("reading data (attempt %d)", t);

    do
      rdevice = playerc_client_read(client);
    while (rdevice == client);

    if (rdevice == device)
    {
      PASS();

      // Decompress the image
      csize = device->image_size;
      playerc_camera_decompress(device);
      usize = device->image_size;
      
      printf("camera: [w %d h %d d %d] [%d/%d bytes]\n", 
             device->width, device->height, device->depth, csize, usize);

      snprintf(filename, sizeof(filename), "camera_%03d.ppm", t);
      printf("camera: saving [%s]\n", filename);
      test_camera_save(device, filename);
    }
    else
      FAIL();
  }

  TEST("unsubscribing");
  if (playerc_camera_unsubscribe(device) != 0)
  {
    FAIL();
    return -1;
  }
  PASS();
  
  playerc_camera_destroy(device);
  
  return 0;
}


// Save a camera image
// Assumes the image is RGB888
void test_camera_save(playerc_camera_t *device, const char *filename)
{
  int i;
  uint8_t pix;
  FILE *file;

  file = fopen(filename, "w+");
  if (file == NULL)
    return;

  // Write ppm header
  fprintf(file, "P6\n%d %d\n%d\n", device->width, device->height, 255);
  
  // Write data here
  for (i = 0; i < device->image_size; i++)
  {
    if (device->format == PLAYER_CAMERA_FORMAT_RGB888)
    {
      pix = device->image[i];
      fputc(pix, file);
    }
    else if (device->format == PLAYER_CAMERA_FORMAT_GREY8)
    {
      pix = device->image[i];
      fputc(pix, file);
      fputc(pix, file);
      fputc(pix, file);
    }
    else
    {
      printf("unsupported image format");
      break;
    }
  }

  fclose(file);

  return;
}

