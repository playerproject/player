/* some basic structures and conversions needed by everything
 *
 * data for the conversions and everything else will be grabbed out of
 * the general configuration file
 *
 * note - they way it is now is the right way to do it, BUT:
 * if your a hacker and want things to run fast - put all the configurations
 * here as #define's, this will allow the precompiler to precompute combined
 * conversions, this is much preferable to modifying the rest of the code
 * if you really need the speed that badly
 */

// NOTICE! - this file declares rflex_configs extern, intended to link to 
// the rflex_configs declared in rflex.cc

#ifndef RFLEX_CONFIGS_H
#define RFLEX_CONFIGS_H

#include <math.h>
#include <player.h>

//holds the pose of a sonar in robot relative coordinates
typedef struct
{
  double x;
  double y;
  double t;
} sonar_pose_t;

//normalizes an angle in radians to -M_PI<theta<M_PI
inline double normalize_theta(double theta){
  while(theta>M_PI)
    theta-=2*M_PI;
  while(theta<-M_PI)
    theta+=2*M_PI;
  return theta;
}

//structures for holding general configuration of robot
typedef struct rflex_config_t{
  char serial_port[256];
  //length of the robot in mm
  double mm_length;
  //width of the robot in mm
  double mm_width;
  //mm*odo_distance_conversion : mm to rflex arbitrary odometry units (trans)
  double odo_distance_conversion;
  //rad*odo_angle_conversion : rad to rflex arbitrary odometry units (rot)
  double odo_angle_conversion;
  //mm*range_distance_conversion : mm to rflex arbitrary range units
  double range_distance_conversion;
  //default translational acceleration in mm/sec
  double mmPsec2_trans_acceleration;
  //default rotational acceleration in rad/sec
  double radPsec2_rot_acceleration;
  //maximum number of sonar supported by modules
  //(generally 16*number of sonar controller boards, or banks)
  int max_num_sonars;
  //total number of physical sonar
  int num_sonars;
  //how long to buffer for filter (filter just takes smallest of last n readings)
  int sonar_age;
  //number of physical sonar sonar controller boards or banks on your robot
  int num_sonar_banks;
  // number of sonars that can be attached to each sonar controller (16)
  int num_sonars_possible_per_bank;
  // number of actual sonar attached to each sonar controller, (in each bank)
  int *num_sonars_in_bank;
  // pose of each sonar on the robot (x,y,t) in rad and mm
  // note i is forwards, j is left
  sonar_pose_t *mmrad_sonar_poses;
  //not sure what these do yet actually
  long sonar_echo_delay;
  long sonar_ping_delay;
  long sonar_set_delay;
  unsigned short bumper_count;
  int bumper_address;
  player_bumper_define_t * bumper_def;
  int power_offset;
}  rflex_config_t;

//notice - every file that includes this header gets a GLOBAL rflex_configs!!
// be careful
extern rflex_config_t rflex_configs;

/************ conversions ********************/
/*** rather obvious - ARB stands for arbitrary (look above) ***/

//player uses degrees, but I use radians (so cos, and sin work correctly)
#define RAD2DEG_CONV(x)  ((x)*(180/M_PI))
#define DEG2RAD_CONV(x) ((x)*(M_PI/180))
#define ARB2RAD_ODO_CONV(x) ((x)/rflex_configs.odo_angle_conversion)
#define RAD2ARB_ODO_CONV(x) ((x)*rflex_configs.odo_angle_conversion)
#define ARB2MM_ODO_CONV(x) ((x)/rflex_configs.odo_distance_conversion)
#define MM2ARB_ODO_CONV(x) ((x)*rflex_configs.odo_distance_conversion)

#define ARB2MM_RANGE_CONV(x) (x/rflex_configs.range_distance_conversion)
#define MM2ARB_RANGE_CONV(x) (x*rflex_configs.range_distance_conversion)

#endif









