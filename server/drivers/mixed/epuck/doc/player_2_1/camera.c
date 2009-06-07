#include <stdio.h>
#include <libplayerc/playerc.h>

int main(int argc, char *argv[])
{
  playerc_client_t *client;
  playerc_camera_t *camera;

  client = playerc_client_create(NULL, "localhost", 6665);
  if (playerc_client_connect(client) != 0)
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    return -1;
  }
  camera = playerc_camera_create(client, 0);
  if(playerc_camera_subscribe(camera, PLAYERC_OPEN_MODE) != 0)
  {
    fprintf(stderr, "CameraRerror: %s\n", playerc_error_str());
    return -1;
  }

  if(playerc_client_datamode(client, PLAYERC_DATAMODE_PULL) != 0)
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    return -1;
  }
  if(playerc_client_set_replace_rule(client, -1, -1,
                                     PLAYER_MSGTYPE_DATA, -1, 1) != 0)
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    return -1;
  }

  /* Get up to 50 images until we have a valid frame (width > 0) */
  int i, width;
  for (i=0, width=0;
       i < 50 && width==0 && NULL != playerc_client_read(client);
       ++i)
  {
    printf("camera: [w %d h %d d %d] [%d bytes]\n",
           camera->width, camera->height, camera->bpp, camera->image_count);

    width  = camera->width;
  }

  if(camera->format == PLAYER_CAMERA_FORMAT_MONO8)
  {
    playerc_camera_save(camera, "image.ppm");
  }
  else if(camera->format == PLAYER_CAMERA_FORMAT_RGB565)
  {
    char str_width[4], str_height[4];
    sprintf(str_width, "%d", camera->width);
    sprintf(str_height, "%d", camera->height);

    FILE *image = fopen("image.ppm", "w");
    fprintf(image, "P6\n%s %s\n255\n", str_width, str_height);

    unsigned char pixel1, pixel2;
    unsigned char red, green, blue;
    int i;
    for(i=0; i<camera->image_count; i+=2)
    {
      pixel1 = camera->image[i];
      pixel2 = camera->image[i+1];

      red   = (pixel1 & 0xF8) >> 3;
      green = ((pixel1 & 0x07)<<3)|((pixel2 & 0xE0)>>5);
      blue  = (pixel2 & 0x1F);

      red   = ((float)red/0x1F) * 255;
      green = ((float)green/0x3F) * 255;
      blue  = ((float)blue/0x1F) * 255;

      fputc(red, image);
      fputc(green, image);
      fputc(blue, image);
    }

    fclose(image);
  }

  playerc_camera_unsubscribe(camera);
  playerc_camera_destroy(camera);
  playerc_client_disconnect(client);
  playerc_client_destroy(client);

  return 0;
}
