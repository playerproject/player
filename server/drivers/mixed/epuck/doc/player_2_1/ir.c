/*
Copyright (c) 2008 Renato Florentino Garcia <fgar.renato@gmail.com>
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright notice,
      this list of conditions and the following disclaimer in the documentation
      and/or other materials provided with the distribution.
    * Neither the name of the Player Project nor the names of its contributors
      may be used to endorse or promote products derived from this software
      without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <libplayerc/playerc.h>

int main(int argc, const char **argv)
{
  int i, stop;
  playerc_client_t *client;
  playerc_ir_t *ir1;

  client = playerc_client_create(NULL, "localhost", 6665);
  if(playerc_client_connect(client) != 0)
  {
    fprintf(stderr, "error: %s\n", playerc_error_str());
    return -1;
  }
  ir1 = playerc_ir_create(client, 0);
  if(playerc_ir_subscribe(ir1, PLAYERC_OPEN_MODE) != 0)
  {
    fprintf(stderr, "IRRerror: %s\n", playerc_error_str());
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

  /* It Read the IR sensors until an object distance 2cm or lesser, *
   * from any of eight sensors.                                     */
  stop = 0;
  while(!stop)
  {
    playerc_client_read(client);

    for(i=0; i < ir1->data.ranges_count; i++)
    {
      printf("ir%d: %f   ", i, ir1->data.ranges[i]);
      if(ir1->data.ranges[i] < 0.02)
        stop = 1;
    }
      printf("\n");
  }

  /* Shutdown and tidy up */
  playerc_ir_unsubscribe(ir1);
  playerc_ir_destroy(ir1);
  playerc_client_disconnect(client);
  playerc_client_destroy(client);

  return 0;
}
