/*
 * $Id$
 * 
 *   class for encapsulating all the data pertaining to a single
 *   client
 */

#ifndef CLIENTDATA
#define CLIENTDATA

#include <pthread.h>

typedef enum
{
  CONTINUOUS,
  REQUESTREPLY
} server_mode_t;

class CClientData {
  unsigned char requested[20];
  pthread_mutex_t access, requesthandling;

  void MotorStop();
  void PrintRequested(char*);

 public:
  int client_index;
  int writeThreadId, readThreadId;
  pthread_t writeThread, readThread;

  server_mode_t mode;
  unsigned short frequency;  // Hz
  pthread_mutex_t datarequested, socketwrite;

  int socket;

  CClientData();
  ~CClientData();

  void HandleRequests( unsigned char *buffer, int readcnt );
  void RemoveBlanks();  
  void RemoveReadRequests();
  void RemoveWriteRequests();
  void UpdateRequested( unsigned char *request );
  bool CheckPermissions( unsigned char *command );
  unsigned char FindPermission( unsigned char device );
  int BuildMsg( unsigned char *data );
  void Unsubscribe( unsigned char device );
  int Subscribe( unsigned char device );
};


#endif
