/*
Copyright (c) 2006, Brad Kratochvil
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
#include <playerconfig.h>
#include <libplayerc/playerc.h>

#include "sharedstruct.h"

int
main(int argc, const char **argv)
{
  playerc_client_t *client;
  playerc_opaque_t *opaque;
  test_t t;
  int i;

  // Create a client and connect it to the server.
  client = playerc_client_create(NULL, "localhost", 6665);
  if (0 != playerc_client_connect(client))
    return -1;

  // Create and subscribe to a opaque device.
  opaque = playerc_opaque_create(client, 0);
  if (playerc_opaque_subscribe(opaque, PLAYER_OPEN_MODE))
    return -1;


  for (i=0; i<10; ++i)
  {
    // Wait for new data from server
    playerc_client_read(client);

    t = *((test_t*)opaque->data);

    printf("test data %i\n", i);
    printf("%i\n", t.uint8);
    printf("%i\n", t.int8);
    printf("%i\n", t.uint16);
    printf("%i\n", t.int16);
    printf("%i\n", t.uint32);
    printf("%i\n", t.int32);
    printf("%0.3f\n", t.doub);
  }

  // Shutdown
  playerc_opaque_unsubscribe(opaque);
  playerc_opaque_destroy(opaque);
  playerc_client_disconnect(client);
  playerc_client_destroy(client);

  return 0;
}
