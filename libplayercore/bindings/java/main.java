import playercore.*;

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
  
    System.loadLibrary("playercore_java");

    // Initialization stuff
    playercore_java.player_register_drivers();
    playercore_java.ErrorInit(9);

    ConfigFile cf = new ConfigFile(0, 0);

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

    // Create a message queue on which to receive messages from the devices
    MessageQueue mq = new MessageQueue(false, 1000);

    // Find the laser
    player_devaddr_t addr = new player_devaddr_t();
    addr.setHost(0);
    addr.setRobot(0);
    addr.setInterf(playercore_javaConstants.PLAYER_LASER_CODE);
    addr.setIndex(0);
    Device dev = playercore_java.getDeviceTable().GetDevice(addr);
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

    // Main loop; receive data
    while(!quit)
    {
      // Allow non-threaded drivers to process messages
      playercore_java.getDeviceTable().UpdateDevices();

      // Pop a message off the queue
      Message msg = mq.Pop();
      if(msg == null)
      {
        // no messages waiting; yield the processor
        try { Thread.currentThread().sleep(10); }
        catch (InterruptedException e) { }

        continue;
      }

      player_msghdr_t hdr = msg.GetHeader();
      SWIGTYPE_p_void payload = msg.GetPayload();
      addr = hdr.getAddr();
      if(addr.getInterf() == playercore_javaConstants.PLAYER_LASER_CODE)
      {
        player_laser_data_t data = playercore_java.buf_to_player_laser_data_t(payload);
        System.out.println("\nrange count: " + data.getRanges_count());
        for(int j=0;j<data.getRanges_count();j++)
        {
          System.out.print(data.getRanges()[j] + " ");
        }

        SWIGTYPE_p_void newpayload = playercore_java.player_laser_data_t_to_buf(data);
        Message newmsg = new Message(hdr, newpayload, 0, mq);

      }

    }

    dev.Unsubscribe(mq);
  }
}

