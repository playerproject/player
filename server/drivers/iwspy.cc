/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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

/* Desc: Driver for getting signal strengths from access points.
 * Author: Andrew Howard ahoward@usc.edu
 * Date: 26 Nov 2002
 * $Id$
 *
 * This driver works like iwspy; it uses the linux wireless extensions
 * to get signal strengths to wireless NICS.
 *  
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <device.h>
#include <configfile.h>
#include <playertime.h>
#include <drivertable.h>
#include <player.h>

extern PlayerTime *GlobalTime;

class Iwspy : public CDevice
{
  public: Iwspy(char *interface, ConfigFile *cf, int section);

  // Initialize driver
  public: virtual int Setup();

  // Finalize driver
  public: virtual int Shutdown();

  // Main function for device thread.
  public: virtual void Main();

  // Initialize the iwspy watch list
  private: int InitIwSpy();

  // Update the iwspy values
  private: void UpdateIwSpy();

  // Parse the iwspy output
  private: void Parse(int fd);

  // Start pinging
  private: int StartPing();

  // Stop pinging
  private: void StopPing();

  // Data for each NIC to be monitored
  private: struct nic_t
  {
    // MAC address of NIC
    const char *mac;

    // Link properties
    int link, level, noise;
  };

  // The list of NIC's to be monitored
  private: int nic_count;
  private: nic_t nics[8];

  // PID of the ping process
  private: pid_t ping_pid;
};


////////////////////////////////////////////////////////////////////////////////
// Instantiate driver for given interface
CDevice * Iwspy_Init(char *interface, ConfigFile *cf, int section)
{ 
  if(strcmp(interface, PLAYER_WIFI_STRING))
  {
    PLAYER_ERROR1("driver \"iwspy\" does not support interface \"%s\"\n",
                  interface);
    return NULL;
  }
  else
  {
    return ((CDevice*)(new Iwspy(interface, cf, section)));
  }
}


////////////////////////////////////////////////////////////////////////////////
// Register driver type
void Iwspy_Register(DriverTable *table)
{
  table->AddDriver("iwspy", PLAYER_READ_MODE, Iwspy_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
Iwspy::Iwspy(char *interface, ConfigFile *cf, int section)
    : CDevice(0, 0, 0, 1)
{
  // TODO: read from config file
  this->nic_count = 0;
  this->nics[this->nic_count++].mac = "00:30:AB:15:3D:D7";
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Initialize driver
int Iwspy::Setup()
{
  // Start the device thread
  StartThread();

  // Initialize the watch list
  if (this->InitIwSpy() != 0)
    return -1;

  // Start pinging
  if (this->StartPing() != 0)
    return -1;
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Finalize driver
int Iwspy::Shutdown()
{
  // Stop device thread
  StopThread();

  // Stop pinging
  this->StopPing();
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Main function for device thread
void Iwspy::Main() 
{
  while (true)
  {
    // Test if we are supposed to terminate.
    pthread_testcancel();
    usleep(100000);

    // Ping each of the IP's on our watch list.
    // TODO

    // Get the updated iwspy info
    this->UpdateIwSpy();

    // Output the new data
    // TODO
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Initialize the iwspy watch list
int Iwspy::InitIwSpy()
{
  int i;
  int status;
  pid_t pid;
  int argc;
  char *args[16];

  // Fork here
  pid = fork();

  // If in the child process...
  if (pid == 0)
  {
    argc = 0;
    args[argc++] = "iwspy";
    args[argc++] = "eth0";

    // Add the list of MAC addresses to be monitored.
    for (i = 0; i < this->nic_count; i++)
      args[argc++] = (char*) this->nics[i].mac;
    
    args[argc++] = NULL;
    
    // Run iwspy
    if (execv("/sbin/iwspy", args) != 0)
    {
      PLAYER_ERROR1("error on exec: [%s]", strerror(errno));
      exit(errno);
    }
    assert(false);
  }

  // If in the parent process...
  else
  {
    // Wait for the child to finish
    if (waitpid(pid, &status, 0) < 0)
    {
      PLAYER_ERROR1("error on waitpid: [%s]", strerror(errno));
      return -1;
    }
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Update the iwspy values
void Iwspy::UpdateIwSpy()
{
  int status;
  pid_t pid;
  int stdout_pipe[2];

  // Create pipes
  if (pipe(stdout_pipe) < 0)
  {
    PLAYER_ERROR1("error on pipe: [%s]", strerror(errno));
    return;
  }

  // Fork here
  pid = fork();

  // If in the child process...
  if (pid == 0)
  {
    close(1);
    dup(stdout_pipe[1]);
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);

    // Run iwspy
    if (execl("/sbin/iwspy", "iwspy", "eth0", NULL) != 0)
    {
      PLAYER_ERROR1("error on exec: [%s]", strerror(errno));
      exit(errno);
    }
    assert(false);
  }

  // If in the parent process...
  else
  {
    // Wait for the child to finish
    if (waitpid(pid, &status, 0) < 0)
    {
      PLAYER_ERROR1("error on waitpid: [%s]", strerror(errno));
      return;
    }

    // Parse the output
    this->Parse(stdout_pipe[0]);
    
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
  }
  
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Parse the iwspy output
void Iwspy::Parse(int fd)
{
  int i, j;
  ssize_t bytes;
  char buffer[80 * 25];
  char line[1024];
  char mac[16];
  int link, level, noise;

  bytes = read(fd, buffer, sizeof(buffer));
  if (bytes < 0)
  {
    PLAYER_ERROR1("error on read: [%s]", strerror(errno));
    return;
  }

  printf("%s\n", buffer);

  for (i = 0; i < bytes;)
  {
    // Suck out a line
    for (j = i; j < bytes && buffer[j] != '\n'; j++)
      line[j - i] = buffer[j];
    line[j - i] = 0;
    i = j + 1;
    
    //printf("[%s]\n", line);

    if (sscanf(line, " %s : Quality:%d/%*d Signal level:%d dBm  Noise level:%d dBm",
               mac, &link, &level, &noise) < 4)
      continue;

    printf("iwspy: %s %d %d %d\n", mac, link, level, noise);
  }

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Start ping
int Iwspy::StartPing()
{
  // Fork here
  this->ping_pid = fork();

  // If in the child process...
  if (this->ping_pid == 0)
  {
    // Run ping
    if (execl("/bin/ping", "ping", "10.0.1.254", NULL) != 0)
    {
      PLAYER_ERROR1("error on exec: [%s]", strerror(errno));
      exit(errno);
    }
    assert(false);
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Stop ping
void Iwspy::StopPing()
{
  int status;
  
  // Wait for the child to finish
  if (waitpid(this->ping_pid, &status, 0) < 0)
  {
    PLAYER_ERROR1("error on waitpid: [%s]", strerror(errno));
    return;
  }
  return;
}
