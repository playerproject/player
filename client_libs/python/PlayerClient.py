#
# PlayerClient.py : Python module for Player
# Copyright (C) 2001  Boyoon Jung
#                     boyoon@robotics.usc.edu
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

import socket
import struct
import sys


# global constants - device codes
player_player_code      = 1
player_misc_code        = 2
player_gripper_code     = 3
player_position_code    = 4
player_sonar_code       = 5
player_laser_code       = 6
player_vision_code      = 7
player_ptz_code         = 8
player_audio_code       = 9
player_laserbeacon_code = 10
player_broadcast_code   = 11
    
# global costants - access modes
player_read_mode  = 'r'
player_write_mode = 'w'
player_all_mode   = 'a'
player_close_mode = 'c'

# global constants - wheel velocity control modes
player_direct_mode   = 0
player_separate_mode = 1
    
# global constants - misc
player_num_sonar_samples = 16
player_num_laser_samples = 361

# global constants - error codes
player_socket_error = 0
player_device_error     = 1


# data structure - packet headers
class player_msghdr_t:
    "Data structure for packet headers"
    def __init__(self):
        self.stx = 0
        self.type = 0
        self.device = 0
        self.index = 0
        self.time1 = 0
        self.time2 = 0
        self.timestamp1 = 0
        self.timestamp2 = 0
        self.reserved = 0
        self.size = 0

# data structure - general devices
class player_device_t:
    "Data structure for general devices"
    pass        # member data will be added dynamically

# data structure - general data
class player_data_t:
    "Data structure for general data"
    pass        # member data will be added dynamically


# exception
class PlayerError(Exception):
    pass


# class - python binding for Player client
class playerclient:
    "Python binding for Player client"

    # local constants - ACTS constants
    acts_num_channels = 32
    acts_len_header = 2*acts_num_channels
    acts_len_blob = 10
    acts_max_blobs = 10

    # local constants - message types
    msg_data = 1
    msg_cmd  = 2
    msg_req  = 3
    msg_resp = 4

    # local constants - request messages
    req_device   = 1
    req_data     = 2
    req_datamode = 3
    req_datafreq = 4

    # local constants - format strings
    fmt_header          = '!2sHHHLLLLLL'
    fmt_device_req      = '!HHHc'
    fmt_misc            = '!BBB'
    fmt_gripper         = '!BB'
    fmt_position        = '!llHhhHB'
    fmt_sonar           = '!H'
    fmt_laser           = '!H'
    fmt_ptz             = '!hhH'
    fmt_laserbeacon     = '!BHhh'
    fmt_laserbeacon_hdr = '!H'
    fmt_position_cmd    = '!hh'
    fmt_gripper_cmd     = '!BB'
    fmt_ptz_cmd         = '!hhH'
    fmt_velocity_ctl    = '!sB'
    fmt_laser_config    = '!HHB'
    fmt_frequency       = '!HH'
    fmt_motor_state     = '!sB'

    # local constants - default values
    default_port = 6665
    ident_strlen = 32
    player_stx   = 'xX'
    laserbeacon_len = struct.calcsize(fmt_laserbeacon)

    # conversion table - device codes -> device name
    code2name = {1:'player', 2:'misc', 3:'gripper', 4:'position', 5:'sonar',
                 6:'laser', 7:'vision', 8:'ptz', 9:'audio', 10:'laserbeacon',
                 11:'broadcast'}

    # constructor
    def __init__(self):
        "Constructor"
        
        # set default values for connections
        self.socket = None
        self.host = 'localhost'
        self.port = self.default_port

        # create data structure for devices
        self.misc = {}
        self.gripper = {}
        self.position = {}
        self.newposition = {}
        self.sonar = {}
        self.laser = {}
        self.vision = {}
        self.ptz = {}
        self.newptz = {}
        self.audio = {}
        self.laserbeacon = {}
        self.broadcast = {}

        # create data structure to hold info. about requested devices
        self.devicedatatable = {}
        

    # connect to Player server
    def connect(self, host=None, port=None):
        "Connect to Player server"
	# take default values when no parameters
        if host is None: host = self.host
        if port is None: port = self.port

        try:
	    # make a socket
            self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	    # connect to a server
            self.socket.connect((host,port))
	    # retrieve an identity string
            self.banner = self.socket.recv(self.ident_strlen)
        except socket.error, (errno, errstr):
            self.socket = None
            raise PlayerError, (player_socket_error, errstr)
        else:
            print "The client connected to (%s, %s)." % (host, port)


    # disconnect to Player server
    def disconnect(self):
        "Disconnect to Player server"
        if self.socket is not None:
            try:
	        # close a socket
                self.socket.shutdown(2)
                self.socket.close()
            except socket.error, (errno, errstr):
                raise PlayerError, (player_socket_error, errstr)
            else:
                print "The connection is closed"
                self.socket = None
        else:
            raise PlayerError, (player_socket_error,
                                "The connection hasn't been established yet.")


    def request_device_access(self, device, index=0, access=player_all_mode):
        "Request access to a single device"
	# make a binary string with payload info.
        payload = struct.pack(self.fmt_device_req,
                              self.req_device,
                              device,
                              index,
                              access)
        try:
	    # send a request
            hdr, pl = self.player_request(player_player_code, 0, payload)
	    # check if the request got accepted
            if payload != pl:
                raise PlayerError, (player_device_error,
                                    "Device request (%s) got declined"%self.code2name[device])
        except socket.error, (errno, errstr):
            raise PlayerError(player_socket_error, errstr)
        else:
            # add to / delete from a device table
            if access == player_close_mode:
                del self.devicedatatable[(device, index)]
            else:
                self.devicedatatable[(device, index)] = access
                # MISC
                if device == player_misc_code:
                    self.misc[index] = player_device_t()
                    self.misc[index].frontbumpers = 0
                    self.misc[index].rearbumpers = 0
                    self.misc[index].voltage = 0
                # GRIPPER
                elif device == player_gripper_code:
                    self.gripper[index] = player_device_t()
                    self.gripper[index].cmd = 0
                    self.gripper[index].arg = 0
                    self.gripper[index].state = 0
                    self.gripper[index].beams = 0
                # POSITION
                elif device == player_position_code:
                    self.position[index] = player_device_t()
                    self.position[index].xpos = 0
                    self.position[index].ypos = 0
                    self.position[index].theta = 0
                    self.position[index].speed = 0
                    self.position[index].turnrate = 0
                    self.position[index].compass = 0
                    self.position[index].stalls = 0
                    self.newposition[index] = player_device_t()
                    self.newposition[index].speed = 0
                    self.newposition[index].turnrate = 0
                # SONAR
                elif device == player_sonar_code:
                    self.sonar[index] = range(player_num_sonar_samples)
                # LASER
                elif device == player_laser_code:
                    self.laser[index] = range(player_num_laser_samples)
                # VISION
                elif device == player_vision_code:
                    self.vision[index] = {}
                    for i in range(self.acts_num_channels):
                        self.vision[index][i] = []
                # PTZ
                elif device == player_ptz_code:
                    self.ptz[index] = player_device_t()
                    self.ptz[index].pan = 0
                    self.ptz[index].tilt = 0
                    self.ptz[index].zoom = 0
                    self.newptz[index] = player_device_t()
                    self.newptz[index].pan = 0
                    self.newptz[index].tilt = 0
                    self.newptz[index].zoom = 0
                # AUDIO
                elif device == player_audio_code:
                    print "TODO: register audio data"
                # LASERBEACON
                elif device == player_laserbeacon_code:
                    self.laserbeacon[index] = []
                # BROADCAST
                elif device == player_broadcast_code:
                    print "TODO: register broadcast data"
                # Invalid device
                else:
                    print "Read Error: unrecognized device (%d)" % device
            

    # query the current device access
    def query_device_access(self, device, index=0):
        "Query the current device access"
        return self.devicedatatable.get((device, index))


    # read one set of data from server
    def read(self):
        "Read one set of data from server"
        # calculate the number of packets to be read
        count = 0
        for (device, index) in self.devicedatatable.keys():
            access = self.devicedatatable[(device,index)]
            if access == player_all_mode or access == player_read_mode:
                count = count + 1
        # fill out data
        cnt_read = 0
        while cnt_read < count:
            try:
                hdr, pl = self.player_read()
            except socket.error, (errno, errstr):
                raise PlayerError, (player_socket_error, errstr)
            access = self.devicedatatable[(hdr.device, hdr.index)]
            if access == player_all_mode or access == player_read_mode:
                cnt_read = cnt_read + 1
                # MISC
                if hdr.device == player_misc_code:
                    self.misc[hdr.index].frontbumpers,\
                    self.misc[hdr.index].rearbumpers,\
                    self.misc[hdr.index].voltage\
                    = struct.unpack(self.fmt_misc, pl)
                # GRIPPER
                elif hdr.device == player_gripper_code:
                    self.gripper[hdr.index].state,\
                    self.gripper[hdr.index].beams\
                    = struct.unpack(self.fmt_gripper, pl)
                # POSITION
                elif hdr.device == player_position_code:
                    self.position[hdr.index].xpos,\
                    self.position[hdr.index].ypos,\
                    self.position[hdr.index].theta,\
                    self.position[hdr.index].speed,\
                    self.position[hdr.index].turnrate,\
                    self.position[hdr.index].compass,\
                    self.position[hdr.index].stalls\
                    = struct.unpack(self.fmt_position, pl)
                # SONAR
                elif hdr.device == player_sonar_code:
                    for i in range(player_num_sonar_samples):
                        self.sonar[hdr.index][i]\
                        = struct.unpack(self.fmt_sonar, pl[i*2:(i+1)*2])[0]
                # LASER
                elif hdr.device == player_laser_code:
                    for i in range(player_num_laser_samples):
                        self.laser[hdr.index][i]\
                        = struct.unpack(self.fmt_laser, pl[i*2:(i+1)*2])[0]
                # VISION
                elif hdr.device == player_vision_code:
                    # clear old data
                    for i in range(self.acts_num_channels):
                        del self.vision[hdr.index][i][:]
                    # fill out new data
                    ptr = self.acts_len_header
                    for i in range(self.acts_num_channels):
                        num_blobs = ord(pl[2*i+1])-1
                        if num_blobs > 0:
                            for j in range(num_blobs):
                                # read a color blob
                                blob = player_data_t()
                                blob.area = ord(pl[ptr]) * 262144 + \
                                            ord(pl[ptr+1]) * 4096 + \
                                            ord(pl[ptr+2]) * 64 + \
                                            ord(pl[ptr+3])
                                blob.x = ord(pl[ptr+4]) - 1
                                blob.y = ord(pl[ptr+5]) - 1
                                blob.left = ord(pl[ptr+6]) - 1
                                blob.right = ord(pl[ptr+7]) - 1
                                blob.top = ord(pl[ptr+8]) - 1
                                blob.bottom = ord(pl[ptr+9]) - 1
                                ptr = ptr + self.acts_len_blob
                                # add the color blob
                                self.vision[hdr.index][i].append(blob)
                # PTZ
                elif hdr.device == player_ptz_code:
                    self.ptz[hdr.index].pan,\
                    self.ptz[hdr.index].tilt,\
                    self.ptz[hdr.index].zoom\
                    = struct.unpack(self.fmt_ptz, pl)
                # AUDIO
                elif hdr.device == player_audio_code:
                    print "TODO: read audio data"
                # LASERBEACON
                elif hdr.device == player_laserbeacon_code:
                    # clear old data
                    del self.laserbeacon[hdr.index][:]
                    # fill out new data
                    (cnt,) = struct.unpack(self.fmt_laserbeacon_hdr, pl[0:2])
                    if cnt > 0:
                        for i in range(cnt):
                            lbeacon = player_data_t()
                            lbeacon.id,\
                            lbeacon.range,\
                            lbeacon.bearing,\
                            lbeacon.orient\
                            = struct.unpack(self.fmt_laserbeacon,
                                   pl[2+i*self.laserbeacon_len:2+(i+1)*self.laserbeacon_len])
                            if lbeacon.id != 0:
                                self.laserbeacon[hdr.index].append(lbeacon)
                # BROADCAST
                elif hdr.device == player_broadcast_code:
                    print "TODO: read broadcast data"
                # Invalid device
                else:
                    raise PlayerError, (player_device_error,
                                        "Read Error: unrecognized device (%d)" % hdr.device)
                    

    # write commands to server
    def write(self, device=None, index=0):
        "Write commands to server"
        if device is not None:
            try:
                self.player_write(device, index)
            except socket.error, (errno, errstr):
                raise PlayerError, (player_socket_error, errstr)
        else:
            for (device, index) in self.devicedatatable.keys():
                access = self.devicedatatable[(device,index)]
                if access == player_all_mode or access == player_write_mode:
                    try:
                        self.player_write(device, index)
                    except socket.error, (errno, errstr):
                        raise PlayerError, (player_socket_error, errstr)


    # print current data (for debug)
    def print_info(self):
        "Print current data (for debug)"
        for (device, index) in self.devicedatatable.keys():
            # MISC
            if device == player_misc_code:
                print '* MISC data'
                print '  - front bumpers: %d' % self.misc[index].frontbumpers
                print '  - rear bumpers: %d' % self.misc[index].rearbumpers
                print '  - voltage: %d' % self.misc[index].voltage
            # GRIPPER
            elif device == player_gripper_code:
                print '* GRIPPER data'
                print '  - state: %d' % self.gripper[index].state
                print '  - beams: %d' % self.gripper[index].beams
            # POSITION
            elif device == player_position_code:
                print '* POSITION data'
                print '  - pos: (%d,%d)' % (self.position[index].xpos,\
                                            self.position[index].ypos)
                print '  - theta: %d' % self.position[index].theta
                print '  - speed: %d' % self.position[index].speed
                print '  - turnrate: %d' % self.position[index].turnrate
                print '  - compass: %d' % self.position[index].compass
                print '  - stalls: %d' % self.position[index].stalls
            # SONAR
            elif device == player_sonar_code:
                print '* SONAR data'
                print self.sonar[index]
            # LASER
            elif device == player_laser_code:
                print '* LASER data'
                print self.laser[index]
            # VISION
            elif device == player_vision_code:
                print '* VISION data'
                nodata = 1
                for i in range(self.acts_num_channels):
                    if len(self.vision[index][i]) > 0:
                        nodata = 0
                        print '  - Channel #%d has %d blobs:' %\
                                           (i,len(self.vision[index][i]))
                        for blob in self.vision[index][i]:
                            print '    . Area: ', blob.area
                            print '      Position: (%d,%d)' % (blob.x, blob.y)
                            print '      Boundary: (%d,%d, %d,%d)' %\
                                  (blob.left,blob.right, blob.top,blob.bottom)
                if nodata:
                    print '  - no color blob has been detected.'
            # PTZ
            elif device == player_ptz_code:
                print '* PTZ data'
                print '  - pan: %d' % self.ptz[index].pan
                print '  - tilt: %d' % self.ptz[index].tilt
                print '  - zoom: %d' % self.ptz[index].zoom
            # AUDIO
            elif device == player_audio_code:
                print "TODO: print audio data"
            # LASERBEACON
            elif device == player_laserbeacon_code:
                print '* LASERBEACON data'
                if len(self.laserbeacon[index]) > 0:
                    for lbeacon in self.laserbeacon[index]:
                        print '  - ID: ', lbeacon.id
                        print '    Range: ', lbeacon.range
                        print '    Bearing: ', lbeacon.bearing
                        print '    Orient: ', lbeacon.orient
                else:
                    print '  - no laserbeacon has been detected.'
            # BROADCAST
            elif device == player_broadcast_code:
                print "TODO: read broadcast data"
            # Invalid device
            else:
                raise PlayerError, (player_device_error,
                                    "Print Error: unrecognized device (%d)" % device)


    # change vecocity control mode
    def change_velocity_control(self, mode):
        """Change velocity control mode.
        'mode' should be either 'player_direct_mode' (default) or 'player_separate_mode'"""
        if mode == player_direct_mode or mode == player_separate_mode:
            payload = struct.pack(self.fmt_velocity_ctl, 'v', mode)
            try:
                self.player_request(player_position_code, 0, payload)
            except socket.error, (errno, errstr):
                raise PlayerError, (error_socket_error, errstr)
        else:
            raise PlayerError, (error_device_error,
                                "Error: invalid request (velocity control mode:%d)" % mode)


    # set the laser configuration
    def set_laser_config(self, min, max, intensity):
        """Set the laser configuration.
        Use min and max to specify a restricted scan.
        Set intensity to true to get intensity data
        in the top three bits of the range scan data"""
        payload = struct.pack(self.fmt_laser_config, min, max, intensity)
        try:
            self.player_request(player_laser_code, 0, payload)
        except socket.error, (errno, errstr):
            raise PlayerError, (error_socket_error, errstr)


    # enable/disable the motors
    def change_motor_state(self, state):
        """Enable/disable the motors:
        if 'state' is non-zero, then enable motors
        else disable motors"""
        payload = struct.pack(self.fmt_motor_state, 'm', state)
        try:
            self.player_request(player_position_code, 0, payload)
        except socket.error, (errno, errstr):
            raise PlayerError, (player_socket_error, errstr)


    # enable/disable sonars
    def change_sonar_state(self, state):
        "Enable/disable sonars"
        pass


    # change the update frequency
    def set_frequency(self, freq):
        "Change the update frequency at which this client receives data"
        payload = struct.pack(self.fmt_frequency, self.req_datafreq, freq)
        try:
            self.player_request(player_player_code, 0, payload)
        except socket.error, (errno, errstr):
            raise PlayerError, (error_socket_error, errstr)


    # set the broadcast message
    def set_broadcast_message(self, message, size):
        "Set the broadcast message for this player"
        print "TODO: set_broadcast_message"


    # set the broadcast_message
    def get_broadcast_message(self):
        "Get the n'th broadcast message that was received by the last read"
        print "TODO: get_braodcast_message"
     

    # receive data from a player server
    def player_read(self):
        "Internal use only: Receive data from a player server"
        # read a header from a socket
        len_total = struct.calcsize(self.fmt_header)
        len_read = 0
        hdrstr = ''
        while len_read < len_total:
            r = self.socket.recv(len_total-len_read)
            hdrstr = hdrstr + r
	    len_read = len(hdrstr)
        # make a header structure with the received data
        hdr = player_msghdr_t()
        hdr.stx,\
        hdr.type, hdr.device, hdr.index,\
        hdr.time1, hdr.time2, hdr.timestamp1, hdr.timestamp2,\
        hdr.reserved,\
        hdr.size = struct.unpack(self.fmt_header, hdrstr)
        if hdr.stx != self.player_stx:
            raise PlayerError, (player_socket_error, 'Invalid packet')
        # read a payload from a socket
        len_total = hdr.size
        len_read = 0
        plstr = ''
        while len_read < len_total:
            p = self.socket.recv(len_total-len_read)
            plstr = plstr + p
	    len_read = len(plstr)
	# return data received
        return hdr, plstr


    # send commands to da player server
    def player_write(self, device, index):
        "Internal use only: Send command to a player server"
        # fill out common data
        header = player_msghdr_t()
        header.stx = self.player_stx
        header.type = self.msg_cmd
        header.device = device
        header.index = index

        # GRIPPER
        if device == player_gripper_code:
            header.size = struct.calcsize(self.fmt_gripper_cmd)
            payload = struct.pack(self.fmt_gripper_cmd,
                                  self.gripper[index].cmd,
                                  self.gripper[index].arg)
        # POSITION
        elif device == player_position_code:
            header.size = struct.calcsize(self.fmt_position_cmd)
            payload = struct.pack(self.fmt_position_cmd,
                                  self.newposition[index].speed,
                                  self.newposition[index].turnrate)
        # PTZ
        elif device == player_ptz_code:
            header.size = struct.calcsize(self.fmt_ptz_cmd)
            payload = struct.pack(self.fmt_ptz_cmd,
                                  self.newptz[index].pan,
                                  self.newptz[index].tilt,
                                  self.newptz[index].zoom)
        # AUDIO
        elif device == player_audio_code:
            print "TODO: write audio data"
            header.size = 0
            payload = ''
        # BROADCAST
        elif device == player_broadcast_code:
            print "TODO: write broadcast data"
            header.size = 0
            payload = ''
        # Invalid device
        else:
            header.size = 0
            payload = ''
            raise PlayerError, (player_device_error,
                                "Invalid device (%s)" % device)

        # send command
        hdrstr = struct.pack(self.fmt_header,
                             header.stx,
                             header.type,
                             header.device,
                             header.index,
                             header.time1,
                             header.time2,
                             header.timestamp1,
                             header.timestamp2,
                             header.reserved,
                             header.size)
        self.socket.send(hdrstr+payload)
        

    # send requests to a player server
    def player_request(self, device, index, payload):
        "Internal use only: Send requests to a player server"
	# fill out a header
        header = player_msghdr_t()
        header.stx = self.player_stx
        header.type = self.msg_req
        header.device = device
        header.index = index
        header.size = len(payload)
	# make a binary string with header info.
        hdrstr = struct.pack(self.fmt_header,
                             header.stx,
                             header.type,
                             header.device,
                             header.index,
                             header.time1,
                             header.time2,
                             header.timestamp1,
                             header.timestamp2,
                             header.reserved,
                             header.size)
	# send data to a player server
        self.socket.send(hdrstr+payload)
	# receive a reply from the server
	hdr, pl = self.player_read()
        while hdr.type != self.msg_resp or hdr.device != header.device or hdr.index != header.index:
            hdr, pl = self.player_read()
        # return a reply from the server
        return hdr, pl

