ADD_LIBRARY ( mote STATIC
	      MoteProtocol.cpp
	      MoteIF.cpp
	      MoteException.cpp )

TARGET_LINK_LIBRARIES (mote)