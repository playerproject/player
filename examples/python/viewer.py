#
# viewer.py: player viewer
# Copyrigh (C) 2001  Boyoon Jung
#                    boyoon@robotics.usc.edu
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


from Tkinter import *
from threading import *
import sys
import time
from math import *

# in case PlayerClient module hasn't been installed
sys.path.append('../../client_libs/python/')
from PlayerClient import *


# Player Viewer: show sensory readings & allow to send commands
class Viewer:
    "Player Viewer: show sensory readings & allow to send commands"

    # local variable - for thread synchronization
    lock_robot = Lock()

    # local variables - flag if should be shown
    flag_sonar = 0
    flag_laser = 0
    flag_laserbeacon = 0
    flag_position = 0
    flag_vision = 0
    flag_ptz = 0
    flag_misc = 0

    # local variables - for canvas drawing
    sonar_list = []
    sonar_angle = [pi, pi/180*140, pi/180*120, pi/180*100, pi/180*80, pi/180*60, pi/180*40, 0,
                   0, pi/180*320, pi/180*300, pi/180*280, pi/180*260, pi/180*240, pi/180*220, pi]
    laser_list = []
    laserbeacon_list = []
    blob_list = []
    blob_colors = ['red', 'green', 'blue', 'cyan', 'magenta', 'yellow']
    
    def __init__(self, root):
        "Constructor"

        # make a connection to a player server
        try:
            self.robot = playerclient()
            self.parse_arguments()
            self.robot.connect()
            self.robot.set_frequency(2)
        except PlayerError, (errno, errstr):
            print '[Error %s] %s' % (errno, errstr)
            print 'Critical error... Terminating...'
            sys.exit()
        
        # main frame for user interface
        main = Frame(root)
        main.pack(fill=BOTH, expand=1, padx=3, pady=3)

        # robot frmae
        rwin_frame = Frame(main, borderwidth=2, relief=GROOVE)
        rwin_frame.grid(row=0, column=0, rowspan=3, padx=3, pady=3)

        self.show_sonar = IntVar()
        Checkbutton(rwin_frame, text='Sonar', fg='blue', variable=self.show_sonar,
                    command=self.show_sonar_cb).grid(row=0, column=0, padx=3, pady=3, sticky=W)

        self.show_laser = IntVar()
        Checkbutton(rwin_frame, text='Laser', fg='blue', variable=self.show_laser,
                    command=self.show_laser_cb).grid(row=0, column=1, padx=3, pady=3, sticky=W)

        self.show_laserbeacon = IntVar()
        Checkbutton(rwin_frame, text='Laserbeacon', fg='blue', variable=self.show_laserbeacon,
                    command=self.show_laserbeacon_cb).\
                    grid(row=0, column=2, padx=3, pady=3, sticky=W)

        self.robotwin = Canvas(rwin_frame, bg='black', width=500, height=500)
        self.robotwin.grid(row=1, column=0, columnspan=3, padx=3, pady=3)
        rb = 240, 235, 260, 265
        self.robot_rect = self.robotwin.create_rectangle(rb, fill='red')

        # position frame
        position_frame = Frame(main, borderwidth=2, relief=GROOVE)
        position_frame.grid(row=0, column=1, padx=3, pady=3, sticky=N+E+W)

        self.show_position = IntVar()
        Checkbutton(position_frame, text='Position', fg='blue', variable=self.show_position,
                    command=self.show_position_cb).\
                    grid(row=0, column=0, columnspan=3, padx=3, pady=3, sticky=W)

        Label(position_frame, text='Position:').grid(row=1, column=0, padx=3, sticky=W)
        self.lbl_position = Label(position_frame, text='(0,0)')
        self.lbl_position.grid(row=1, column=1, columnspan=2, padx=3)

        Label(position_frame, text='Theta:').grid(row=2, column=0, padx=3, sticky=W)
        self.lbl_theta = Label(position_frame, text='0')
        self.lbl_theta.grid(row=2, column=1, columnspan=2, padx=3)

        Label(position_frame, text='Compass:').grid(row=3, column=0, padx=3, sticky=W)
        self.lbl_compass = Label(position_frame, text='0')
        self.lbl_compass.grid(row=3, column=1, columnspan=2, padx=3)

        Label(position_frame, text='Stall:').grid(row=4, column=0, padx=3, sticky=W)
        self.lbl_stall = Label(position_frame, text='0')
        self.lbl_stall.grid(row=4, column=1, columnspan=2, padx=3)

        Label(position_frame, text='Speed:').grid(row=5, column=0, padx=3, sticky=W)
        self.lbl_speed = Label(position_frame, text='0')
        self.lbl_speed.grid(row=5, column=1, columnspan=2, padx=3)

        Scale(position_frame, orient=HORIZONTAL, width=10, length=120, from_=-1000, to=1000,
              resolution=50, command=self.scale_speed_cb).\
              grid(row=6, column=0, columnspan=3, padx=3, pady=3)

        Label(position_frame, text='Turnrate:').grid(row=7, column=0, padx=3, sticky=W)
        self.lbl_turnrate = Label(position_frame, text='0')
        self.lbl_turnrate.grid(row=7, column=1, columnspan=2, padx=3)

        Scale(position_frame, orient=HORIZONTAL, width=10, length=120, from_=-360, to=360,
              resolution=10, command=self.scale_turnrate_cb).\
              grid(row=8, column=0, columnspan=3, padx=3, pady=3)

        # Vision
        vision_frame = Frame(main, borderwidth=2, relief=GROOVE)
        vision_frame.grid(row=0, column=2, rowspan=2, padx=3, pady=3, sticky=N)

        self.show_vision = IntVar()
        Checkbutton(vision_frame, text='Vision', fg='blue', variable=self.show_vision,
                    command=self.show_vision_cb).\
                    grid(row=0, column=0, columnspan=5, padx=3, pady=3, sticky=W)

        self.visionwin = Canvas(vision_frame, bg='black', width=160, height=120)
        self.visionwin.grid(row=1, column=0, columnspan=5, padx=3, pady=3)

        # PTZ
        self.show_ptz = IntVar()
        Checkbutton(vision_frame, text='PTZ', fg='blue', variable=self.show_ptz,
                    command=self.show_ptz_cb).\
                    grid(row=2, column=0, columnspan=3, padx=3, pady=3, sticky=W)

        Label(vision_frame, text='Pan:').grid(row=3, column=0, padx=3, sticky=W)
        self.lbl_pan = Label(vision_frame, text='0')
        self.lbl_pan.grid(row=3, column=1, columnspan=2, padx=3)

        Scale(vision_frame, orient=HORIZONTAL, width=10, length=150, from_=-100, to=100,
              resolution=5, command=self.scale_pan_cb).\
              grid(row=4, column=0, columnspan=3, padx=3, pady=3)

        Label(vision_frame, text='Tilt:').grid(row=5, column=0, padx=3, sticky=W)
        self.lbl_tilt = Label(vision_frame, text='   0')
        self.lbl_tilt.grid(row=5, column=1, columnspan=2, padx=3)

        Scale(vision_frame, orient=HORIZONTAL, width=10, length=150, from_=-25, to=25,
              command=self.scale_tilt_cb).grid(row=6, column=0, columnspan=3, padx=3, pady=3)

        Label(vision_frame, text='Zoom:').grid(row=7, column=0, padx=3, sticky=W)
        self.lbl_zoom = Label(vision_frame, text='   0')
        self.lbl_zoom.grid(row=7, column=1, columnspan=2, padx=3)

        Scale(vision_frame, orient=HORIZONTAL, width=10, length=150, from_=0, to=1023,
              resolution=10, command=self.scale_zoom_cb).\
              grid(row=8, column=0, columnspan=3, padx=3, pady=3)

        # Misc
        misc_frame = Frame(main, borderwidth=2, relief=GROOVE)
        misc_frame.grid(row=1, column=1, padx=3, pady=3, sticky=N+E+W)

        self.show_misc = IntVar()
        Checkbutton(misc_frame, text='Misc', fg='blue', variable=self.show_misc,
                    command=self.show_misc_cb).grid(row=0,column=0, padx=3,pady=3, sticky=W)

        Label(misc_frame, text='Front Bumper:').grid(row=1, column=0, padx=3, sticky=W)
        self.lbl_frontbumper = Label(misc_frame, text='0')
        self.lbl_frontbumper.grid(row=1, column=1, padx=3)

        Label(misc_frame, text='Rear Bumper:').grid(row=2, column=0, padx=3, sticky=W)
        self.lbl_rearbumper = Label(misc_frame, text='0')
        self.lbl_rearbumper.grid(row=2, column=1, padx=3)

        Label(misc_frame, text='Voltage:').grid(row=3, column=0, padx=3, sticky=W)
        self.lbl_voltage = Label(misc_frame, text='0')
        self.lbl_voltage.grid(row=3, column=1, padx=3)

        # Functions
        func_frame = Frame(main, borderwidth=2, relief=GROOVE)
        func_frame.grid(row=2, column=1, columnspan=2, padx=3, pady=3, sticky=N+E+W)

        Label(func_frame, text='Options', fg='blue').\
                          grid(row=0, column=0, columnspan=3, padx=3, pady=3, sticky=W)

        Label(func_frame, text='Velocity Control:').grid(row=1, column=0, padx=3, sticky=E)
	self.vc_mode = IntVar()
	self.vc_mode.set(0)
	Radiobutton(func_frame, text='Direct', variable=self.vc_mode, value=0,
                    command=self.vc_mode_cb).\
                    grid(row=1, column=1, columnspan=2, padx=3, pady=3, sticky=W)
	Radiobutton(func_frame, text='Separate', variable=self.vc_mode, value=1,
                    command=self.vc_mode_cb).\
                    grid(row=1, column=3, columnspan=2, padx=3, pady=3, sticky=W)

        Label(func_frame, text='Motor State:').grid(row=2, column=0, padx=3, sticky=E)
	self.m_state = IntVar()
	Checkbutton(func_frame, text='Enable', variable=self.m_state,
                    command=self.m_state_cb).\
                    grid(row=2, column=1, columnspan=4, padx=3, pady=3, sticky=W)

        Label(func_frame, text='Laser Control:').grid(row=3, column=0, padx=3, sticky=E)
	Label(func_frame, text='min').grid(row=3, column=1, padx=3, pady=3, sticky=W)
	self.lc_min = Entry(func_frame, width=7)
        self.lc_min.grid(row=3, column=2, padx=3, pady=3)
	Label(func_frame, text='max').grid(row=3, column=3, padx=3, pady=3, sticky=W)
	self.lc_max = Entry(func_frame, width=7)
        self.lc_max.grid(row=3, column=4, padx=3, pady=3)
        self.lc_intensity = IntVar()
        self.lc_intensity.set(0)
	Checkbutton(func_frame, text='Intensity', variable=self.lc_intensity).\
                                grid(row=4, column=1, columnspan=2, padx=3, pady=3, sticky=W)
        Button(func_frame, text='Set', command=self.laser_config_cb).\
                           grid(row=4, column=3, columnspan=2, padx=3, pady=3, sticky=E)


    # callback function - show sonar
    def show_sonar_cb(self):
        try:
            if self.show_sonar.get():
                self.robot.request_device_access(player_sonar_code, access=player_read_mode)
                print 'Sonar sensor is enabled.'
                self.flag_sonar = 1
            else:
                self.flag_sonar = 0
                self.robot.request_device_access(player_sonar_code, access=player_close_mode)
                print 'Sonar sensor is disabled.'
        except PlayerError, (errno, errstr):
            print "[Error %s] %s" % (errno, errstr)

    # callback function - show laser
    def show_laser_cb(self):
        try:
            if self.show_laser.get():
                self.robot.request_device_access(player_laser_code, access=player_read_mode)
                print 'Laser sensor is enabled.'
                self.flag_laser = 1
            else:
                self.flag_laser = 0
                self.robot.request_device_access(player_laser_code, access=player_close_mode)
                print 'Laser sensor is disabled.'
        except PlayerError, (errno, errstr):
            print "[Error %s] %s" % (errno, errstr)

    # callback function - show laserbeacon
    def show_laserbeacon_cb(self):
        try:
            if self.show_laserbeacon.get():
                self.robot.request_device_access(player_laserbeacon_code, access=player_read_mode)
                self.robot.set_laser_config(0, 360, 1)
                print 'Laserbeacon sensor is enabled.'
                self.flag_laserbeacon = 1
            else:
                self.flag_laserbeacon = 0
                self.robot.set_laser_config(0, 360, 0)
                self.robot.request_device_access(player_laserbeacon_code, access=player_close_mode)
                print 'Laserbeacon sensor is disabled.'
        except PlayerError, (errno, errstr):
            print "[Error %s] %s" % (errno, errstr)

    # callback function - show position
    def show_position_cb(self):
        try:
            if self.show_position.get():
                self.robot.request_device_access(player_position_code, access=player_all_mode)
                print 'Position sensor is enabled.'
                self.flag_position = 1
            else:
                self.flag_position = 0
                self.robot.request_device_access(player_position_code, access=player_close_mode)
                print 'Position sensor is disabled.'
        except PlayerError, (errno, errstr):
            print "[Error %s] %s" % (errno, errstr)

    # callback function - show vision
    def show_vision_cb(self):
        try:
            if self.show_vision.get():
                self.robot.request_device_access(player_vision_code, access=player_read_mode)
                print 'Vision sensor is enabled.'
                self.flag_vision = 1
            else:
                self.flag_vision = 0
                self.robot.request_device_access(player_vision_code, access=player_close_mode)
                print 'Vision sensor is disabled.'
        except PlayerError, (errno, errstr):
            print "[Error %s] %s" % (errno, errstr)
            
    # callback function - show ptz
    def show_ptz_cb(self):
        try:
            if self.show_ptz.get():
                self.robot.request_device_access(player_ptz_code, access=player_all_mode)
                print 'PTZ sensor is enabled.'
                self.flag_ptz = 1
            else:
                self.flag_ptz = 0
                self.robot.request_device_access(player_ptz_code, access=player_close_mode)
                print 'PTZ sensor is disabled.'
        except PlayerError, (errno, errstr):
            print "[Error %s] %s" % (errno, errstr)
            
    # callback function - show misc
    def show_misc_cb(self):
        try:
            if self.show_misc.get():
                self.robot.request_device_access(player_misc_code, access=player_read_mode)
                print 'Misc sensor is enabled.'
                self.flag_misc = 1
            else:
                self.flag_misc = 0
                self.robot.request_device_access(player_misc_code, access=player_close_mode)
                print 'Misc sensor is disabled.'
        except PlayerError, (errno, errstr):
            print "[Error %s] %s" % (errno, errstr)

    # callback function - scale speed
    def scale_speed_cb(self, value):
        if self.flag_position:
            self.robot.newposition[0].speed = int(value)

    # callback function - scale turnrate
    def scale_turnrate_cb(self, value):
        if self.flag_position:
            self.robot.newposition[0].turnrate = int(value)

    # callback function - scale pan
    def scale_pan_cb(self, value):
        if self.flag_ptz:
            self.robot.newptz[0].pan = int(value)

    # callback function - scale tilt
    def scale_tilt_cb(self, value):
        if self.flag_ptz:
            self.robot.newptz[0].tilt = int(value)

    # callback function - scale zoom
    def scale_zoom_cb(self, value):
        if self.flag_ptz:
            self.robot.newptz[0].zoom = int(value)

    # callback function - velocity control mode
    def vc_mode_cb(self):
        try:
            if self.vc_mode.get():      # separate mode
                self.robot.change_velocity_control(player_separate_mode)
                print "Velocity control: 'separate mode' is selected."
            else:                       # direct mode (default)
                self.robot.change_velocity_control(player_direct_mode)
                print "Velocity control: 'direct' mode' is selected."
        except PlayerError, (errno, errstr):
            print "[Error %s] %s" % (errno, errstr)
            
    # callback function - motor state
    def m_state_cb(self):
        try:
            if self.m_state.get():      # enabled
                self.robot.change_motor_state(self.m_state.get())
                print "Motor state: enabled."
            else:                       # disabled
                self.robot.change_motor_state(self.m_state.get())
                print "Motor state: disabled."
        except PlayerError, (errno, errstr):
            print "[Error %s] %s" % (errno, errstr)

    # callback function - laser config
    def laser_config_cb(self):
        if self.lc_min.get() == '' or self.lc_max.get() == '':
            print 'Laser config: [Error] missing data.'
        else:
            min = int(self.lc_min.get())
            max = int(self.lc_max.get())
            intensity = self.lc_intensity.get()
            try:
                self.robot.set_laser_config(min, max, intensity)
                print 'Laser config: min(%d), max(%d), intensity(%d)' % (min, max, intensity)
            except PlayerError, (errno, errstr):
                print '[Error %s] %s' % (errno, errstr)


    # thread - update method
    def update(self):
        while not thread_quit:
            try:
                # update robot info.
#                print 'T'
                self.robot.write()      # send commands
                self.robot.read()       # update sensory information

                # Laser
                if self.flag_laser:
                    # clear old data
                    for item in self.laser_list:
                        self.robotwin.delete(item)
                    del self.laser_list[:]
                    # draw laser
                    for i in range(player_num_laser_samples-1):
                        froma = pi/360*i
                        toa = pi/360*(i+1)
                        lc = 250 + self.robot.laser[0][i]/35 * cos(froma),\
                             250 - self.robot.laser[0][i]/35 * sin(froma),\
                             250 + self.robot.laser[0][i+1]/35 * cos(toa),\
                             250 - self.robot.laser[0][i+1]/35 * sin(toa)
                        item = self.robotwin.create_line(lc, fill='yellow')
                        self.laser_list.append(item)
                else:
                    # clear old data
                    for item in self.laser_list:
                        self.robotwin.delete(item)
                    del self.laser_list[:]

                # Sonar
                if self.flag_sonar:
                    # clear old data
                    for item in self.sonar_list:
                        self.robotwin.delete(item)
                    del self.sonar_list[:]
                    # draw front sonar
                    for i in range(8):
                        lc = 250, 240,\
                             250 + self.robot.sonar[0][i]/35 * cos(self.sonar_angle[i]),\
                             240 - self.robot.sonar[0][i]/35 * sin(self.sonar_angle[i])
                        item = self.robotwin.create_line(lc, fill='green')
                        self.sonar_list.append(item)
                    # draw rear sonar
                    for i in range(8, 16):
                        lc = 250, 260,\
                             250 + self.robot.sonar[0][i]/35 * cos(self.sonar_angle[i]),\
                             260 - self.robot.sonar[0][i]/35 * sin(self.sonar_angle[i])
                        item = self.robotwin.create_line(lc, fill='green')
                        self.sonar_list.append(item)
                else:
                    # clear old data
                    for item in self.sonar_list:
                        self.robotwin.delete(item)
                    del self.sonar_list[:]

                # draw a robot
                if self.flag_sonar or self.flag_laser:
                    self.robotwin.tkraise(self.robot_rect)

                # Laserbeacon
                if self.flag_laserbeacon:
                    # clear old data
                    for item in self.laserbeacon_list:
                        self.robotwin.delete(item)
                    del self.laserbeacon_list[:]
                    # draw laserbeacons
                    if len(self.robot.laserbeacon[0]) > 0:
                        drawn = []
                        for lbeacon in self.robot.laserbeacon[0]:
                            if drawn.count(lbeacon.id) == 0:
                                angle = pi/180*(lbeacon.bearing+90)
                                lt = 250 + lbeacon.range/10 * cos(angle),\
                                     250 - lbeacon.range/10 * sin(angle)
                                lb = lt[0]-15, lt[1]-8, lt[0]+15, lt[1]+8
                                item_bg = self.robotwin.create_rectangle(lb, fill='white')
                                item_text = self.robotwin.create_text(lt, text=hex(lbeacon.id)[2:])
                                self.laserbeacon_list.append(item_bg)
                                self.laserbeacon_list.append(item_text)
                                drawn.append(lbeacon.id)
                else:
                    # clear old data
                    for item in self.laserbeacon_list:
                        self.robotwin.delete(item)
                    del self.laserbeacon_list[:]


                # Position
                if self.flag_position:
                    self.lbl_position.config(text='(%d,%d)' %\
                                  (self.robot.position[0].xpos, self.robot.position[0].ypos))
                    self.lbl_theta.config(text=self.robot.position[0].theta)
                    self.lbl_compass.config(text=self.robot.position[0].compass)
                    self.lbl_stall.config(text=self.robot.position[0].stalls)
                    self.lbl_speed.config(text=self.robot.position[0].speed)
                    self.lbl_turnrate.config(text=self.robot.position[0].turnrate)

                # Vision
                if self.flag_vision:
                    # clear screen
                    for item in self.blob_list:
                        self.visionwin.delete(item)
                    del self.blob_list[:]
                    # draw blobs
                    for i in range(len(self.robot.vision[0])):
                        if len(self.robot.vision[0][i]) > 0:
                            for blob in self.robot.vision[0][i]:
                                bb = blob.left, blob.top, blob.right, blob.bottom
                                item = self.visionwin.create_rectangle(bb,
                                                                       fill=self.blob_colors[i%6])
                                self.blob_list.append(item)
                else:
                    # clear screen
                    for item in self.blob_list:
                        self.visionwin.delete(item)
                    del self.blob_list[:]

                # PTZ
                if self.flag_ptz:
                    self.lbl_pan.config(text=self.robot.ptz[0].pan)
                    self.lbl_tilt.config(text=self.robot.ptz[0].tilt)
                    self.lbl_zoom.config(text=self.robot.ptz[0].zoom)

                # Misc
                if self.flag_misc:
                    self.lbl_frontbumper.config(text=self.robot.misc[0].frontbumpers)
                    self.lbl_rearbumper.config(text=self.robot.misc[0].rearbumpers)
                    self.lbl_voltage.config(text='%2.1f' % (self.robot.misc[0].voltage/10))
                
            except PlayerError, (errno, errstr):
                print "[Error %s] %s" % (errno, errstr)
            except KeyError, (errno, errstr):
                print "[Error %s] %s" % (errno, errstr)
        

    # parse command-line arguments
    def parse_arguments(self):
        "parse command-line arguments"
        index = 1
        while index < len(sys.argv):
            if sys.argv[index] == '-h':     # set host
                index = index + 1
                if index < len(sys.argv):
                    self.robot.host = sys.argv[index]
                    index = index + 1
                else:
                    print "[Error] '-h' requires a following host name"
                    sys.exit()
            elif sys.argv[index] == '-p':   # set port number
                index = index + 1
                if index < len(sys.argv):
                    self.robot.port = int(sys.argv[index])
                    index = index + 1
                else:
                    print "[Error] '-p' requires a following port number"
                    sys.exit()
            else:
                print "[Error] invalid argument (%s)" % sys.argv[index]
                sys.exit()


# main script --------------------------------------

# build UI
root = Tk()
viewer = Viewer(root)

# make a thread to update info.
thread_quit = 0
update_thread = Thread(target=viewer.update)
update_thread.start()

# event loop
root.mainloop()

# quit
thread_quit = 1
viewer.robot.disconnect()
sys.exit()
