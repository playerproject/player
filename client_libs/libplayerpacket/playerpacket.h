
/* Functions to pack and unpack the Player message structures 
 * - 
 * - important to keep this in sync with player.h!
 *
 * TODO - inline documentation and parser for this file?
 *
 * Created 2002-11-18 rtv
 * $Id$
 */


#include <player.h>

#include "jpeg.h"


#ifdef __cplusplus
extern "C" {
#endif
  
/* libplayerpacket provides the following functions to pack and
   unpack Player's message structures using native types and SI
   units: */
  
  void SonarDataPack( player_sonar_data_t* data, 
                      int num_samples, double ranges[]);
  
  void SonarDataUnpack( player_sonar_data_t* data, 
                        int* num_samples, double ranges[]);
  
  void SonarGeomPack( player_sonar_geom_t* geom, 
                      int num_samples, double poses[][3] );
  
  void SonarGeomUnpack( player_sonar_geom_t* geom, 
                        int* num_samples, double poses[][3] );
  
  void PositionDataPack( player_position_data_t* data,
                         double xpos, double ypos, double yaw,
                         double xspeed, double yspeed, double yawspeed, 
                         int stall );
  
  void PositionDataUnpack( player_position_data_t* data,
                           double* xpos, double* ypos, double* yaw,
                           double* xspeed, double* yspeed, double* yawspeed,
                           int* stall );
  
  void PositionCmdUnpack( player_position_cmd_t* cmd,
                          double* xpos, double* ypos, double* yaw,
                          double* xspeed, double* yspeed, double* yawspeed );
  
  void PositionGeomPack( player_position_geom_t* geom,
                         double x, double y, double a, 
                         double width, double height );
  
  void PositionSetOdomReqUnpack( player_position_set_odom_req_t* req,
                                 double* x, double* y, double* a );
  
  void FiducialDataPack(  player_fiducial_data_t* data, 
                          int count, int ids[], double poses[][3], double pose_errors[][3] );
  
  void FiducialDataUnpack(  player_fiducial_data_t* data, 
                            int *count, int ids[], double poses[][3], double pose_errors[][3] );
  
  void FiducialGeomPack(  player_fiducial_geom_t* geom,
                          double px, double py, double pth,
                          double sensor_width, double sensor_height,
                          double target_width, double target_height );
  
  void FiducialGeomUnpack(  player_fiducial_geom_t* geom,
                            double* px, double* py, double* pth,
                            double* sensor_width, double* sensor_height,
                            double* target_width, double* target_height );
  
  void FiducialFovPack( player_fiducial_fov_t* fov, int setflag,
                        double min_range, double max_range, 
                        double view_angle );

  
  void FiducialFovUnpack( player_fiducial_fov_t* fov,
                          double* min_range, double* max_range, 
                          double* view_angle );

#ifdef __cplusplus
}
#endif




