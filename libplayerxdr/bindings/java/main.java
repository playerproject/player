public class main {
  public static void main(String argv[]) {
    System.loadLibrary("playerxdr_java");


    //////////////////////////////////////////////////////////
    // create and fill a message
    player_bumper_data_t outdata = new player_bumper_data_t();
    outdata.setBumpers_count(8);
    short[] outarr = outdata.getBumpers();
    for(int i=0; i<outdata.getBumpers_count(); i++)
      outarr[i] = (short)i;
    outdata.setBumpers(outarr);
    //////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////
    // allocate space to store the encoded message
    byte[] buf = new byte[64];
    // encode the message into the buffer
    int encoded_length = 
      playerxdr_java.player_bumper_data_pack(buf, buf.length, outdata,
                                             playerxdr_javaConstants.PLAYERXDR_ENCODE);
    if(encoded_length < 0)
    {
      System.out.println("packing failed");
      System.exit(-1);
    }
    else
    {
      System.out.println("encoded length: " + encoded_length);
    }
    //////////////////////////////////////////////////////////


    //////////////////////////////////////////////////////////
    // decode the message into a new object
    player_bumper_data_t indata = new player_bumper_data_t();
    if(playerxdr_java.player_bumper_data_pack(buf, encoded_length, indata,
                                              playerxdr_javaConstants.PLAYERXDR_DECODE) < 0)
    {
      System.out.println("unpacking failed");
      System.exit(-1);
    }
    else
    {
      System.out.println("count: " + indata.getBumpers_count());
      short[] arr = indata.getBumpers();
      for(int i=0;i<indata.getBumpers_count();i++)
        System.out.println("bumpers[" + i + "]: " + indata.getBumpers()[i]);
    }
  }
}

