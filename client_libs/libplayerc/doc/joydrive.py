
# Desc: A simply joystick controller
# Author: Andrew Howard

import math
from playerc import *


def main(host, port):

    # Connect to the server
    client = playerc_client(None, host, port)
    if client.connect() != 0:
        raise playerc_error_str()

    # Open a joystick device
    joystick = playerc_joystick(client, 0)
    if joystick.subscribe(PLAYERC_READ_MODE) != 0:
        raise playerc_error_str()

    # Open a position device
    position = playerc_position(client, 0)
    if position.subscribe(PLAYER_ALL_MODE) != 0:
        raise playerc_error_str()

    # Enable the motors
    position.enable(1)

    while 1:

        # Read data from the server
        pid = client.read()

        # Print out current position
        if pid == position.info.id:
            print '%+.3f %+.3f' % (position.px, position.py)

        # Send velocity commands to the robot
        if pid == client.id:
            vel_x = -joystick.py * 0.50
            vel_yaw = -joystick.px * math.pi / 4
            position.set_cmd_vel(vel_x, 0, vel_yaw, 1)
            
    return




if __name__ == '__main__':

    main('localhost', 6665)
