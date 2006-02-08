#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "roomba_comms.h"

int
main(void)
{
  int i;
  roomba_comm_t* r;

  puts("foo");
  r = roomba_create("/dev/ttyS3");
  puts("foo");
  if(roomba_open(r) < 0)
    exit(-1);
  puts("foo");

  for(i=0;i<100;i++)
  {
    if(roomba_get_sensors(r, -1) < 0)
    {
      exit(-1);
    }
    puts("got data");
    usleep(20000);
  }
  roomba_close(r);
  return(0);
}
