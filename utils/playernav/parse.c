#include <string.h>
#include "playernav.h"

/* Parse command line arguments, of the form host:port */
int
parse_args(int argc, char** argv,
           int* num_bots, char** hostnames, int* ports)
{
  char* idx;
  int port;
  int hostlen;
  int i;

  if(argc < 1)
    return(-1);

  *num_bots=0;
  for(i=0; i < argc; i++)
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
  }

  return(0);
}


