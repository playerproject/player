
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "playernav.h"

extern double dumpfreq;

/* Parse command line arguments, of the form host:port */
int
parse_args(int argc, char** argv,
           int* num_bots, char** hostnames, int* ports)
{
  char* idx;
  int port;
  int hostlen;
  int i;

  // first look for -foo options
  for(i=0; (i<argc) && (argv[i][0] == '-'); i++)
  {
    if(!strcmp(argv[i],"-fps"))
    {
      if(++i < argc)
        dumpfreq = atof(argv[i]);
      else
        return(-1);
    }
  }

  if(i>=argc)
    return(-1);

  *num_bots=0;
  while(i<argc)
  {
    // Look for ':' (colon), and extract the trailing port number.  If not
    // given, then use the default Player port (6665)
    if((idx = strchr(argv[i],':')) && (strlen(idx) > 1))
    {
      port = atoi(idx+1);
      hostlen = idx - argv[i];
    }
    else
    {
      port = PLAYER_PORTNUM;
      hostlen = strlen(argv[i]);
    }

    
    // Store the hostnames and port numbers
    assert((hostlen > 0) && (hostlen < (MAX_HOSTNAME_LEN - 1)));
    argv[i][hostlen] = '\0';
    hostnames[*num_bots] = strdup(argv[i]);
    ports[*num_bots] = port;
    (*num_bots)++;
    i++;
  }

  return(0);
}


