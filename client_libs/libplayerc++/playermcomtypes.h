/*  $Id$
 *
 *  Player - One Hell of a Robot Server
 *
 *  Copyright (C) 2000, 2001, 2002, 2003  Brian Gerkey   &  Kasper Stoy
 *                      gerkey@usc.edu    kaspers@robotics.usc.edu
 *  MCom device by Matthew Brewer <mbrewer@andrew.cmu.edu> and 
 *  Reed Hedges <reed@zerohour.net> at the Laboratory for Perceptual 
 *  Robotics, Dept. of Computer Science, University of Massachusetts,
 *  Amherst.
 *
 *  This program is free software; you can redistribute it and/or modify
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

#ifndef _PLAYER_MCOM_TYPES_HH_
#define _PLAYER_MCOM_TYPES_HH_

#include <stdint.h>


/** @file playermcomtypes.h
 *  Constant and type definitions that are useful with the MCom device,
 *  but are optional.
 *  You must include <playerclient.h> or <player.h> before including this 
 *  file.
 *  If you would have things that you think would be useful to other people,
 *  send them to Reed Hedges <reed@zerohour.net> or to the Player developers
 *  mailing list for inclusion in this header file.
 */

/** Channels */
//@{
#define MCOM_CHANNEL_PATHPLAN   "Path"      ///< Path planner
#define MCOM_CHANNEL_TELEOP     "Tele"      ///< Teleoperation tools
#define MCOM_CHANNEL_FEEDBACK   "OperMsg"   ///< User feedback
#define MCOM_CHANNEL_LOCALIZE   "Loca"      ///< Localizer
//@}


/** Message types (See data structures below) */
enum MComMessageType {
    MCOM_MSG_NULL,
    MCOM_MSG_STRING,        ///< Just a string
    MCOM_MSG_VELOCITY,      ///< Set velocities
    MCOM_MSG_POSITION,      ///< Set goal position and orientation
    MCOM_MSG_HEADING,       ///< Set goal heading
    MCOM_MSG_FEEDBACK,      ///< Send user feedback

    /** define your own types relative to this offset (but remember to 
     *  recompile all your clients if this file changes!)
     */
    MCOM_MSG_first_available    
};


/** Data structures (See message types above).
 *  You should probably use the htons, htonl, ntohs and ntohl functions to 
 *  convert multibyte numeric values to and from network byte order, MComProxy
 *  won't do this: it only knows about strings (ie, MComMessage.command).
 */

typedef union {

    /** This is the one you pass to MComProxy functions, or use with 
        MCOM_MSG_STRING
    */
    char command[MCOM_DATA_LEN];

    /// Use with MCOM_MSG_POSITION
    struct {
        int32_t x; 
        int32_t y; 
        uint16_t theta; 
        bool clear;
    } __attribute__ ((packed))  goal;


    /// MCOM_MSG_VELOCITY
    struct {
        int32_t translation;
        int32_t rotation;
        int32_t secondaryTranslation;
        int32_t secondaryRotation;
        int32_t tertiaryTranslation;
        int32_t tertiaryRotation;
    } __attribute__ ((packed)) velocity;

    /// MCOM_MSG_HEADING
    struct {
        int32_t theta;
        bool isRelative; 
        bool goForward;
    } __attribute__ ((packed)) heading;

    /// MCOM_MSG_FEEDBACK
    struct {
        char channel[MCOM_CHANNEL_LEN];
        uint32_t code;
        char message[MCOM_DATA_LEN - MCOM_CHANNEL_LEN - 4];
    } __attribute__ ((packed)) feedback;

} __attribute__ ((packed)) MComMessage;



/** Codes to use with Feedback (See feedback struct above) */
enum MComFeedbackCode {
    MCOM_FB_NULL,
    MCOM_FB_POSTED_GOAL,
    MCOM_FB_REACHED_GOAL,
    MCOM_FB_INVALID_LOCATION,
    MCOM_FB_DIFFICULT_OBSTACLES,
    MCOM_FB_MOTORS_STALLED,
    MCOM_FB_CLIENT_ENABLED,
    MCOM_FB_CLIENT_DISABLED,
    MCOM_FB_first_available
};


#endif 

