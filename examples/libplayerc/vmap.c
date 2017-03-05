/*
Copyright (c) 2007, Ben Morelli, Toby Collett
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

#include <libplayerc/playerc.h>

void PrintMapInfo(playerc_vectormap_t* vmap);
void PrintLayerInfo(playerc_vectormap_t* vmap);
void PrintLayerData(playerc_vectormap_t* vmap);
void PrintFeatureData(playerc_vectormap_t* vmap);

int main()
{
  playerc_client_t* client = NULL;
  playerc_vectormap_t* vmap = NULL;

  printf("Creating client\n");
  client = playerc_client_create(NULL, "localhost", 6665);
  if (0 != playerc_client_connect(client))
  {
          printf("Error connecting client\n");
          return -1;
  }

  printf("Creating vectormap\n");
  vmap = playerc_vectormap_create(client, 0);
  if (vmap == NULL)
  {
          printf("vmap is NULL\n");
          return -1;
  }

  printf("Subscribing\n");
  if (playerc_vectormap_subscribe(vmap, PLAYER_OPEN_MODE))
  {
          printf("Error subscribing\n");
          return -1;
  }

  assert(vmap != NULL);

  printf("Getting map info\n");
  if (playerc_vectormap_get_map_info(vmap))
  {
          printf("Error getting info\n");
          return -1;
  }

  PrintMapInfo(vmap);
  PrintLayerInfo(vmap);

  printf("Getting layer data\n");
  if (playerc_vectormap_get_layer_data(vmap, 0))
  {
          printf("Error getting layer data\n");
  }

  PrintMapInfo(vmap);
  if (vmap->layers_count > 0)
  {
    PrintLayerInfo(vmap);
    PrintLayerData(vmap);
    if (vmap->layers_data[0]->features_count) PrintFeatureData(vmap);
  }

  printf("\n");
  printf("Unsubscribing\n");
  playerc_vectormap_unsubscribe(vmap);
  return 0;
}

void PrintMapInfo(playerc_vectormap_t* vmap)
{
  player_extent2d_t extent = vmap->extent;
  printf("MapInfo\n");
  printf("srid = %d\nlayer_count = %d\n", vmap->srid, vmap->layers_count);
  printf("extent = (%f %f, %f %f)\n", extent.x0, extent.y0, extent.x1, extent.y1);
}

void PrintLayerInfo(playerc_vectormap_t* vmap)
{
  player_extent2d_t extent = vmap->layers_info[0]->extent;
  printf("LayerInfo\n");
  printf("extent = (%f %f, %f %f)\n", extent.x0, extent.y0, extent.x1, extent.y1);
  printf("name = %s\n", vmap->layers_info[0]->name);
}

void PrintLayerData(playerc_vectormap_t* vmap)
{
  printf("LayerData\n");
  printf("feature count = %d\n", vmap->layers_data[0]->features_count);
}

void PrintFeatureData(playerc_vectormap_t* vmap)
{
  printf("FeatureData\n");
  printf("wkb count = %d\n", vmap->layers_data[0]->features[0].wkb_count);
  printf("name = %s\n", vmap->layers_data[0]->features[0].name);
}
