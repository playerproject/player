/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *     Nik Melchior
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *     Nik Melchior
 *                      
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <playerclient.h>
#include <netinet/in.h>
#include <string.h>

void
DIOProxy::FillData(player_msghdr_t hdr, const char *buffer)
{
  if(hdr.size != sizeof(player_dio_data_t)) 
  {
    if(player_debug_level(-1) >= 1)
      fprintf(stderr,"WARNING: DIOProxy expected %d bytes of "
              "power data, but received %d. Unexpected results may "
              "ensue.\n",
              sizeof(player_dio_data_t),hdr.size);
  }

  const player_dio_data_t* lclBuffer =
    reinterpret_cast<const player_dio_data_t*>(buffer);

  count = lclBuffer->count;
  digin = ntohl(lclBuffer->digin);
}

// interface that all proxies SHOULD provide
void
DIOProxy::Print()
{
  printf("#DIO(%d:%d) - %c\n", m_device_id.code, 
         m_device_id.index, access);
  printf("%d bit:  ",count);
  for (int i=0; i< count; i++)
  {
	printf( (digin << i) & 0x80000000 ? "1" : "0");
	if (i %4 == 3)
		printf(" ");
  }
  printf("\n");
}

// Outputs a bitfield to the DIO
// Returns:
//   0 if everything's ok
//   -1 otherwise (that's bad)

int
DIOProxy::SetOutput(uint8_t output_count, uint32_t digout)
{

  if(!client)
    return(-1);

  player_dio_cmd_t cmd;
  memset( &cmd, 0, sizeof(cmd) );

  cmd.count  = output_count;
  cmd.digout = htonl(digout);

  return(client->Write(m_device_id,
                       reinterpret_cast<char*>(&cmd),sizeof(cmd)));
}