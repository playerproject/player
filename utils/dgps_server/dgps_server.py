#!/usr/bin/env python


import os, termios
from socket import *


import sys
import time



# DGPS server class
class DGPSServer:
    """ Collect DGPS corrections from a base-station and broadcast
    them over the network using UDP.  """

    def __init__(self, serial_port, udp_ip, udp_port):

        self.serial_port = serial_port
        self.udp_ip = udp_ip
        self.udp_port = udp_port

        self.udp_open()
        self.serial_open()

        return


    def main(self):
        """Serve up corrections forever."""

        while 1:

            self.serial_read()

            # TODO: test for and re-transmit RTCM messages
            
            self.udp_write('this is a test\0')

        return


    def udp_open(self):
        """Create a multicast UDP socket."""

        self.udp_sock = socket(AF_INET, SOCK_DGRAM)
        self.udp_sock.setsockopt(IPPROTO_IP, IP_MULTICAST_TTL, 1)
        return


    def udp_write(self, msg):
        """Write a data packet."""

        self.udp_sock.sendto(msg, 0, (self.udp_ip, self.udp_port))
        return


    def serial_open(self):
        """Open and configure the serial port."""

        self.serial_fd = os.open(self.serial_port, os.O_RDWR)

        attr = termios.tcgetattr(self.serial_fd)
        attr[4] = termios.B4800
        termios.tcsetattr(self.serial_fd, termios.TCSAFLUSH, attr)
        return


    def serial_read(self):
        """Read packets from the serial port."""

        data = os.read(self.serial_fd, 100)
        print data

        return
    


if __name__ == '__main__':

    serial_port = '/dev/ttyS0'
    udp_ip = '225.0.0.43'
    udp_port = 7778

    server = DGPSServer(serial_port, udp_ip, udp_port)

    try:
        server.main()
    except KeyboardInterrupt:
        pass

    
    
