/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2005 -
 *     Brian Gerkey
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
 * Interface to libplayertcp
 *
 * $Id$
 */

/** @defgroup libplayertcp libplayertcp
@{

This library moves messages between Player message queues and TCP sockets.

@todo
 - More verbose documentation on this library.
*/

#ifndef _PLAYERTCP_H_
#define _PLAYERTCP_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#include <libplayercore/playercore.h>

/** Default TCP port */
#define PLAYERTCP_DEFAULT_PORT 6665

/** We try to read() incoming messages in chunks of this size.  We also
    calloc() and realloc() read buffers in multiples of this size. */
#define PLAYERTCP_READBUFFER_SIZE 65536

/** We try to write() outgoing messages in chunks of this size.  We also
    calloc() and realloc() write buffers in multiples of this size. */
#define PLAYERTCP_WRITEBUFFER_SIZE 65536

// Forward declarations
struct pollfd;

struct playertcp_listener;
struct playertcp_conn;

class PlayerTCP
{
  private:
    uint32_t host;
    int num_listeners;
    playertcp_listener* listeners;
    struct pollfd* listen_ufds;

    pthread_mutex_t clients_mutex;
    int size_clients;
    int num_clients;
    playertcp_conn* clients;
    struct pollfd* client_ufds;

    /** Buffer in which to store decoded incoming messages */
    char* decode_readbuffer;
    /** Total size of @p decode_readbuffer */
    int decode_readbuffersize;

  public:
    PlayerTCP();
    ~PlayerTCP();

    pthread_t thread;

    int Listen(int* ports, int num_ports);
    MessageQueue* AddClient(struct sockaddr_in* cliaddr, 
                            unsigned int local_host,
                            unsigned int local_port,
                            int newsock,
                            bool send_banner,
                            int* kill_flag);
    int Accept(int timeout);
    void Close(int cli);
    int ReadClient(int cli);
    int Read(int timeout);
    int Write();
    int WriteClient(int cli);
    void DeleteClients();
    void ParseBuffer(int cli);
    int HandlePlayerMessage(int cli, Message* msg);
    void DeleteClient(MessageQueue* q);
};

/** @} */

#endif
