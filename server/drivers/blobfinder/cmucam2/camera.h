/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 *   $Id$
 *   by: Richard Vaughan, Pouya Bastani      2004/05/1
 *   the CMUcam2 vision device. It takes a color range in RGB and
 *   returns the color blob data gathered from C,
 *   which this device spawns and then talks to.
 */

#ifndef CAMERA_H
#define CAMERA_H

#include <fcntl.h>
#include <termios.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
//#include <g2.h>
//#include <g2_X11.h>
                                
/**************************************************************************
			    *** CONSTANST ***
**************************************************************************/
#define SERIALPORT       "/dev/ttyS0"   // serial port device address
#define IMAGE_WIDTH      87             // the width of the image the camera sends
#define IMAGE_HEIGHT     143            // the height of the image the camera sends
#define CONTRAST         5              // camera's contrast register #
#define BRIGHTNESS       6		// camera's brightness register #
#define COLORMODE        18		// camera's colormode register #
#define RGB_AWT_ON       44             // camera's RGB auto white balance on
#define RGB_AWT_OFF      40             // camera'sRGB auto white balance off
#define YCRCB_AWT_ON     36             // camera'sYCrCb auto white balance on
#define YCRCB_AWT_OFF    32             // camera'sYCrCb auto white balance off
#define AUTOGAIN         19		          // camera's autogain register #
#define AUTOGAIN_ON      33             // camera's autogain on
#define AUTOGAIN_OFF     32             // camera's autogain off                
#define MIN_PAN_ANGLE    -30            // min angle of pan servo
#define MAX_PAN_ANGLE    30             // max angle of pan servo
#define MIN_TILT_ANGLE   -50   	        // min angle of tilt servo
#define MAX_TILT_ANLGE   50    	        // max angle of tilt servo
#define ZERO_POSITION    128            // servos' middle position as defiend by camera
#define ANGLE_INCEREMENT 10             // the angle increment while the camera rotates to find a blob
#define DELAY            0.1            // the time in seconds in which the camera is tracking
#define MIN_RGB          16		          // camera's min rgb value
#define MAX_RGB          240		        // camera's max rgb value
#define T_PACKET_LENGTH  33             // max length of T packet that camera returns
#define INT_SIZE         4              // byte size of int
#define MIN_CONFIDENCE   50             // the minimum confidence that is allowed when finding blobs.
                                        // if the camera does not find a blob with higher confidence
                                        // than MIN_CONFIDENCE, we assume there was no blob found

/**************************************************************************
			                      *** T PACKET ***
**************************************************************************/
typedef struct                          // camera's output packet for tracking blobs
{
   int middle_x, middle_y; 		// the blob entroid (image coords)
   int left_x;				// the left most corner's x value
   int left_y;				// the left msot corner's y value
   int right_x;				// the right most corner's x vlaue
   int right_y;				// the right most corner's y value
   int blob_area;	        	// number of pixles int he tracked regtion,
					// scaled and capped at 255:(pixles+4)/8
   int confidence;			// the (# of pixles/area)*256 of the bounded
					// rectangle and capped at 255
}packet_t; 

/**************************************************************************
			                    *** IMAGER CONFIG ***
**************************************************************************/
typedef struct                           // camera's internal register controlling image quality
{ 
   int brightness;                       // contrast:      -1 = no change.  (0-255)
   int contrast;                         // brightness:    -1 = no change.  (0-255)
   int  colormode;                       // color mode:    -1 = no change.
					                               //		             0  = RGB/auto white balance Off,
                                         //                1  = RGB/AutoWhiteBalance On,
                                         //                2  = YCrCB/AutoWhiteBalance Off, 
                                         //                3  = YCrCb/AWB On)  
  int  autogain;                         // auto gain:     -1 = no change.
		                                     //		   0  = off, 
                               					 //		   1  = on.         
} imager_config;

/**************************************************************************
			                    *** CONFIG CONFIG ***
**************************************************************************/
typedef struct
{ 
  int rmin, rmax;                   // RGB minimum and max values (0-255)
  int gmin, gmax;
  int bmin, bmax;
} color_config;

/**************************************************************************
			                        *** RGB ***
**************************************************************************/
typedef struct                          // RGB values
{
	int red;
	int green;
	int blue;
} rgb;

/**************************************************************************
			    *** CONFIG CONFIG ***
**************************************************************************/
typedef struct                          // camera's image
{
  int width;
  int height;
  rgb **pixel;
} image;

/**************************************************************************
			 *** FUNCTION PROTOTYPES ***
**************************************************************************/

class Camera 
{
  private:

  public:  
    Camera();
    void display_frame(int *window_id, image cam_img, int ambient_light);
    image get_frame(int fd);
    int  write_check(int fd, char *msg);
    int  power(int fd, int on);
    int  reset_camera(int fd);
    void get_t_packet(int fd, packet_t *tpacket);
    int  set_imager_config(int fd, imager_config ic);
    int  get_bytes(int fd, char *buf, size_t len);
    int  open_port();
    void close_port(int fd);
    void read_t_packet(int fd, char *tpackChars);
    void set_t_packet( packet_t *tpacket, char output[] );
    int  set_servo_position(int fd, int servo_num, int angle);
    int  get_servo_position(int fd, int servo_num);
    void stop_tracking(int fd);
    int  find_blob(int fd, color_config cc);
    int  poll_mode(int fd, int on);
    void make_command(char *cmd, int *n, size_t size, char *fullCommand);
    int  track_blob(int fd, color_config cc);
};

#endif
