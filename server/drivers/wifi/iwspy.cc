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
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include <driver.h>
#include <error.h>
#include <configfile.h>
#include <playertime.h>
#include <drivertable.h>
#include <player.h>

extern PlayerTime *GlobalTime;

class Iwspy : public Driver
{
  public: Iwspy( ConfigFile *cf, int section);

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

  // Lookup a MAC address
  private: int ArpLookup(const char *ip, char *mac);

  // Start pinging
  private: int StartPing();

  // Stop pinging
  private: void StopPing();

  // Data for each NIC to be monitored
  private: struct nic_t
  {
    // IP address of the NIC
    char ip[64];
    
    // MAC address of NIC
    char mac[64];

    // Link properties
    int link, level, noise;

    // Counters to keep track of new/old information.
    int in_count, out_count;
  };

  // Interface to be monitored
  private: const char *ethx;

  // The list of NIC's to be monitored
  private: int nic_count;
  private: nic_t nics[8];

  // PID of the ping process
  private: int ping_count;
  private: pid_t ping_pid[8];
};


////////////////////////////////////////////////////////////////////////////////
// Instantiate driver for given interface
Driver * Iwspy_Init( ConfigFile *cf, int section)
{ 
  return ((Driver*)(new Iwspy( cf, section)));
}


////////////////////////////////////////////////////////////////////////////////
// Register driver type
void Iwspy_Register(DriverTable *table)
{
  table->AddDriver("iwspy", Iwspy_Init);
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Constructor
Iwspy::Iwspy( ConfigFile *cf, int section)
    : Driver(cf, section, PLAYER_WIFI_CODE, PLAYER_READ_MODE,
             sizeof(player_wifi_data_t), 0, 0, 0)
{
  int i;
  char key[64];
  const char *ip, *mac;

  // Read the number of pings to start
  this->ping_count = 5;

  // Ethernet interface to monitor
  this->ethx = cf->ReadString(section, "eth", "eth1");
  
  // Read IP addresses to monitor
  this->nic_count = 0;
  for (i = 0; i < 8; i++)
  {
    snprintf(key, sizeof(key), "nic_%d", i);
    ip = cf->ReadTupleString(section, key, 0, NULL);
    mac = cf->ReadTupleString(section, key, 1, NULL);

    if (!ip || !mac)
      break;

    strcpy(this->nics[this->nic_count].ip, ip);
    strcpy(this->nics[this->nic_count].mac, mac);
    this->nic_count++;
  }
  return;
}


////////////////////////////////////////////////////////////////////////////////
// Initialize driver
int Iwspy::Setup()
{
  // Start pinging
  if (this->StartPing() != 0)
    return -1;

  // Give the ping time to refresh the arp table before
  // trying to spy
  usleep(2000000);

  // Arp lookup is unreliable for some reason
  /* REMOVE?
  // Wait a while until the arp table is refreshed
  usleep(1000000);
  
  // Get the mac addresses
  for (i = 0; i < this->nic_count; i++)
  {
    if (this->ArpLookup(this->nics[i].ip, mac) != 0)
      return -1;
    strcpy(this->nics[i].mac, mac);
  }
  */
  
  // Initialize the watch list
  if (this->InitIwSpy() != 0)
    return -1;

  // Start the device thread
  StartThread();

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
  int i;
  nic_t *nic;
  struct timeval time;
  player_wifi_link_t *link;
  player_wifi_data_t data;
  
  while (true)
  {
    // Test if we are supposed to terminate.
    pthread_testcancel();
    usleep(100000);

    // Get the time at which we started reading.
    // This is not a great estimate of when the phenomena occurred.
    GlobalTime->GetTime(&time);

    // Get the updated iwspy info
    this->UpdateIwSpy();

    // Construct data packet
    data.link_count = 0;
    for (i = 0; i < this->nic_count; i++)
    {
      nic = this->nics + i;

      if (nic->in_count > nic->out_count)
      {
        link = data.links + data.link_count++;
        strcpy(link->ip, nic->ip);
        link->qual = htons(nic->link);
        link->level = htons(nic->level);
        link->noise = htons(nic->noise);
        nic->out_count = nic->in_count;
      }
    }
    data.link_count = htons(data.link_count);

    // Send data
    PutData((uint8_t*) &data, sizeof(data), &time);
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
    args[argc++] = strdup(this->ethx);

    // Add the list of MAC addresses to be monitored.
    for (i = 0; i < this->nic_count; i++)
      args[argc++] = this->nics[i].mac;
    
    args[argc++] = NULL;
    
    // Run iwspy
    if (execvp("iwspy", args) != 0)
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
  int dummy_fd;
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

    // Pipe stderr output to /dev/null
    dummy_fd = open("/dev/null", O_RDWR);
    dup2(dummy_fd, 2);

    // Run iwspy
    if (execlp("iwspy", "iwspy", this->ethx, NULL) != 0)
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
  char mac[32];
  int link, level, noise;
  char status[16];
  nic_t *nic;

  bytes = read(fd, buffer, sizeof(buffer));
  if (bytes < 0)
  {
    PLAYER_ERROR1("error on read: [%s]", strerror(errno));
    return;
  }

  //printf("%s\n", buffer);

  for (i = 0; i < bytes;)
  {
    // Suck out a line
    for (j = i; j < bytes && buffer[j] != '\n'; j++)
      line[j - i] = buffer[j];
    line[j - i] = 0;
    i = j + 1;
    
    //printf("[%s]\n", line);

    // See if there is new data (data is new if we get a status value)
    if (sscanf(line, " %s : Quality:%d/%*d Signal level:%d dBm  Noise level:%d dBm (%s)",
               mac, &link, &level, &noise, status) < 5)
    {
      //status[0] = 0;
      //if (sscanf(line, " %s : Quality:%d/%*d Signal level:%d dBm  Noise level:%d dBm",
      //           mac, &link, &level, &noise) < 4)
      continue;
    }

    //printf("mac [%s]\n", mac);
    
    // Update the appropriate entry in the nic list.
    for (j = 0; j < this->nic_count; j++)
    {
      nic = this->nics + j;

      if (strcmp(nic->mac, mac) == 0)
      {
        nic->link = link;
        nic->level = level;
        nic->noise = noise;
        nic->in_count++;
        //printf("iwspy: %s %d %d %d\n", nic->ip, link, level, noise);
        break;
      }
    }
  }

  return;
}


////////////////////////////////////////////////////////////////////////////////
// Lookup a MAC address
int Iwspy::ArpLookup(const char *ip, char *mac)
{
  int bytes;
  char line[1024];
  int status;
  pid_t pid;
  int stdout_pipe[2];

  // Create pipes
  if (pipe(stdout_pipe) < 0)
  {
    PLAYER_ERROR1("error on pipe: [%s]", strerror(errno));
    return -1;
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
    if (execl("/sbin/arp", "arp", "-n", "-a", ip, NULL) != 0)
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

    // Parse the output
    bytes = read(stdout_pipe[0], line, sizeof(line));
    if (bytes < 0)
    {
      PLAYER_ERROR1("error reading data: [%s]", strerror(errno));
      return -1;
    }

    //printf("arp [%s]\n", line);

    if (sscanf(line, "%*s %*s at %s ", mac) < 1)
    {
      PLAYER_ERROR1("unable to get hardware address for [%s]", ip);
      return -1;
    }
    
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
  }
  
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Start ping.  This is a hack; we really should generate our own ICMP packets.
int Iwspy::StartPing()
{
  int i;
  int dummy_fd;
  
  for (i = 0; i < this->ping_count; i++)
  {
    assert(i < (int) (sizeof(this->ping_pid) / sizeof(this->ping_pid[0])));

    // Space the pings out over 1 second
    usleep(1000000 / this->ping_count);
    
    // Fork here
    this->ping_pid[i] = fork();

    // If in the child process...
    if (this->ping_pid[i] == 0)
    {
      // Pipe all the output to /dev/null
      dummy_fd = open("/dev/null", O_RDWR);
      dup2(dummy_fd,0);
      dup2(dummy_fd,1);
      dup2(dummy_fd,2);

      // Run ping
      if (execlp("ping", "ping", "-b", "10.0.0.0", NULL) != 0)
      {
        PLAYER_ERROR1("error on exec: [%s]", strerror(errno));
        exit(errno);
      }
      assert(false);
    }
  }
  return 0;
}


////////////////////////////////////////////////////////////////////////////////
// Stop ping
void Iwspy::StopPing()
{
  int i;
  int status;

  for (i = 0; i < this->ping_count; i++)
  {
    // Kill ping
    kill(this->ping_pid[i], SIGKILL);
    
    // Wait for the child to finish
    if (waitpid(this->ping_pid[i], &status, 0) < 0)
    {
      PLAYER_ERROR1("error on waitpid: [%s]", strerror(errno));
      return;
    }
  }
  return;
}
