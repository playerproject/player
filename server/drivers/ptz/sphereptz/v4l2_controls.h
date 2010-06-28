/*******************************************************************************#
#           guvcview              http://guvcview.berlios.de                    #
#                                                                               #
#           Paulo Assis <pj.assis@gmail.com>                                    #
#                                                                               #
# This program is free software; you can redistribute it and/or modify          #
# it under the terms of the GNU General Public License as published by          #
# the Free Software Foundation; either version 2 of the License, or             #
# (at your option) any later version.                                           #
#                                                                               #
# This program is distributed in the hope that it will be useful,               #
# but WITHOUT ANY WARRANTY; without even the implied warranty of                #
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                 #
# GNU General Public License for more details.                                  #
#                                                                               #
# You should have received a copy of the GNU General Public License             #
# along with this program; if not, write to the Free Software                   #
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA     #
#                                                                               #
********************************************************************************/
#ifndef V4L2_CONTROLS_H
#define V4L2_CONTROLS_H

/*
 * Private V4L2 control identifiers from UVC driver.  - this seems to change acording to driver version
 * all other User-class control IDs are defined by V4L2 (videodev2.h)
 */

/*------------------------- new camera class controls ---------------------*/
#define V4L2_CTRL_CLASS_USER_NEW		0x00980000
#define V4L2_CID_BASE_NEW			(V4L2_CTRL_CLASS_USER_NEW | 0x900)
#define V4L2_CID_POWER_LINE_FREQUENCY_NEW	(V4L2_CID_BASE_NEW+24)
#define V4L2_CID_HUE_AUTO_NEW			(V4L2_CID_BASE_NEW+25) 
#define V4L2_CID_WHITE_BALANCE_TEMPERATURE_NEW	(V4L2_CID_BASE_NEW+26) 
#define V4L2_CID_SHARPNESS_NEW			(V4L2_CID_BASE_NEW+27) 
#define V4L2_CID_BACKLIGHT_COMPENSATION_NEW 	(V4L2_CID_BASE_NEW+28)
#define V4L2_CID_LAST_NEW			(V4L2_CID_BASE_NEW+31)

#define V4L2_CTRL_CLASS_CAMERA_NEW 0x009A0000	/* Camera class controls */
#define V4L2_CID_CAMERA_CLASS_BASE_NEW 		(V4L2_CTRL_CLASS_CAMERA_NEW | 0x900)

#define V4L2_CID_EXPOSURE_AUTO_NEW		(V4L2_CID_CAMERA_CLASS_BASE_NEW+1)
#define V4L2_CID_EXPOSURE_ABSOLUTE_NEW		(V4L2_CID_CAMERA_CLASS_BASE_NEW+2)
#define V4L2_CID_EXPOSURE_AUTO_PRIORITY_NEW	(V4L2_CID_CAMERA_CLASS_BASE_NEW+3)

#define V4L2_CID_PAN_RELATIVE_NEW		(V4L2_CID_CAMERA_CLASS_BASE_NEW+4)
#define V4L2_CID_TILT_RELATIVE_NEW		(V4L2_CID_CAMERA_CLASS_BASE_NEW+5)
#define V4L2_CID_PAN_RESET_NEW			(V4L2_CID_CAMERA_CLASS_BASE_NEW+6)
#define V4L2_CID_TILT_RESET_NEW			(V4L2_CID_CAMERA_CLASS_BASE_NEW+7)

#define V4L2_CID_PAN_ABSOLUTE_NEW		(V4L2_CID_CAMERA_CLASS_BASE_NEW+8)
#define V4L2_CID_TILT_ABSOLUTE_NEW		(V4L2_CID_CAMERA_CLASS_BASE_NEW+9)

#define V4L2_CID_FOCUS_ABSOLUTE_NEW		(V4L2_CID_CAMERA_CLASS_BASE_NEW+10)
#define V4L2_CID_FOCUS_RELATIVE_NEW		(V4L2_CID_CAMERA_CLASS_BASE_NEW+11)
#define V4L2_CID_FOCUS_AUTO_NEW			(V4L2_CID_CAMERA_CLASS_BASE_NEW+12)
#define V4L2_CID_CAMERA_CLASS_LAST		(V4L2_CID_CAMERA_CLASS_BASE_NEW+13)

/*--------------- old private class controls ------------------------------*/

#define V4L2_CID_PRIVATE_BASE_OLD		0x08000000
#define V4L2_CID_BACKLIGHT_COMPENSATION_OLD	(V4L2_CID_PRIVATE_BASE_OLD+0)
#define V4L2_CID_POWER_LINE_FREQUENCY_OLD	(V4L2_CID_PRIVATE_BASE_OLD+1)
#define V4L2_CID_SHARPNESS_OLD			(V4L2_CID_PRIVATE_BASE_OLD+2)
#define V4L2_CID_HUE_AUTO_OLD			(V4L2_CID_PRIVATE_BASE_OLD+3)

#define V4L2_CID_FOCUS_AUTO_OLD			(V4L2_CID_PRIVATE_BASE_OLD+4)
#define V4L2_CID_FOCUS_ABSOLUTE_OLD		(V4L2_CID_PRIVATE_BASE_OLD+5)
#define V4L2_CID_FOCUS_RELATIVE_OLD		(V4L2_CID_PRIVATE_BASE_OLD+6)

#define V4L2_CID_PAN_RELATIVE_OLD		(V4L2_CID_PRIVATE_BASE_OLD+7)
#define V4L2_CID_TILT_RELATIVE_OLD		(V4L2_CID_PRIVATE_BASE_OLD+8)
#define V4L2_CID_PANTILT_RESET_OLD		(V4L2_CID_PRIVATE_BASE_OLD+9)

#define V4L2_CID_EXPOSURE_AUTO_OLD		(V4L2_CID_PRIVATE_BASE_OLD+10)
#define V4L2_CID_EXPOSURE_ABSOLUTE_OLD		(V4L2_CID_PRIVATE_BASE_OLD+11)

#define V4L2_CID_WHITE_BALANCE_TEMPERATURE_AUTO_OLD	(V4L2_CID_PRIVATE_BASE_OLD+12)
#define V4L2_CID_WHITE_BALANCE_TEMPERATURE_OLD		(V4L2_CID_PRIVATE_BASE_OLD+13)

#define V4L2_CID_PRIVATE_LAST			(V4L2_CID_WHITE_BALANCE_TEMPERATURE_OLD+1)

#endif
