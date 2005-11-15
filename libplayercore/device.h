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
    /// @param addr : Device address
    /// @param driver : Pointer to the underlying driver
    Device(player_devaddr_t addr, Driver *driver);

    /// @brief Destructor
    ~Device();

    /// @brief Subscribe the given queue to this device.
    ///
    /// @returns 0 on success, non-zero otherwise.
    int Subscribe(MessageQueue* sub_queue);

    /// @brief Unsubscribe the given queue from this device.
    ///
    /// @returns 0 on success, non-zero otherwise.
    int Unsubscribe(MessageQueue* sub_queue);

    /// @brief Send a message to this device.
    ///
    /// This method is the basic way of sending a message to a device.  The
    /// header will be assembled and the message will get pushed on to the 
    /// underlying driver's InQueue.
    ///
    /// @param resp_queue : Where to push any reply (e.g., your InQueue)
    /// @param type : Message type
    /// @param subtype : Message subtype
    /// @param src : Message payload
    /// @param len : Length of the message payload
    /// @param timestamp : If non-NULL, the timestamp to attach to the
    /// message; otherwise, the current time is filled in.
    void PutMsg(MessageQueue* resp_queue,
                uint8_t type, 
                uint8_t subtype,
                void* src, 
                size_t len,
                double* timestamp);

    /// @brief Send a message to this device (short form)
    ///
    /// This form of PutMsg is useful if you already have the message
    /// header assembled (e.g., when you're forwarding a message).
    /// 
    /// @param resp_queue Where to push any reply
    /// @param hdr The message header.
    /// @param src The message body (its size is stored in hdr->size).
    void PutMsg(MessageQueue* resp_queue,
                player_msghdr_t* hdr,
                void* src);

    /// @brief Make a request of another device.
    ///
    /// This method send a request message to a device
    /// and waits for the reply.
    ///
    /// @param resp_queue : Where to push the reply (e.g., your InQueue)
    /// @param type : Message type (usually PLAYER_MSGTYPE_REQ).
    /// @param subtype : Message subtype (interface-specific)
    /// @param src : Message body
    /// @param len : Length of src.
    /// @param timestamp : If non-NULL, the timestamp to attach to the
    /// request; otherwise, the current time is filled in.
    ///
    /// @returns A pointer to the reply message.  The caller is responsible
    ///          for deleting this pointer.
    Message* Request(MessageQueue* resp_queue,
                     uint8_t type,
                     uint8_t subtype,
                     void* src,
                     size_t len,
                     double* timestamp,
                     bool threaded = true);

    /// @brief Compare two addresses
    ///
    /// This static method returns true if all 4 components of the two
    /// addresses match exactly.  It's useful in Driver::ProcessMessage
    /// when you're deciding how to handle a message.
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
    char drivername[PLAYER_MAX_DRIVER_STRING_LEN];

    /// Pointer to the underlying driver
    Driver* driver;

    /// Linked list of subscribed queues
    MessageQueue** queues;

    /// Length of @p queues
    size_t len_queues;
};

#endif
