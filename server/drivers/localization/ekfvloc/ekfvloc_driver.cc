/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2010
 *     Alejandro R. Mosteo
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_ekfvmap ekfvmap
 * @brief ScanMatching

This driver implements segment-based EKF localization:

"Mobile Robot Localization and Map Building: A Multisensor Fusion Approach"
J.A. Castellanos and J.D. Tardós
Kluwer Academic Publishers, Boston, 1999
ISBN 0-7923-7789-3

@par Compile-time dependencies

- none

@par Provides

- @ref interface_position2d
    - The pose estimation
- @ref interface_localize
    - The full data about the estimation
- @ref interface_opaque
    - Publishes a 9-tuple of doubles which is the covariance matrix.
    - Under the key "covariance"

@par Requires

- @ref interface_position2d : odometry source of pose and velocity information
- @ref interface_laser : Pose-stamped laser scans (subtype PLAYER_LASER_DATA_SCANPOSE)
- @ref interface_map : Vector map with segments modelling the environment
- @ref interface_graphics2d : optional. If provided, it will be used to display debug data

@par Configuration requests

- Position requests are forwarded to the odometry position2 interface.
  - They are expected to be in the ekfvloc ref. frame, and will be automatically
    converted to odometry reference.
- Velocity requests are forwarded to the odometry position2 interface

@par Configuration file options

- max_laser_range (float)
  - Default: 7.9 m
  - Maximum laser range.

- laser_noise (tuple (doubles): [m rad])
  - Default: [0.004 0.045]
  - Noise in laser range/bearing readings; default is for a SICK LMS200

- odom_noise (tuple (doubles): [m m rad])
  - Default: [0.4 0.2 0.2]
  - Noise in motion model
  - This is, in terms of covariance, 2*sigma

- robot_pose (tuple (doubles): [m m rad])
  - Default: [0.0 0.0 0.0]
  - Initial pose estimation

- robot_pose_initial_error (tuple (doubles): [m m rad])
  - Default: [1.0 1.0 0.2]
  - Initial error the initial robot pose. It EX = 1.0, robot is at X +/- (E/2)

- mapfile (string)
  - Default: none
  - Parameter that allows the loading of a mapfile instead of using
        a map interface. Mapfile formap is #segments [x0 y0 x1 y1]...
  - When *not* using this but a requires, use alwayson 1 in the vmapfile driver
  -     if you get errors initializing this driver

- truth_model (string)
  - Default : none
  - Named model in stage for ground truth. Used for debugging.
  - When using this, also use a "simulation" requires

- FINE TUNING OPTIONS (units (default))
  - Note that player unit conversions are applied, so these units are only informative for the defaults

- max_region_empty_angle (rad (0.035 ~ 2deg))
  - Angular distance to split regions

- max_region_empty_distance (m (0.1))
  - Euclidean distance to split regions

- min_region_length (m (0.2))
  - Minimum distance between region endpoints

- min_points_in_region (int (4))
  - Minimum laser returns in a region

- split_confidence (percent (95.0))
  - chi-2 confidence test for segment splitting

- check_residual (boolean_int (0))
  - perform residual testing for segment splitting

- max_ang_ebe (rad (0.0))
  - ???

- min_split_segment_distance (m (0.0))
  - minimum dist between new segment endpoints for segment splitting

- min_odom_distance_delta (m (0.0))
- min_odom_angle_delta (rad (0.0))
  - minimum change in odometry before performing update

- backoff_period (s (0.05))
  - minimum time between updates; if lasers arrive faster they'll be dropped

- DEBUG OPTIONS
  - send_debug (int (0))
  - port for custom socket connection to an external debugging GUI
  - If != 0 it will be used.

@par Example

@verbatim
# Using degrees
driver
(
  name "ekfvmap"
  provides ["position2d:1" "localize:0" "covariance:::0"]
  requires ["position2d:0" "laser:0" "map:0"]

  max_laser_range 31.9
  laser_noise [0.004 0.5]
  odom_noise  [0.4 0.2 10]
  robot_pose  [-49 -39 0]
  robot_pose_initial_error [1.0 1.0 20]
  # mapfile "upc.vec"
  # Use either "requires" or "mapfile"

  # truth_model "robot"
)

@endverbatim

@author J.A. Castellanos, J.D. Tardós (underlying algorithm), Mayte Lázaro (code), A. Mosteo (player integration)
*/
/** @} */

#include <arpa/inet.h>
#include <errno.h>
#include <math.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stddef.h>
#include <unistd.h>

#include <libplayercore/device.h>
#include <libplayercore/driver.h>
#include <libplayercore/playercore.h>

#include "localize.hh"
#include "params.hh"
#include "types.hh"

const bool kThreaded = true;

GuiData GUI_DATA;

class Ekfvloc : public ThreadedDriver
{

public:

    Ekfvloc(ConfigFile* cf, int section);
    virtual ~Ekfvloc();

    virtual int MainSetup();
    virtual void MainQuit();

    virtual int ProcessMessage(QueuePointer &resp_queue,
            player_msghdr * hdr,
            void * data);
private:
    Localize            localize_;
    string              mapfile_;

    player_devaddr_t    odom_addr_;
    player_devaddr_t    laser_addr_;
    player_devaddr_t    map_addr_;
    player_devaddr_t    sim_addr_;
    player_devaddr_t    p2d_addr_; // Position address
    player_devaddr_t    loc_addr_; // Localization address
    player_devaddr_t    cov_opaque_addr_;
    player_devaddr_t    g2d_addr_;

    Device              *odom_;
    Device              *laser_;
    Device              *map_;
    Device              *sim_;
    Device              *g2d_;

    player_position2d_data position_; // Odometry pose
    bool                   have_pose_;

    Pose                   global_initial_pose_; // Given initial pose

//    Transf                 odom_base_ref_;
//    Transf                 global_base_ref_;

    int                    scan_count_;

    struct timeval      prev_scan_timestamp_;

    std::string         sim_model_;
    Pose                sim_pose_; // Ground truth if simulating

    int                 debug_sock_;
    
    bool                   publish_cov_;
    bool                   use_g2d_;

    std::vector<double> cov_pub_; // published covariance via opaque

    Transf              laser_pose_;

    double laser_gap_; // angular gap in laser scans

    // Main function for device thread.
    virtual void Main();

    // Say if we should process another scan, to avoid excessive CPU usage
    bool CheckElapsed(void);

    Pose GroundTruth(void);

    void PublishInterfaces(double timestamp); // Publish all output interfaces

    bool PrepareDebug(int port); // False on failure
    void SendDebug(const GuiData &gui_data) const;
    void DrawDebug(const GuiData &gui_data);
    void DrawClear(void);
    void DrawLine(player_color_t color, player_point_2d_t ini, player_point_2d_t fin);
    void DrawLaser(player_color_t color,
                   double rho0, double phi0,
                   double rho1, double phi1); // Take laser coords and draw in robot frame
    void DrawEllipse(void);
};

////////////////////////////////////////////////////////////////////////////////
Driver* ekfvloc_Init(ConfigFile* cf, int section)
{
    return static_cast<Driver*>(new Ekfvloc(cf, section));
}


////////////////////////////////////////////////////////////////////////////////
void ekfvloc_Register(DriverTable* table)
{
    table->AddDriver("ekfvloc", ekfvloc_Init);
}

bool Ekfvloc::CheckElapsed(void)
{

    struct timeval now;

    if (gettimeofday(&now, NULL))
    {
        PLAYER_ERROR1("Ekfvloc:CheckElapsed: %s", strerror(errno));
        return true;
    }

    const uint64_t elapsed =
        (now.tv_sec  - prev_scan_timestamp_.tv_sec)  * 1000 +
        (now.tv_usec - prev_scan_timestamp_.tv_usec) / 1000;

    PLAYER_MSG1(5, "Ekfvloc::CheckElapsed: %u elapsed", elapsed);

    prev_scan_timestamp_ = now;

    return elapsed >= static_cast<uint64_t>(kMinMillisBetweenScans);
}

////////////////////////////////////////////////////////////////////////////////
int Ekfvloc::MainSetup()
{
    if (mapfile_.length() > 0)
    {
        try
        {
            localize_.LoadMap(mapfile_.c_str());
            printf("Map loaded from %s\n", mapfile_.c_str());
        }
        catch (...)
        {
            PLAYER_ERROR("File could not be loaded");
        }
    }
    else
    {
        if (!(map_ = deviceTable->GetDevice(map_addr_)))
        {
            PLAYER_ERROR("Unable to locate map device at given address");
            return -1;
        }

        if (map_->Subscribe(InQueue) != 0)
        {
            PLAYER_ERROR("Unable to subscribe to map interface");
            return -1;
        }

        Message *msg = map_->Request(InQueue, PLAYER_MSGTYPE_REQ, PLAYER_MAP_REQ_GET_VECTOR, NULL, 0, NULL, kThreaded);

        if (msg == NULL)
        {
            PLAYER_ERROR("Unable to request map");
            return -1;
        }
        else
        {
            const player_map_data_vector_t &map = *static_cast<player_map_data_vector_t *>(msg->GetPayload());
            SegmentsVector segments;

            for (uint32_t i = 0; i < map.segments_count; i++)
            {
                segments.push_back(Segment(
                            map.segments[i].x0,
                            map.segments[i].y0,
                            map.segments[i].x1,
                            map.segments[i].y1));
            }

            localize_.SetMap(segments);

            delete msg;
        }
    }

    if (!(odom_ = deviceTable->GetDevice(odom_addr_)))
    {
        PLAYER_ERROR("Unable to locate position device at given address");
        return -1;
    }

    if (odom_->Subscribe(InQueue) != 0)
    {
        PLAYER_ERROR("Unable to subscribe to position device");
        return -1;
    }

    if (!(laser_ = deviceTable->GetDevice(laser_addr_)))
    {
        PLAYER_ERROR("Unable to locate laser device at given address");
        return -1;
    }

    if (laser_->Subscribe(InQueue) != 0)
    {
        PLAYER_ERROR("Unable to subscribe to laser device");
        return -1;
    }

    if (use_g2d_)
    {
        if (!(g2d_ = deviceTable->GetDevice(g2d_addr_)))
        {
            PLAYER_ERROR("Unable to locate graphics2d device");
            return -1;
        }
        if (g2d_->Subscribe(InQueue) != 0)
        {
            PLAYER_ERROR("Unable to subscribe to graphics2d device");
            return -1;
        }
    }

    Message *msg = laser_->Request(InQueue, PLAYER_MSGTYPE_REQ, PLAYER_LASER_REQ_GET_GEOM, NULL, 0, NULL);

    if (msg)
    {
        const player_laser_geom_t &laser_geom = *static_cast<player_laser_geom_t *>(msg->GetPayload());

        PLAYER_MSG3(0, "Ekfvloc: Reported laser pose: %8.3f %8.3f %8.3f",
                laser_geom.pose.px, laser_geom.pose.py, laser_geom.pose.pyaw);

        localize_.SetLaserPose(
            laser_geom.pose.px,
            laser_geom.pose.py,
            laser_geom.pose.pyaw);

        laser_pose_ = Transf(laser_geom.pose.px, laser_geom.pose.py, laser_geom.pose.pyaw);
    }
    else
    {
        PLAYER_WARN("Laser didn't provide its pose!");
        laser_pose_ = Transf(0, 0, 0);
    }

    delete msg;

    return 0;
}

////////////////////////////////////////////////////////////////////////////////
void Ekfvloc::MainQuit()
{
}

////////////////////////////////////////////////////////////////////////////////
Ekfvloc::Ekfvloc(ConfigFile* cf, int section)
        : ThreadedDriver(cf, section),
        localize_(cf->ReadLength(section,       "max_laser_range",  7.9),
                0.0, 0.0, 0.0, // Laser pose is corrected via GEOM request
                cf->ReadTupleLength(section,  "laser_noise", 0,   0.045),
                cf->ReadTupleAngle(section,   "laser_noise", 1,   0.004),
                cf->ReadTupleLength(section,  "odom_noise", 0,    0.4),
                cf->ReadTupleLength(section,  "odom_noise", 1,    0.2),
                cf->ReadTupleAngle(section,   "odom_noise", 2,    0.2)),
        mapfile_(cf->ReadString(section, "mapfile", "")),
        have_pose_(false),
        global_initial_pose_(0, 0, 0),
//        global_base_ref_(cf->ReadTupleLength(section, "robot_pose", 0, 0.0),
//                         cf->ReadTupleLength(section, "robot_pose", 1, 0.0),
//                         cf->ReadTupleAngle(section, "robot_pose", 2, 0.0)),
        scan_count_(0),
        sim_pose_(0.0, 0.0, 0.0),
        debug_sock_(-1),
        publish_cov_(false),
        use_g2d_(false)
{
    odom_ = laser_ = NULL;
    prev_scan_timestamp_.tv_sec = 0;

    global_initial_pose_ = Pose (cf->ReadTupleLength(section, "robot_pose", 0, 0.0),
                                 cf->ReadTupleLength(section, "robot_pose", 1, 0.0),
                                 cf->ReadTupleAngle(section, "robot_pose", 2, 0.0));

//  This was very wrong for odometries not starting at zero
//    localize_.SetRobotPose(cf->ReadTupleLength(section, "robot_pose", 0, 0.0),
//            cf->ReadTupleLength(section, "robot_pose", 1, 0.0),
//            cf->ReadTupleAngle(section, "robot_pose", 2, 0.0));

    localize_.SetRobotPoseError(
        cf->ReadTupleLength(section, "robot_pose_initial_error", 0, 1.0),
        cf->ReadTupleLength(section, "robot_pose_initial_error", 1, 1.0),
        cf->ReadTupleAngle(section, "robot_pose_initial_error", 2, 0.2));

    // Device basic checking

    if (cf->ReadDeviceAddr(&odom_addr_, section, "requires",
            PLAYER_POSITION2D_CODE, -1, NULL))
    {
        PLAYER_ERROR("Missing address of position required interface");
        SetError(-1);
        return;
    }

    if (cf->ReadDeviceAddr(&laser_addr_, section, "requires",
            PLAYER_LASER_CODE, -1, NULL))
    {
        PLAYER_ERROR("Missing address of laser required interface");
        SetError(-1);
        return;
    }

    if (mapfile_.empty())
        if (cf->ReadDeviceAddr(&map_addr_, section, "requires",
                PLAYER_MAP_CODE, -1, NULL))
        {
            PLAYER_ERROR("Missing address of map required interface");
            SetError(-1);
            return;
        }

    if (cf->ReadDeviceAddr(&p2d_addr_, section, "provides",
            PLAYER_POSITION2D_CODE, -1, NULL))
    {
        PLAYER_ERROR("Missing address of position provided interface");
        SetError(-1);
        return;
    }
    else
    {
         if (AddInterface(p2d_addr_) != 0)
         {
            PLAYER_ERROR("Cannot add position2d interface");
            SetError(-1);
            return;
         }
    }
    
    if (cf->ReadDeviceAddr(&loc_addr_, section, "provides",
            PLAYER_LOCALIZE_CODE, -1, NULL))
    {
        PLAYER_ERROR("Missing address of localize provided interface");
        SetError(-1);
        return;
    }
    else
    {
         if (AddInterface(loc_addr_) != 0)
         {
            PLAYER_ERROR("Cannot add localize interface");
            SetError(-1);
            return;
         }
    }

    if (cf->ReadDeviceAddr(&cov_opaque_addr_, section, "provides",
        PLAYER_OPAQUE_CODE, -1, "covariance") == 0)
    {
        if (AddInterface(cov_opaque_addr_) != 0)
        {
            PLAYER_ERROR("Cannot add cov opaque interface");
            SetError(-1);
            return;
        }
        else
            printf("Ekfvloc: using opaque interface for covariance\n");
    }
    else
    {
        publish_cov_ = false;
    }

    if (!(cf->ReadDeviceAddr(&sim_addr_, section, "requires", PLAYER_SIMULATION_CODE, -1, NULL)))
    {
        if ((sim_ = deviceTable->GetDevice(sim_addr_)))
        {
            sim_model_ = std::string(cf->ReadString(section, "truth_model", "missing!"));
            printf("Ekfvloc: using simulation model [%s] for ground truth\n", sim_model_.c_str());
        }
    }
    else
        sim_ = NULL;

    if (cf->ReadDeviceAddr(&g2d_addr_, section, "requires",
        PLAYER_GRAPHICS2D_CODE, -1, NULL) == 0)
    {
        printf("Ekfvloc: using graphics2d interface for display\n");
        use_g2d_ = true;
    }

    if (cf->ReadInt(section, "send_debug", 0) != 0)
        if (!PrepareDebug(cf->ReadInt(section, "send_debug", 0)))
        {
            PLAYER_ERROR("Cannot connect to remote debugger");
            SetError(-1);
            return;
        }

    // Rest of initialization at MainSetup, when first client connects

    // PARAMETERS FINE TUNING
    kMaxEmptyAngle          = cf->ReadAngle(section, "max_region_empty_angle", kMaxEmptyAngle);
    kMaxEmptyDistance       = cf->ReadLength(section, "max_region_empty_distance", kMaxEmptyDistance);
    kMinRegionLength        = cf->ReadLength(section, "min_region_length", kMinRegionLength);
    kMinPointsInRegion      = cf->ReadInt(section, "min_points_in_region", kMinPointsInRegion);
    kMinPointsInSegment     = cf->ReadInt(section, "min_points_in_segment", kMinPointsInSegment);
    kConfidence             = cf->ReadFloat(section, "split_confidence", kConfidence);
    kCheckResidual          = (cf->ReadInt(section, "check_residual", kCheckResidual ? 1 : 0) != 0);
    kMaxAngEBE              = cf->ReadAngle(section, "max_ang_ebe", kMaxAngEBE);
    kMinDistBetweenEndpoints = cf->ReadLength(section, "min_split_segment_distance", kMinDistBetweenEndpoints);
    kMinOdomDistChange      = cf->ReadLength(section, "min_odom_distance_delta", kMinOdomDistChange);
    kMinOdomAngChange       = cf->ReadAngle(section, "min_odom_angle_delta", kMinOdomAngChange);
    kMinMillisBetweenScans  = static_cast<long>(cf->ReadFloat(section, "backoff_period", kMinMillisBetweenScans / 1000.0) * 1000.0);

    PLAYER_MSG2(1, "Ekfvloc: %30s: %8.3f", "max_region_empty_angle", kMaxEmptyAngle);
    PLAYER_MSG2(1, "Ekfvloc: %30s: %8.3f", "max_region_empty_distance", kMaxEmptyDistance);
    PLAYER_MSG2(1, "Ekfvloc: %30s: %8.3f", "min_region_length", kMinRegionLength);
    PLAYER_MSG2(1, "Ekfvloc: %30s: %8d",    "min_points_in_region", kMinPointsInRegion);
    PLAYER_MSG2(1, "Ekfvloc: %30s: %8d",    "min_points_in_segment", kMinPointsInSegment);
    PLAYER_MSG2(1, "Ekfvloc: %30s: %8.3f", "split_confidence", kConfidence);
    PLAYER_MSG2(1, "Ekfvloc: %30s: %s",    "check_residual", kCheckResidual ? "true" : "false");
    PLAYER_MSG2(1, "Ekfvloc: %30s: %8.3f", "max_ang_ebe", kMaxAngEBE);
    PLAYER_MSG2(1, "Ekfvloc: %30s: %8.3f", "min_split_segment_distance", kMinDistBetweenEndpoints);
    PLAYER_MSG2(1, "Ekfvloc: %30s: %8.3f", "min_odom_distance_delta", kMinOdomDistChange);
    PLAYER_MSG2(1, "Ekfvloc: %30s: %8.3f", "min_odom_angle_delta", kMinOdomAngChange);
    PLAYER_MSG2(1, "Ekfvloc: %30s: %8.3f", "backoff_period(ms)", kMinMillisBetweenScans);
}


////////////////////////////////////////////////////////////////////////////////
Ekfvloc::~Ekfvloc()
{
}

bool Ekfvloc::PrepareDebug(int port)
{
    if ((debug_sock_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
    {
        perror("Can't create gui socket");
        return false;
    }

    struct sockaddr_in localhost;
    memset (&localhost, 0, sizeof(localhost));
    localhost.sin_family = AF_INET;
    localhost.sin_port   = htons(port);
    if (inet_aton("127.0.0.1", &localhost.sin_addr) == 0)
    {
        perror("Can't set localhost address");
        return false;
    }
    
    PLAYER_MSG2(1, "Ekfvloc: Connecting to %s:%d...\n", inet_ntoa(localhost.sin_addr), ntohs(localhost.sin_port));
    if (connect(debug_sock_, (sockaddr*)&localhost, sizeof(localhost)) != 0)
    {
        perror("Can't connect to gui listener");
        return false;
    }
    PLAYER_MSG0(1, "Ekfvloc: Connected.");
    
    return true;
}

void Ekfvloc::SendDebug(const GuiData &gui_data) const
{
    int i, j;

    {
    const int nregs = gui_data.regions.size();
    std::vector<double> regs;
    
    for (j = 0, i = 0; i < nregs; i++)
    {
        regs.push_back(gui_data.regions.at(i).rho0());
        regs.push_back(gui_data.regions.at(i).phi0());
        regs.push_back(gui_data.regions.at(i).rho1());
        regs.push_back(gui_data.regions.at(i).phi1());
    }
    // regions
    if (write(debug_sock_, (void*)&nregs, sizeof(int)) == -1)
        perror("Ekfvloc error sending gui region count");
    if (write(debug_sock_, (void*)&regs[0], nregs * 4 * sizeof(double)) == -1)
        perror("Ekfvloc error sending gui regions");
    }

    {
    const int nsplt = gui_data.splits.size();
    std::vector<double> splt;
    
    for (j = 0, i = 0; i < nsplt; i++)
    {
        splt.push_back(gui_data.splits.at(i).rho0());
        splt.push_back(gui_data.splits.at(i).phi0());
        splt.push_back(gui_data.splits.at(i).rho1());
        splt.push_back(gui_data.splits.at(i).phi1());
    }
    // splits
    if (write(debug_sock_, (void*)&nsplt, sizeof(int)) == -1)
        perror("Ekfvloc error sending gui split count");
    if (write(debug_sock_, (void*)&splt[0], nsplt * 4 * sizeof(double)) == -1)
        perror("Ekfvloc error sending gui splits");
    }

    {
    const int nmtch = gui_data.matches.size();
    std::vector<double> mtch;

    for (j = 0, i = 0; i < nmtch; i++)
    {
        mtch.push_back(gui_data.matches.at(i).rho0());
        mtch.push_back(gui_data.matches.at(i).phi0());
        mtch.push_back(gui_data.matches.at(i).rho1());
        mtch.push_back(gui_data.matches.at(i).phi1());
    }
    // matches
    if (write(debug_sock_, (void*)&nmtch, sizeof(int)) == -1)
        perror("Ekfvloc error sending gui match count");
    if (write(debug_sock_, (void*)&mtch[0], nmtch * 4 * sizeof(double)) == -1)
        perror("Ekfvloc error sending gui matches");
    }

    // Covs
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
        {
            double x = localize_.GetCovariance()(i, j);
            if (write(debug_sock_, (void*)&x, sizeof(x)) == -1)
            {
                perror("Error sending covariance");
                printf("Ekfvloc error sending gui Cov(%d, %d)", i, j);
            }
        }
}

void Ekfvloc::DrawClear(void)
{
    g2d_->PutMsg(InQueue, PLAYER_MSGTYPE_CMD, PLAYER_GRAPHICS2D_CMD_CLEAR,
                 static_cast<void*>(NULL), 0.0, NULL);
}

void Ekfvloc::DrawLine(player_color_t color, player_point_2d_t ini, player_point_2d_t fin)
{
   player_point_2d_t points[2] = {ini, fin};
   
   player_graphics2d_cmd_polyline_t line = { 2, points, color };

   // We use a request to ensure that &points remains valid until return
   // Message *msg =
   g2d_->PutMsg(InQueue, PLAYER_MSGTYPE_CMD, PLAYER_GRAPHICS2D_CMD_POLYLINE,
                static_cast<void*>(&line), 0.0, NULL);
}

void Ekfvloc::DrawLaser(player_color_t color,
                        double rho0, double phi0,
                        double rho1, double phi1)
{
    const Transf tpini(rho0 * cos(phi0), rho0 * sin(phi0), 0.0);
    const Transf tpfin(rho1 * cos(phi1), rho1 * sin(phi1), 0.0);
    const Transf tini(Compose(laser_pose_, tpini));
    const Transf tfin(Compose(laser_pose_, tpfin));
    const player_point_2d_t ini = {tini.tX(), tini.tY()};
    const player_point_2d_t fin = {tfin.tX(), tfin.tY()};

    DrawLine(color, ini, fin);
}

void Ekfvloc::DrawEllipse(void)
{
    const double k = 5.99; // 95% confidence.
    const int points = 20; // segments in ellipse
    const player_color_t black = {0, 0, 0, 0};
    const player_color_t blue  = {0, 0, 0, 255};

    const Matrix cov = localize_.GetCovariance();
    Matrix sigma(2, 2);

    for (int r = 0; r < 2; r++)
        for (int c = 0; c < 2; c++)
            sigma(r, c) = cov(r, c);

    sigma *= k; // Matrix * scalar

    Matrix D, V; // eigenvalues & eigenvectors
    Eigenv(sigma, &V, &D);

    Matrix Dsqrt = D; // Keep sqrt(D) for reuse
    for (int r = 0; r < 2; r++)
        for (int c = 0; c < 2; c++)
            Dsqrt(r, c) = sqrt(Dsqrt(r, c));

    Matrix U(2, points); // 2 x points, to approximate the ellipse
    for (int c = 0; c < points; c++)
    {
        U(0, c) = cos(2.0 * M_PI / points * c);
        U(1, c) = sin(2.0 * M_PI / points * c);
    }

    const Matrix W = (V * Dsqrt) * U; // ((2x2)*(2x2))*(2xn) = (2xn)
    // This does the ellipse approximation somehow

    // Obtain relative position if we have ground truth:
    const Pose truth = GroundTruth();
    const Transf ttruth = Transf(truth.x, truth.y, truth.th);
    const Pose local = localize_.pose();
    const Transf tlocal = Transf(local.x, local.y, local.th);
        
    const Transf nu = (sim_ ? TRel(ttruth, tlocal) : Transf(0, 0, 0));

    for (int c1 = 0; c1 < points; c1++)
    {
        const int c2 = (c1 + 1 == points ? 0 : c1 + 1);

        const player_point_2d_t p1 = { W(0, c1) + nu.tX(), W(1, c1) + nu.tY()};
        const player_point_2d_t p2 = { W(0, c2) + nu.tX(), W(1, c2) + nu.tY()};
        DrawLine(black, p1, p2);
    }
    
    // 2 points for angular incertitude
    {
    Matrix Z(2, 2);
    Z(0, 0) = Z(0, 1) = cos(3.84 * 2.0 * sqrt(cov(2, 2)));
    Z(1, 0) =           sin(3.84 * 2.0 * sqrt(cov(2, 2)));
    Z(1, 1) = -Z(1, 0);
    const Matrix WZ = V * Z; // Apply orientation somehow, but don't scale to ellipse
    // This does the ellipse approximation somehow
    const player_point_2d_t ow = { nu.tX(), nu.tY() };
    const player_point_2d_t p1 = { WZ(0, 0) + nu.tX(), WZ(1, 0) + nu.tY()};
    const player_point_2d_t p2 = { WZ(0, 1) + nu.tX(), WZ(1, 1) + nu.tY()};
    DrawLine(blue, ow, p1);
    DrawLine(blue, ow, p2);
    }

    // Axes
    {
    const player_point_2d_t a1 = { nu.tX() - Dsqrt(0, 0) * V(0, 0), nu.tY() - Dsqrt(0, 0) * V(1, 0) };
    const player_point_2d_t a2 = { nu.tX() + Dsqrt(0, 0) * V(0, 0), nu.tY() + Dsqrt(0, 0) * V(1, 0) };
    const player_point_2d_t a3 = { nu.tX() - Dsqrt(1, 1) * V(0, 1), nu.tY() - Dsqrt(1, 1) * V(1, 1) };
    const player_point_2d_t a4 = { nu.tX() + Dsqrt(1, 1) * V(0, 1), nu.tY() + Dsqrt(1, 1) * V(1, 1) };
    DrawLine(black, a1, a2);
    DrawLine(black, a3, a4);
    }
}

void Ekfvloc::DrawDebug(const GuiData &gui_data)
{
    const GuiData &gd = gui_data;

    const player_color_t blue = { 0, 0, 0, 255 };
    
    DrawClear();

    //  Draw laser scans
    {
        const double semigap = laser_gap_ / 2.0;
        for (size_t i = 1; i < gd.laser_rho.size(); i++)
        {
            const double r = gd.laser_rho[i];
            const double p = gd.laser_phi[i];
            DrawLaser(blue, r, p - semigap, r, p + semigap);
        }
    }
    
    {
        for (size_t i = 0; i < gui_data.regions.size(); i++)
        {
            player_color_t color = {0, 168, 168, 168};
            DrawLaser(color, 0.0, 0.0, gd.regions[i].rho0(), gd.regions[i].phi0());
            DrawLaser(color, 0.0, 0.0, gd.regions[i].rho1(), gd.regions[i].phi1());
        }
    }

    {
        for (size_t i = 0; i < gui_data.splits.size(); i++)
        {
            player_color_t color = {0, 0, 200, 0};
            DrawLaser(color,
                      0.5 * gd.splits[i].rho0(), gd.splits[i].phi0(),
                      gd.splits[i].rho0(), gd.splits[i].phi0());
            DrawLaser(color,
                      0.5 * gd.splits[i].rho1(), gd.splits[i].phi1(),
                      gd.splits[i].rho1(), gd.splits[i].phi1());
//             DrawLaser(color,
//                       gd.splits[i].rho0(), gd.splits[i].phi0(),
//                       gd.splits[i].rho1(), gd.splits[i].phi1());
        }
    }

    {
        // Use as reference for the feat either our estimation or ground truth if available
        const Pose estim = localize_.pose();
        const Transf pose(estim.x, estim.y, estim.th);
        const Pose truth = GroundTruth();
        const Transf ttruth = Transf(truth.x, truth.y, truth.th);
        const Transf base = (sim_ ? ttruth : pose);

        for (size_t i = 0; i < gui_data.matches.size(); i++)
        {
            player_color_t color_laser = {0, 255, 0, 0};
            const GuiSplit &m = gd.matches[i]; // current match
            DrawLaser(color_laser,
                      m.rho0(), m.phi0(),
                      m.rho1(), m.phi1());
                      
            const double err = gd.mahala[i];
            const Transf cross(0, 0.5 * err, 0), crossn(0, -0.5 * err, 0);
            const Transf tpini(m.rho0() * cos(m.phi0()), m.rho0() * sin(m.phi0()), 0.0);
            const Transf tpfin(m.rho1() * cos(m.phi1()), m.rho1() * sin(m.phi1()), 0.0);
            const Transf tini(Compose(laser_pose_, tpini));
            const Transf tfin(Compose(laser_pose_, tpfin));
            const Transf tobs((tini.x() + tfin.x()) / 2,
                              (tini.y() + tfin.y()) / 2,
                              atan2(tfin.y() - tini.y(), tfin.x() - tini.x()));

            // Draw error 
            const Transf c1 = Compose(tobs, cross);
            const Transf c2 = Compose(tobs, crossn);
            const player_point_2d_t p1 = {c1.tX(), c1.tY()};
            const player_point_2d_t p2 = {c2.tX(), c2.tY()};
            DrawLine(color_laser, p1, p2);
        }
    }

    DrawEllipse();

    GUI_DATA.Clear();

}

////////////////////////////////////////////////////////////////////////////////
void Ekfvloc::Main()
{
    while (true)
    {
        // Wait till we get new data
        Wait();

        // Test if we are supposed to cancel this thread.
        pthread_testcancel();

        // Process any pending requests.
        ProcessMessages(0);
    }
}

Pose Ekfvloc::GroundTruth(void)
{
    // For some **** reason the simulation driver doesn't track subscriptions
    //   and the player request setup doesn't work. So we have to do it the
    //   ugly way ourselves.
    // That is, we wait for the reply in the ProcessMessage side of things.
    
    if (!sim_)
        return Pose(0.0, 0.0, 0.0);

    player_simulation_pose2d_req_t req =
        { sim_model_.size() + 1, const_cast<char*>(sim_model_.c_str())};

    sim_->PutMsg(InQueue, PLAYER_MSGTYPE_REQ, PLAYER_SIMULATION_REQ_GET_POSE2D,
                 static_cast<void*>(&req), -1, NULL);

    return sim_pose_;
}

void Ekfvloc::PublishInterfaces(double timestamp)
{
    // Publish updated pose
    const Pose new_pose = localize_.pose();
    player_position2d_data_t publish_pose =
    { { new_pose.x, new_pose.y, new_pose.th },
        position_.vel,
        position_.stall
    };
    
    Publish(p2d_addr_, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE,
            static_cast<void*>(&publish_pose));
    PLAYER_MSG4(3, "Ekfvloc loclz pose: %8.3f %8.3f %8.3f (%d)", new_pose.x, new_pose.y, new_pose.th, scan_count_);

    // Publish localize data
    const Matrix cov = localize_.GetCovariance();
    player_localize_hypoth_t hyp[1] =
        {{{new_pose.x, new_pose.y, new_pose.th}, {cov(0, 0), cov(1, 1), cov(2, 2)}, 1.0}};
    //    {{{new_pose.x, new_pose.y, new_pose.th}, {cov(0, 0), cov(1, 1), cov(2, 2),
    //                                              cov(0, 1), cov(1, 2), cov(0,2) }, 1.0}};
    // This was contributed as a patch for the extended localization interface,
    // but I though this had dissapeared at some point (2.x -> 3.x ?)
    // So here it is, but commented.
        
    player_localize_data_t loc_data = { 0, timestamp, 1, hyp};
    Publish(loc_addr_, PLAYER_MSGTYPE_DATA, PLAYER_LOCALIZE_DATA_HYPOTHS,
            static_cast<void*>(&loc_data));

    // Publish full covariance via opaque
    if (publish_cov_)
    {
        cov_pub_.clear();
        for (int i = 0; i < 3; i++)
            for (int j = 0; j < 3; j++)
                cov_pub_.push_back(cov(i, j));

        player_opaque_data_t data = { 9 * sizeof(double),
                                        reinterpret_cast<uint8_t*>(&cov_pub_[0]) };


        Publish(cov_opaque_addr_, PLAYER_MSGTYPE_DATA, PLAYER_OPAQUE_DATA_STATE,
                static_cast<void*>(&data));
    }

    //  Check error if simulating
    if (sim_)
    {
        const Pose truth(GroundTruth());
        const Transf error(TRel(Transf(truth.x, truth.y, truth.th),
                                Transf(new_pose.x, new_pose.y, new_pose.th)));
        PLAYER_MSG3(2, "Ekfvloc: Error: %8.3f %8.3f %8.3f", error.tX(), error.tY(), error.tPhi());

        if (error.Distance(Transf(0.0, 0.0, 0.0)) >= kTruthWarnDistance)
            PLAYER_WARN3("Ekfvloc: Error: %8.3f %8.3f %8.3f", error.tX(), error.tY(), error.tPhi());
    }
}

////////////////////////////////////////////////////////////////////////////////
int Ekfvloc::ProcessMessage(QueuePointer &resp_queue, player_msghdr * hdr, void * data)
{
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_POSITION2D_DATA_STATE, odom_addr_))
    {
        const player_position2d_data_t &p2d = *static_cast<player_position2d_data_t *>(data);
        position_  = p2d;
        PLAYER_MSG3(3, "Ekfvloc odomz pose: %8.3f %8.3f %8.3f", p2d.pos.px, p2d.pos.py, p2d.pos.pa);

        if (!have_pose_)
        {
            have_pose_ = true;
            localize_.SetPoses (p2d.pos.px,             p2d.pos.py,             p2d.pos.pa,
                                global_initial_pose_.x, global_initial_pose_.y, global_initial_pose_.th);
        }

        const Pose sim_pose = GroundTruth();
        if ((sim_pose.x != 0) || (sim_pose.y != 0) || (sim_pose.th != 0))
            PLAYER_MSG3(3, "Ekfvloc truth pose: %8.3f %8.3f %8.3f", sim_pose.x, sim_pose.y, sim_pose.th);

        return 0;
    }

    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_LASER_DATA_SCAN, laser_addr_))
    {
        const player_laser_data_t &scan = *static_cast<player_laser_data_t *>(data);

        if (!CheckElapsed())
        {
            PLAYER_WARN("Ekfvloc: Laserscans arriving too fast");
        }
        else if (have_pose_)
        {
            DoublesVector ranges_, bearings_;

            scan_count_++;

            // Process the data
            double angle = scan.min_angle;
            const double gap   = (scan.max_angle - scan.min_angle) /
                    (scan.ranges_count - 1);
            laser_gap_ = gap;

            GUI_DATA.laser_rho.clear();
            GUI_DATA.laser_phi.clear();
            
            for (int i = 0; i < static_cast<int>(scan.ranges_count); i++)
            {
                ranges_.push_back(scan.ranges[i]);
                bearings_.push_back(angle);
                GUI_DATA.laser_rho.push_back(scan.ranges[i]);
                GUI_DATA.laser_phi.push_back(angle);
                angle += gap;
            }
                
            if (localize_.Update(position_.pos.px, position_.pos.py, position_.pos.pa, ranges_, bearings_))
            {
                if (debug_sock_ != -1)
                    SendDebug(GUI_DATA);
                if (use_g2d_)
                    DrawDebug(GUI_DATA);
            }
            
            PublishInterfaces(hdr->timestamp);
        }
        else
        {
            PLAYER_WARN("Received scan but pose is unknown yet");
        }

        return 0;
    }

    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_VEL, p2d_addr_))
    {
        // make a copy of the header and change the address
        player_msghdr_t newhdr = *hdr;
        newhdr.addr = odom_addr_;
        odom_->PutMsg(InQueue, &newhdr, (void*)data);

        return 0;
    }

    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, PLAYER_POSITION2D_CMD_POS, p2d_addr_))
    {
        // make a copy of the header and change the address
        player_msghdr_t newhdr = *hdr;
        newhdr.addr = odom_addr_;

        // Change the target pose to odom reference frame
        const player_position2d_cmd_pos_t &glob_target =
            *static_cast<player_position2d_cmd_pos_t*>(data);

        // Transformation from localized reference to odometry reference
        const Transf odom_pose_ref(position_.pos.px, position_.pos.py, position_.pos.pa);
        const Transf glob_pose_ref(localize_.pose().x, localize_.pose().y, localize_.pose().th);
        const Transf glob_trgt_ref(glob_target.pos.px, glob_target.pos.py, glob_target.pos.pa);
        const Transf odom_trgt_ref = Compose(odom_pose_ref,
                                             TRel(glob_pose_ref, glob_trgt_ref));

        // Marshalling
        player_position2d_cmd_pos_t odom_target = glob_target;
        odom_target.pos.px = odom_trgt_ref.tX();
        odom_target.pos.py = odom_trgt_ref.tY();
        odom_target.pos.pa = odom_trgt_ref.tPhi();

        odom_->PutMsg(InQueue, &newhdr, static_cast<void*>(&odom_target));
        
        //printf("TARGET (%8.3f %8.3f %8.3f)-->(%8.3f %8.3f %8.3f)\n",
        //       glob_target.pos.px, glob_target.pos.py, glob_target.pos.pa,
        //       odom_target.pos.px, odom_target.pos.py, odom_target.pos.pa);

        return 0;
    }

    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1, p2d_addr_))
    {
        // Pass the request on to the underlying position device and wait for the reply.
        Message* msg;

        if (!(msg = odom_->Request(
                    InQueue,
                    hdr->type,
                    hdr->subtype,
                    (void*)data,
                    hdr->size,
                    &hdr->timestamp)))
        {
            PLAYER_WARN1("failed to forward config request with subtype: %d\n",
                    hdr->subtype);
            return -1;
        }

        player_msghdr_t* rephdr = msg->GetHeader();

        void* repdata = msg->GetPayload();
        // Copy in our address and forward the response
        rephdr->addr = p2d_addr_;
        this->Publish(resp_queue, rephdr, repdata);
        delete msg;
        return 0;
    }

    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_ACK, PLAYER_SIMULATION_REQ_GET_POSE2D, sim_addr_))
    {
        //  The ground truth pose is back
        const player_simulation_pose2d_req_t &reply =
            *static_cast<const player_simulation_pose2d_req_t *>(data);
            
        sim_pose_ = Pose(reply.pose.px, reply.pose.py, reply.pose.pa);
        
        return 0;
    }

    //  Dismiss clear messages


    return -1;
}
