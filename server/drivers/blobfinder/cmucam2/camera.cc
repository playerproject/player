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
 *   returns the color blob data gathered from the camera.
 */

#include "camera.h"

Camera::Camera()
{
}

/**************************************************************************
			       *** RESET CAMERA ***
**************************************************************************/
/* Description: This function resets the camera. The servo positions,
                imager configurations, and etc. will all be set to default. 
   Parameters:  fd: serial port file descriptor
   Returns:     1: if command was successfully sent to the camera
                0: otherwise
*/
int Camera::reset_camera(int fd)
{
  // To be written.
  return 1;
}

/**************************************************************************
			       *** CAMERA POWER  ***
**************************************************************************/
/* Description: This function toggles the camera's module power. This would
                be used in situations where battery life needs to be extended,
                while the camera is not actively processing image data.
   Parameters:  fd: serial port file descriptor
                on: a value of 0 puts the camera module into a power down
                    a value of 1 turns the camera back on while maintaining  
                    the current camera registers values.
   Returns:     1: if command was successfully sent to the camera
                0: otherwise
*/
int Camera::power(int fd, int on)
{
  if(on)
     return write_check(fd, "CP 1\r");
  return write_check(fd, "CP 0\r");
}

/**************************************************************************
			                      *** SET IMAGER CONFIG ***
**************************************************************************/
/* Description: This function sets the camera's internal resiger values
                for controlling image qualities.
   Parameters:  fd: serial port file descriptor
                imager_config: the packet containing camera's internal register values:
                contrast, brightness, color mode, Exposure
   Returns:     1: If the command was successfully sent to the camera
                0: Otherwise
*/

int Camera::set_imager_config(int fd, imager_config ic)
{
   int value[8], size = 0;                    // The numbers used in the command: 
   char command[26];                          // ex. CR 5 255 19 33
   if(ic.contrast != -1)                      // If ther is a change set the values
   {
      value[size++] = CONTRAST;
      value[size++] = ic.contrast;
   }
   if(ic.brightness != -1)
   {
      value[size++] = BRIGHTNESS;
      value[size++] = ic.brightness;
   }
   if(ic.colormode != -1)
   {
      value[size++] = COLORMODE;
      if(ic.colormode == 0)
      	value[size++] = RGB_AWT_OFF;
      if(ic.colormode == 1)
      	value[size++] = RGB_AWT_ON;
      if(ic.colormode == 2)
      	value[size++] = YCRCB_AWT_OFF;
      if(ic.colormode == 3)
      	value[size++] = YCRCB_AWT_ON;
   }
   if(ic.autogain != -1)
   {
      value[size++] = AUTOGAIN;
      if(ic.autogain == 0)
      	value[size++] = AUTOGAIN_OFF;
      if(ic.autogain == 1)
      	value[size++] = AUTOGAIN_ON;
   }
   make_command("CR ", value, size, command);  // Put the values into camera's command format:
                                               // ex. CR 6 105 18 44
   return write_check(fd, command);            // send the command to the camera
}

/**************************************************************************
			                      *** GET T PACKET ***
**************************************************************************/
/* Description: This function puts the camera's output during tracking into
                a T packet, which contrains information about the blob.
   Parameters:  fd: serial port file descriptor
                tpacket: the packet that will contain the blob info
   Returns:     void
*/
void Camera::get_t_packet(int fd, packet_t *tpacket)
{
  char tpack_chars[T_PACKET_LENGTH];
  read_t_packet(fd, tpack_chars);                   // read the output of the camera
  set_t_packet(tpacket, tpack_chars);               // convert it into T packet
}

/**************************************************************************
			                      *** POLL MODE ***
**************************************************************************/
/* Description: This functions determines whether the camera should send a
                continuous stream of packets or just one packet.
   Parameters:  fd: serial port file descriptor
                on: if on == 1, only one packet is send
                    if on == 0, a continuous stream of packets is send
   Returns:     1: If the command was successfully sent to the camera
                0: Otherwise
*/
int Camera::poll_mode(int fd, int on)
{
  if(on)
    return write_check(fd, "PM 1\r");
  else
    return write_check(fd, "PM 0\r");
}

/**************************************************************************
			                      *** FIND BLOB ***
**************************************************************************/
/* Description: This functions makes the camera look around for a blob.
                At each position, the camera finds a blob with certain
                0 <= confidence <= 255. Depending on the value of MIN_CONFIDENCE
                we report where there was a blob or not.
   Parameters:  fd: serial port file descriptor
                cc: the color range of the blob to look for
   Returns:     if confidence > MIN_CONFIDENCE: the servo angle at the max confidence
                otherwise: -1, indicating the blob was not found
*/
/*int Camera::find_blob(int fd, color_config cc)
{
  int i;
  int max_conf = 0;
  int servo_pos = 0;
  clock_t end, start;
  packet_t blob_info;
  poll_mode(fd, 1);                                 // make the camera send only one packet
                                                    // not a continuous stream

  // start looking around from one side to the other                                                                                                                                                
  for(i = MIN_PAN_ANGLE; i <= MAX_PAN_ANGLE; i += ANGLE_INCEREMENT)
  {
     set_servo_position(fd, 0, i);                  // put the servo at the specified angle
     end = DELAY*CLOCKS_PER_SEC;                    // wait for DELAY number of seconds
     start = clock();                               // while the camera is adjusting itself
     while(clock() - start < end);
     track_blob(fd, cc);                            // start tracking
     get_t_packet(fd, &blob_info);                  // get the blob info
     if(blob_info.confidence > max_conf)            // get the hight confidence and remember
     {                                              // the position at which the servo was at
	     max_conf = blob_info.confidence;
	     servo_pos = i;
     }
  }
  printf("max conf: %d\n", max_conf);
  if(max_conf > MIN_CONFIDENCE)                     // if the maximum confidence is acceptable
  {                                                 // ie. it is higher than the minimum allowed,
    poll_mode(fd,0);                                // put the camera in its default 'continuous
    printf("found blob at angle: %d\n", servo_pos); // stream of packets and return the servo
    return servo_pos;                               // position at which the confidence was max.
  }
  else
  {
    printf("did not find the blob with high confidence!\n");
    return -1;
  }
}*/
   
/**************************************************************************
			                      *** SET SERVO POSITION ***
**************************************************************************/
/* Description: This functions sets the servo position given the servo number
                and the angle (note: angle = 0 denotes servo position = 128
                in terms of camera's values)
   Parameters:  fd: serial port file descriptor
                servo_num: the servo which we are setting the position
                I am using 0:pan  1:tilt
   Returns:     1: If the command was successfully sent to the camera
                0: Otherwise
*/
int Camera::set_servo_position(int fd, int servo_num, int angle)
{
   int position = ZERO_POSITION + angle;                      // change the angle into camera's format
   char comm[10];                                             // for servo position. I am using angle 0
   int value[] = {servo_num, position};                       // corresponding to the default servo pos. 128
   make_command("SV ", value, sizeof(value)/INT_SIZE, comm);  // generate the command using the values
   printf("servo %d new position: %d\n", servo_num, angle);
   return write_check(fd, comm);                              // write the command to the camera
}

/**************************************************************************
			                      *** MAKE COMMAND ***
**************************************************************************/
/* Description: This function gets a sets of values and a camera command header
                to generate the command for the camera.
   Parameters:  cmd: the command header, for example SF or CR (see CMUcam 2 user guide)
                n: the set of values to be used in the command
                size: the number of values used
                full_command: the final command in characters to be send to the camera
   Returns:     void
*/
void Camera::make_command(char *cmd, int *n, size_t size, char *full_command)
{
  char value[3];                            // the values are all withing 3 digits
  int length, i;
  for(i = 0; i < 10; i++)                   // set all to null so that if there are
    full_command[i] = '\0';                 // unsed characters at the end, the camera
                                            // does not complain about the command.
                                            // there is probably a better way to do this!
  strcat(full_command, cmd);                // attach the command header, ex. SF
  for(i = 0; i < (int)size; i++)                 // for all the values, convert them into char
  {                                         // and attach them to the end of the command
    length = sprintf(value, "%d", n[i]);    // plus a space
    strcat(full_command, value);
    strcat(full_command, " ");
  }
  strcat(full_command, "\r");               // attach the return character to the end
}

/**************************************************************************
			                      *** OPEN PORT ***
**************************************************************************/
/* Description: This function opens the serial port for communication with the camera.
   Parameters:  NONE
   Returns:     the file descriptor
*/
int Camera::open_port()
{
   int fd = open( SERIALPORT, O_RDWR );             // open the serial port
   struct termios term;
 
   if( tcgetattr( fd, &term ) < 0 )                 // get device attributes
   {
        puts( "unable to get device attributes");
        return -1;
   }
  
   cfsetispeed( &term, B115200 );                   // set baudrate to 115200
   cfsetospeed( &term, B115200 );                          
  
   if( tcsetattr( fd, TCSAFLUSH, &term ) < 0 )
   {
        puts( "unable to set device attributes");
        return -1;
   }
   return fd;
}

/**************************************************************************
			                      *** CLOSE PORT ***
**************************************************************************/
/* Description: This function closes the serial port.
   Parameters:  NONE
   Returns:     void
*/
void Camera::close_port(int fd)
{               
   close(fd);
}

/**************************************************************************
			                      *** WRITE CHECK ***
**************************************************************************/
/* Description: This function writes a command to the camera and checks if the
                write was done successfully by checking camera's response.
   Parameters:  fd: serial port file descriptor
                msg: the command to be send to the camera
   Returns:     1: if the write was successful
                0: otherwise
*/
int Camera::write_check(int fd, char *msg)
{
  write(fd, msg, strlen(msg));                    // write the command to the camera
  char respond[5];                                // camera's resond to written command: ACK or NCK
  get_bytes(fd, respond, sizeof(respond));
  if(strchr(respond, 'N') != NULL)                // If NCK is returned, there was an error in writing
  {
     printf("Error writing to the port!\n");
     return 0;
  }
  return 1;
}

/**************************************************************************
			                      *** GET FRAME ***
**************************************************************************/
/* Description: This function gets a frame from the camera
   Parameters:  fd: serial port file descriptor
   Returns:     the image generated from the camera's frame output
*/
image Camera::get_frame(int fd)
{
   // if Send Frame command is not successful, do not continue
   image cam_img;
   if(write_check(fd, "SF\r"))
   {
     char msg[2];
     get_bytes(fd, msg, 2);                              // get the width and height of the image
     cam_img.width  = (unsigned char)msg[0];
     cam_img.height = (unsigned char)msg[1];
     printf("width: %d   height: %d\n", cam_img.width, cam_img.height);

     // find out out many characters are in the whole packet (see the CMUcam2 user guide for
     // the format of this packet
     int bytes = (cam_img.width)*(cam_img.height)*3 + (cam_img.height) + 1;
     char frame[bytes];

     printf("Grabing Image...\n");                      
     int bytes_read = get_bytes(fd, frame, bytes);       // get the frame outputed by the camera
   
     if(bytes_read < bytes)                              // if the read bytes where less than expected
     {                                                   // there was an error in reading the frame
       printf("Unable to get frame!\n");
       return cam_img;
     }                                                   // CMUcam2 indicates end of frame by a 3.
     else if((unsigned char)frame[bytes-1] != 3)         // if it doesn't exist and the end, the
     {                                                   // frame was not transmitted correctly.
       printf("Camera frame is corrupt!\n");
       return cam_img;
     }
      
     char *pointer;                                      // allocating space for the 2D array of
     rgb  *array_contig;                                 // pixels according to the width and height we obtained.
     pointer = (char*) malloc( (cam_img.width)*sizeof(rgb*) + (cam_img.width)*(cam_img.height)*sizeof(rgb));
     array_contig = (rgb*)(pointer + (cam_img.width) * sizeof (rgb*));
     cam_img.pixel = (rgb**)pointer;
     int i;
     for (i = 0; i < cam_img.width; ++i)
       cam_img.pixel[i] = array_contig + i * cam_img.height;

     // setting pixels' rgb colours
     int x = 0, y = 0, n;
     i = 0;
     while(i < bytes)
     {
       n = (unsigned char)frame[i++];
       if(n == 2)                                        // 2 indicates new column
       {
	  y++;
	  x = 0;
          continue;
       }
       cam_img.pixel[x][y].red = n;
       cam_img.pixel[x][y].green = (unsigned char)frame[i++];
       cam_img.pixel[x][y].blue = (unsigned char)frame[i++];
       x++;
     }
   } 
   return cam_img;
}

/**************************************************************************
			                      *** DISPLAY FRAME ***
**************************************************************************/
/* Description: This function displays the frame obtained from the camera
   Parameters:  window_id: the graphical window id used by g2 graphics
                cam_img:   the image obtained by get_frame()
   Returns:     the image generated from the camera's frame output
*/
/*void display_frame(int *window_id, image cam_img, int ambient_light)                
{
    *window_id = g2_open_X11(cam_img.width, cam_img.height);
    printf("Displaying Image...\n");
    int color, x, y;
    double r,g,b;

    g2_clear(*window_id);                                 // Erase the previous image
    g2_pen(*window_id, 12);

    double dev = (double)(MAX_RGB - MIN_RGB);
    for(x = 0; x < cam_img.width; x++)
    {
      for(y = 0; y < cam_img.height; y++)
      {
        // Camera's rgb is from MIN_RGB to MAX_RGB. g2 graphics
        // requires rgb values to be from 0 to 1, so we need to adjust them.
	r = (cam_img.pixel[x][y].red - 16 + ambient_light)/dev;
	g = (cam_img.pixel[x][y].green - 16 + ambient_light)/dev;
	b = (cam_img.pixel[x][y].blue - 16 + ambient_light)/dev;
	color = g2_ink(*window_id, r, g, b);
        g2_pen(*window_id, color);
	g2_plot(*window_id, x, y);
      }
    }
}*/

int Camera::get_bytes(int fd, char *buf, size_t len)
{
  int bytes_read = 0, ret_val;
  while(bytes_read < (int)len)
  {
    ret_val = read(fd, buf+bytes_read, len-bytes_read);
    if(ret_val < 0)
    {
      perror("Not enough bytes to read!\n");
      return 0;
    }
    else if(ret_val > 0)
      bytes_read += ret_val;
  }
  return bytes_read;
}
    
int Camera::get_servo_position(int fd, int servo_num)
{
   char c = 0;
   if(servo_num)       // set position of servo 1
	   write(fd, "GS 1\r", 5);
   else		             // set position of servo 0
	   write(fd, "GS 0\r", 5);

   int i, servo_position;
   char number[3];
   c = 0;
   for(i = 0; i < 4; i++)

	    read(fd, &c, 1);
   printf("pos: ");
   for(i = 0; 1; i++)
   {
	   read(fd, &c, 1);
     putchar(c);
     if(c == '\r')
	   {
        printf("\n");
       break;
     }
     number[i] = c;
   }
   servo_position = atoi(number);
   return (servo_position - ZERO_POSITION);
}

/* This functions starts to Track a Color. It takes in the minimum and 
   maximum RGB values and outputs a type T packet. This packet by defualt
   returns the middle mass x and y coordinates, the bounding box, the 
   number of pixles tracked, and a confidence values. 
*/
int Camera::track_blob( int fd, color_config cc)
{
   // Enabling Auto Servoing Mode for both servos             
   //if(!write_check(fd, "SM 15\r"))
   //return 0;

   char cmd[28];
   int value[] = {cc.rmin, cc.rmax, cc.gmin, cc.gmax, cc.bmin, cc.bmax};
   make_command("TC ", value, sizeof(value)/INT_SIZE, cmd); 
   return write_check(fd, cmd);
}

void Camera::stop_tracking(int fd)
{
  write(fd, "\r", 1);
  char c = 0;
  while(c != ':')
       read(fd, &c, 1);

}
  

void Camera::read_t_packet(int fd, char *tpack_chars)
{
   char c = 0;
   int k = 0;
   while(1)
   {
      read( fd, &c, 1 );
      tpack_chars[k++] = c;
      if(c == '\r')
	 break;
   }
}
  


// Extracts the data for type T packet from camera output.
void Camera::set_t_packet( packet_t *tpacket, char output[] )
{
   int k = 0, j = 0, i = 0;
   char substring[] = "    ";
   int m[8];

   for(i = 1; i < T_PACKET_LENGTH; i++)
   {
      if( output[i] == ' ' || output[i] == '\r' )
      {
         substring[3] = ' ';
         m[k] = atoi(substring);
         k++;
         substring[0]= ' ';
         substring[1]= ' ';
         substring[2]= ' ';
         j = 0;
      }
      else
      {
      	 substring[j] = output[i];
         j++;
      }
   }

   (*tpacket).middle_x = m[0];
   (*tpacket).middle_y = m[1];
   (*tpacket).left_x = m[2];
   (*tpacket).left_y = m[3];
   (*tpacket).right_x = m[4];
   (*tpacket).right_y = m[5];
   (*tpacket).blob_area = m[6];
   (*tpacket).confidence = m[7];
}
