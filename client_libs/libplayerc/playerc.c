/***************************************************************************
 *
 * Author: Andrew H
 * Date: 24 Aug 2001
 * Desc: A Player client
 *
 * CVS info:
 * $Source$
 * $Author$
 * $Revision$
 *
 **************************************************************************/


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>       /* for gethostbyname() */
#include <netinet/in.h>  /* for struct sockaddr_in, htons(3) */
#include <errno.h>
#include "playerc.h"

#define DEBUG

/* Useful error macros */
#define PLAYERC_ERR(msg)         printf("playerc error   : " msg "\n")
#define PLAYERC_ERR1(msg, a)     printf("playerc error   : " msg "\n", a)
#define PLAYERC_ERR2(msg, a, b)  printf("playerc error   : " msg "\n", a, b)
#define PLAYERC_ERR3(msg, a, b, c)  printf("playerc error   : " msg "\n", a, b, c)
#define PLAYERC_WARN(msg)        printf("playerc warning : " msg "\n")
#define PLAYERC_WARN1(msg, a)    printf("playerc warning : " msg "\n", a)
#define PLAYERC_WARN2(msg, a, b) printf("playerc warning : " msg "\n", a, b)
#define PLAYERC_MSG3(msg, a, b, c) printf("playerc message : " msg "\n", a, b, c)

// DEBUG macros
#ifdef DEBUG
#define PRINT_DEBUG(m)         printf("\rstage debug : %s %s\n  "m"\n", \
                                     __FILE__, __FUNCTION__)
#define PRINT_DEBUG1(m, a)     printf("\rstage debug : %s %s\n  "m"\n", \
                                     __FILE__, __FUNCTION__, a)
#define PRINT_DEBUG2(m, a, b)  printf("\rstage debug : %s %s\n  "m"\n", \
                                     __FILE__, __FUNCTION__, a, b)
#define PRINT_DEBUG3(m, a, b, c) printf("\rstage debug : %s %s\n  "m"\n", \
                                     __FILE__, __FUNCTION__, a, b, c)
#else
#define PRINT_DEBUG(m)
#define PRINT_DEBUG1(m, a)
#define PRINT_DEBUG2(m, a, b)
#define PRINT_DEBUG3(m, a, b, c)
#endif


/* Player message structure
   for subscibing to devices.
   This one is easier to use than the one defined in
   messages.h.
*/
typedef struct
{
    uint16_t subtype;
    uint16_t device;
    uint16_t index;
    uint8_t access;
} __attribute__ ((packed)) playerc_msg_subscribe_t;


/* Declare local functions
 */
int playerc_mclient_addclient(playerc_mclient_t *mclient, playerc_client_t *client);
int playerc_mclient_read_sock(playerc_mclient_t *mclient, int timeout);
int playerc_mclient_read_log(playerc_mclient_t *mclient);

int playerc_client_connect_sock(playerc_client_t *client);
int playerc_client_disconnect_sock(playerc_client_t *client);
int playerc_client_read_sock(playerc_client_t *client);
int playerc_client_connect_log(playerc_client_t *client);
int playerc_client_disconnect_log(playerc_client_t *client);
int playerc_client_read_log(playerc_client_t *client);
int playerc_client_subscribe(playerc_client_t *client, int code, int index, int access);
int playerc_client_request(playerc_client_t *client, playerc_device_t *device,
                           char *req_data, int req_len, char *rep_data, int rep_len);
int playerc_client_readpacket(playerc_client_t *client, player_msghdr_t *header,
                              char *data, int len);
int playerc_client_writepacket(playerc_client_t *client, player_msghdr_t *header,
                               char *data, int len);



/***************************************************************************
 * Multi-client
 **************************************************************************/


/* Create a multi-client
 */
playerc_mclient_t *playerc_mclient_create()
{
    playerc_mclient_t *mclient;

    mclient = malloc(sizeof(playerc_mclient_t));
    memset(mclient, 0, sizeof(playerc_mclient_t));

    return mclient;
}


/* Destroy a multi-client
 */
void playerc_mclient_destroy(playerc_mclient_t *mclient)
{
    free(mclient);
}


/* Add a client to this multi-client
 */
int playerc_mclient_addclient(playerc_mclient_t *mclient, playerc_client_t *client)
{
    if (mclient->client_count >= sizeof(mclient->client) / sizeof(mclient->client[0]))
    {
        PLAYERC_ERR("too many clients in multi-client; ignoring new client");
        return -1;
    }

    mclient->client[mclient->client_count] = client;
    mclient->client_count++;

    return 0;
}


/* Connect a bunch of clients
 */
int playerc_mclient_connect(playerc_mclient_t *mclient)
{
    int i;

    for (i = 0; i < mclient->client_count; i++)
    {
        if (playerc_client_connect(mclient->client[i]) < 0)
            return -1;
    }
    return 0;
}


/* Disconnect a bunch of clients
 */
int playerc_mclient_disconnect(playerc_mclient_t *mclient)
{
    int i;

    for (i = 0; i < mclient->client_count; i++)
    {
        if (playerc_client_disconnect(mclient->client[i]) < 0)
            return -1;
    }
    return 0;
}


/* Read from a bunch of clients
 */
int playerc_mclient_read(playerc_mclient_t *mclient, int timeout)
{
    if (mclient->client_count > 0 && mclient->client[0]->port < 0)
        return playerc_mclient_read_log(mclient);
    else
        return playerc_mclient_read_sock(mclient, timeout);
}


/* Read from a socket
 */
int playerc_mclient_read_sock(playerc_mclient_t *mclient, int timeout)
{
    int i, count;

    /* Configure poll structure to wait for incoming data */
    for (i = 0; i < mclient->client_count; i++)
    {
        mclient->pollfd[i].fd = mclient->client[i]->sock;
        mclient->pollfd[i].events = POLLIN;
        mclient->pollfd[i].revents = 0;
    }

    /* Wait for incoming data */
    count = poll(mclient->pollfd, mclient->client_count, timeout);
    if (count < 0)
    {
        PLAYERC_ERR1("poll returned error [%s]", strerror(errno));
        return -1;
    }

    /* Now read from each of the waiting sockets */
    for (i = 0; i < mclient->client_count; i++)
    {
        if ((mclient->pollfd[i].revents & POLLIN) > 0)
            playerc_client_read(mclient->client[i]);
    }
    return count;
}


/* Read from a bunch of log files
 */
int playerc_mclient_read_log(playerc_mclient_t *mclient)
{
    int i, read_client;
    double time, read_time;
    char line[4096];
    int argc, bytes;
    char *argv[500];
    
    read_client = -1;
    read_time = 1e32;

    /* Find the client with the earliest time */
    for (i = 0; i < mclient->client_count; i++)
    {
        /* Read in a single line */
        if (!fgets(line, sizeof(line), mclient->client[i]->logfile))
            continue;
        bytes = strlen(line);

        /* Tokenize the line */
        argc = 0;
        argv[argc] = strtok(line, " \t\n\r");
        while (argv[argc] != NULL)
        {
            assert(argc < sizeof(argv) / sizeof(argv[0]));
            argv[++argc] = strtok(NULL, " \t\n\r");
        }

        /* Ignore blank lines, comment lines and format lines
         */
        if (argc < 2)
        {
            i--;
            continue;
        }
        if (strcmp(argv[0], "#") == 0 || strcmp(argv[0], "format") == 0)
        {
            i--;
            continue;
        }

        /* Extract the time */
        time = atof(argv[1]);        
        if (time < read_time)
        {
            read_time = time;
            read_client = i;
        }       

        /* 'Unread' the stuff we have just read */
        fseek(mclient->client[i]->logfile, -bytes, SEEK_CUR);
    }

    if (read_client < 0)
    {
        PLAYERC_WARN("end of file");
        return -1;
    }

    /* Get the client to read an process the next line */
    return playerc_client_read(mclient->client[read_client]);
}


/***************************************************************************
 * Single client
 **************************************************************************/


/* Create a player client
 */
playerc_client_t *playerc_client_create(playerc_mclient_t *mclient, char *hostname, int port)
{
    playerc_client_t *client;

    client = malloc(sizeof(playerc_client_t));
    memset(client, 0, sizeof(playerc_client_t));
    client->hostname = strdup(hostname);
    client->port = port;

    if (mclient)
        playerc_mclient_addclient(mclient, client);

    return client;
}


/* Destroy a player client
 */
void playerc_client_destroy(playerc_client_t *client)
{
    free(client->hostname);
    free(client);
}


/* Connect to the server
 */
int playerc_client_connect(playerc_client_t *client)
{
    if (client->port < 0)
        return playerc_client_connect_log(client);
    else
        return playerc_client_connect_sock(client);
}


/* Disconnect from the server
 */
int playerc_client_disconnect(playerc_client_t *client)
{
    if (client->port < 0)
        return playerc_client_disconnect_log(client);
    else
        return playerc_client_disconnect_sock(client);
}


/* Read and process a packet (blocking)
 */
int playerc_client_read(playerc_client_t *client)
{
    if (client->port < 0)
        return playerc_client_read_log(client);
    else
        return playerc_client_read_sock(client);
}


/* Add a device proxy
 */
int playerc_client_adddevice(playerc_client_t *client, playerc_device_t *device,
                             int code, int index, int access,
                             playerc_putdata_fn_t putdata, playerc_getcmd_fn_t getcmd)
{
    if (client->device_count >= sizeof(client->device) / sizeof(client->device[0]))
    {
        PLAYERC_ERR("too many subscribed devices; ignoring new device");
        return -1;
    }

    client->device[client->device_count++] = device;
    device->client = client;
    device->code = code;
    device->index = index;
    device->access = access;
    device->putdata = putdata;
    device->getcmd = getcmd;
    device->datatime = 0;
    device->callback_count = 0;

    return 0;
}


/* Register a callback
   Will be called when after data has been read by the indicated device.
 */
int playerc_client_addcallback(playerc_client_t *client, playerc_device_t *device,
                               playerc_callback_fn_t callback, void *data)
{
    if (device->callback_count >= sizeof(device->callback) / sizeof(device->callback[0]))
    {
        PLAYERC_ERR("too many registered callbacks; ignoring new callback");
        return -1;
    }
    device->callback[device->callback_count] = callback;
    device->callback_data[device->callback_count] = data;
    device->callback_count++;

    return 0;
}


/* Unregister a callback
 */
int playerc_client_delcallback(playerc_client_t *client, playerc_device_t *device,
                               playerc_callback_fn_t callback, void *data)
{
    int i;
    
    for (i = 0; i < device->callback_count; i++)
    {
        if (device->callback[i] != callback)
            continue;
        if (device->callback_data[i] != data)
            continue;
        memmove(device->callback + i, device->callback + i + 1,
                (device->callback_count - i - 1) * sizeof(device->callback[0]));
        memmove(device->callback_data + i, device->callback_data + i + 1,
                (device->callback_count - i - 1) * sizeof(device->callback_data[0]));
        device->callback_count--;
    }
    return 0;
}


/* Connect to the server
 */
int playerc_client_connect_sock(playerc_client_t *client)
{
    int i;
    playerc_device_t *device;
    struct hostent* entp;
    struct sockaddr_in server;
    char banner[32];

    /* Construct socket */
    client->sock = socket(PF_INET, SOCK_STREAM, 0);
    if (client->sock < 0)
    {
        PLAYERC_ERR1("socket call failed with error [%s]", strerror(errno));
        return -1;
    }

    /* Construct server address */
    entp = gethostbyname(client->hostname);
    if (entp == NULL)
    {
        PLAYERC_ERR1("gethostbyname failed with error [%s]", strerror(errno));
        return -1;
    }
    server.sin_family = PF_INET;
    memcpy(&server.sin_addr, entp->h_addr_list[0], entp->h_length);
    server.sin_port = htons(client->port);

    /* Connect the socket */
    if (connect(client->sock, (struct sockaddr*) &server, sizeof(server)) < 0)
    {
        PLAYERC_ERR3("connect call on [%s:%d] failed with error [%s]",
                     client->hostname, client->port, strerror(errno));
        return -1;
    }

    /* Get the banner */
    if (recv(client->sock, banner, sizeof(banner), 0) < sizeof(banner))
    {
        PLAYERC_ERR("incomplete initialization string");
        return -1;
    }

    /* PLAYERC_MSG3("[%s] connected on [%s:%d]", banner, client->hostname, client->port);*/

    /* Subscribe currently listed devices */
    for (i = 0; i < client->device_count; i++)
    {
        device = client->device[i];
        playerc_client_subscribe(client, device->code, device->index, device->access);
    }
    
    return 0;
}


/* Disconnect from the server
 */
int playerc_client_disconnect_sock(playerc_client_t *client)
{
    if (close(client->sock) < 0)
    {
        PLAYERC_ERR1("close failed with error [%s]", strerror(errno));
        return -1;
    }
    return 0;
}


/* Subscribe to a device
 */
int playerc_client_subscribe(playerc_client_t *client, int code, int index, int access)
{
    playerc_msg_subscribe_t body;

    body.subtype = htons(PLAYER_PLAYER_DEV_REQ);
    body.device = htons(code);
    body.index = htons(index);
    body.access = access;

    if (playerc_client_request(client, NULL,
                               (char*) &body, sizeof(body), (char*) &body, sizeof(body)) < 0)
        return -1;

    if (body.access != access)
        PLAYERC_WARN2("requested [%d] access, but got [%d] access", access, body.access);

    return 0;
}


/* Dispatch a packet
 */
void playerc_client_dispatch(playerc_client_t *client,
                             player_msghdr_t *header, char *data, int len)
{
    int i, j;
    playerc_device_t *device;

    /* Look for a device proxy to handle this data */
    for (i = 0; i < client->device_count; i++)
    {
        device = client->device[i];
        
        if (device->code == header->device && device->index == header->device_index)
        {
            /* Fill out timing info */
            device->datatime = header->timestamp_sec + header->timestamp_usec * 1e-6;

            /* Call the registerd handler for this device */
            (*device->putdata) (device, (char*) header, data, len);

            /* Call any additional registered callbacks */
            for (j = 0; j < device->callback_count; j++)
                (*device->callback[j]) (device->callback_data[j]);
        }
    }
}


/* Read and process a packet (blocking)
 */
int playerc_client_read_sock(playerc_client_t *client)
{
    player_msghdr_t header;
    int len;
    char data[8192];

    len = sizeof(data);

    /* Read a packet (header and data) */
    len = playerc_client_readpacket(client, &header, data, len);
    if (len < 0)
        return -1;

    /* Check the return type */
    if (header.type != PLAYER_MSGTYPE_DATA)
    {
        PLAYERC_WARN1("unexpected message type [%d]", header.type);
        return -1;
    }

    /* Dispatch */
    playerc_client_dispatch(client, &header, data, len);

    return 0;
}


/* Write a command
 */
int playerc_client_write(playerc_client_t *client, playerc_device_t *device, char *cmd, int len)
{
    player_msghdr_t header;

    if (!(device->access == PLAYER_WRITE_MODE || device->access == PLAYER_ALL_MODE))
        PLAYERC_WARN("writing to device without write permission");
    
    header.stx = PLAYER_STXX;
    header.type = PLAYER_MSGTYPE_CMD;
    header.device = device->code;
    header.device_index = device->index;
    header.size = len;

    return playerc_client_writepacket(client, &header, cmd, len);
}


/* Issue request and await reply (blocking)
 */
int playerc_client_request(playerc_client_t *client, playerc_device_t *deviceinfo,
                           char *req_data, int req_len, char *rep_data, int rep_len)
{
    int i, len;
    char data[8192];
    player_msghdr_t req_header, rep_header;

    if (deviceinfo == NULL)
    {
        req_header.stx = PLAYER_STXX;
        req_header.type = PLAYER_MSGTYPE_REQ;
        req_header.device = PLAYER_PLAYER_CODE;
        req_header.device_index = 0;
        req_header.size = req_len;
    }
    else
    {
        req_header.stx = PLAYER_STXX;
        req_header.type = PLAYER_MSGTYPE_REQ;
        req_header.device = deviceinfo->code;
        req_header.device_index = deviceinfo->index;
        req_header.size = req_len;
    }

    if (playerc_client_writepacket(client, &req_header, req_data, req_len) < 0)
        return -1;

    /* Read and dispatch regular data packets until we get a reply to
       out request */
    for (i = 0; i < 1000; i++)
    {
        len = playerc_client_readpacket(client, &rep_header, data, sizeof(data));
        if (len  < 0)
            return -1;

        if (rep_header.type == PLAYER_MSGTYPE_DATA)
        {
            playerc_client_dispatch(client, &rep_header, data, len);
        }
        else if (rep_header.type == PLAYER_MSGTYPE_RESP)
        {
            assert(rep_header.device == req_header.device);
            assert(rep_header.device_index == req_header.device_index);
            assert(rep_header.size == rep_len);
            memcpy(rep_data, data, rep_len);
            break;
        }
    }

    if (i == 1000)
    {
        PLAYERC_ERR("timed out waiting for server reply to request");
        return -1;
    }
    
    return 0;
}


/* Read a raw packet
 */
int playerc_client_readpacket(playerc_client_t *client, player_msghdr_t *header,
                              char *data, int len)
{
  int bytes, total_bytes;

  /* Look for STX */
  bytes = recv(client->sock, header, 2, 0);
  if (bytes < 0)
  {
    PLAYERC_ERR1("recv on stx failed with error [%s]", strerror(errno));
    return -1;
  }
  else if (bytes < 2)
  {
    PLAYERC_ERR2("got incomplete stx, %d of %d bytes", bytes, 2);
    return -1;
  }
  if (ntohs(header->stx) != PLAYER_STXX)
  {
    PLAYERC_ERR("malformed packet; discarding");
    return -1;
  }

  bytes = recv(client->sock, ((char*) header) + 2, sizeof(player_msghdr_t) - 2, 0);
  if (bytes < 0)
  {
    PLAYERC_ERR1("recv on header failed with error [%s]", strerror(errno));
    return -1;
  }
  else if (bytes < sizeof(player_msghdr_t) - 2)
  {
    PLAYERC_ERR2("got incomplete header, %d of %d bytes", bytes, sizeof(player_msghdr_t) - 2);
    return -1;
  }

  /* Do the network byte re-ordering */
  header->stx = ntohs(header->stx);
  header->type = ntohs(header->type);
  header->device = ntohs(header->device);
  header->device_index = ntohs(header->device_index);
  header->size = ntohl(header->size);
  header->timestamp_sec = ntohl(header->timestamp_sec);
  header->timestamp_usec = ntohl(header->timestamp_usec);

  if (header->size > len)
  {
    PLAYERC_ERR1("packet is too large, %d bytes", ntohl(header->size));
    return -1;
  }

  // Read in the body of the packet
  total_bytes = 0;
  while (total_bytes < header->size)
  {
    bytes = recv(client->sock, data + total_bytes, header->size - total_bytes, 0);
    if (bytes < 0)
    {
      PLAYERC_ERR1("recv on body failed with error [%s]", strerror(errno));
      return -1;
    }
    total_bytes += bytes;
  }
    
  return total_bytes;
}


/* Write a raw packet
 */
int playerc_client_writepacket(playerc_client_t *client, player_msghdr_t *header,
                               char *data, int len)
{
  int bytes;

  /* Do the network byte re-ordering */
  header->stx = htons(header->stx);
  header->type = htons(header->type);
  header->device = htons(header->device);
  header->device_index = htons(header->device_index);
  header->size = htonl(header->size);
    
  bytes = send(client->sock, header, sizeof(player_msghdr_t), 0);
  if (bytes < 0)
  {
    PLAYERC_ERR1("send on header failed with error [%s]", strerror(errno));
    return -1;
  }
  else if (bytes < sizeof(player_msghdr_t))
  {
    PLAYERC_ERR2("sent incomplete header, %d of %d bytes", bytes, sizeof(player_msghdr_t));
    return -1;
  }

  /* Now undo the network byte re-ordering */
  header->stx = ntohs(header->stx);
  header->type = ntohs(header->type);
  header->device = ntohs(header->device);
  header->device_index = ntohs(header->device_index);
  header->size = ntohl(header->size);
    
  bytes = send(client->sock, data, len, 0);
  if (bytes < 0)
  {
    PLAYERC_ERR1("send on body failed with error [%s]", strerror(errno));
    return -1;
  }
  else if (bytes < len)
  {
    PLAYERC_ERR2("sent incomplete body, %d of %d bytes", bytes, len);
    return -1;
  }

  return 0;
}


/* Connect to log file instead of socket
 */
int playerc_client_connect_log(playerc_client_t *client)
{
    client->logfile = fopen(client->hostname, "r");
    if (!client->logfile)
    {
        PLAYERC_ERR2("unable to open log file [%s], error [%s]", client->hostname,
                     strerror(errno));
        return -1;
    }
    return 0;
}


/* Disconnect from log file
 */
int playerc_client_disconnect_log(playerc_client_t *client)
{
    fclose(client->logfile);
    return 0;
}


/* Read data from a log file
 */
int playerc_client_read_log(playerc_client_t *client)
{
    int i, j;
    int argc;
    char *argv[500];
    char line[4096];
    playerc_device_t *device;
    
    /* Get a single line */
    if (!fgets(line, sizeof(line), client->logfile))
    {
        PLAYERC_WARN("end of file");
        return -1;
    }
    /* Tokenize the line */
    argc = 0;
    argv[argc] = strtok(line, " \t\n\r");
    while (argv[argc] != NULL)
    {
        assert(argc < sizeof(argv) / sizeof(argv[0]));
        argv[++argc] = strtok(NULL, " \t\n\r");
    }
    if (argc < 1)
        return 0;

    /* Look for a device proxy to handle this data */
    for (i = 0; i < client->device_count; i++)
    {
        device = client->device[i];

        if (strcmp(argv[0], device->logname) == 0)
        {
            /* Call the registerd handler for this device */
            (*device->putlogdata) (device, argc, argv);

            /* Call any additional registered callbacks */
            for (j = 0; j < device->callback_count; j++)
                (*device->callback[j]) (device->callback_data[j]);
        }
    }
    return 0;
}



/***************************************************************************
 * Position device
 **************************************************************************/


/* Local declarations
 */
void playerc_position_putdata(playerc_position_t *device, player_msghdr_t *header,
                              player_position_data_t *data, size_t len);
void playerc_position_putlogdata(playerc_position_t *device, int argc, char **argv);


/* Create a new position proxy
 */
playerc_position_t *playerc_position_create(playerc_client_t *client, int index, int access)
{
    playerc_position_t *device;

    device = malloc(sizeof(playerc_position_t));
    playerc_client_adddevice(client, (playerc_device_t*) device,
                             PLAYER_POSITION_CODE, index, access,
                             (playerc_putdata_fn_t) playerc_position_putdata,
                             NULL);

    device->info.logname = "position";
    device->info.putlogdata = (playerc_putlogdata_fn_t) playerc_position_putlogdata;
    
    return device;
}


/* Destroy a position proxy
 */
void playerc_position_destroy(playerc_position_t *device)
{
    free(device);
}


/* Process incoming data
 */
void playerc_position_putdata(playerc_position_t *device, player_msghdr_t *header,
                              player_position_data_t *data, size_t len)
{
    device->px = (long) ntohl(data->xpos) / 1000.0;
    device->py = (long) ntohl(data->ypos) / 1000.0;
    device->pa = (short) ntohs(data->theta) * M_PI / 180.0;
    device->vx = (short) ntohs(data->speed) / 1000.0;
    device->vy = (short) ntohs(data->sidespeed) / 1000.0;
    device->va = (short) ntohs(data->turnrate) * M_PI / 180.0;
    device->stall = data->stalls;
}


/* Process incoming log data
 */
void playerc_position_putlogdata(playerc_position_t *device, int argc, char **argv)
{
    device->px = atoi(argv[3]) / 1000.0;
    device->py = atoi(argv[4]) / 1000.0;
    device->pa = atoi(argv[5]) * M_PI / 180.0;
}


/* Enable/disable the motors */
int playerc_position_enable(playerc_position_t *device, int enable)
{
  player_position_config_t config;

  config.request = PLAYER_POSITION_MOTOR_POWER_REQ;
  config.value = enable;

  return playerc_client_request(device->info.client, &device->info,
                                (char*) &config, sizeof(config),
                                (char*) &config, sizeof(config));    
}


/* Set the robot speed */
int playerc_position_setspeed(playerc_position_t *device, double vx, double vy, double va)
{
  player_position_cmd_t cmd;

  cmd.speed = htons((int) (vx * 1000.0));
  cmd.sidespeed = htons((int) (vy * 1000.0));
  cmd.turnrate = htons((int) (va * 180.0 / M_PI));

  return playerc_client_write(device->info.client, &device->info, (char*) &cmd, sizeof(cmd));
}


/***************************************************************************
 * Laser device
 **************************************************************************/


/* Local declarations
 */
void playerc_laser_putdata(playerc_laser_t *device, player_msghdr_t *header,
                           player_laser_data_t *data, size_t len);
void playerc_laser_putlogdata(playerc_laser_t *device, int argc, char **argv);


/* Create a new laser proxy
 */
playerc_laser_t *playerc_laser_create(playerc_client_t *client, int index, int access)
{
    playerc_laser_t *device;

    device = malloc(sizeof(playerc_laser_t));
    playerc_client_adddevice(client, (playerc_device_t*) device,
                             PLAYER_LASER_CODE, index, access,
                             (playerc_putdata_fn_t) playerc_laser_putdata,
                             NULL);
    device->info.logname = "laser";
    device->info.putlogdata = (playerc_putlogdata_fn_t) playerc_laser_putlogdata;
    
    return device;
}


/* Destroy a laser proxy
 */
void playerc_laser_destroy(playerc_laser_t *device)
{
    free(device);
}


/* Process incoming data
 */
void playerc_laser_putdata(playerc_laser_t *device, player_msghdr_t *header,
                           player_laser_data_t *data, size_t len)
{
    int i;
    double r, b, db;
    
    data->min_angle = ntohs(data->min_angle);
    data->max_angle = ntohs(data->max_angle);
    data->resolution = ntohs(data->resolution);
    data->range_count = ntohs(data->range_count);

    b = data->min_angle / 100.0 * M_PI / 180.0;
    db = data->resolution / 100.0 * M_PI / 180.0;    
    for (i = 0; i < data->range_count; i++)
    {
        r = (ntohs(data->ranges[i]) & 0x1FFF) / 1000.0;
        device->scan[i][0] = r;
        device->scan[i][1] = b;
        device->point[i][0] = r * cos(b);
        device->point[i][1] = r * sin(b);
        device->intensity[i] = ((ntohs(data->ranges[i]) & 0xE000) >> 13);
        b += db;
    }
    device->scan_count = data->range_count;
}


/* Process incoming log data
 */
void playerc_laser_putlogdata(playerc_laser_t *device, int argc, char **argv)
{
    int i, count;
    double r, b, db;
    
    b = atoi(argv[4]) / 100.0 * M_PI / 180;
    db = atoi(argv[3]) / 100.0 * M_PI / 180;
    count = atoi(argv[6]);

    for (i = 0; i < count; i++)
    {
        r = (atoi(argv[i + 7]) & 0x1FFF) / 1000.0;
        device->scan[i][0] = r;
        device->scan[i][1] = b;
        device->point[i][0] = r * cos(b);
        device->point[i][1] = r * sin(b);
        device->intensity[i] = ((atoi(argv[i + 7]) & 0xE000) >> 13);
        b += db;
    }
    device->scan_count = count;
}


/* Configure the laser
 */
int playerc_laser_configure(playerc_laser_t *device, double min_angle, double max_angle,
                            double resolution, int intensity)
{
    player_laser_config_t config;

    config.min_angle = htons((int) (min_angle * 180.0 / M_PI * 100));
    config.max_angle = htons((int) (max_angle * 180.0 / M_PI * 100));
    config.resolution = htons((int) (resolution * 180.0 / M_PI * 100));
    config.intensity = (intensity ? 1 : 0);

    return playerc_client_request(device->info.client, &device->info,
                                  (char*) &config, sizeof(config), (char*) &config, sizeof(config));    
}
   

/***************************************************************************
 * Laser beacon device
 **************************************************************************/


/* Local declarations
 */
void playerc_laserbeacon_putdata(playerc_laserbeacon_t *device, player_msghdr_t *header,
                                 player_laserbeacon_data_t *data, size_t len);


/* Create a new laserbeacon proxy
 */
playerc_laserbeacon_t *playerc_laserbeacon_create(playerc_client_t *client, int index, int access)
{
    playerc_laserbeacon_t *device;

    device = malloc(sizeof(playerc_laserbeacon_t));
    memset(device, 0, sizeof(device));
    playerc_client_adddevice(client, (playerc_device_t*) device,
                             PLAYER_LASERBEACON_CODE, index, access,
                             (playerc_putdata_fn_t) playerc_laserbeacon_putdata,
                             NULL);

    device->info.logname = "";
    device->info.putlogdata = NULL;

    return device;
}


/* Destroy a laserbeacon proxy
 */
void playerc_laserbeacon_destroy(playerc_laserbeacon_t *device)
{
    free(device);
}


/* Process incoming data
 */
void playerc_laserbeacon_putdata(playerc_laserbeacon_t *device, player_msghdr_t *header,
                                 player_laserbeacon_data_t *data, size_t len)
{
    int i;

    device->beacon_count = ntohs(data->count);

    for (i = 0; i < device->beacon_count; i++)
    {
        device->beacons[i].id = data->beacon[i].id;
        device->beacons[i].range = ntohs(data->beacon[i].range) / 1000.0;
        device->beacons[i].bearing = ((int) (int16_t) ntohs(data->beacon[i].bearing)) * M_PI / 180;
        device->beacons[i].orient = ((int) (int16_t) ntohs(data->beacon[i].orient)) * M_PI / 180;
    }
}


/* Configure the laserbeacon device
 */
int playerc_laserbeacon_configure(playerc_laserbeacon_t *device, int bit_count, double bit_width)
{
    player_laserbeacon_setbits_t config;

    config.subtype = PLAYER_LASERBEACON_SUBTYPE_SETBITS;
    config.bit_count = bit_count;
    config.bit_size = htons(bit_width * 1000);
    
    return playerc_client_request(device->info.client, &device->info,
                                  (char*) &config, sizeof(config),
                                  (char*) &config, sizeof(config));    
}
   

/***************************************************************************
 * GPS device
 **************************************************************************/


/* Local declarations
 */
void playerc_gps_putdata(playerc_gps_t *device, player_msghdr_t *header,
                         player_gps_data_t *data, size_t len);
void playerc_gps_putlogdata(playerc_gps_t *device, int argc, char **argv);


/* Create a new gps proxy
 */
playerc_gps_t *playerc_gps_create(playerc_client_t *client, int index, int access)
{
    playerc_gps_t *device;

    device = malloc(sizeof(playerc_gps_t));
    playerc_client_adddevice(client, (playerc_device_t*) device,
                             PLAYER_GPS_CODE, index, access,
                             (playerc_putdata_fn_t) playerc_gps_putdata,
                             NULL);

    device->info.logname = "gps";
    device->info.putlogdata = (playerc_putlogdata_fn_t) playerc_gps_putlogdata;;

    return device;
}


/* Destroy a gps proxy
 */
void playerc_gps_destroy(playerc_gps_t *device)
{
    free(device);
}


/* Process incoming data
 */
void playerc_gps_putdata(playerc_gps_t *device, player_msghdr_t *header,
                           player_gps_data_t *data, size_t len)
{

    data->xpos = (int) ntohl(data->xpos);
    data->ypos = (int) ntohl(data->ypos);
    data->heading = (int) ntohl(data->heading);

    device->px = data->xpos / 1000.0;
    device->py = data->ypos / 1000.0;
    device->pa = data->heading * M_PI / 180.0;
}


/* Process incoming log data
 */
void playerc_gps_putlogdata(playerc_gps_t *device, int argc, char **argv)
{
    device->px = atof(argv[3]) / 1000.0;
    device->py = atof(argv[4]) / 1000.0;
    device->pa = atof(argv[5]) * M_PI / 180.0;
}


/* Teleport GPS device
 */
int playerc_gps_teleport(playerc_gps_t *device, double px, double py, double pa)
{
    player_gps_config_t body;

    body.xpos = htonl((int) (px * 1000));
    body.ypos = htonl((int) (py * 1000));
    body.heading = htonl((int) (pa * 180 / M_PI));

    return playerc_client_request(device->info.client, (playerc_device_t*) device,
                                  (char*) &body, sizeof(body), (char*) &body, sizeof(body));
}


/***************************************************************************
 * BPS device
 **************************************************************************/


/* Local declarations
 */
void playerc_bps_putdata(playerc_bps_t *device, player_msghdr_t *header,
                         player_bps_data_t *data, size_t len);
void playerc_bps_putlogdata(playerc_bps_t *device, int argc, char **argv);

/* Create a new bps proxy
 */
playerc_bps_t *playerc_bps_create(playerc_client_t *client, int index, int access)
{
    playerc_bps_t *device;

    device = malloc(sizeof(playerc_bps_t));
    playerc_client_adddevice(client, (playerc_device_t*) device,
                             PLAYER_BPS_CODE, index, access,
                             (playerc_putdata_fn_t) playerc_bps_putdata,
                             NULL);

    device->info.logname = "bps";
    device->info.putlogdata = (playerc_putlogdata_fn_t) playerc_bps_putlogdata;

    return device;
}


/* Destroy a bps proxy
 */
void playerc_bps_destroy(playerc_bps_t *device)
{
    free(device);
}


/* Process incoming data
 */
void playerc_bps_putdata(playerc_bps_t *device, player_msghdr_t *header,
                         player_bps_data_t *data, size_t len)
{
    device->px = (int) ntohl(data->px) / 1000.0;
    device->py = (int) ntohl(data->py) / 1000.0;
    device->pa = (int) ntohl(data->pa) * M_PI / 180.0;
    device->err = (double) ntohl(data->err) * 1e-6;
}


/* Process incoming log data
 */
void playerc_bps_putlogdata(playerc_bps_t *device, int argc, char **argv)
{
    device->px = atof(argv[3]) / 1000.0;
    device->py = atof(argv[4]) / 1000.0;
    device->pa = atof(argv[5]) * M_PI / 180.0;
    device->err = 0; 
}


/* Set the gain
 */
int playerc_bps_setgain(playerc_bps_t *device, double gain)
{
    player_bps_setgain_t body;

    body.subtype = PLAYER_BPS_SUBTYPE_SETGAIN;
    body.gain = htonl((int) (gain * 1e6));

    return playerc_client_request(device->info.client, (playerc_device_t*) device,
                                  (char*) &body, sizeof(body), (char*) &body, sizeof(body));
}


/* Set the pose of the laser relative to the robot
 */
int playerc_bps_setlaser(playerc_bps_t *device, double px, double py, double pa)
{
    player_bps_setlaser_t body;

    body.subtype = PLAYER_BPS_SUBTYPE_SETLASER;
    body.px = htonl((int) (px * 1000));
    body.py = htonl((int) (py * 1000));
    body.pa = htonl((int) (pa * 180.0 / M_PI));
    
    return playerc_client_request(device->info.client, (playerc_device_t*) device,
                                  (char*) &body, sizeof(body), (char*) &body, sizeof(body));
}


/* Set the true pose of a beacon
 */
int playerc_bps_setbeacon(playerc_bps_t *device, int id, double px, double py, double pa,
                           double ux, double uy, double ua)
{
    player_bps_setbeacon_t body;

    body.subtype = PLAYER_BPS_SUBTYPE_SETBEACON;
    body.id = id;
    body.px = htonl((int) (px * 1000));
    body.py = htonl((int) (py * 1000));
    body.pa = htonl((int) (pa * 180.0 / M_PI));
    body.ux = htonl((int) (ux * 1000));
    body.uy = htonl((int) (uy * 1000));
    body.ua = htonl((int) (ua * 180.0 / M_PI));
    
    return playerc_client_request(device->info.client, (playerc_device_t*) device,
                                  (char*) &body, sizeof(body), (char*) &body, sizeof(body));
}


/***************************************************************************
 * Broadcast device
 **************************************************************************/

/* Local declarations */
void playerc_broadcast_putdata(playerc_broadcast_t *device, player_msghdr_t *header,
                               player_broadcast_data_t *data, size_t len);
void playerc_broadcast_putlogdata(playerc_broadcast_t *device, int argc, char **argv);

/* Create a new broadcast proxy */
playerc_broadcast_t *playerc_broadcast_create(playerc_client_t *client, int index, int access)
{
  playerc_broadcast_t *device;

  device = malloc(sizeof(playerc_broadcast_t));
  playerc_client_adddevice(client, (playerc_device_t*) device,
                           PLAYER_BROADCAST_CODE, index, access,
                           (playerc_putdata_fn_t) playerc_broadcast_putdata,
                           NULL);

  device->in_len = 0;
  device->in_size = 1024;
  device->in_data = malloc(device->in_size);
    
  return device;
}


/* Destroy a broadcast proxy */
void playerc_broadcast_destroy(playerc_broadcast_t *device)
{
  free(device->in_data);
  free(device);
}


/* Process incoming data */
void playerc_broadcast_putdata(playerc_broadcast_t *device, player_msghdr_t *header,
                               player_broadcast_data_t *data, size_t len)
{
  data->len = ntohs(data->len);
  
  /* PRINT_DEBUG2("len %d data->len %d", len, data->len); */
  
  /* Add incoming data to queue */
  if (device->in_len + data->len > device->in_size)
  {
    device->in_size += data->len;
    device->in_data = realloc(device->in_data, device->in_size);
  }
  memcpy(device->in_data + device->in_len, data->buffer, data->len);
  device->in_len += data->len;
}



/* Write a message to the outgoing queue
   Returns 0 on succes; -1 on error */
int playerc_broadcast_write(playerc_broadcast_t *device, const char *data, int len)
{
  player_broadcast_cmd_t cmd;

  if (len + sizeof(uint16_t) > sizeof(cmd.buffer))
    return -1;

  cmd.len = htons(len + sizeof(uint16_t));
  *((uint16_t*) cmd.buffer) = htons(len);
  memcpy(cmd.buffer + sizeof(uint16_t), data, len);

  return playerc_client_write(device->info.client, &device->info,
                              (char*) &cmd, sizeof(cmd.len) + sizeof(uint16_t) + len);  
}


/* Read a message from the incoming queue
 Returns message length on success; -1 on error */
int playerc_broadcast_read(playerc_broadcast_t *device, char *data, int len)
{
  int msg_len;
  char *msg_data;

  /* See if there is any data available */
  if (device->in_len == 0)
    return -1;
  
  /* Extract message from queue */
  msg_len = ntohs(*(uint16_t*) device->in_data);
  msg_data = device->in_data + sizeof(uint16_t);

  /* PRINT_DEBUG2("in_len %d msg_len %d", device->in_len, msg_len); */
  
  if (msg_len > len)
  {
    PLAYERC_ERR("message buffer is too short; message truncated");
    return -1;
  }

  /* Copy message to caller's buffer */
  memcpy(data, msg_data, msg_len);

  /* Remove message from queue */
  device->in_len -= sizeof(uint16_t) + msg_len;
  memmove(device->in_data, device->in_data + sizeof(uint16_t) + msg_len,
          device->in_len);

  return msg_len;
}




/***************************************************************************
 * Self-test functions
 **************************************************************************/


/* Basic self-test function
 */
int playerc_test_client()
{
    int i;
    playerc_client_t *client;
    playerc_laser_t *laser;

    client = playerc_client_create(NULL, "localhost", 6665);
    laser = playerc_laser_create(client, 0, PLAYER_READ_MODE);

    if (playerc_client_connect(client) < 0)
        return -1;

    for (i = 0; i < 100; i++)
    {
        playerc_client_read(client);
        printf("laser : %d\n", laser->scan_count);
    }

    playerc_client_disconnect(client);
    playerc_laser_destroy(laser);
    playerc_client_destroy(client);

    return 0;
}


/* Multi-client self-test function
 */
int playerc_test_mclient(int port, int numservers, int numclients)
{
    int i, count;
    playerc_mclient_t *mclient;
    playerc_client_t **client;
    playerc_laser_t **laser;

    client = malloc(numclients * sizeof(playerc_client_t*));
    laser = malloc(numclients * sizeof(playerc_laser_t*));
    
    mclient = playerc_mclient_create();

    for (i = 0; i < numclients; i++)
    {
        client[i] = playerc_client_create(mclient, "localhost", port + (i % numservers));
        laser[i] = playerc_laser_create(client[i], 0, PLAYER_READ_MODE);
    }

    if (playerc_mclient_connect(mclient) < 0)
        return -1;
    
    for (i = 0; i < 10000; i++)
    {
        count = playerc_mclient_read(mclient, 50);
        printf("read %d packets\n", count);
    }

    playerc_mclient_disconnect(mclient);

    for (i = 0; i < numclients; i++)
    {
        playerc_laser_destroy(laser[i]);
        playerc_client_destroy(client[i]);
    }

    playerc_mclient_destroy(mclient);
    
    free(laser);
    free(client);

    return 0;
}


/* Entry point for self-test
 */
int xmain(int argc, char **argv)
{
    playerc_test_client();
    /*playerc_test_mclient(6665, 8, 100);*/
    return 0;
}



