/*  Player - One Hell of a Robot Server
 *  Copyright (C) 2000
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
 *
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

#ifndef RFLEX_COMMANDS_H
#define RFLEX_COMMANDS_H

  int rflex_open_connection(char *dev_name, int *fd);
  int rflex_close_connection(int *fd);

  void rflex_sonars_on(int fd);
  void rflex_sonars_off(int fd);

  void rflex_ir_on(int fd);
  void rflex_ir_off(int fd);

  void rflex_brake_on(int fd);
  void rflex_brake_off(int fd);

  void rflex_odometry_off( int fd );
  void rflex_odometry_on( int fd, long period );

  void rflex_motion_set_defaults(int fd);

  void rflex_initialize(int fd, int trans_acceleration,
			       int rot_acceleration,
			       int trans_pos,
			       int rot_pos);

  void rflex_update_status(int fd, int *distance, 
				  int *bearing, int *t_vel,
				  int *r_vel);

  void rflex_update_system(int fd, int *battery,
				   int *brake);

  int rflex_update_sonar(int fd, int num_sonars,
				 int *ranges);
  void rflex_update_bumpers(int fd, int num_bumpers,
				   char *values);
  void rflex_update_ir(int fd, int num_irs,
			     unsigned char *ranges);

  void rflex_set_velocity(int fd, long t_vel, long r_vel, 
				 long acceleration);
  void rflex_stop_robot(int fd, int deceleration);

//int clear_incoming_data(int fd);

#endif
