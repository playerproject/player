/*
 * $Id$
 *
 * a test for the C++ CameraProxy
 */

#include "playerclient.h"
#include "test.h"
#include <unistd.h>
#include <math.h>

int test_camera(PlayerClient *client, int index)
{
  unsigned char image[PLAYER_CAMERA_IMAGE_SIZE];

  unsigned char access;
  CameraProxy cp(client, 0, 'c');

  printf("device [camera] index[%d]\n", index);
  
  TEST("subscribing (read)");

  if ((cp.ChangeAccess(PLAYER_READ_MODE, &access) < 0) ||
    (access != PLAYER_READ_MODE))
  {
    FAIL();
    printf("DRIVER: %s\n", cp.driver_name);
    return -1;
  }
  
  PASS();

  printf("DRIVER: %s\n", cp.driver_name);

  for (;;)
  {
  client->Read();

  printf ("Width [%d] Height[%d] ImageSize[%d]\n",cp.width, cp.height, cp.imageSize);

  FILE *fp = fopen("testFrame.ppm", "wb");

  if (fp == NULL)
  {
    printf("Couldn't create image file");
    return -1;
  }

  fprintf(fp,"P6\n%u %u\n255\n",cp.width, cp.height);

  fwrite ((unsigned char*)cp.image, 1, cp.imageSize, fp);

  fclose(fp);
  }
}
