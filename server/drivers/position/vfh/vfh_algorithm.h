#ifndef VFH_ALGORITHM_H
#define VFH_ALGORITHM_H

#include <vector>
#include "player.h"

class VFH_Algorithm
{
public:
    VFH_Algorithm( double cell_size,
                              int window_diameter,
                              int sector_angle,
                              double safety_dist,
                              int max_speed,
                              int min_turnrate,
                              int max_turnrate,
                              double free_space_cutoff,
                              double obs_cutoff,
                              double weight_desired_dir,
                              double weight_current_dir );
    ~VFH_Algorithm();

    int Init();
    
    // Get a new speed and turnrate based on the given laser data.
    int Update_VFH( double laser_ranges[PLAYER_LASER_MAX_SAMPLES][2], int &speed, int &turnrate );

    // Get methods
    int GetMinTurnrate() { return MIN_TURNRATE; }
    int GetMaxTurnrate() { return MAX_TURNRATE; }
    int GetMaxSpeed()    { return MAX_SPEED; }

    // Set methods
    void SetRobotRadius( float robot_radius ) { this->ROBOT_RADIUS = robot_radius; }
    void SetDesiredAngle( float Desired_Angle ) { this->Desired_Angle = Desired_Angle; }
    void SetMinTurnrate( int min_turnrate ) { MIN_TURNRATE = min_turnrate; }
    void SetMaxTurnrate( int max_turnrate ) { MAX_TURNRATE = max_turnrate; }
    void SetMaxSpeed( int max_speed )       { MAX_SPEED = max_speed; }

private:

    // Functions

    int VFH_Allocate();

    float Delta_Angle(int a1, int a2);
    float Delta_Angle(float a1, float a2);
    int Bisect_Angle(int angle1, int angle2);

    int Calculate_Cells_Mag( double laser_ranges[PLAYER_LASER_MAX_SAMPLES][2] );
    int Build_Primary_Polar_Histogram( double laser_ranges[PLAYER_LASER_MAX_SAMPLES][2] );
    int Build_Binary_Polar_Histogram();
    int Build_Masked_Polar_Histogram(int speed);
    int Read_Min_Turning_Radius_From_File(char *filename);
    int Select_Candidate_Angle();
    int Select_Direction();
    int Set_Motion( int &speed, int &turnrate );

    void Print_Cells_Dir();
    void Print_Cells_Mag();
    void Print_Cells_Dist();
    void Print_Cells_Sector();
    void Print_Cells_Enlargement_Angle();
    void Print_Hist();

    // Data

    bool Goal_Behind;

    float ROBOT_RADIUS;         // millimeters
    int CENTER_X;
    int CENTER_Y;
    int HIST_SIZE;

    float CELL_WIDTH;           // millimeters
    int WINDOW_DIAMETER;        // cells
    int SECTOR_ANGLE;           // degrees
    float SAFETY_DIST;          // millimeters
    int MAX_SPEED;
    int MIN_TURNRATE;
    int MAX_TURNRATE;
    float Binary_Hist_Low, Binary_Hist_High;
    float U1, U2;
    float Desired_Angle, Picked_Angle, Last_Picked_Angle;

    std::vector<std::vector<float> > Cell_Direction;
    std::vector<std::vector<float> > Cell_Base_Mag;
    std::vector<std::vector<float> > Cell_Mag;
    std::vector<std::vector<float> > Cell_Dist;
    std::vector<std::vector<float> > Cell_Enlarge;
    std::vector<std::vector<std::vector<int> > > Cell_Sector;
    std::vector<float> Candidate_Angle;

    double dist_eps;
    double ang_eps;

    float *Hist, *Last_Binary_Hist;

    // Minimum turning radius at different speeds, in millimeters
    int *Min_Turning_Radius;
};

#endif
