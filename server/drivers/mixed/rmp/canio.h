#ifndef _CANIO_H_
#define _CANIO_H_

#include <canlib.h>

#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>

struct can_packet_t
{
  long			id;
  uint8_t		msg[8];
  uint32_t		dlc;
  uint32_t		flags;

  can_packet_t() : dlc(8), flags(0) {
    for (int i =0; i < 8; i++) {
      msg[i] = 0;
    }
  }

  uint16_t GetSlot(int s)  const {
    return (uint16_t) ((msg[s*2] << 8) | (msg[s*2+1]));
  }

  void PutSlot(const int slot, const uint16_t val) {
    msg[slot] = (val >> 8) & 0xFF;
    msg[slot+1] = val & 0xFF;
  }

  char * toString() {
    static char buf[256];
    sprintf(buf, "id:%04X %02X %02X %02X %02X %02X %02X %02X %02X",
	    id, msg[0], msg[1], msg[2], msg[3], msg[4], msg[5], 
	    msg[6], msg[7]);

    return buf;
  }
} __attribute__((packed));
  

/* this class encapsulates the low level CAN stuff.... so it deals
   with reading and writing packets on the dual CAN channels.
   We make the assumption that we only have to read off of one
   channel though (looking at rmi_demo, it appears that this is
   OK.)
   A higher level entity will make sense of the packets, and call
   the read/write methods with the required timing.

   It wouldn't take much to make this an abstract base class so that
   the SegwayIO can use it, and then have different CAN hardwares
   implement the virtual methods.  So then we can just drop in 
   a new CAN hardware driver class and everything would still work.
   Would also be able to split up the files, so we could keep
   canio.[cc,h] in player, and the CAN hardware specific files
   can be local.
*/
class DualCANIO
{
public:
  DualCANIO();
  virtual ~DualCANIO();

  int Init(long channel_freq);
  int ReadPacket(can_packet_t *pkt, int channel);
  int WritePacket(can_packet_t &pkt);
  int Shutdown();

protected:

  canHandle channels[2];
};






#endif
