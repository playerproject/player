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

/*
 * $Id$
 *
 * A device is a driver/interface pair.
 */

#ifndef _DEVICE_H
#define _DEVICE_H

#include <libplayercore/player.h>
#include <libplayercore/message.h>

// Forward declarations
class Driver;

/// @brief Encapsulates a device (i.e., a driver bound to an interface)
///
/// A device describes an instantiated driver/interface
/// combination.  Drivers may support more than one interface,
/// and hence appear more than once in the device table.
class Device
{
  private:
  public:
  
    /// @brief Constructor
    ///
    /// @p addr is the device address
    /// @p driver is a pointer to the underlying driver
    Device(player_devaddr_t addr, Driver *driver);

    /// @brief Destructor
    ~Device();

    /// @brief Subscribe the given queue to this device.
    int Subscribe(MessageQueue* sub_queue);

    /// @brief Unsubscribe the given queue from this device.
    int Unsubscribe(MessageQueue* sub_queue);

    /// @brief Send a message to this device.
    ///
    /// - @p type is the message type
    /// - @p subtype is the message subtype
    /// - @p src is the message payload
    /// - @p len is the length of the message payload
    /// - @p timestamp will be attached to the message; if it is NULL, then
    ///   the current time will be used.
    void PutMsg(MessageQueue* resp_queue,
                uint8_t type, 
                uint8_t subtype,
                void* src, 
                size_t len,
                double* timestamp);

    void PutMsg(MessageQueue* resp_queue,
                player_msghdr_t* hdr,
                void* src);

    Message* Request(MessageQueue* resp_queue,
                     uint8_t type,
                     uint8_t subtype,
                     void* src,
                     size_t len,
                     double* timestamp);

    static bool MatchDeviceAddress(player_devaddr_t addr1,
                                   player_devaddr_t addr2)
    {
      return((addr1.host == addr2.host) &&
             (addr1.robot == addr2.robot) &&
             (addr1.interf == addr2.interf) &&
             (addr1.index == addr2.index));
    }

    /// Next entry in the device table (this is a linked-list)
    Device* next;

    /// Address for this device
    player_devaddr_t addr;

    /// The string name for the underlying driver
    char drivername[PLAYER_MAX_DEVICE_STRING_LEN];

    /// Pointer to the underlying driver
    Driver* driver;

    /// Linked list of subscribed queues
    MessageQueue** queues;

    /// Length of @p queues
    size_t len_queues;

    /// Number of valid (i.e., non-NULL) elements in @p queues
    size_t num_queues;
};

#endif
