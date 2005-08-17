public class main {
  public static void main(String argv[]) {
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
    for(int i=0;i<100;i++)
    {
      // Block until data is available
      System.out.println("waiting");
      mq.Wait();
      // Pop a message off the queue
      Message msg = mq.Pop();
      if(msg == null)
      {
        System.out.println("got null message after waiting");
        continue;
      }

      player_msghdr_t hdr = msg.GetHeader();
      SWIGTYPE_p_unsigned_char payload = msg.GetPayload();
      addr = hdr.getAddr();
      if(addr.getInterf() == playercore_javaConstants.PLAYER_LASER_CODE)
      {
        System.out.println("got laser message");
        player_laser_data_t data = playercore_java.BufToLaserData(payload);
        System.out.println("\nrange count: " + data.getRanges_count());
        for(int j=0;j<data.getRanges_count();j++)
        {
          System.out.print(data.getRanges()[j] + " ");
        }
      }
    }

    dev.Unsubscribe(mq);
  }
}

