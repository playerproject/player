#ifndef GZCAMERA_INIT
#define GZCAMERA_INIT

// Initialization function
CDevice* CMGzCamera_Init(char* interface, ConfigFile* cf, int section)
{ 
  //  GzClient::Init("default");

  if (GzClient::client == NULL)
  {
    PLAYER_ERROR("unable to instantiate Gazebo driver; did you forget the -g option?");
    return (NULL);
  }
  /*  if (strcmp(interface, PLAYER_CAMERA_STRING) != 0)
  {
    PLAYER_ERROR1("driver \"gz_camera\" does not support interface \"%s\"\n", interface);
    return (NULL);
    }*/
  return ((CDevice*) (new CMGzCamera(interface, cf, section)));
}

#endif
