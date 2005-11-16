/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2005 -
 *     Brian Gerkey
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

/*
 * $Id$
 *
 * Functions for looking up the appropriate XDR pack/unpack function for a
 * given message type and subtype.
 */

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "playerxdr.h"
#include "functiontable.h"

typedef struct
{
  uint16_t interf;
  uint8_t type;
  uint8_t subtype;
  player_pack_fn_t func;
} playerxdr_function_t;

static playerxdr_function_t init_ftable[] =
{
  /* This list is currently alphabetized, please keep it that way! */

  /* actarray messages */
  {PLAYER_ACTARRAY_CODE, PLAYER_MSGTYPE_DATA, PLAYER_ACTARRAY_DATA_STATE,
   (player_pack_fn_t)player_actarray_data_pack},
  {PLAYER_ACTARRAY_CODE, PLAYER_MSGTYPE_REQ, PLAYER_ACTARRAY_POWER_REQ,
   (player_pack_fn_t)player_actarray_power_config_pack},
  {PLAYER_ACTARRAY_CODE, PLAYER_MSGTYPE_REQ, PLAYER_ACTARRAY_BRAKES_REQ,
   (player_pack_fn_t)player_actarray_brakes_config_pack},
  {PLAYER_ACTARRAY_CODE, PLAYER_MSGTYPE_REQ, PLAYER_ACTARRAY_GET_GEOM_REQ,
   (player_pack_fn_t)player_actarray_geom_pack},
  {PLAYER_ACTARRAY_CODE, PLAYER_MSGTYPE_REQ, PLAYER_ACTARRAY_SPEED_REQ,
   (player_pack_fn_t)player_actarray_speed_config_pack},
  {PLAYER_ACTARRAY_CODE, PLAYER_MSGTYPE_CMD, PLAYER_ACTARRAY_POS_CMD,
   (player_pack_fn_t)player_actarray_position_cmd_pack},
  {PLAYER_ACTARRAY_CODE, PLAYER_MSGTYPE_CMD, PLAYER_ACTARRAY_SPEED_CMD,
   (player_pack_fn_t)player_actarray_speed_cmd_pack},
  {PLAYER_ACTARRAY_CODE, PLAYER_MSGTYPE_CMD, PLAYER_ACTARRAY_HOME_CMD,
   (player_pack_fn_t)player_actarray_home_cmd_pack},

  /* aio messages */
  {PLAYER_AIO_CODE, PLAYER_MSGTYPE_DATA, PLAYER_AIO_DATA_STATE,
   (player_pack_fn_t)player_aio_data_pack},
  {PLAYER_ACTARRAY_CODE, PLAYER_MSGTYPE_REQ, PLAYER_AIO_CMD_STATE,
   (player_pack_fn_t)player_aio_cmd_pack},

  /* blobfinder messages */
  {PLAYER_BLOBFINDER_CODE, PLAYER_MSGTYPE_DATA, PLAYER_BLOBFINDER_DATA_BLOBS,
   (player_pack_fn_t)player_blobfinder_data_pack},

  /* bumper messages */
  {PLAYER_BUMPER_CODE, PLAYER_MSGTYPE_DATA, PLAYER_BUMPER_DATA_STATE,
   (player_pack_fn_t)player_bumper_data_pack},
  {PLAYER_BUMPER_CODE, PLAYER_MSGTYPE_DATA, PLAYER_BUMPER_DATA_GEOM,
   (player_pack_fn_t)player_bumper_geom_pack},
  {PLAYER_BUMPER_CODE, PLAYER_MSGTYPE_REQ, PLAYER_BUMPER_GET_GEOM,
   (player_pack_fn_t)player_bumper_geom_pack},

  /* camera messages */
  {PLAYER_CAMERA_CODE, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE,
   (player_pack_fn_t)player_camera_data_pack},

  /* fiducial messages */
  {PLAYER_FIDUCIAL_CODE, PLAYER_MSGTYPE_DATA, PLAYER_FIDUCIAL_DATA_SCAN,
   (player_pack_fn_t)player_fiducial_data_pack},
  {PLAYER_FIDUCIAL_CODE, PLAYER_MSGTYPE_REQ, PLAYER_FIDUCIAL_REQ_GET_GEOM,
   (player_pack_fn_t)player_fiducial_geom_pack},
  {PLAYER_FIDUCIAL_CODE, PLAYER_MSGTYPE_REQ, PLAYER_FIDUCIAL_REQ_GET_FOV,
   (player_pack_fn_t)player_fiducial_fov_pack},
  {PLAYER_FIDUCIAL_CODE, PLAYER_MSGTYPE_REQ, PLAYER_FIDUCIAL_REQ_SET_FOV,
   (player_pack_fn_t)player_fiducial_fov_pack},
  {PLAYER_FIDUCIAL_CODE, PLAYER_MSGTYPE_REQ, PLAYER_FIDUCIAL_REQ_GET_ID,
   (player_pack_fn_t)player_fiducial_id_pack},
  {PLAYER_FIDUCIAL_CODE, PLAYER_MSGTYPE_REQ, PLAYER_FIDUCIAL_REQ_SET_ID,
   (player_pack_fn_t)player_fiducial_id_pack},

  /* gripper messages */
  {PLAYER_GRIPPER_CODE, PLAYER_MSGTYPE_DATA, PLAYER_GRIPPER_DATA_STATE,
   (player_pack_fn_t)player_gripper_data_pack},
  {PLAYER_GRIPPER_CODE, PLAYER_MSGTYPE_CMD, PLAYER_GRIPPER_CMD_STATE,
   (player_pack_fn_t)player_gripper_cmd_pack},
  {PLAYER_GRIPPER_CODE, PLAYER_MSGTYPE_REQ, PLAYER_GRIPPER_REQ_GET_GEOM,
   (player_pack_fn_t)player_gripper_geom_pack},

  /* ir messages */
  {PLAYER_IR_CODE, PLAYER_MSGTYPE_DATA, PLAYER_IR_DATA_RANGES,
    (player_pack_fn_t)player_ir_data_pack},
  {PLAYER_IR_CODE, PLAYER_MSGTYPE_REQ, PLAYER_IR_POSE,
    (player_pack_fn_t)player_ir_pose_pack},
  {PLAYER_IR_CODE, PLAYER_MSGTYPE_REQ, PLAYER_IR_POWER,
    (player_pack_fn_t)player_ir_power_req_pack},

  /* laser messages */
  {PLAYER_LASER_CODE, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN,
    (player_pack_fn_t)player_laser_data_pack},
  {PLAYER_LASER_CODE, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCANPOSE,
    (player_pack_fn_t)player_laser_data_scanpose_pack},
  {PLAYER_LASER_CODE, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_GET_GEOM,
    (player_pack_fn_t)player_laser_geom_pack},
  {PLAYER_LASER_CODE, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_GET_CONFIG,
    (player_pack_fn_t)player_laser_config_pack},
  {PLAYER_LASER_CODE, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_SET_CONFIG,
    (player_pack_fn_t)player_laser_config_pack},
  {PLAYER_LASER_CODE, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_POWER,
    (player_pack_fn_t)player_laser_power_config_pack},

  /* limb messages */
  {PLAYER_LIMB_CODE, PLAYER_MSGTYPE_DATA, PLAYER_LIMB_DATA,
    (player_pack_fn_t)player_limb_data_pack},
  {PLAYER_LIMB_CODE, PLAYER_MSGTYPE_CMD, PLAYER_LIMB_HOME_CMD,
    (player_pack_fn_t)player_limb_home_cmd_pack},
  {PLAYER_LIMB_CODE, PLAYER_MSGTYPE_CMD, PLAYER_LIMB_STOP_CMD,
    (player_pack_fn_t)player_limb_stop_cmd_pack},
  {PLAYER_LIMB_CODE, PLAYER_MSGTYPE_CMD, PLAYER_LIMB_SETPOSE_CMD,
    (player_pack_fn_t)player_limb_setpose_cmd_pack},
  {PLAYER_LIMB_CODE, PLAYER_MSGTYPE_CMD, PLAYER_LIMB_SETPOSITION_CMD,
    (player_pack_fn_t)player_limb_setposition_cmd_pack},
  {PLAYER_LIMB_CODE, PLAYER_MSGTYPE_CMD, PLAYER_LIMB_VECMOVE_CMD,
    (player_pack_fn_t)player_limb_vecmove_cmd_pack},
  {PLAYER_LIMB_CODE, PLAYER_MSGTYPE_REQ, PLAYER_LIMB_POWER_REQ,
    (player_pack_fn_t)player_limb_power_req_pack},
  {PLAYER_LIMB_CODE, PLAYER_MSGTYPE_REQ, PLAYER_LIMB_BRAKES_REQ,
    (player_pack_fn_t)player_limb_brakes_req_pack},
  {PLAYER_LIMB_CODE, PLAYER_MSGTYPE_REQ, PLAYER_LIMB_GEOM_REQ,
    (player_pack_fn_t)player_limb_geom_req_pack},
  {PLAYER_LIMB_CODE, PLAYER_MSGTYPE_REQ, PLAYER_LIMB_SPEED_REQ,
    (player_pack_fn_t)player_limb_speed_req_pack},

  /* localize messages */
  {PLAYER_LOCALIZE_CODE, PLAYER_MSGTYPE_DATA, PLAYER_LOCALIZE_DATA_HYPOTHS,
    (player_pack_fn_t)player_localize_data_pack},
  {PLAYER_LOCALIZE_CODE, PLAYER_MSGTYPE_REQ, PLAYER_LOCALIZE_REQ_SET_POSE,
    (player_pack_fn_t)player_localize_set_pose_pack},
  {PLAYER_LOCALIZE_CODE, PLAYER_MSGTYPE_REQ, PLAYER_LOCALIZE_REQ_GET_PARTICLES,
    (player_pack_fn_t)player_localize_get_particles_pack},

  /* log messages */
  {PLAYER_LOG_CODE, PLAYER_MSGTYPE_REQ, PLAYER_LOG_REQ_SET_WRITE_STATE,
    (player_pack_fn_t)player_log_set_write_state_pack},
  {PLAYER_LOG_CODE, PLAYER_MSGTYPE_REQ, PLAYER_LOG_REQ_SET_READ_STATE,
    (player_pack_fn_t)player_log_set_read_state_pack},
  {PLAYER_LOG_CODE, PLAYER_MSGTYPE_REQ, PLAYER_LOG_REQ_GET_STATE,
    (player_pack_fn_t)player_log_get_state_pack},
  {PLAYER_LOG_CODE, PLAYER_MSGTYPE_REQ, PLAYER_LOG_REQ_SET_READ_REWIND,
    (player_pack_fn_t)player_log_set_read_rewind_pack},
  {PLAYER_LOG_CODE, PLAYER_MSGTYPE_REQ, PLAYER_LOG_REQ_SET_FILENAME,
    (player_pack_fn_t)player_log_set_filename_pack},

  /* map messages */
  {PLAYER_MAP_CODE, PLAYER_MSGTYPE_REQ, PLAYER_MAP_REQ_GET_INFO,
    (player_pack_fn_t)player_map_info_pack},
  {PLAYER_MAP_CODE, PLAYER_MSGTYPE_REQ, PLAYER_MAP_REQ_GET_DATA,
    (player_pack_fn_t)player_map_data_pack},
  {PLAYER_MAP_CODE, PLAYER_MSGTYPE_DATA, PLAYER_MAP_DATA_INFO,
    (player_pack_fn_t)player_map_info_pack},
  {PLAYER_MAP_CODE, PLAYER_MSGTYPE_REQ, PLAYER_MAP_REQ_GET_VECTOR,
    (player_pack_fn_t)player_map_data_vector_pack},

  /* planner messages */
  {PLAYER_PLANNER_CODE, PLAYER_MSGTYPE_DATA, PLAYER_PLANNER_DATA_STATE,
    (player_pack_fn_t)player_planner_data_pack},
  {PLAYER_PLANNER_CODE, PLAYER_MSGTYPE_CMD, PLAYER_PLANNER_CMD_GOAL,
    (player_pack_fn_t)player_planner_cmd_pack},
  {PLAYER_PLANNER_CODE, PLAYER_MSGTYPE_REQ, PLAYER_PLANNER_REQ_ENABLE,
    (player_pack_fn_t)player_planner_enable_req_pack},
  {PLAYER_PLANNER_CODE, PLAYER_MSGTYPE_REQ, PLAYER_PLANNER_REQ_GET_WAYPOINTS,
    (player_pack_fn_t)player_planner_waypoints_req_pack},

  /* player messages */
  {PLAYER_PLAYER_CODE, PLAYER_MSGTYPE_REQ, PLAYER_PLAYER_REQ_DEVLIST,
    (player_pack_fn_t)player_device_devlist_pack},
  {PLAYER_PLAYER_CODE, PLAYER_MSGTYPE_REQ, PLAYER_PLAYER_REQ_DRIVERINFO,
    (player_pack_fn_t)player_device_driverinfo_pack},
  {PLAYER_PLAYER_CODE, PLAYER_MSGTYPE_REQ, PLAYER_PLAYER_REQ_DEV,
    (player_pack_fn_t)player_device_req_pack},
  {PLAYER_PLAYER_CODE, PLAYER_MSGTYPE_REQ, PLAYER_PLAYER_REQ_DATA,
    (player_pack_fn_t)player_device_data_req_pack},
  {PLAYER_PLAYER_CODE, PLAYER_MSGTYPE_REQ, PLAYER_PLAYER_REQ_DATAMODE,
    (player_pack_fn_t)player_device_datamode_req_pack},
  {PLAYER_PLAYER_CODE, PLAYER_MSGTYPE_REQ, PLAYER_PLAYER_REQ_DATAFREQ,
    (player_pack_fn_t)player_device_datafreq_req_pack},
  {PLAYER_PLAYER_CODE, PLAYER_MSGTYPE_REQ, PLAYER_PLAYER_REQ_AUTH,
    (player_pack_fn_t)player_device_auth_req_pack},
  {PLAYER_PLAYER_CODE, PLAYER_MSGTYPE_REQ, PLAYER_PLAYER_REQ_NAMESERVICE,
    (player_pack_fn_t)player_device_nameservice_req_pack},

  /* position2d messages */
  {PLAYER_POSITION2D_CODE, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE,
    (player_pack_fn_t)player_position2d_data_pack},
  {PLAYER_POSITION2D_CODE, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_STATE,
    (player_pack_fn_t)player_position2d_cmd_pack},
  {PLAYER_POSITION2D_CODE, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_GET_GEOM,
    (player_pack_fn_t)player_position2d_geom_pack},
  {PLAYER_POSITION2D_CODE, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_MOTOR_POWER,
    (player_pack_fn_t)player_position2d_power_config_pack},
  {PLAYER_POSITION2D_CODE, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_VELOCITY_MODE,
    (player_pack_fn_t)player_position2d_velocity_mode_config_pack},
  {PLAYER_POSITION2D_CODE, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_POSITION_MODE,
    (player_pack_fn_t)player_position2d_position_mode_req_pack},
  {PLAYER_POSITION2D_CODE, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_SET_ODOM,
    (player_pack_fn_t)player_position2d_set_odom_req_pack},
  {PLAYER_POSITION2D_CODE, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_SPEED_PID,
    (player_pack_fn_t)player_position2d_speed_pid_req_pack},
  {PLAYER_POSITION2D_CODE, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_POSITION_PID,
    (player_pack_fn_t)player_position2d_position_pid_req_pack},
  {PLAYER_POSITION2D_CODE, PLAYER_MSGTYPE_REQ, PLAYER_POSITION2D_REQ_SPEED_PROF,
    (player_pack_fn_t)player_position2d_speed_prof_req_pack},

  /* position3d messages */
  {PLAYER_POSITION3D_CODE, PLAYER_MSGTYPE_DATA, PLAYER_POSITION3D_DATA_STATE,
    (player_pack_fn_t)player_position3d_data_pack},

  /* power messages */
  {PLAYER_POWER_CODE, PLAYER_MSGTYPE_DATA, PLAYER_POWER_DATA_VOLTAGE,
    (player_pack_fn_t)player_power_data_pack},

  /* ptz messages */
  {PLAYER_PTZ_CODE, PLAYER_MSGTYPE_DATA, PLAYER_PTZ_DATA_STATE,
    (player_pack_fn_t)player_ptz_data_pack},
  {PLAYER_PTZ_CODE, PLAYER_MSGTYPE_CMD, PLAYER_PTZ_CMD_STATE,
    (player_pack_fn_t)player_ptz_cmd_pack},
  {PLAYER_PTZ_CODE, PLAYER_MSGTYPE_REQ, PLAYER_PTZ_REQ_GEOM,
    (player_pack_fn_t)player_ptz_geom_pack},
  {PLAYER_PTZ_CODE, PLAYER_MSGTYPE_REQ, PLAYER_PTZ_REQ_GENERIC,
    (player_pack_fn_t)player_ptz_req_generic_pack},
  {PLAYER_PTZ_CODE, PLAYER_MSGTYPE_REQ, PLAYER_PTZ_REQ_CONTROL_MODE,
    (player_pack_fn_t)player_ptz_req_control_mode_pack},

  /* simulation messages */
  {PLAYER_SIMULATION_CODE, PLAYER_MSGTYPE_REQ, PLAYER_SIMULATION_REQ_SET_POSE2D,
    (player_pack_fn_t)player_simulation_pose2d_req_pack},
  {PLAYER_SIMULATION_CODE, PLAYER_MSGTYPE_REQ, PLAYER_SIMULATION_REQ_GET_POSE2D,
    (player_pack_fn_t)player_simulation_pose2d_req_pack},

  /* sonar messages */
  {PLAYER_SONAR_CODE, PLAYER_MSGTYPE_DATA, PLAYER_SONAR_DATA_RANGES,
    (player_pack_fn_t)player_sonar_data_pack},
  {PLAYER_SONAR_CODE, PLAYER_MSGTYPE_REQ, PLAYER_SONAR_REQ_GET_GEOM,
    (player_pack_fn_t)player_sonar_geom_pack},

  /* This NULL element signals the end of the list; don't remove it */
  {0,0,0,NULL}
};

static playerxdr_function_t* ftable;
static int ftable_len;

void
playerxdr_ftable_init()
{
  ftable_len = 0;
  playerxdr_function_t* f;
  for(f = init_ftable; f->func; f++)
    ftable_len++;

  ftable = (playerxdr_function_t*)calloc(ftable_len,
                                         sizeof(playerxdr_function_t));
  assert(ftable);

  memcpy(ftable,init_ftable,ftable_len*sizeof(playerxdr_function_t));
}

void
playerxdr_ftable_add(playerxdr_function_t f)
{
  ftable = (playerxdr_function_t*)realloc(ftable,
                                          ((ftable_len+1)*
                                           sizeof(playerxdr_function_t)));
  assert(ftable);
  ftable[ftable_len++] = f;
}

player_pack_fn_t
playerxdr_get_func(uint16_t interf, uint8_t type, uint8_t subtype)
{
  playerxdr_function_t* curr=NULL;
  int i;

  if(!ftable_len)
    return(NULL);

  // The supplied type can be RESP_ACK if the registered type is REQ.
  if (type == PLAYER_MSGTYPE_RESP_ACK || type == PLAYER_MSGTYPE_RESP_NACK)
    type = PLAYER_MSGTYPE_REQ;

  for(i=0;i<ftable_len;i++)
  {
    curr = ftable + i;
    // Make sure the interface and subtype match exactly.
    if(curr->interf == interf &&
      curr->type == type &&
      curr->subtype == subtype)
      return(curr->func);
  }

  printf( "interface %d %d\n", curr->interf, interf );
  printf( "type %d %d\n", curr->type, type );
  printf( "subtype %d %d\n", curr->subtype, subtype );

  return(NULL);
}

