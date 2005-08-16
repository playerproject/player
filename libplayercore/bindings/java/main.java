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

    // Create a message queue on which to receive message from the devices
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
    dev.Subscribe(mq);
  }
}

