#include "playernav.h"

/*
 * Connect to player at each host:port pair, as specified by the global
 * vars 'hostnames' and 'ports'.  Also subscribes to each device, and adds 
 * each client into a new multiclient (which is returned)
 */
playerc_mclient_t*
init_player(playerc_client_t** clients,
            playerc_map_t** maps,
            playerc_localize_t** localizes,
            playerc_planner_t** planners,
            int num_bots,
            char** hostnames,
            int* ports,
            int data_freq)
{
  int i;
  playerc_mclient_t* mclient;

  /* Connect to Player */
  assert(mclient = playerc_mclient_create());
  for(i=0; i<num_bots; i++)
  {
    assert(clients[i] = 
           playerc_client_create(mclient, hostnames[i], ports[i]));
    if(playerc_client_connect(clients[i]) < 0)
    {
      fprintf(stderr, "Failed to connect to %s:%d\n", 
              hostnames[i], ports[i]);
      return(NULL);
    }
    if(playerc_client_datafreq(clients[i],data_freq) < 0)
    {
      fprintf(stderr, "Failed to set data frequency\n");
      return(NULL);
    }
    // request ALL data, rather than just NEW data, because the localizer
    // may only send out updates occasionally.
    if(playerc_client_datamode(clients[i],PLAYER_DATAMODE_PUSH_ALL) < 0)
    {
      fprintf(stderr, "Failed to set data mode\n");
      return(NULL);
    }
    assert(maps[i] = playerc_map_create(clients[i], 0));
    if(playerc_map_subscribe(maps[i],PLAYER_READ_MODE) < 0)
    {
      fprintf(stderr, "Failed to subscribe to map\n");
      return(NULL);
    }
    assert(localizes[i] = playerc_localize_create(clients[i], 0));
    if(playerc_localize_subscribe(localizes[i],PLAYER_READ_MODE) < 0)
    {
      fprintf(stderr, "Warning: Failed to subscribe to localize on robot %d; you won't be able to set its pose.\n",i);
      playerc_localize_destroy(localizes[i]);
      localizes[i] = NULL;
    }
    assert(planners[i] = playerc_planner_create(clients[i], 0));
    if(playerc_planner_subscribe(planners[i],PLAYER_ALL_MODE) < 0)
    {
      fprintf(stderr, "Warning: Failed to subscribe to planner on robot %d; you won't be able to give it goals.\n",i);
      playerc_planner_destroy(planners[i]);
      planners[i] = NULL;
    }
  }

  /* Get the map from the first robot */
  puts("requesting map");
  if(playerc_map_get_map(maps[0]) < 0)
  {
    fprintf(stderr, "Failed to get map\n");
    return(NULL);
  }
  puts("done");

#if 0
  /* Get at least one round of data from each robot */
  for(;;)
  {
    if(playerc_mclient_read(mclient,-1) < 0)
    {
      fprintf(stderr, "Error on read\n");
      return(NULL);
    }

    for(i=0; i<num_bots; i++)
    {
      if(!truths[i]->info.fresh || 
         !lasers[i]->info.fresh || 
         !planners[i]->info.fresh)
        break;
    }
    if(i==num_bots)
      break;
  }
#endif

  return(mclient);
}

void
fini_player(playerc_mclient_t* mclient,
            playerc_client_t** clients,
            playerc_map_t** maps,
            int num_bots)
{
  int i;

  for(i=0;i<num_bots;i++)
  {
    playerc_map_destroy(maps[i]);
    playerc_client_destroy(clients[i]);
  }
  playerc_mclient_destroy(mclient);
}

