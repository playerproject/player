import java.io.*;
import java.net.*;

/** This class is meant to hide all the truble concerning communications with the robot. Provides methods for reading data and sending motor commands */
public class PlayerClient {
    
    DataOutputStream os;
    BufferedOutputStream buffer;
    Socket socket;
    DataInputStream is;
    int numberOfSensorsUsed=0;

    final int ACTS_NUM_CHANNELS = 32; // n.of colors system can be trained for
    final int ACTS_BLOB_SIZE = 10;   // the number of bytes encoding a blob
    final int ACTS_MAX_NO_BLOBS = 10; // max n.o. blobs of each color


    public class ColorChannel {

	public short noBlobs;
	public ColorBlob[] blob = new ColorBlob[ACTS_MAX_NO_BLOBS];
	
	public ColorChannel() {
	   
	    int i;
	    for (i=0; i<ACTS_MAX_NO_BLOBS; ++i)
		blob[i] = new ColorBlob();
	}

	public boolean anyBlobs() {
	    return (noBlobs>0);
	}

    }


    public class ColorBlob {

	int area;
	short x, y, left, right, top, bottom; // really only unsigned bytes.

    }


    public class CameraFeedback {
	short pan, tilt, zoom; // camera feedback 
    }

    public short readUnsignedByte(DataInputStream is) {
	
	// The vision system returns unsigned bytes, but Java
	// doesn't have unsigned bytes, so here we will represent
	// values as shorts.
	try {
	    byte ret = is.readByte();
	    if (ret<0)
		return (short)(256+ret);
	    else
		return (short)(ret);
	}
	catch(Exception e) {
	    System.err.println("readUnsignedByte()-error. "+e.toString());
	    return -1;
	}
    }
    
    /** Sonar data */
    public short sonar[]=new short[16];

    /** Laser data */
    public short laser[]=new short[361];

    /** Position data */
    public int time,x,y;
    public short heading,speed,turnRate,compass;
    public boolean stalled=false;

    /** Gripper data */
    public byte gripperData1, gripperData2;

    /** Misc. data */
    public short frontBumperbits, backBumperbits, battery;
    public boolean frontBumper[] = new boolean[5];
    public boolean backBumper[] = new boolean[5];

    // frontBumpers 0 - 4 clockwise from left most bumper.
    // backBumpers 0 - 4 clockwise from right most bumper.


  /** Vision data */
    public ColorChannel[] channel = new ColorChannel[ACTS_NUM_CHANNELS];
    // the list of active channels holds numbers of channels with #blobs > 0
    // (confer with method howManyColorsInCameraImage() !):
    public int[] activeChannel = new int[ACTS_NUM_CHANNELS];

    public CameraFeedback camfeedback= new CameraFeedback();

  // 32 channels, each has at most 10 color blobs, each blob has
  // at most 10 bytes of data, including these:
  // area, x-pos, y-pos, left-, right-, top-, bottom-boundaries.


    /** Initialises robot communications */
    public PlayerClient(String serverName, int portNumber) {
	try {
	    socket=new Socket(serverName,portNumber);
	    is=new DataInputStream(socket.getInputStream());
	    buffer=new BufferedOutputStream(socket.getOutputStream(),128);
	    os=new DataOutputStream(new DataOutputStream(buffer));
	} catch (Exception e) { 
	    System.err.println("Error in RobotSocket init:"+e.toString()); 
	};

	// set up color channel information:

	for (int i=0; i<ACTS_NUM_CHANNELS; ++i)
	    channel[i] = new ColorChannel();
    }

    /** Returns number of different color channels that have
     *  blobs in the current camera image  */
    public int howManyColorsInCameraImage() {
	int n=0;
	for (int i=0; i<ACTS_NUM_CHANNELS; ++i) {
	    if (channel[i].noBlobs>0)
		++n;
	}
	return n;
    }

    
    /** Handles request for devices */
    public void request(String requestString) {
	try {
	    os.write("dr".getBytes());
	    os.writeShort(requestString.length());
	    os.write(requestString.getBytes());
	    buffer.flush();
	    if ((char)is.read()=='r') {
		int j=is.readShort();
		byte cBuf[]=new byte[j];
		is.read(cBuf); 
		numberOfSensorsUsed=0;
		for (int i=0; i<j; i=i+2) {
		    if ((char)cBuf[i+1]=='e') System.err.println("Request for device "+(char)cBuf[i]+" failed!");
		    if ((char)cBuf[i+1]=='r') numberOfSensorsUsed++;
		    if ((char)cBuf[i+1]=='a') numberOfSensorsUsed++;
		}
		System.err.println("Reading "+numberOfSensorsUsed+" device(s)");
	    } else {
		throw(new Exception("Non-proper reply"));
	    }
	} catch (Exception e) { System.err.println("configure error:"+e.toString()); }
	}
    
    /** Set translation and rotation speed for robot */
    public void setSpeed(int t, int r) { // ( translation, rotation )
	try {
	    os.write(("cp").getBytes());
	    os.writeShort(4);
	    os.writeShort(t);
	    os.writeShort(r);
	    buffer.flush();
	} catch (Exception e) {
	    System.err.println("setSpeed error:"+e.toString()); }
    }

    /** Sets pan, tilt and zoom for camera */
    public void setVision(int p, int t, int z) {   // ( pan, tilt, zoom )
	try {
	    os.write(("cz").getBytes());
	    os.writeShort(6);
	    os.writeShort(p);
	    os.writeShort(t);
	    os.writeShort(z);
	    buffer.flush();
	} catch (Exception e) {
	    System.err.println("setVision error:"+e.toString()); }
    }
    
    /** Sets the gripper command */
    public void setGripper(int g) {   // ( gripper_command )
	try {
	    os.write(("cg").getBytes());
	    os.writeShort(2);
	    os.writeShort(g);
	    buffer.flush();
	} catch (Exception e) {
	    System.err.println("setGripper error:"+e.toString()); }
    }

    /** Updates all fields of data for all devices with read acces */
    public void getData() { 
	for (int i=0; i<numberOfSensorsUsed; i++) {
	    try {
		int pony = is.read();
		if (pony==-1)
		    {
			System.err.println("Connection lost to server.");
			System.exit(0);
		    }
		byte b = (byte)pony;
		if ((char)b=='s') { 
		    getSonar();
		} else if ((char)b=='l') {
		    getLaser();
		} else if ((char)b=='g') {
		    getGripper();
		} else if ((char)b=='m') {
		    getMisc();
		} else if ((char)b=='p') {
		    getPosition();
		} else if ((char)b=='v') {
		    getVision();
		} else if ((char)b=='z') {
		    getCameraFeedback();
		} else { 
		    System.err.println(b+"Nothing"+(byte)Character.getNumericValue('s'));
		}
	    } catch (Exception e) { System.err.println("getDataError: "+e.toString()); } 
	}
    }

    void getVision() {   // 1 has been added to all values by ACTS
	try {
	    int cntr = 0;
	    int checksum = 0;
	    int j=is.readShort();
	    int i, k;
	    int area;
	    for (i=0; i<ACTS_NUM_CHANNELS; i++) {
		is.readByte(); // not used: index of channel i in buffer
		//#blobs in this channel:
		channel[i].noBlobs = (short)(readUnsignedByte(is)-1);
		if (channel[i].noBlobs>0) { // this channel is active
		    checksum += channel[i].noBlobs;
		    activeChannel[cntr] = i; // add it to the active list:
		    ++cntr;
		}
	    }
	    if ((checksum*10+64) != j) {
		throw new Exception("Checksum error while getting vision data");
	    }
	    for (i=0; i<ACTS_NUM_CHANNELS; i++) {
		for (j=0; j<channel[i].noBlobs; ++j) {
		    
		    // get area of blob, encoded in 4 bytes, each of
		    // which only use the first 6 bits:
		    area=0;
		    for (k=0; k<4; ++k) {
			area = area << 6;

			area += (short)(readUnsignedByte(is)-1);
		    }
		    // area is area of blob,
		    // x,y is centroid (or center?) of blob,
		    // left, right are x-coordinates of bounding box,
		    // top, bottom are y-coordinates of bounding box.

		    channel[i].blob[j].area = area;
		    channel[i].blob[j].x = (short)(readUnsignedByte(is)-1);
		    channel[i].blob[j].y = (short)(readUnsignedByte(is)-1);
		    channel[i].blob[j].left = (short)(readUnsignedByte(is)-1);
		    channel[i].blob[j].right = (short)(readUnsignedByte(is)-1);
		    channel[i].blob[j].top = (short)(readUnsignedByte(is)-1);
		    channel[i].blob[j].bottom =(short)(readUnsignedByte(is)-1);
		}
	    }

	} catch (Exception e) {
	    System.err.println("getVision error:"+e.toString()); 
	}
    }

    void getSonar() {   
	try {
	    int j=is.readShort();
	    if (j==32) {
		for (int i=0; i<16; i++) {
		    sonar[i]=is.readShort();
		}
	    } else {
		System.err.println("Not the correct number of sonar data");
	    }
	} catch (Exception e) {
	    System.err.println("getSonar error:"+e.toString()); 
	}
    }

    void getCameraFeedback() {   
	try {
	    int j=is.readShort();
	    if (j==6) {
		camfeedback.pan = is.readShort();
		camfeedback.tilt = is.readShort();
		camfeedback.zoom = is.readShort();
	    } else {
		System.err.println("Not the correct number of camera feedback data");
	    }
	} catch (Exception e) {
	    System.err.println("getCameraFeedback error:"+e.toString()); 
	}
    }

    void getLaser() {   
	try {
	    int j=is.readShort();
	    if (j==722) {
		for (int i=0; i<361; i++) {
		    laser[i]=is.readShort();
		}
	    } else {
		System.err.println("Not the correct number of laser data");
	    }
	} catch (Exception e) {
	    System.err.println("getLaser error:"+e.toString()); 
	}
    }

    void getPosition() {   
	try {
	    int j=is.readShort();
	    if (j==21) {
		time=is.readInt();
		x=is.readInt();
		y=is.readInt();
		heading=is.readShort();
		speed=is.readShort();
		turnRate=is.readShort();
		compass=is.readShort();
		if (is.readByte()==0) { stalled=false;
 		} else { stalled=true; }
	    } else {
		System.err.println("Not the correct number of position data");
	    }
	} catch (Exception e) {
	    System.err.println("getPosition error:"+e.toString()); 
	}
    }

    void getGripper() {   
	try {
	    int j=is.readShort();
	    if (j==2) {
		gripperData1=is.readByte();
		gripperData2=is.readByte();
	    } else {
		System.err.println("Not the correct number of gripper data");
	    }
	} catch (Exception e) {
	    System.err.println("getGripper error:"+e.toString()); 
	}
    }

    void getMisc() {   
	try {
	    int j=is.readShort();
	    if (j==3) {
		frontBumperbits=is.readByte();
		backBumperbits=readUnsignedByte(is);
		battery=readUnsignedByte(is);
		int b=1;
		for (int i=0; i<5; ++i) {
		    // interpret the bits:
		    frontBumper[i] = ((frontBumperbits & b) == b);
		    backBumper[i] = ((backBumperbits & b) == b);
		    b = b << 1;
		}
		
	    } else {
		System.err.println("Not the correct number of misc. data");
	    }
	} catch (Exception e) {
	    System.err.println("getMisc error:"+e.toString()); 
	}
    }
    
}  

