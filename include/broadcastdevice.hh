///////////////////////////////////////////////////////////////////////////
//
// File: broadcastdevice.hh
// Author: Andrew Howard
// Date: 5 Deb 2000
// Desc: Device for inter-player communication using broadcast sockets.
//
// CVS info:
//  $Source$
//  $Author$
//  $Revision$
//
// Usage:
//  (empty)
//
// Theory of operation:
//  (empty)
//
// Known bugs:
//  (empty)
//
// Possible enhancements:
//  (empty)
//
///////////////////////////////////////////////////////////////////////////

#ifndef BROADCASTDEVICE_HH
#define BROADCASTDEVICE_HH

#include <stddef.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "device.h"
#include "messages.h"

class CBroadcastDevice : public CDevice
{
  // Init function
  public: 
    static CDevice* Init(int argc, char** argv)
    {
      return((CDevice*)(new CBroadcastDevice(argc,argv)));
    }
  
  // Constructor
  public: CBroadcastDevice(int argc, char** argv);

  // Override the subscribe/unsubscribe calls, since we need to
  // maintain our own subscription list.
  public: virtual int Subscribe(void *client);
  public: virtual int Unsubscribe(void *client);

  // Setup/shutdown routines
  public: virtual int Setup();
  public: virtual int Shutdown();

  // Handle requests.  We dont queue them up, but handle them immediately.
  public: virtual int PutConfig(CClientData* client, unsigned char* data, size_t len);

  // Main function for device thread
  private: virtual void Main();

  // Setup the message queues
  private: int SetupQueues();

  // Shutdown the message queues
  private: int ShutdownQueues();
  
  // Create a new queue
  private: int AddQueue(void *client);

  // Delete queue for a client
  private: int DelQueue(void *client);

  // Find the queue for a particular client.
  private: int FindQueue(void *client);

  // Push a message onto all of the queues
  private: int PushQueue(void *msg, int len);

  // Pop a message from a particular client's queue
  private: int PopQueue(void *client, void *msg, int len);
  
  // Initialise the broadcast sockets
  private: int SetupSockets();

  // Shutdown the broadcast sockets
  private: int ShutdownSockets();
  
  // Send a packet to the broadcast socket.
  private: void SendPacket(void *packet, size_t size);

  // Receive a packet from the broadcast socket.  This will block.
  private: int RecvPacket(void *packet, size_t size);
  
  // Queue used for each client.
  private: struct queue_t
  {
    void *client;
    int size, count, start, end;
    int *msglen;
    void **msg;
  };

  // Max messages to have in any given queue
  private: int max_queue_size;
  
  // Message queue list (one for each client).
  private: int qlist_size, qlist_count;
  private: queue_t **qlist;
  
  // Address and port to broadcast on
  private: char *addr;
  private: int port;

  // Write socket info
  private: int write_socket;
  private: sockaddr_in write_addr;

  // Read socket info
  private: int read_socket;
  private: sockaddr_in read_addr;
};

#endif

