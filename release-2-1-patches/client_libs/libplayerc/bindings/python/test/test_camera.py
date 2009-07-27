
# Desc: Test the camera
# Author: Andrew Howard
# Date: 15 Sep 2004
# CVS: $Id$

from playerc import *


def test_camera(client, index, context):
    """Basic test of the camera interface."""

    camera = playerc_camera(client, index)
    if camera.subscribe(PLAYERC_OPEN_MODE) != 0:
        raise playerc_error_str()    

    for i in range(10):

        while 1:
            id = client.read()
            if id == camera.info.id:
                break

        if context:
            print context,            
        print "camera: [%14.3f] [%d %d %d %d]" % \
              (camera.info.datatime, camera.width, camera.height,
               camera.depth, camera.image_size),
        print

        # Save the image
        filename = 'camera_%03d.ppm' % i
        print 'camera: saving [%s] (only works for RGB888)' % filename
        test_camera_save(camera, filename);

    camera.unsubscribe()    
    return




def test_camera_save(camera, filename):
    """Save a camera image. Assumes the image is RGB888"""

    file = open(filename, 'w+');
    assert(file)

    # Write ppm header
    file.write('P6\n%d %d\n%d\n' % (camera.width, camera.height, 255))

    # TODO: ?
    # Write image data
    file.write(camera.image)

    return
