#!/usr/bin/env python


from libplayerc import *



def test():

    c = playerc_client_create(None, 'localhost', 6665)
    if playerc_client_connect(c) != 0:
        raise playerc_error_str()

    pos = playerc_position_create(c, 0)

    if playerc_position_subscribe(pos, PLAYERC_READ_MODE) != 0:
        raise playerc_error_str()    

    while 1:

        proxy = playerc_client_read(c)

        print pos.pose
        print pos.px, pos.py, pos.pa
    
    return




def test_laser(client):

    laser = playerc_laser_create(c, 0)

    if playerc_laser_subscribe(laser, PLAYERC_READ_MODE) != 0:
        raise playerc_error_str()    

    while 1:
        proxy = playerc_client_read(c)
        print laser.scan
    
    return



if __name__ == '__main__':

    print PLAYERC_READ_MODE

    c = playerc_client_create(None, 'localhost', 6665)
    if playerc_client_connect(c) != 0:
        raise playerc_error_str()

    test_laser(c)
