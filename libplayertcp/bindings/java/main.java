/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2005
 *     Brian Gerkey
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

// Jplayercore defines the JNI classes that wrap the underlying C structs

// and C++ classes.
// Jplayercore also defines the standard (i.e., non-JNI) classes that correspond
// to Player messages.  For player C message struct 'player_foo_t',
// Jplayercore defines a 'Jplayer_foo_t' that contains equivalent data,
// in a Serializable form.
import net.sourceforge.playerstage.Jplayercore.*;

// Jplayertcp defines the JNI classes used to handles clients over sockets,
// including XDR marshaling.
import net.sourceforge.playerstage.Jplayertcp.*;

// The class playercore.player contains static methods that translate
// between the void* message buffers used in C++ and the corresponding 
// Java classes.   For every C message struct 'player_foo_t', there are
// two methods in playercore.player:
//
//    Jplayer_foo_t buf_to_Jplayer_foo_t(SWIGTYPE_p_void buf);
//    SWIGTYPE_p_void Jplayer_foo_t_to_buf(Jplayer_foo_t Jdata);
//
// Use the first method to create a Java object from the void* buffer returned
// when calling playercore.Message.GetPayload on a message popped on the
// player queue.  The resulting Java object can be sent via Jini.
//
// Use the second method to a create a void* buffer from a Java object
// that was received via Jini.  This void* buffer can then be passed to
// the playercore.Message constructor to create a new Message that can be
// pushed onto a player queue.

public class main 
{
  static boolean quit = false;

  public static void main(String argv[]) 
  {
  
    Runtime.getRuntime().addShutdownHook(new Thread()
    {
      public void run()
      {
        quit = true;
      }
    });

    // libplayercore_java.so (on Darwin, it should be a .jnilib)
    // implements the C/C++ <-> Java mappings.
    // 
    // After loading this library, you have access to two packages:
    //    playercore_java : The libplayercore code, wrapped in Java
    //    playercore_javaConstants : The #define'd constants as Java consts
    System.loadLibrary("playercore_java");
    // Also load the playertcp library.
    System.loadLibrary("playertcp_java");


    //////////////////////////////////////////////////////////////////
    // Initialization stuff

    playercore_java.player_register_drivers();
    playercore_java.ErrorInit(9,NULL);
    playertcp_java.playerxdr_ftable_init();

    // Convert "localhost" to a 32-bit address.  It will be used to
    // lookup available devices.
    long port = 6665;
    // The calling convention is a little funny here, because of the way
    // that SWIG wraps a C function that returns its results via a pointer
    // that was passed in.
    long[] hostnamearr = {0};
    playercore_java.hostname_to_packedaddr(hostnamearr,"localhost");
    long hostname = hostnamearr[0];

    // Create the configuration file parsing object, setting the default
    // host and port.
    ConfigFile cf = new ConfigFile(hostname, port);

    if(!cf.Load(argv[0]))
    {
      System.out.println("failed to load cfg file");
      System.exit(-1);
    }

    if(!cf.ParseAllDrivers())
    {
      System.out.println("failed to parse cfg file");
      System.exit(-1);
    }
    if(playercore_java.getDeviceTable().StartAlwaysonDrivers() != 0)
    {
      System.out.println("failed to start alwayson drivers");
      System.exit(-1);
    }

    // Setup the socket server on the default Player port, 6665
    PlayerTCP ptcp = new PlayerTCP();
    int ports[] = { 6665 };
    ptcp.Listen(ports,1);
    System.out.print("Listening on ports: ");
    for(int i=0;i<ports.length;i++)
      System.out.print(ports[i] + " ");
    System.out.println("");
  
    // Initialization stuff
    //////////////////////////////////////////////////////////////////

    // Create a message queue on which to receive messages from the devices
    MessageQueue mq = new MessageQueue(false, 1000);

    /////////////////////////////////////////////////////////////////
    // Subscribe to the device laser:1, which will produce corrected
    // scan-pose messages from the mapper.

    // Find the laser
    player_devaddr_t addr = new player_devaddr_t();
    addr.setHost(hostname);
    addr.setRobot(port);
    addr.setInterf(playercore_javaConstants.PLAYER_LASER_CODE);
    addr.setIndex(0);
    Device dev = playercore_java.getDeviceTable().GetDevice(addr,false);
    if(dev == null)
    {
      System.out.println("failed to find laser");
      System.exit(-1);
    }

    // Subscribe to the laser
    if(dev.Subscribe(mq) != 0)
    {
      System.out.println("failed to subscribe to laser");
      System.exit(-1);
    }

    // Subscribe to the device laser:0
    /////////////////////////////////////////////////////////////////

    // Main loop; receive data
    while(!quit)
    {
      // Allow non-threaded drivers to process messages
      playercore_java.getDeviceTable().UpdateDevices();
  
      // Let the socket server accept new connections
      if(ptcp.Accept(0) < 0)
      {
        System.out.println("failed while accepting new connections");
        break;
      }
      // Let the socket server read from current connections
      if(ptcp.Read(1) < 0)
      {
        System.out.println("failed while reading");
        break;
      }
      // Let the socket server write to current connections
      if(ptcp.Write() < 0)
      {
        System.out.println("failed while reading");
        break;
      }

      // Pop a message off our queue
      Message msg = mq.Pop();
      if(msg == null)
      {
        // no messages waiting
        continue;
      }

      // Extract the message header
      player_msghdr_t hdr = msg.GetHeader();
      // Extract a pointer to the message payload
      SWIGTYPE_p_void payload = msg.GetPayload();
      // Extract the address from the message header
      addr = hdr.getAddr();

      // Decide what to do with the message, based on what kind of message
      // it is

      // Is it a "scanpose" data message from laser:0?
      if((hdr.getType() == playercore_javaConstants.PLAYER_MSGTYPE_DATA) &&
         (hdr.getSubtype() == playercore_javaConstants.PLAYER_LASER_DATA_SCANPOSE) &&
         (addr.getInterf() == playercore_javaConstants.PLAYER_LASER_CODE) &&
         (addr.getIndex() == 0))
      {
        // Convert to normal Java class
        Jplayer_laser_data_scanpose_t data = player.buf_to_Jplayer_laser_data_scanpose_t(payload);

        System.out.println("\npose: " + 
                           data.pose.px + " " + 
                           data.pose.py + " " +
                           data.pose.pa);
        System.out.println("\nrange count: " + data.scan.ranges_count);
        for(int j=0;j<data.scan.ranges_count;j++)
        {
          System.out.print(data.scan.ranges[j] + " ");
        }
      }
    }

    // Unsubscribe from laser:0
    dev.Unsubscribe(mq);
  }
}

