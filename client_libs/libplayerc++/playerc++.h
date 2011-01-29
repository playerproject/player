/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000-2003
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
/********************************************************************
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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 ********************************************************************/

/***************************************************************************
 * Desc: Player v2.0 C++ client
 * Authors: Brad Kratochvil, Toby Collett
 *
 * Date: 23 Sep 2005
 # CVS: $Id$
 **************************************************************************/


#ifndef PLAYERCC_H
#define PLAYERCC_H

#include <cstddef>
#include <cmath>
#include <string>
#include <list>
#include <vector>
#include <cstring>

#include "libplayerc/playerc.h"
#include "libplayerc++/utility.h"
#include "libplayerc++/playerclient.h"
#include "libplayerc++/playererror.h"
#include "libplayerc++/clientproxy.h"
#include "libplayerinterface/interface_util.h"

#if defined (WIN32)
  #if defined (PLAYER_STATIC)
    #define PLAYERCC_EXPORT
  #elif defined (playerc___EXPORTS)
    #define PLAYERCC_EXPORT    __declspec (dllexport)
  #else
    #define PLAYERCC_EXPORT    __declspec (dllimport)
  #endif
#else
  #define PLAYERCC_EXPORT
#endif

// Don't think we need to include these here
/*
#ifdef HAVE_BOOST_SIGNALS
  #include <boost/signal.hpp>
  #include <boost/bind.hpp>
#endif

#ifdef HAVE_BOOST_THREAD
  #include <boost/thread/mutex.hpp>
  #include <boost/thread/thread.hpp>
  #include <boost/thread/xtime.hpp>
#endif
*/

namespace PlayerCc
{

// /**
// * The @p SomethingProxy class is a template for adding new subclasses of
// * ClientProxy.  You need to have at least all of the following:
// */
// class SomethingProxy : public ClientProxy
// {
//
//   private:
//
//     // Subscribe
//     void Subscribe(uint32_t aIndex);
//     // Unsubscribe
//     void Unsubscribe();
//
//     // libplayerc data structure
//     playerc_something_t *mDevice;
//
//   public:
//     // Constructor
//     SomethingProxy(PlayerClient *aPc, uint32_t aIndex=0);
//     // Destructor
//     ~SomethingProxy();
//
// };

/** @ingroup player_clientlib_cplusplus
 * @addtogroup player_clientlib_cplusplus_proxies Proxies
 * @brief A proxy class is associated with each kind of device

  The proxies all inherit from @p ClientProxy and implement the functions
  from @ref player_clientlib_libplayerc.

 @{

 */

// ==============================================================
//
// These are alphabetized, please keep them that way!!!
//
// ==============================================================

/**
The @p ActArrayProxy class is used to control a @ref interface_actarray
device.
 */
class PLAYERCC_EXPORT ActArrayProxy : public ClientProxy
{
  private:

   void Subscribe(uint32_t aIndex);
   void Unsubscribe();

   // libplayerc data structure
   playerc_actarray_t *mDevice;

  public:

    /// Default constructor
    ActArrayProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Default destructor
    ~ActArrayProxy();

    /// Geometry request - call before getting the
    /// geometry of a joint through the accessor method
    void RequestGeometry(void);

    /// Power control
    void SetPowerConfig(bool aVal);
    /// Brakes control
    void SetBrakesConfig(bool aVal);
    /// Speed control
    void SetSpeedConfig(uint32_t aJoint, float aSpeed);
    /// Acceleration control
    void SetAccelerationConfig(uint32_t aJoint, float aAcc);

    /// Send an actuator to a position
    void MoveTo(uint32_t aJoint, float aPos);
    /// Send actuators 0 thru n to the designated positions
    void MoveToMulti(std::vector<float> aPos);
    /// Move an actuator at a speed
    void MoveAtSpeed(uint32_t aJoint, float aSpeed);
    /// Move actuators 0 thru n at the designated speeds
    void MoveAtSpeedMulti(std::vector<float> aSpeed);
    /// Send an actuator, or all actuators, home
    void MoveHome(int aJoint);
    /// Set an actuator to a given current
    void SetActuatorCurrent(uint32_t aJoint, float aCurrent);
    /// Set actuators 0 thru n to the given currents
    void SetActuatorCurrentMulti(std::vector<float> aCurrent);

    /// Gets the number of actuators in the array
    uint32_t GetCount(void) const { return GetVar(mDevice->actuators_count); }
    /// Accessor method for getting an actuator's data
    player_actarray_actuator_t GetActuatorData(uint32_t aJoint) const;
    /// Same again for getting actuator geometry
    player_actarray_actuatorgeom_t GetActuatorGeom(uint32_t aJoint) const;
    /// Accessor method for getting the base position
    player_point_3d_t GetBasePos(void) const { return GetVar(mDevice->base_pos); }
    /// Accessor method for getting the base orientation
    player_orientation_3d_t GetBaseOrientation(void) const { return GetVar(mDevice->base_orientation); }


    /// Actuator data access operator.
    ///    This operator provides an alternate way of access the actuator data.
    ///    For example, given a @p ActArrayProxy named @p ap, the following
    ///    expressions are equivalent: @p ap.GetActuatorData[0] and @p ap[0].
    player_actarray_actuator_t operator [](uint32_t aJoint)
      { return(GetActuatorData(aJoint)); }
};

/**
The @p AioProxy class is used to read from a @ref interface_aio
(analog I/O) device.  */
class PLAYERCC_EXPORT AioProxy : public ClientProxy
{
  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_aio_t *mDevice;

  public:

    /// Constructor
    AioProxy (PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~AioProxy();

    /// Get number of voltages
    uint32_t GetCount() const { return(GetVar(mDevice->voltages_count)); };

    /// Get an input voltage 
    double GetVoltage(uint32_t aIndex)  const
      { return(GetVar(mDevice->voltages[aIndex])); };

    /// Set an output voltage
    void SetVoltage(uint32_t aIndex, double aVoltage);

    /// AioProxy data access operator.
    ///    This operator provides an alternate way of access the voltage data.
    ///    For example, given a @p AioProxy named @p bp, the following
    ///    expressions are equivalent: @p ap.GetVoltage(0) and @p ap[0].
    double operator [](uint32_t aIndex) const
      { return GetVoltage(aIndex); }

};


/**
The @p AudioProxy class controls an @ref interface_audio device.
*/
class PLAYERCC_EXPORT AudioProxy : public ClientProxy
{

  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_audio_t *mDevice;

  public:

    /// Constructor
    AudioProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~AudioProxy();

    /** @brief Get Mixer Details Count */
    uint32_t GetMixerDetailsCount() const {return(GetVar(mDevice->channel_details_list.details_count));};
    /** @brief Get Mixer Detail */
    player_audio_mixer_channel_detail_t GetMixerDetails(int aIndex) const  {return(GetVar(mDevice->channel_details_list.details[aIndex]));};
    /** @brief Get Default output Channel */
    uint32_t GetDefaultOutputChannel() const  {return(GetVar(mDevice->channel_details_list.default_output));};
    /** @brief Get Default input Channel */
    uint32_t GetDefaultInputChannel() const {return(GetVar(mDevice->channel_details_list.default_input));};

    /** @brief Get Wav data length */
    uint32_t GetWavDataLength() const {return(GetVar(mDevice->wav_data.data_count));};
    /// @brief Get Wav data
    /// This function copies the wav data into the buffer aImage.
    /// The buffer should be allocated according to the length of the wav data
    /// The size can be found by calling @ref GetWavDataLength().
    void GetWavData(uint8_t* aData) const
      {
        return GetVarByRef(mDevice->wav_data.data,
                           mDevice->wav_data.data+GetWavDataLength(),
                           aData);
      };

    /** @brief Get Seq data count */
    uint32_t GetSeqCount() const {return(GetVar(mDevice->seq_data.tones_count));};
    /** @brief Get Sequence item */
    player_audio_seq_item_t GetSeqItem(int aIndex) const  {return(GetVar(mDevice->seq_data.tones[aIndex]));};

    /** @brief Get Channel data count */
    uint32_t GetChannelCount() const {return(GetVar(mDevice->mixer_data.channels_count));};
    /** @brief Get Sequence item */
    player_audio_mixer_channel_t GetChannel(int aIndex) const  {return(GetVar(mDevice->mixer_data.channels[aIndex]));};
    /** @brief Get driver state */
    uint32_t GetState(void) const {return(GetVar(mDevice->state));};



    /** @brief Command to play an audio block */
    void PlayWav(uint32_t aDataCount, uint8_t *aData, uint32_t aFormat);

    /** @brief Command to set recording state */
    void SetWavStremRec(bool aState);

    /** @brief Command to play prestored sample */
    void PlaySample(int aIndex);

    /** @brief Command to play sequence of tones */
    void PlaySeq(player_audio_seq_t * aTones);

    /** @brief Command to set multiple mixer levels */
    void SetMultMixerLevels(player_audio_mixer_channel_list_t * aLevels);

    /** @brief Command to set a single mixer level */
    void SetMixerLevel(uint32_t index, float amplitude, uint8_t active);

    /** @brief Request to record a single audio block
    result is stored in wav_data */
    void RecordWav();

    /** @brief Request to load an audio sample */
    void LoadSample(int aIndex, uint32_t aDataCount, uint8_t *aData, uint32_t aFormat);

    /** @brief Request to retrieve an audio sample
      Data is stored in wav_data */
    void GetSample(int aIndex);

    /** @brief Request to record new sample */
    void RecordSample(int aIndex, uint32_t aLength);

    /** @brief Request mixer channel data
    result is stored in mixer_data*/
    void GetMixerLevels();

    /** @brief Request mixer channel details list
    result is stored in channel_details_list*/
    void GetMixerDetails();

};

/**
 * The BlackBoardProxy class is used to subscribe to a blackboard device.
 * A blackboard is a data-store which sends updates when an entry is changed.
 * It also returns the current value of an entry when a proxy first subcribes
 * to that entries key.
 * If an entry does not exist, the default value of that entry is returned.
 */
class PLAYERCC_EXPORT BlackBoardProxy : public ClientProxy
{
  private:
    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_blackboard_t *mDevice;

  public:
	  /** Constructor */
  	BlackBoardProxy(PlayerClient *aPc, uint32_t aIndex=0);
  	/** Destructor */
  	~BlackBoardProxy();
  	/** Subscribe to a key. If the key does not exist the default value is returned. The user must free the entry. */
  	player_blackboard_entry_t *SubscribeToKey(const char *key, const char* group = "");
  	/** Stop receiving updates about this key. */
  	void UnsubscribeFromKey(const char *key, const char* group = "");
  	/** Subscribe to a group. The event handler must be set to retrieve the current group entries. */
  	void SubscribeToGroup(const char* key);
  	/** Stop receiving updates about this group. */
  	void UnsubscribeFromGroup(const char* group);
  	/** Set a key value */
  	void SetEntry(const player_blackboard_entry_t &entry);
  	/** Get a value for a key */
  	player_blackboard_entry_t *GetEntry(const char* key, const char* group);
  	/** Set the function pointer which will be called when an entry is updated. */
  	void SetEventHandler(void (*on_blackboard_event)(playerc_blackboard_t *, player_blackboard_entry_t));
};

// /**
// The @p BlinkenlightProxy class is used to enable and disable
// a flashing indicator light, and to set its period, via a @ref
// interface_blinkenlight device */
// class PLAYERCC_EXPORT BlinkenLightProxy : public ClientProxy
// {
//   private:
//
//     void Subscribe(uint32_t aIndex);
//     void Unsubscribe();
//
//     // libplayerc data structure
//     playerc_blinkenlight_t *mDevice;
//
//   public:
//     /** Constructor.
//         Leave the access field empty to start unconnected.
//     */
//     BlinkenLightProxy(PlayerClient *aPc, uint32_t aIndex=0);
//     ~BlinkenLightProxy();
//
//     // true: indicator light enabled, false: disabled.
//     bool GetEnable();
//
//     /** The current period (one whole on/off cycle) of the blinking
//         light. If the period is zero and the light is enabled, the light
//         is on.
//     */
//     void SetPeriod(double aPeriod);
//
//     /** Set the state of the indicator light. A period of zero means
//         the light will be unblinkingly on or off. Returns 0 on
//         success, else -1.
//       */
//     void SetEnable(bool aSet);
// };

/**
The @p BlobfinderProxy class is used to control a  @ref
interface_blobfinder device.  It contains no methods.  The latest
color blob data is stored in @p blobs, a dynamically allocated 2-D array,
indexed by color channel.
*/
class PLAYERCC_EXPORT BlobfinderProxy : public ClientProxy
{
  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_blobfinder_t *mDevice;

  public:
    /// Constructor
    BlobfinderProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~BlobfinderProxy();

    /// returns the number of blobs
    uint32_t GetCount() const { return GetVar(mDevice->blobs_count); };
    /// returns a blob
    playerc_blobfinder_blob_t GetBlob(uint32_t aIndex) const
      { return GetVar(mDevice->blobs[aIndex]);};

    /// get the width of the image
    uint32_t GetWidth() const { return GetVar(mDevice->width); };
    /// get the height of the image
    uint32_t GetHeight() const { return GetVar(mDevice->height); };

    /// Blobfinder data access operator.
    ///    This operator provides an alternate way of access the actuator data.
    ///    For example, given a @p BlobfinderProxy named @p bp, the following
    ///    expressions are equivalent: @p bp.GetBlob[0] and @p bp[0].
    playerc_blobfinder_blob_t operator [](uint32_t aIndex) const
      { return(GetBlob(aIndex)); }

/*
    // Set the color to be tracked
    void SetTrackingColor(uint32_t aReMin=0,   uint32_t aReMax=255, uint32_t aGrMin=0,
                          uint32_t aGrMax=255, uint32_t aBlMin=0,   uint32_t aBlMax=255);
    void SetImagerParams(int aContrast, int aBrightness,
                         int aAutogain, int aColormode);
    void SetContrast(int aC);
    void SetColorMode(int aM);
    void SetBrightness(int aB);
    void SetAutoGain(int aG);*/

};

/**
The @p BumperProxy class is used to read from a @ref
interface_bumper device.
*/
class PLAYERCC_EXPORT BumperProxy : public ClientProxy
{

  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_bumper_t *mDevice;

  public:

    /// Constructor
    BumperProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~BumperProxy();

    /// Returns how many bumpers are in underlying device
    uint32_t GetCount() const { return GetVar(mDevice->bumper_count); };

    /// Returns true if the specified bumper has been bumped, false otherwise.
    uint32_t IsBumped(uint32_t aIndex) const
      { return GetVar(mDevice->bumpers[aIndex]); };

    /// Returns true if any bumper has been bumped, false otherwise.
    bool IsAnyBumped();

    /// Requests the geometries of the bumpers.
    void RequestBumperConfig();

    /// Returns the number bumper poses
    uint32_t GetPoseCount() const { return GetVar(mDevice->pose_count); };

    /// Returns a specific bumper pose
    player_bumper_define_t GetPose(uint32_t aIndex) const
      { return GetVar(mDevice->poses[aIndex]); };

    /// BumperProxy data access operator.
    ///    This operator provides an alternate way of access the actuator data.
    ///    For example, given a @p BumperProxy named @p bp, the following
    ///    expressions are equivalent: @p bp.IsBumped[0] and @p bp[0].
    bool operator [](uint32_t aIndex) const
	  { return IsBumped(aIndex) != 0 ? true : false; }

};

/**
The @p CameraProxy class can be used to get images from a @ref
interface_camera device. */
class PLAYERCC_EXPORT CameraProxy : public ClientProxy
{

  private:

    virtual void Subscribe(uint32_t aIndex);
    virtual void Unsubscribe();

    // libplayerc data structure
    playerc_camera_t *mDevice;

    std::string mPrefix;
    int mFrameNo;

  public:

    /// Constructor
    CameraProxy (PlayerClient *aPc, uint32_t aIndex=0);

    /// Destructor
    virtual ~CameraProxy();

    /// Save the frame
    /// @arg aPrefix is the string prefix to name the image.
    /// @arg aWidth is the number of 0s to pad the image numbering with.
    void SaveFrame(const std::string aPrefix, uint32_t aWidth=4);

    /// decompress the image
    void Decompress();

    /// Image color depth
    uint32_t GetDepth() const { return GetVar(mDevice->bpp); };

    /// Image dimensions (pixels)
    uint32_t GetWidth() const { return GetVar(mDevice->width); };

    /// Image dimensions (pixels)
    uint32_t GetHeight() const { return GetVar(mDevice->height); };

    /// @brief Image format
    /// Possible values include
    /// - @ref PLAYER_CAMERA_FORMAT_MONO8
    /// - @ref PLAYER_CAMERA_FORMAT_MONO16
    /// - @ref PLAYER_CAMERA_FORMAT_RGB565
    /// - @ref PLAYER_CAMERA_FORMAT_RGB888
    uint32_t GetFormat() const { return GetVar(mDevice->format); };

    /// Size of the image (bytes)
    uint32_t GetImageSize() const { return GetVar(mDevice->image_count); };

    /// @brief Image data
    /// This function copies the image data into the data buffer aImage.
    /// The buffer should be allocated according to the width, height, and
    /// depth of the image.  The size can be found by calling @ref GetImageSize().
    void GetImage(uint8_t* aImage) const
      {
        return GetVarByRef(mDevice->image,
                           mDevice->image+GetVar(mDevice->image_count),
                           aImage);
      };

    /// @brief What is the compression type?
    /// Currently supported compression types are:
    /// - @ref PLAYER_CAMERA_COMPRESS_RAW
    /// - @ref PLAYER_CAMERA_COMPRESS_JPEG
    uint32_t GetCompression() const { return GetVar(mDevice->compression); };

};


/**
The @p DioProxy class is used to read from a @ref interface_dio
(digital I/O) device.
*/
class PLAYERCC_EXPORT DioProxy : public ClientProxy
{
  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_dio_t *mDevice;

  public:
    /// Constructor
    DioProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~DioProxy();

    /// The number of valid digital inputs.
    uint32_t GetCount() const { return GetVar(mDevice->count); };

    /// A bitfield of the current digital inputs.
    uint32_t GetDigin() const { return GetVar(mDevice->digin); };

    /// Get a specific bit
    bool GetInput(uint32_t aIndex) const;

    /// Set the output to the bitfield aDigout
    void SetOutput(uint32_t aCount, uint32_t aDigout);

    /// DioProxy data access operator.
    ///    This operator provides an alternate way of access the dio data.
    ///    For example, given a @p DioProxy named @p dp, the following
    ///    expressions are equivalent: @p dp.GetInput(0) and @p dp[0].
    uint32_t operator [](uint32_t aIndex) const
      { return GetInput(aIndex); }
};

/**
The @p FiducialProxy class is used to control @ref
interface_fiducial devices.  The latest set of detected beacons
is stored in the @p beacons array.
*/
class PLAYERCC_EXPORT FiducialProxy : public ClientProxy
{
  private:
    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_fiducial_t *mDevice;

  public:
    /// Constructor
    FiducialProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~FiducialProxy();

    /// The number of beacons detected
    uint32_t GetCount() const { return GetVar(mDevice->fiducials_count); };

    /// Get detected beacon description
    player_fiducial_item_t GetFiducialItem(uint32_t aIndex) const
      { return GetVar(mDevice->fiducials[aIndex]);};

    /// The pose of the sensor
    player_pose3d_t GetSensorPose() const
      { return GetVar(mDevice->fiducial_geom.pose);};

    /// The size of the sensor
    player_bbox3d_t GetSensorSize() const
      { return GetVar(mDevice->fiducial_geom.size);};

    /// The size of the most recently detected fiducial
    player_bbox2d_t GetFiducialSize() const
      { return GetVar(mDevice->fiducial_geom.fiducial_size);};

    /// Get the sensor's geometry configuration
    void RequestGeometry();

    /// FiducialProxy data access operator.
    ///    This operator provides an alternate way of access the actuator data.
    ///    For example, given a @p FiducialProxy named @p fp, the following
    ///    expressions are equivalent: @p fp.GetFiducialItem[0] and @p fp[0].
    player_fiducial_item_t operator [](uint32_t aIndex) const
      { return GetFiducialItem(aIndex); }
};

/**
The @p GpsProxy class is used to control a @ref interface_gps
device.  The latest pose data is stored in three class attributes.  */
class PLAYERCC_EXPORT GpsProxy : public ClientProxy
{

  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_gps_t *mDevice;

  public:

    /// Constructor
    GpsProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~GpsProxy();

    /// Latitude and longitude, in degrees.
    double GetLatitude() const { return GetVar(mDevice->lat); };
    double GetLongitude() const { return GetVar(mDevice->lon); };

    /// Altitude, in meters.
    double GetAltitude() const { return GetVar(mDevice->alt); };

    /// Number of satellites in view.
    uint32_t GetSatellites() const { return GetVar(mDevice->sat_count); };

    /// Fix quality
    uint32_t GetQuality() const { return GetVar(mDevice->quality); };

    /// Horizontal dilution of position (HDOP)
    double GetHdop() const { return GetVar(mDevice->hdop); };

    /// Vertical dilution of position (HDOP)
    double GetVdop() const { return GetVar(mDevice->vdop); };

    /// UTM easting and northing (meters).
    double GetUtmEasting() const { return GetVar(mDevice->utm_e); };
    double GetUtmNorthing() const { return GetVar(mDevice->utm_n); };

    /// Time, since the epoch
    double GetTime() const { return GetVar(mDevice->utc_time); };

    /// Errors
    double GetErrHorizontal() const { return GetVar(mDevice->err_horz); };
    double GetErrVertical() const { return GetVar(mDevice->err_vert); };
};

/**
 * The @p Graphics2dProxy class is used to draw simple graphics into a
 * rendering device provided by Player using the graphics2d
 * interface. For example, the Stage plugin implements this interface
 * so you can draw into the Stage window. This is very useful to
 * visualize what's going on in your controller.
 */
class PLAYERCC_EXPORT Graphics2dProxy : public ClientProxy
{

  private:

    // Subscribe
    void Subscribe(uint32_t aIndex);
    // Unsubscribe
    void Unsubscribe();

    // libplayerc data structure
    playerc_graphics2d_t *mDevice;

  public:
    /// Constructor
    Graphics2dProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~Graphics2dProxy();

    /// Set the current pen color
    void Color(player_color_t col);

    /// Set the current pen color
    void Color(uint8_t red,  uint8_t green,  uint8_t blue,  uint8_t alpha);

    /// Clear the drawing area
    void Clear(void);

    /// Draw a set of points
    void DrawPoints(player_point_2d_t pts[], int count);

    /// Draw a polygon defined by a set of points
    void DrawPolygon(player_point_2d_t pts[],
                     int count,
                     bool filled,
                     player_color_t fill_color);

    /// Draw a line connecting  set of points
    void DrawPolyline(player_point_2d_t pts[], int count);


    /// Draw multiple lines. Lines ending points are at pts[2n] pts[2n+1] where 2n+1<count
    void DrawMultiline(player_point_2d_t pts[], int count);
};

/**
 * The @p Graphics3dProxy class is used to draw simple graphics into a
 * rendering device provided by Player using the graphics3d
 * interface.
 */
class PLAYERCC_EXPORT Graphics3dProxy : public ClientProxy
{

  private:

    // Subscribe
    void Subscribe(uint32_t aIndex);
    // Unsubscribe
    void Unsubscribe();

    // libplayerc data structure
    playerc_graphics3d_t *mDevice;

  public:
    /// Constructor
    Graphics3dProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~Graphics3dProxy();

    /// Set the current pen color
    void Color(player_color_t col);

    /// Set the current pen color
    void Color(uint8_t red,  uint8_t green,  uint8_t blue,  uint8_t alpha);

    /// Clear the drawing area
    void Clear(void);

    /// Draw a set of verticies
    void Draw(player_graphics3d_draw_mode_t mode, player_point_3d_t pts[], int count);

};

/**
The @p GripperProxy class is used to control a @ref interface_gripper device.
The latest gripper data is held in a handful of class attributes.
*/
class PLAYERCC_EXPORT GripperProxy : public ClientProxy
{

  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_gripper_t *mDevice;

  public:

    /// Constructor
    GripperProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~GripperProxy();

    /// Geometry request - call before getting the
    /// geometry of the gripper through an accessor method
    void RequestGeometry(void);

    /// Get the gripper state
    uint32_t GetState() const { return GetVar(mDevice->state); };
    /// Get the gripper break beam info
    uint32_t GetBeams() const { return GetVar(mDevice->beams); };
    /// Get the pose of the gripper
    player_pose3d_t GetPose() const { return GetVar(mDevice->pose); };
    /// Get the outer size of the gripper
    player_bbox3d_t GetOuterSize() const { return GetVar(mDevice->outer_size); };
    /// Get the inner size of the gripper
    player_bbox3d_t GetInnerSize() const { return GetVar(mDevice->inner_size); };
    /// Get the number of breakbeams in the gripper
    uint32_t GetNumBeams() const { return GetVar(mDevice->num_beams); };
    /// Get the capacity of the gripper's storage
    uint32_t GetCapacity() const { return GetVar(mDevice->capacity); };
    /// Get the number of currently-stored objects
    uint32_t GetStored() const { return GetVar(mDevice->stored); };

    /// Command the gripper to open
    void Open();
    /// Command the gripper to close
    void Close();
    /// Command the gripper to stop
    void Stop();
    /// Command the gripper to store
    void Store();
    /// Command the gripper to retrieve
    void Retrieve();
};

/**
The @p HealthProxy class is used to get infos of the player-server. */
class PLAYERCC_EXPORT HealthProxy : public ClientProxy
{

  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_health_t *mDevice;

  public:
    /// Constructor
    HealthProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~HealthProxy();

    /// Get idle CPU load in percents
    float GetIdleCPU();

    /// Get system CPU load in percents
    float GetSystemCPU();

    /// Get user CPU load in percents
    float GetUserCPU();

    /// Get total amount of memory
    int64_t GetMemTotal();

    /// Get amount of memory used
    int64_t GetMemUsed();

    /// Get amount of free memory
    int64_t GetMemFree();

    /// Get total amount of swap
    int64_t GetSwapTotal();

    /// Get amount of swap used
    int64_t GetSwapUsed();

    /// Get amount of free swap space
    int64_t GetSwapFree();

    /// Get percentage of used RAM
    float GetPercMemUsed();

    /// Get percentage of used SWAP
    float GetPercSwapUsed();

    /// Get percentage of totally used memory (swap and ram)
    float GetPercTotalUsed();
};



/**
The @p ImuProxy class is used to control an @ref interface_imu
device.
 */
class PLAYERCC_EXPORT ImuProxy : public ClientProxy
{
  private:
    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_imu_t *mDevice;

  public:

    /// Constructor
    ImuProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~ImuProxy();

    /// Get the processed pose of the imu
    player_pose3d_t GetPose() const { return GetVar(mDevice->pose); };
    
    /// Get X Acceleration
    float GetXAccel();
    /// Get Y Acceleration
    float GetYAccel();
    /// Get Z Acceleration
    float GetZAccel();
    /// Get X Gyro Rate
    float GetXGyro();
    /// Get Y Gyro Rate
    float GetYGyro();
    /// Get Z Gyro Rate
    float GetZGyro();
    /// Get X Magnetism
    float GetXMagn();
    /// Get Y Magnetism
    float GetYMagn();
    /// Get Z Magnetism
    float GetZMagn();

    /// Get all calibrated values
    player_imu_data_calib_t GetRawValues() const
    { return GetVar(mDevice->calib_data); };

    /** Change the data type to one of the predefined data structures. */
    void SetDatatype(int aDatatype);

    /**  Reset orientation. */
    void ResetOrientation(int aValue);


};


/**
The @p IrProxy class is used to control an @ref interface_ir
device.
*/
class PLAYERCC_EXPORT IrProxy : public ClientProxy
{

  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_ir_t *mDevice;

  public:

    /// Constructor
    IrProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~IrProxy();

    /// get the number of IR rangers
    uint32_t GetCount() const { return GetVar(mDevice->data.ranges_count); };
    /// get the current range
    double GetRange(uint32_t aIndex) const
      { return GetVar(mDevice->data.ranges[aIndex]); };
    /// get the current voltage
    double GetVoltage(uint32_t aIndex) const
      { return GetVar(mDevice->data.voltages[aIndex]); };
    /// get the number of poses
    uint32_t GetPoseCount() const { return GetVar(mDevice->poses.poses_count); };
    /// get a particular pose
    player_pose3d_t GetPose(uint32_t aIndex) const
      {return GetVar(mDevice->poses.poses[aIndex]);};

    /// Request IR pose information
    void RequestGeom();

    /// Range access operator.
    /// This operator provides an alternate way of access the range data.
    /// For example, given a @p IrProxy named @p ip, the following
    /// expressions are equivalent: @p ip.GetRange[0] and @p ip[0].
    double operator [](uint32_t aIndex) const
      { return GetRange(aIndex); }

};

/**
The @p LaserProxy class is used to control a @ref interface_laser
device.  The latest scan data is held in two arrays: @p ranges and @p
intensity.  The laser scan range, resolution and so on can be configured
using the Configure() method.  */
class PLAYERCC_EXPORT LaserProxy : public ClientProxy
{
  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_laser_t *mDevice;

    // local storage of config
    double min_angle, max_angle, scan_res, range_res, scanning_frequency;
    bool intensity;

  public:

    /// Constructor
    LaserProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~LaserProxy();

    /// Number of points in scan
    uint32_t GetCount() const { return GetVar(mDevice->scan_count); };

    /// Max range for the latest set of data (meters)
    double GetMaxRange() const { return GetVar(mDevice->max_range); };

    /// Angular resolution of scan (radians)
    double GetScanRes() const { return GetVar(mDevice->scan_res); };

    /// Range resolution of scan (mm)
    double GetRangeRes() const { return GetVar(mDevice->range_res); };

    /// Scanning Frequency (Hz)
    double GetScanningFrequency() const { return GetVar(mDevice->scanning_frequency); };

    /// Scan range for the latest set of data (radians)
    double GetMinAngle() const { return GetVar(mDevice->scan_start); };
    /// Scan range for the latest set of data (radians)
    double GetMaxAngle() const
    {
      scoped_lock_t lock(mPc->mMutex);
      return mDevice->scan_start + (mDevice->scan_count - 1)*mDevice->scan_res;
    };

    /// Scan range from the laser config (call RequestConfigure first) (radians)
    double GetConfMinAngle() const { return min_angle; };
    /// Scan range from the laser config (call RequestConfigure first) (radians)
    double GetConfMaxAngle() const { return max_angle; };

    /// Whether or not reflectance (i.e., intensity) values are being returned.
    bool IntensityOn() const { return GetVar(mDevice->intensity_on) != 0 ? true : false; };

//    // Scan data (polar): range (m) and bearing (radians)
//    double GetScan(uint32_t aIndex) const
//      { return GetVar(mDevice->scan[aIndex]); };

    /// Scan data (Cartesian): x,y (m)
    player_point_2d_t GetPoint(uint32_t aIndex) const
      { return GetVar(mDevice->point[aIndex]); };


    /// get the range
    double GetRange(uint32_t aIndex) const
      { return GetVar(mDevice->ranges[aIndex]); };

    /// get the bearing
    double GetBearing(uint32_t aIndex) const
      { return GetVar(mDevice->scan[aIndex][1]); };


    /// get the intensity
    int GetIntensity(uint32_t aIndex) const
      { return GetVar(mDevice->intensity[aIndex]); };

    /// get the laser ID, call RequestId first
    int GetID() const
      { return GetVar(mDevice->laser_id); };


    /// Configure the laser scan pattern.  Angles @p min_angle and
    /// @p max_angle are measured in radians.
    /// @p scan_res is measured in units of 0.01 degrees;
    /// valid values are: 25 (0.25 deg), 50 (0.5 deg) and
    /// 100 (1 deg).  @p range_res is measured in mm; valid values
    /// are: 1, 10, 100.  Set @p intensity to @p true to
    /// enable intensity measurements, or @p false to disable.
    /// @p scanning_frequency is measured in Hz
    void Configure(double aMinAngle,
                   double aMaxAngle,
                   uint32_t aScanRes,
                   uint32_t aRangeRes,
                   bool aIntensity,
                   double aScanningFrequency);

    /// Request the current laser configuration; it is read into the
    /// relevant class attributes.
    void RequestConfigure();

    /// Request the ID of the laser; read it with GetID()
    void RequestID();

    /// Get the laser's geometry; it is read into the
    /// relevant class attributes.
    void RequestGeom();

    /// Accessor for the pose of the laser with respect to its parent
    /// object (e.g., a robot).  Fill it in by calling RequestGeom.
    player_pose3d_t GetPose()
    {
      player_pose3d_t p;
      scoped_lock_t lock(mPc->mMutex);

      p.px = mDevice->pose[0];
      p.py = mDevice->pose[1];
      p.pyaw = mDevice->pose[2];
      return(p);
    }

    /// Accessor for the pose of the laser's parent object (e.g., a robot).
    /// Filled in by some (but not all) laser data messages.
    player_pose3d_t GetRobotPose()
    {
      player_pose3d_t p;
      scoped_lock_t lock(mPc->mMutex);

      p.px = mDevice->robot_pose[0];
      p.py = mDevice->robot_pose[1];
      p.pyaw = mDevice->robot_pose[2];
      return(p);
    }

    /// Accessor for the size (fill it in by calling RequestGeom)
    player_bbox3d_t GetSize()
    {
      player_bbox3d_t b;
      scoped_lock_t lock(mPc->mMutex);

      b.sl = mDevice->size[0];
      b.sw = mDevice->size[1];
      return(b);
    }

    /// Minimum range reading on the left side
    double GetMinLeft() const
      { return GetVar(mDevice->min_left); };

    /// Minimum range reading on the right side
    double GetMinRight() const
      { return GetVar(mDevice->min_right); };

    /// @deprecated Minimum range reading on the left side
    double MinLeft () const
      { return GetMinLeft(); }

    /// @deprecated Minimum range reading on the right side
    double MinRight () const
      { return GetMinRight(); }

    /// Range access operator.  This operator provides an alternate
    /// way of access the range data.  For example, given an @p
    /// LaserProxy named @p lp, the following expressions are
    /// equivalent: @p lp.GetRange(0) and @p lp[0].
    double operator [] (uint32_t index) const
      { return GetRange(index);}

};


/**
The @p LimbProxy class is used to control a @ref interface_limb
device.
 */
class PLAYERCC_EXPORT LimbProxy : public ClientProxy
{
  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

   // libplayerc data structure
    playerc_limb_t *mDevice;

  public:

    /// Constructor
    LimbProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~LimbProxy();

    /// Geometry request - call before getting the
    /// geometry of a joint through the accessor method
    void RequestGeometry(void);

    /// Power control
    void SetPowerConfig(bool aVal);
    /// Brakes control
    void SetBrakesConfig(bool aVal);
    /// Speed control
    void SetSpeedConfig(float aSpeed);

    /// Move the limb to the home position
    void MoveHome(void);
    /// Stop the limb immediately
    void Stop(void);
    /// Move the end effector to a given pose
    void SetPose(float aPX, float aPY, float aPZ,
                 float aAX, float aAY, float aAZ,
                 float aOX, float aOY, float aOZ);
    /// Move the end effector to a given position, ignoring orientation
    void SetPosition(float aX, float aY, float aZ);
    /// Move the end effector along a vector of given length,
    /// maintaining current orientation
    void VectorMove(float aX, float aY, float aZ, float aLength);

    /// Accessor method for getting the limb's data
    player_limb_data_t GetData(void) const;
    /// Same again for getting the limb's geometry
    player_limb_geom_req_t GetGeom(void) const;
};


/**
The @p LinuxjoystickProxy class is used to control a @ref interface_joystick
device.  The most recent joystick range measuremts can be read from the
range attribute, or using the the [] operator.
*/
class PLAYERCC_EXPORT LinuxjoystickProxy : public ClientProxy
{
  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_joystick_t *mDevice;

  public:
    // Constructor
    LinuxjoystickProxy(PlayerClient *aPc, uint32_t aIndex=0);
    // Destructor
    ~LinuxjoystickProxy();

    /// return the sensor count
    uint32_t GetButtons() const { return GetVar(mDevice->buttons); };

    /// return a particular scan value
    double GetAxes(uint32_t aIndex) const
      { if (GetVar(mDevice->axes_count) <= (int32_t)aIndex) return -1.0; return GetVar(mDevice->pos[aIndex]); };
    /// This operator provides an alternate way of access the scan data.
    /// For example, LinuxjoystickProxy[0] == LinuxjoystickProxy.GetRange(0)
    double operator [] (uint32_t aIndex) const { return GetAxes(aIndex); }

    /// Number of valid joystick poses
    uint32_t GetAxesCount() const { return GetVar(mDevice->axes_count); };

    /// Linuxjoystick poses (m,m,radians)
//    player_pose3d_t GetPose(uint32_t aIndex) const
//      { return GetVar(mDevice->poses[aIndex]); };

    // Enable/disable the joysticks.
    // Set @p state to 1 to enable, 0 to disable.
    // Note that when joysticks are disabled the client will still receive joystick
    // data, but the ranges will always be the last value read from the joysticks
    // before they were disabled.
    //void SetEnable(bool aEnable);

    /// Request the joystick geometry.
//    void RequestGeom();
};


/**
The @p LocalizeProxy class is used to control a @ref
interface_localize device, which can provide multiple pose
hypotheses for a robot.
*/
class PLAYERCC_EXPORT LocalizeProxy : public ClientProxy
{

  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_localize_t *mDevice;

  public:

    /// Constructor
    LocalizeProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~LocalizeProxy();

    /// Map X dimension (cells)
    // @todo should these be in a player_pose_t?
    uint32_t GetMapSizeX() const { return GetVar(mDevice->map_size_x); };
    /// Map Y dimension (cells)
    uint32_t GetMapSizeY() const { return GetVar(mDevice->map_size_y); };

    // @todo should these be in a player_pose_t?
    /// Map tile X dimension (cells)
    uint32_t GetMapTileX() const { return GetVar(mDevice->map_tile_x); };
    /// Map tile Y dimension (cells)
    uint32_t GetMapTileY() const { return GetVar(mDevice->map_tile_y); };

    /// Map scale (m/cell)
    double GetMapScale() const { return GetVar(mDevice->map_scale); };

    // Map data (empty = -1, unknown = 0, occupied = +1)
    // is this still needed?  if so,
    //void GetMapCells(uint8_t* aCells) const
    //{
    //  return GetVarByRef(mDevice->map_cells,
    //                     mDevice->image+GetVar(mDevice->??map_cell_cout??),
    //                     aCells);
    //};

    /// Number of pending (unprocessed) sensor readings
    uint32_t GetPendingCount() const { return GetVar(mDevice->pending_count); };

    /// Number of possible poses
    uint32_t GetHypothCount() const { return GetVar(mDevice->hypoth_count); };

    /// Array of possible poses.
    player_localize_hypoth_t GetHypoth(uint32_t aIndex) const
      { return GetVar(mDevice->hypoths[aIndex]); };

    /// Get the particle set
    int GetParticles()
      { return playerc_localize_get_particles(mDevice); }

    /// Get the Pose of a particle
    player_pose2d_t GetParticlePose(int index) const;

    /// Set the current pose hypothesis (m, m, radians).
    void SetPose(double pose[3], double cov[3]);

    /// Get the number of localization hypoths.
    uint32_t GetNumHypoths() const { return GetVar(mDevice->hypoth_count); };

    /// Get the number of particles (for particle filter-based localization
    /// systems).  Returns the number of particles.
    uint32_t GetNumParticles() const { return GetVar(mDevice->num_particles); };
};


/**
The @p LogProxy proxy provides access to a @ref interface_log device.
*/
class PLAYERCC_EXPORT LogProxy : public ClientProxy
{
  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_log_t *mDevice;

  public:
    /// Constructor
    LogProxy(PlayerClient *aPc, uint32_t aIndex=0);

    /// Destructor
    ~LogProxy();

    /// What kind of log device is this? Either PLAYER_LOG_TYPE_READ or
    /// PLAYER_LOG_TYPE_WRITE. Call QueryState() to fill it.
    int GetType() const { return GetVar(mDevice->type); };

    /// Is logging/playback enabled? Call QueryState() to fill it.
    int GetState() const { return GetVar(mDevice->state); };

    /// Query the server for type and state info.
    void QueryState();

    /// Start/stop (1/0) reading from or writing to the log file.
    /// If the type of interface (reader/writer) is unknown, a query package is sent first.
    void SetState(int aState);

    /// Start/stop (1/0) writing to the log file.
    void SetWriteState(int aState);

    /// Start/stop (1/0) reading from the log file.
    void SetReadState(int aState);

    /// Rewind the log file.
    void Rewind();

    /// Set the name of the logfile to write to.
    void SetFilename(const std::string aFilename);
};

/**
The @p map proxy provides access to a @ref interface_map device.
*/
class PLAYERCC_EXPORT MapProxy : public ClientProxy
{
  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_map_t *mDevice;

  public:
    /// Constructor
    MapProxy(PlayerClient *aPc, uint32_t aIndex=0);

    /// Destructor
    ~MapProxy();

    /// Get the map and store it in the proxy
    void RequestMap();

    /// Return the index of the (x,y) item in the cell array
    int GetCellIndex(int x, int y) const
    { return y*GetWidth() + x; };

    /// Get the (x,y) cell
    int8_t GetCell(int x, int y) const
    { return GetVar(mDevice->cells[GetCellIndex(x,y)]); };

    /// Map resolution, m/cell
    double GetResolution() const { return GetVar(mDevice->resolution); };

    /// Map size, in cells
    //    @todo should this be returned as a player_size_t?
    uint32_t GetWidth() const { return GetVar(mDevice->width); };
    /// Map size, in cells
    // @todo should this be returned as a player_size_t?
    uint32_t GetHeight() const { return GetVar(mDevice->height); };

    double GetOriginX() const { return GetVar(mDevice->origin[0]); };
    double GetOriginY() const { return GetVar(mDevice->origin[1]); };

    /// Range of grid data (default: empty = -1, unknown = 0, occupied = +1)
    int8_t GetDataRange() const { return GetVar(mDevice->data_range); };

    /// Occupancy for each cell 
    void GetMap(int8_t* aMap) const
    {
      return GetVarByRef(reinterpret_cast<int8_t*>(mDevice->cells),
                         reinterpret_cast<int8_t*>(mDevice->cells+GetWidth()*GetHeight()),
                         aMap);
    };
};

/**
The @p OpaqueProxy proxy provides an interface to a generic @ref
interface_opaque. See examples/plugins/opaquedriver for an example of using
this interface in combination with a custom plugin.
*/
class PLAYERCC_EXPORT OpaqueProxy : public ClientProxy
{

  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_opaque_t *mDevice;

  public:

    /// Constructor
    OpaqueProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~OpaqueProxy();

    /// How long is the data?
    uint32_t GetCount() const { return GetVar(mDevice->data_count); };

    /// Opaque data
    void GetData(uint8_t* aDest) const
      {
        return GetVarByRef(mDevice->data,
                           mDevice->data+GetVar(mDevice->data_count),
                           aDest);
      };

    /// Send a command
    void SendCmd(player_opaque_data_t* aData);

    /// Send a request
    int SendReq(player_opaque_data_t* aRequest);

};

/**
The @p PlannerProxy proxy provides an interface to a 2D motion @ref
interface_planner. */
class PLAYERCC_EXPORT PlannerProxy : public ClientProxy
{

  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_planner_t *mDevice;

  public:

    /// Constructor
    PlannerProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~PlannerProxy();

    /// Set the goal pose (gx, gy, ga)
    void SetGoalPose(double aGx, double aGy, double aGa);

    /// Get the list of waypoints. Writes the result into the proxy
    /// rather than returning it to the caller.
    void RequestWaypoints();

    /// Enable/disable the robot's motion.  Set state to true to enable,
    /// false to disable.
    void SetEnable(bool aEnable);

    /// Did the planner find a valid path?
    uint32_t GetPathValid() const { return GetVar(mDevice->path_valid); };

    /// Have we arrived at the goal?
    uint32_t GetPathDone() const { return GetVar(mDevice->path_done); };

    /// @brief Current pose (m)
    /// @deprecated use GetPose() instead
    double GetPx() const { return GetVar(mDevice->px); };
    /// @brief Current pose (m)
    /// @deprecated use GetPose() instead
    double GetPy() const { return GetVar(mDevice->py); };
    /// @brief Current pose (radians)
    /// @deprecated use GetPose() instead
    double GetPa() const { return GetVar(mDevice->pa); };

    /// Get the current pose
    player_pose2d_t GetPose() const
    {
      player_pose2d_t p;
      scoped_lock_t lock(mPc->mMutex);
      p.px = mDevice->px;
      p.py = mDevice->py;
      p.pa = mDevice->pa;
      return(p);
    }

    /// @brief Goal location (m)
    /// @deprecated use GetGoal() instead
    double GetGx() const { return GetVar(mDevice->gx); };
    /// @brief Goal location (m)
    /// @deprecated use GetGoal() instead
    double GetGy() const { return GetVar(mDevice->gy); };
    /// @brief Goal location (radians)
    /// @deprecated use GetGoal() instead
    double GetGa() const { return GetVar(mDevice->ga); };

    /// Get the goal
    player_pose2d_t GetGoal() const
    {
      player_pose2d_t p;
      scoped_lock_t lock(mPc->mMutex);
      p.px = mDevice->gx;
      p.py = mDevice->gy;
      p.pa = mDevice->ga;
      return(p);
    }

    /// @brief Current waypoint location (m)
    /// @deprecated use GetCurWaypoint() instead
    double GetWx() const { return GetVar(mDevice->wx); };
    /// @brief Current waypoint location (m)
    /// @deprecated use GetCurWaypoint() instead
    double GetWy() const { return GetVar(mDevice->wy); };
    /// @brief Current waypoint location (rad)
    /// @deprecated use GetCurWaypoint() instead
    double GetWa() const { return GetVar(mDevice->wa); };

    /// Get the current waypoint
    player_pose2d_t GetCurrentWaypoint() const
    {
      player_pose2d_t p;
      scoped_lock_t lock(mPc->mMutex);
      p.px = mDevice->wx;
      p.py = mDevice->wy;
      p.pa = mDevice->wa;
      return(p);
    }

    /// @brief Grab a particular waypoint location (m)
    /// @deprecated use GetWaypoint() instead
    double GetIx(int i) const;
    /// @brief Grab a particular waypoint location (m)
    /// @deprecated use GetWaypoint() instead
    double GetIy(int i) const;
    /// @brief Grab a particular waypoint location (rad)
    /// @deprecated use GetWaypoint() instead
    double GetIa(int i) const;

    /// Get the waypoint
    player_pose2d_t GetWaypoint(uint32_t aIndex) const
    {
      assert(aIndex < GetWaypointCount());
      player_pose2d_t p;
      scoped_lock_t lock(mPc->mMutex);
      p.px = mDevice->waypoints[aIndex][0];
      p.py = mDevice->waypoints[aIndex][1];
      p.pa = mDevice->waypoints[aIndex][2];
      return(p);
    }

    /// Current waypoint index (handy if you already have the list
    /// of waypoints). May be negative if there's no plan, or if
    /// the plan is done
    int GetCurrentWaypointId() const
      { return GetVar(mDevice->curr_waypoint); };

    /// Number of waypoints in the plan
    uint32_t GetWaypointCount() const
      { return GetVar(mDevice->waypoint_count); };

    /// Waypoint access operator
    /// This operator provides an alternate way of access the waypoint data.
    /// For example, given a @p PlannerProxy named @p pp, the following
    /// expressions are equivalent: @p pp.GetWaypoint(0) and @p pp[0].
    player_pose2d_t operator [](uint32_t aIndex) const
      { return GetWaypoint(aIndex); }

};

/**
The Pointcloud3d proxy provides an interface to a pointcloud3d device.
*/
class PLAYERCC_EXPORT Pointcloud3dProxy : public ClientProxy
{
  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_pointcloud3d_t *mDevice;

  public:
    /// Constructor
    Pointcloud3dProxy(PlayerClient *aPc, uint32_t aIndex=0);

    /// Destructor
    ~Pointcloud3dProxy();

    /// return the point count
    uint32_t GetCount() const { return GetVar(mDevice->points_count); };

    /// return a particular scan value
    player_pointcloud3d_element_t GetPoint(uint32_t aIndex) const
      { return GetVar(mDevice->points[aIndex]); };

    /// This operator provides an alternate way of access the scan data.
    /// For example, SonarProxy[0] == SonarProxy.GetRange(0)
    player_pointcloud3d_element_t operator [] (uint32_t aIndex) const { return GetPoint(aIndex); }

};


/**
The @p Position1dProxy class is used to control a @ref
interface_position1d device.  The latest position data is contained
in the attributes pos, vel , etc.  */
class PLAYERCC_EXPORT Position1dProxy : public ClientProxy
{

  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_position1d_t *mDevice;

  public:

    /// Constructor
    Position1dProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~Position1dProxy();

    /// Send a motor command for velocity control mode.
    /// Specify the forward, sideways, and angular speeds in m/sec, m/sec,
    /// and radians/sec, respectively.
    void SetSpeed(double aVel);

    /// Send a motor command for position control mode.  Specify the
    /// desired pose of the robot in [m] or [rad]
    /// desired motion in [m/s] or [rad/s]
    void GoTo(double aPos, double aVel);

    /// Get the device's geometry; it is read into the
    /// relevant class attributes.
    void RequestGeom();

    /// Accessor for the pose (fill it in by calling RequestGeom)
    player_pose3d_t GetPose() const
    {
      player_pose3d_t p;
      scoped_lock_t lock(mPc->mMutex);
      p.px = mDevice->pose[0];
      p.py = mDevice->pose[1];
      p.pyaw = mDevice->pose[2];
      return(p);
    }

    /// Accessor for the size (fill it in by calling RequestGeom)
    player_bbox3d_t GetSize() const
    {
      player_bbox3d_t b;
      scoped_lock_t lock(mPc->mMutex);
      b.sl = mDevice->size[0];
      b.sw = mDevice->size[1];
      return(b);
    }

    /// Enable/disable the motors.
    /// Set @p state to 0 to disable or 1 to enable.
    /// Be VERY careful with this method!  Your robot is likely to run across the
    /// room with the charger still attached.
    void SetMotorEnable(bool enable);

    /// Sets the odometry to the pose @p aPos.
    /// @note aPos is in either [m] or [rad] depending on the actuator type
    void SetOdometry(double aPos);

    /// Reset odometry to 0.
    void ResetOdometry() { SetOdometry(0); };

    // Set PID terms
    //void SetSpeedPID(double kp, double ki, double kd);

    // Set PID terms
    //void SetPositionPID(double kp, double ki, double kd);

    // Set speed ramping profile
    // spd rad/s, acc rad/s/s
    //void SetPositionSpeedProfile(double spd, double acc);

    /// Get current position
    double  GetPos() const { return GetVar(mDevice->pos); };

    /// Get current velocity
    double  GetVel() const { return GetVar(mDevice->vel); };

    /// Get whether or not the device is stalled
    bool GetStall() const { return GetVar(mDevice->stall) != 0 ? true : false; };

    /// Get device status
    uint8_t GetStatus() const { return GetVar(mDevice->status); };

    /// Is the device at the min limit?
    bool IsLimitMin() const
      { return (GetVar(mDevice->status) &
               (1 << PLAYER_POSITION1D_STATUS_LIMIT_MIN)) > 0; };

    /// Is the device at the center limit?
    bool IsLimitCen() const
      { return (GetVar(mDevice->status) &
               (1 << PLAYER_POSITION1D_STATUS_LIMIT_CEN)) > 0; };

    /// Is the device at the max limit?
    bool IsLimitMax() const
      { return (GetVar(mDevice->status) &
               (1 << PLAYER_POSITION1D_STATUS_LIMIT_MAX)) > 0; };

    /// Is the device over current limits?
    bool IsOverCurrent() const
      { return (GetVar(mDevice->status) &
               (1 << PLAYER_POSITION1D_STATUS_OC)) > 0; };

    /// Is the device finished moving?
    bool IsTrajComplete() const
      { return (GetVar(mDevice->status) &
               (1 << PLAYER_POSITION1D_STATUS_TRAJ_COMPLETE)) > 0; };

    /// Is the device enabled?
    bool IsEnabled() const
      { return (GetVar(mDevice->status) &
               (1 << PLAYER_POSITION1D_STATUS_ENABLED)) > 0; };

};

/**
The @p Position2dProxy class is used to control a @ref
interface_position2d device.  The latest position data is contained
in the attributes xpos, ypos, etc.  */
class PLAYERCC_EXPORT Position2dProxy : public ClientProxy
{

  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_position2d_t *mDevice;

  public:

    /// Constructor
    Position2dProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~Position2dProxy();

    /// Send a motor command for velocity control mode.
    /// Specify the forward, sideways, and angular speeds in m/sec, m/sec,
    /// and radians/sec, respectively.
    void SetSpeed(double aXSpeed, double aYSpeed, double aYawSpeed);

    /// Same as the previous SetSpeed(), but doesn't take the yspeed speed
    /// (so use this one for non-holonomic robots).
    void SetSpeed(double aXSpeed, double aYawSpeed)
        { return SetSpeed(aXSpeed, 0, aYawSpeed);}

    /// Overloaded SetSpeed that takes player_pose2d_t as an argument
    void SetSpeed(player_pose2d_t vel)
        { return SetSpeed(vel.px, vel.py, vel.pa);}

    /// Send a motor command for velocity/heading control mode.
    /// Specify the forward and sideways velocity (m/sec), and angular
    /// heading (rads).
    void SetVelHead(double aXSpeed, double aYSpeed, double aYawHead);

    /// Same as the previous SetVelHead(), but doesn't take the yspeed speed
    /// (so use this one for non-holonomic robots).
    void SetVelHead(double aXSpeed, double aYawHead)
        { return SetVelHead(aXSpeed, 0, aYawHead);}


    /// Send a motor command for position control mode.  Specify the
    /// desired pose of the robot as a player_pose_t.
    /// desired motion speed  as a player_pose_t.
    void GoTo(player_pose2d_t pos, player_pose2d_t vel);

    /// Same as the previous GoTo(), but doesn't take speed
    void GoTo(player_pose2d_t pos)
      { player_pose2d_t vel = {0,0,0}; GoTo(pos, vel); }

    /// Same as the previous GoTo(), but only takes position arguments,
    /// no motion speed setting
    void GoTo(double aX, double aY, double aYaw)
      { player_pose2d_t pos = {aX,aY,aYaw}; player_pose2d_t vel = {0,0,0}; GoTo(pos, vel); }

    /// Sets command for carlike robot
    void SetCarlike(double aXSpeed, double aDriveAngle);

    /// Get the device's geometry; it is read into the
    /// relevant class attributes.  Call GetOffset() to access this data.
    void RequestGeom();

    /// Accessor for the robot's pose with respect to its
    //  body (fill it in by calling RequestGeom()).
    player_pose3d_t GetOffset()
    {
      player_pose3d_t p;
      scoped_lock_t lock(mPc->mMutex);
      p.px = mDevice->pose[0];
      p.py = mDevice->pose[1];
      p.pyaw = mDevice->pose[2];
      return(p);
    }

    /// Accessor for the size (fill it in by calling RequestGeom)
    player_bbox3d_t GetSize()
    {
      player_bbox3d_t b;
      scoped_lock_t lock(mPc->mMutex);
      b.sl = mDevice->size[0];
      b.sw = mDevice->size[1];
      return(b);
    }

    /// Enable/disable the motors.
    /// Set @p state to 0 to disable or 1 to enable.
    /// Be VERY careful with this method!  Your robot is likely to run across the
    /// room with the charger still attached.
    void SetMotorEnable(bool enable);

    // Select velocity control mode.
    //
    // For the the p2os_position driver, set @p mode to 0 for direct wheel
    // velocity control (default), or 1 for separate translational and
    // rotational control.
    //
    // For the reb_position driver: 0 is direct velocity control, 1 is for
    // velocity-based heading PD controller (uses DoDesiredHeading()).
    //void SelectVelocityControl(unsigned char mode);

    /// Reset odometry to (0,0,0).
    void ResetOdometry();

    // Select position mode
    // Set @p mode for 0 for velocity mode, 1 for position mode.
    //void SelectPositionMode(unsigned char mode);

    /// Sets the odometry to the pose @p (x, y, yaw).
    /// Note that @p x and @p y are in m and @p yaw is in radians.
    void SetOdometry(double aX, double aY, double aYaw);

    // Set PID terms
    //void SetSpeedPID(double kp, double ki, double kd);

    // Set PID terms
    //void SetPositionPID(double kp, double ki, double kd);

    // Set speed ramping profile
    // spd rad/s, acc rad/s/s
    //void SetPositionSpeedProfile(double spd, double acc);

    //
    // void DoStraightLine(double m);

    //
    //void DoRotation(double yawspeed);

    //
    //void DoDesiredHeading(double yaw, double xspeed, double yawspeed);

    //
    //void SetStatus(uint8_t cmd, uint16_t value);

    //
    //void PlatformShutdown();

    /// Get the device's X position
    double  GetXPos() const { return GetVar(mDevice->px); };

    /// Get the device's Y position
    double  GetYPos() const { return GetVar(mDevice->py); };

    /// Get the device's Yaw position (angle)
    double GetYaw() const { return GetVar(mDevice->pa); };

    /// Get the device's X speed
    double  GetXSpeed() const { return GetVar(mDevice->vx); };

    /// Get the device's Y speed
    double  GetYSpeed() const { return GetVar(mDevice->vy); };

    /// Get the device's angular (yaw) speed
    double  GetYawSpeed() const { return GetVar(mDevice->va); };

    /// Is the device stalled?
    bool GetStall() const { return GetVar(mDevice->stall) != 0 ? true : false; };

};

/**

The @p Position3dProxy class is used to control
a interface_position3d device.  The latest position data is
contained in the attributes xpos, ypos, etc.
*/
class PLAYERCC_EXPORT Position3dProxy : public ClientProxy
{

  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_position3d_t *mDevice;

  public:

    /// Constructor
    Position3dProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~Position3dProxy();

    /// Send a motor command for a planar robot.
    /// Specify the forward, sideways, and angular speeds in m/s, m/s, m/s,
    /// rad/s, rad/s, and rad/s, respectively.
    void SetSpeed(double aXSpeed, double aYSpeed, double aZSpeed,
                  double aRollSpeed, double aPitchSpeed, double aYawSpeed);

    /// Send a motor command for a planar robot.
    /// Specify the forward, sideways, and angular speeds in m/s, m/s,
    /// and rad/s, respectively.
    void SetSpeed(double aXSpeed, double aYSpeed,
                  double aZSpeed, double aYawSpeed)
      { SetSpeed(aXSpeed,aYSpeed,aZSpeed,0,0,aYawSpeed); }

    /// simplified version of SetSpeed
    void SetSpeed(double aXSpeed, double aYSpeed, double aYawSpeed)
      { SetSpeed(aXSpeed, aYSpeed, 0, 0, 0, aYawSpeed); }

    /// Same as the previous SetSpeed(), but doesn't take the sideways speed
    /// (so use this one for non-holonomic robots).
    void SetSpeed(double aXSpeed, double aYawSpeed)
      { SetSpeed(aXSpeed,0,0,0,0,aYawSpeed);}

    /// Overloaded SetSpeed that takes player_pose3d_t as input
    void SetSpeed(player_pose3d_t vel)
      { SetSpeed(vel.px,vel.py,vel.pz,vel.proll,vel.ppitch,vel.pyaw);}

    /// Send a motor command for position control mode.  Specify the
    /// desired pose of the robot as a player_pose3d_t structure
    /// desired motion speed as a player_pose3d_t structure
    void GoTo(player_pose3d_t aPos, player_pose3d_t aVel);

    /// Same as the previous GoTo(), but does'n take vel argument
    void GoTo(player_pose3d_t aPos)
      { player_pose3d_t vel = {0,0,0,0,0,0}; GoTo(aPos, vel); }


    /// Same as the previous GoTo(), but only takes position arguments,
    /// no motion speed setting
    void GoTo(double aX, double aY, double aZ,
              double aRoll, double aPitch, double aYaw)
      { player_pose3d_t pos = {aX,aY,aZ,aRoll,aPitch,aYaw}; 
        player_pose3d_t vel = {0,0,0,0,0,0}; 
        GoTo(pos, vel);
      }

    /// Enable/disable the motors.
    /// Set @p state to 0 to disable or 1 to enable.
    /// @attention Be VERY careful with this method!  Your robot is likely
    /// to run across the room with the charger still attached.
    void SetMotorEnable(bool aEnable);

    /// Select velocity control mode.
    /// This is driver dependent.
    void SelectVelocityControl(int aMode);

    /// Reset odometry to (0,0,0,0,0,0).
    void ResetOdometry();

    /// Sets the odometry to the pose @p (x, y, z, roll, pitch, yaw).
    /// Note that @p x, @p y, and @p z are in m and @p roll,
    /// @p pitch, and @p yaw are in radians.
    void SetOdometry(double aX, double aY, double aZ,
                     double aRoll, double aPitch, double aYaw);

    /// Get the device's geometry; it is read into the
    /// relevant class attributes.
    void RequestGeom();

    // Select position mode
    // Set @p mode for 0 for velocity mode, 1 for position mode.
    //void SelectPositionMode(unsigned char mode);

    //
    //void SetSpeedPID(double kp, double ki, double kd);

    //
    //void SetPositionPID(double kp, double ki, double kd);

    // Sets the ramp profile for position based control
    // spd rad/s, acc rad/s/s
    //void SetPositionSpeedProfile(double spd, double acc);

    /// Get device X position
    double  GetXPos() const { return GetVar(mDevice->pos_x); };

    /// Get device Y position
    double  GetYPos() const { return GetVar(mDevice->pos_y); };

    /// Get device Z position
    double  GetZPos() const { return GetVar(mDevice->pos_z); };

    /// Get device Roll angle
    double  GetRoll() const { return GetVar(mDevice->pos_roll); };

    /// Get device Pitch angle
    double  GetPitch() const { return GetVar(mDevice->pos_pitch); };

    /// Get device Yaw angle
    double  GetYaw() const { return GetVar(mDevice->pos_yaw); };

    /// Get device X speed
    double  GetXSpeed() const { return GetVar(mDevice->vel_x); };

    /// Get device Y speed
    double  GetYSpeed() const { return GetVar(mDevice->vel_y); };

    /// Get device Z speed
    double  GetZSpeed() const { return GetVar(mDevice->vel_z); };

    /// Get device Roll speed
    double  GetRollSpeed() const { return GetVar(mDevice->vel_roll); };

    /// Get device Pitch speed
    double  GetPitchSpeed() const { return GetVar(mDevice->vel_pitch); };

    /// Get device Yaw speed
    double  GetYawSpeed() const { return GetVar(mDevice->vel_yaw); };

    /// Is the device stalled?
    bool GetStall () const { return GetVar(mDevice->stall) != 0 ? true : false; };
};
/**
The @p PowerProxy class controls a @ref interface_power device. */
class PLAYERCC_EXPORT PowerProxy : public ClientProxy
{
  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_power_t *mDevice;

  public:
    /// Constructor
    PowerProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~PowerProxy();

    /// Returns the current charge.
    double GetCharge() const { return GetVar(mDevice->charge); };

    /// Returns the percent of power
    double GetPercent() const {return GetVar(mDevice->percent); };

    /// Returns the joules
    double GetJoules() const {return GetVar(mDevice->joules); };

    /// Returns the watts
    double GetWatts() const {return GetVar(mDevice->watts); };

    /// Returns whether charging is taking place
    bool GetCharging() const {return GetVar(mDevice->charging) != 0 ? true : false;};

    // Return whether the power data is valid
    bool IsValid() const {return GetVar(mDevice->valid) != 0 ? true : false;};
};

/**

The @p PtzProxy class is used to control a @ref interface_ptz
device.  The state of the camera can be read from the pan, tilt, zoom
attributes and changed using the SetCam() method.
*/
class PLAYERCC_EXPORT PtzProxy : public ClientProxy
{

  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_ptz_t *mDevice;

  public:
    // Constructor
    PtzProxy(PlayerClient *aPc, uint32_t aIndex=0);
    // Destructor
    ~PtzProxy();

  public:

    /// Change the camera state.
    /// Specify the new @p pan, @p tilt, and @p zoom values
    /// (all degrees).
    void SetCam(double aPan, double aTilt, double aZoom);

    /// Specify new target velocities
    void SetSpeed(double aPanSpeed=0, double aTiltSpeed=0, double aZoomSpeed=0);

    /// Select new control mode.  Use either @ref PLAYER_PTZ_POSITION_CONTROL
    /// or @ref PLAYER_PTZ_VELOCITY_CONTROL.
    void SelectControlMode(uint32_t aMode);

    /// Return Pan (rad)
    double GetPan() const { return GetVar(mDevice->pan); };
    /// Return Tilt (rad)
    double GetTilt() const { return GetVar(mDevice->tilt); };
    /// Return Zoom
    double GetZoom() const { return GetVar(mDevice->zoom); };

    /// Return Status
    int GetStatus();


};

/**
The @p RangerProxy class is used to control a @ref interface_ranger device. */
class PLAYERCC_EXPORT RangerProxy : public ClientProxy
{
  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_ranger_t *mDevice;

  public:
    /// Constructor
    RangerProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~RangerProxy();

    /// Return the individual range sensor count
    uint32_t GetElementCount() const { return GetVar(mDevice->element_count); };

    /// Return the device pose
    player_pose3d_t GetDevicePose() const { return GetVar(mDevice->device_pose); };
    /// Return the device size
    player_bbox3d_t GetDeviceSize() const { return GetVar(mDevice->device_size); };

    /// Return the pose of an individual sensor
    player_pose3d_t GetElementPose(uint32_t aIndex) const;
    /// Return the size of an individual sensor
    player_bbox3d_t GetElementSize(uint32_t aIndex) const;

    /// Return the number of range readings
    uint32_t GetRangeCount() const { return GetVar(mDevice->ranges_count); };
    /// Get a range reading
    double GetRange(uint32_t aIndex) const;
    /// Operator to get a range reading
    double operator[] (uint32_t aIndex) const { return GetRange(aIndex); }

    /// Return the number of intensity readings
    uint32_t GetIntensityCount() const { return GetVar(mDevice->intensities_count); } ;
    /// Get an intensity reading
    double GetIntensity(uint32_t aIndex) const;

    /// Turn the device power on or off.
    /// Set @p state to true to enable, false to disable.
    void SetPower(bool aEnable);

    /// Turn intensity data on or off.
    /// Set @p state to true to enable, false to disable.
    void SetIntensityData(bool aEnable);

    /// Request the ranger geometry.
    void RequestGeom();

    /// Configure the ranger scan pattern. Angles @p aMinAngle and
    /// @p aMaxAngle are measured in radians. @p aResolution is measured in
    /// radians. @p aMinRange, @p aMaxRange and @p aRangeRes is measured in metres.
    /// @p aFrequency is measured in Hz.
    void Configure(double aMinAngle,
                   double aMaxAngle,
                   double aAngularRes,
                   double aMinRange,
                   double aMaxRange,
                   double aRangeRes,
                   double aFrequency);

    /// Get the current ranger configuration; it is read into the
    /// relevant class attributes.
    void RequestConfigure();

    /// Start angle of a scan (configured value)
    double GetMinAngle() const { return GetVar(mDevice->min_angle); };

    /// Stop angle of a scan (configured value)
    double GetMaxAngle() const { return GetVar(mDevice->max_angle); };

    /// Angular resolution of a scan (configured value)
    double GetAngularRes() const { return GetVar(mDevice->angular_res); };

    /// Minimum detectable range of a scan (configured value)
    double GetMinRange() const { return GetVar(mDevice->min_range); };

    /// Maximum detectable range of a scan (configured value)
    double GetMaxRange() const { return GetVar(mDevice->max_range); };

    /// Linear resolution (configured value)
    double GetRangeRes() const { return GetVar(mDevice->range_res); };

    /// Scanning frequency (configured value)
    double GetFrequency() const { return GetVar(mDevice->frequency); };
};

/**
The @p RFIDProxy class is used to control a  @ref interface_rfid device. */
class PLAYERCC_EXPORT RFIDProxy : public ClientProxy
{

  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_rfid_t *mDevice;

  public:
    /// Constructor
    RFIDProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~RFIDProxy();

    /// returns the number of RFID tags
    uint32_t GetTagsCount() const { return GetVar(mDevice->tags_count); };
    /// returns a RFID tag
    playerc_rfidtag_t GetRFIDTag(uint32_t aIndex) const
      { return GetVar(mDevice->tags[aIndex]);};

    /// RFID data access operator.
    ///    This operator provides an alternate way of access the actuator data.
    ///    For example, given a @p RFIDProxy named @p rp, the following
    ///    expressions are equivalent: @p rp.GetRFIDTag[0] and @p rp[0].
    playerc_rfidtag_t operator [](uint32_t aIndex) const
      { return(GetRFIDTag(aIndex)); }
};

/**
The @p SimulationProxy proxy provides access to a
@ref interface_simulation device.
*/
class PLAYERCC_EXPORT SimulationProxy : public ClientProxy
{
  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_simulation_t *mDevice;

  public:
    /// Constructor
    SimulationProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~SimulationProxy();

    /// set the 2D pose of an object in the simulator, identified by the
    /// std::string. Returns 0 on success, else a non-zero error code.
    void SetPose2d(char* identifier, double x, double y, double a);

    /// get the pose of an object in the simulator, identified by the
    /// std::string Returns 0 on success, else a non-zero error code.
    void GetPose2d(char* identifier, double& x, double& y, double& a);

    /// set the 3D pose of an object in the simulator, identified by the
    /// std::string. Returns 0 on success, else a non-zero error code.
    void SetPose3d(char* identifier, double x, double y, double z,
                   double roll, double pitch, double yaw);

    /// get the 3D pose of an object in the simulator, identified by the
    /// std::string Returns 0 on success, else a non-zero error code.
    void GetPose3d(char* identifier, double& x, double& y, double& z,
                   double& roll, double& pitch, double& yaw, double& time);

    /// Get a simulation property
    void GetProperty(char* identifier, char *name, void *value, size_t value_len );

    /// Set a simulation property
    void SetProperty(char* identifier, char *name, void *value, size_t value_len );
};


/**
The @p SonarProxy class is used to control a @ref interface_sonar
device.  The most recent sonar range measuremts can be read from the
range attribute, or using the the [] operator.
*/
class PLAYERCC_EXPORT SonarProxy : public ClientProxy
{
  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_sonar_t *mDevice;

  public:
    /// Constructor
    SonarProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~SonarProxy();

    /// return the sensor count
    uint32_t GetCount() const { return GetVar(mDevice->scan_count); };

    /// return a particular scan value
    double GetScan(uint32_t aIndex) const
      { if (GetVar(mDevice->scan_count) <= (int32_t)aIndex) return -1.0; return GetVar(mDevice->scan[aIndex]); };
    /// This operator provides an alternate way of access the scan data.
    /// For example, SonarProxy[0] == SonarProxy.GetRange(0)
    double operator [] (uint32_t aIndex) const { return GetScan(aIndex); }

    /// Number of valid sonar poses
    uint32_t GetPoseCount() const { return GetVar(mDevice->pose_count); };

    /// Sonar poses (m,m,radians)
    player_pose3d_t GetPose(uint32_t aIndex) const
      { return GetVar(mDevice->poses[aIndex]); };

    // Enable/disable the sonars.
    // Set @p state to 1 to enable, 0 to disable.
    // Note that when sonars are disabled the client will still receive sonar
    // data, but the ranges will always be the last value read from the sonars
    // before they were disabled.
    //void SetEnable(bool aEnable);

    /// Request the sonar geometry.
    void RequestGeom();
};

/**
The @p SpeechProxy class is used to control a @ref interface_speech
device.  Use the say method to send things to say.
*/
class PLAYERCC_EXPORT SpeechProxy : public ClientProxy
{

  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_speech_t *mDevice;

  public:
    /// Constructor
    SpeechProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~SpeechProxy();

    /// Send a phrase to say.
    /// The phrase is an ASCII std::string.
    void Say(std::string aStr);
};

/**
 The @p SpeechRecognition proxy provides access to a @ref interface_speech_recognition device.
 */
class PLAYERCC_EXPORT SpeechRecognitionProxy : public ClientProxy
{
   void Subscribe(uint32_t aIndex);
   void Unsubscribe();

   ///libplayerc data structure
   playerc_speechrecognition_t *mDevice;
  public:
   /// Constructor
   SpeechRecognitionProxy(PlayerClient *aPc, uint32_t aIndex=0);
   /// Destructor
   ~SpeechRecognitionProxy();
   /// Accessor method for getting speech recognition data i.e. words.
   std::string GetWord(uint32_t aWord) const{
     scoped_lock_t lock(mPc->mMutex);
     return std::string(mDevice->words[aWord]);
   }

   /// Gets the number of words.
   uint32_t GetCount(void) const { return GetVar(mDevice->wordCount); }

   /// Word access operator.
   ///    This operator provides an alternate way of access the speech recognition data.
   std::string operator [](uint32_t aWord) { return(GetWord(aWord)); }
};

/**
 the @p Stereo proxy provides access to the @ref interface_stereo device.
 */
class PLAYERCC_EXPORT StereoProxy : public ClientProxy
{
  private:
    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    ///libplayerc data structure
    playerc_stereo_t *mDevice;

    /// Save a frame
    void SaveFrame(const std::string aPrefix, uint32_t aWidth, playerc_camera_t aDevice, uint8_t aIndex);

    /// Decompress an image
    void Decompress(playerc_camera_t aDevice);

    /// Default image prefix
    std::string mPrefix;

    /// Frame counter for image saving methods
    uint32_t mFrameNo[3];

  public:
    /// Constructor
    StereoProxy(PlayerClient *aPc, uint32_t aIndex=0);

    /// Destructor
    ~StereoProxy();

    /// Save the left frame
    /// @arg aPrefix is the string prefix to name the image.
    /// @arg aWidth is the number of 0s to pad the image numbering with.
    void SaveLeftFrame(const std::string aPrefix, uint32_t aWidth=4) {return SaveFrame(aPrefix, aWidth, mDevice->left_channel, 0); };
    /// Save the right frame
    /// @arg aPrefix is the string prefix to name the image.
    /// @arg aWidth is the number of 0s to pad the image numbering with.
    void SaveRightFrame(const std::string aPrefix, uint32_t aWidth=4) {return SaveFrame(aPrefix, aWidth, mDevice->right_channel, 1); };
    /// Save the disparity frame
    /// @arg aPrefix is the string prefix to name the image.
    /// @arg aWidth is the number of 0s to pad the image numbering with.
    void SaveDisparityFrame(const std::string aPrefix, uint32_t aWidth=4) {return SaveFrame(aPrefix, aWidth, mDevice->disparity, 2); };

    /// decompress the left image
    void DecompressLeft(){ return Decompress(mDevice->left_channel); };
    /// decompress the right image
    void DecompressRight(){ return Decompress(mDevice->right_channel); };
    /// decompress the disparity image
    void DecompressDisparity(){ return Decompress(mDevice->disparity); };

    /// Left image color depth
    uint32_t GetLeftDepth() const { return GetVar(mDevice->left_channel.bpp); };
    /// Right image color depth
    uint32_t GetRightDepth() const { return GetVar(mDevice->right_channel.bpp); };
    /// Disparity image color depth
    uint32_t GetDisparityDepth() const { return GetVar(mDevice->disparity.bpp); };

    /// Left image width (pixels)
    uint32_t GetLeftWidth() const { return GetVar(mDevice->left_channel.width); };
    /// Right image width (pixels)
    uint32_t GetRightWidth() const { return GetVar(mDevice->right_channel.width); };
    /// Disparity image width (pixels)
    uint32_t GetDisparityWidth() const { return GetVar(mDevice->disparity.width); };

    /// Left image height (pixels)
    uint32_t GetLeftHeight() const { return GetVar(mDevice->left_channel.height); };
    /// Right image height (pixels)
    uint32_t GetRightHeight() const { return GetVar(mDevice->right_channel.height); };
    /// Disparity image height (pixels)
    uint32_t GetDisparityHeight() const { return GetVar(mDevice->disparity.height); };

    /// @brief Image format
    /// Possible values include
    /// - @ref PLAYER_CAMERA_FORMAT_MONO8
    /// - @ref PLAYER_CAMERA_FORMAT_MONO16
    /// - @ref PLAYER_CAMERA_FORMAT_RGB565
    /// - @ref PLAYER_CAMERA_FORMAT_RGB888
    uint32_t GetLeftFormat() const { return GetVar(mDevice->left_channel.format); };
    /// @brief Image format
    /// Possible values include
    /// - @ref PLAYER_CAMERA_FORMAT_MONO8
    /// - @ref PLAYER_CAMERA_FORMAT_MONO16
    /// - @ref PLAYER_CAMERA_FORMAT_RGB565
    /// - @ref PLAYER_CAMERA_FORMAT_RGB888
    uint32_t GetRightFormat() const { return GetVar(mDevice->right_channel.format); };
    /// @brief Image format
    /// Possible values include
    /// - @ref PLAYER_CAMERA_FORMAT_MONO8
    /// - @ref PLAYER_CAMERA_FORMAT_MONO16
    /// - @ref PLAYER_CAMERA_FORMAT_RGB565
    /// - @ref PLAYER_CAMERA_FORMAT_RGB888
    uint32_t GetDisparityFormat() const { return GetVar(mDevice->disparity.format); };

    /// Size of the left image (bytes)
    uint32_t GetLeftImageSize() const { return GetVar(mDevice->left_channel.image_count); };
    /// Size of the right image (bytes)
    uint32_t GetRightImageSize() const { return GetVar(mDevice->right_channel.image_count); };
    /// Size of the disparity image (bytes)
    uint32_t GetDisparityImageSize() const { return GetVar(mDevice->disparity.image_count); };

    /// @brief Left image data
    /// This function copies the image data into the data buffer aImage.
    /// The buffer should be allocated according to the width, height, and
    /// depth of the image.  The size can be found by calling @ref GetLeftImageSize().
    void GetLeftImage(uint8_t* aImage) const
      {
        return GetVarByRef(mDevice->left_channel.image,
                           mDevice->left_channel.image+GetVar(mDevice->left_channel.image_count),
                           aImage);
      };
    /// @brief Right image data
    /// This function copies the image data into the data buffer aImage.
    /// The buffer should be allocated according to the width, height, and
    /// depth of the image.  The size can be found by calling @ref GetRightImageSize().
    void GetRightImage(uint8_t* aImage) const
      {
        return GetVarByRef(mDevice->right_channel.image,
                           mDevice->right_channel.image+GetVar(mDevice->right_channel.image_count),
                           aImage);
      };
    /// @brief Disparity image data
    /// This function copies the image data into the data buffer aImage.
    /// The buffer should be allocated according to the width, height, and
    /// depth of the image.  The size can be found by calling @ref GetDisparityImageSize().
    void GetDisparityImage(uint8_t* aImage) const
      {
        return GetVarByRef(mDevice->disparity.image,
                           mDevice->disparity.image+GetVar(mDevice->disparity.image_count),
                           aImage);
      };

    /// @brief Get the left image's compression type
    /// Currently supported compression types are:
    /// - @ref PLAYER_CAMERA_COMPRESS_RAW
    /// - @ref PLAYER_CAMERA_COMPRESS_JPEG
    uint32_t GetLeftCompression() const { return GetVar(mDevice->left_channel.compression); };
    /// @brief Get the right image's compression type
    /// Currently supported compression types are:
    /// - @ref PLAYER_CAMERA_COMPRESS_RAW
    /// - @ref PLAYER_CAMERA_COMPRESS_JPEG
    uint32_t GetRightCompression() const { return GetVar(mDevice->right_channel.compression); };
    /// @brief Get the disparity image's compression type
    /// Currently supported compression types are:
    /// - @ref PLAYER_CAMERA_COMPRESS_RAW
    /// - @ref PLAYER_CAMERA_COMPRESS_JPEG
    uint32_t GetDisparityCompression() const { return GetVar(mDevice->disparity.compression); };

    /// return the point count
    uint32_t GetCount() const { return GetVar(mDevice->points_count); };

    /// return a particular point value
    player_pointcloud3d_stereo_element_t GetPoint(uint32_t aIndex) const
      { return GetVar(mDevice->points[aIndex]); };

    /// This operator provides an alternate way of access the point data.
    /// For example, StereoProxy[0] == StereoProxy.GetPoint(0)
    player_pointcloud3d_stereo_element_t operator [] (uint32_t aIndex) const { return GetPoint(aIndex); }

};

/**
* The @p VectorMapProxy class is used to interface to a vectormap.
*/
class PLAYERCC_EXPORT VectorMapProxy : public ClientProxy
{

  private:

    // Subscribe
    void Subscribe(uint32_t aIndex);
    // Unsubscribe
    void Unsubscribe();

    // libplayerc data structure
    playerc_vectormap_t *mDevice;

    bool map_info_cached;
  public:
    /// Constructor
    VectorMapProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~VectorMapProxy();

    /// Request map information from the underlying device
    void GetMapInfo();

    /// Request data for a specific layer from the underlying device
    void GetLayerData(unsigned layer_index);

    /// Get how many layers are in the map
    int GetLayerCount() const;

    /// Get the names of each of the layers
    std::vector<std::string> GetLayerNames() const;

    /// Get how many features are in a particular layer
    int GetFeatureCount(unsigned layer_index) const;

    /// Get the feature data for a layer and feature
    const uint8_t * GetFeatureData(unsigned layer_index, unsigned feature_index) const;

    /// Get how long the feature data is for a layer and feature
    size_t GetFeatureDataCount(unsigned layer_index, unsigned feature_index) const;
};

/**
The @p WiFiProxy class controls a @ref interface_wifi device.  */
class PLAYERCC_EXPORT WiFiProxy: public ClientProxy
{

  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_wifi_t *mDevice;

  public:
    /// Constructor
    WiFiProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~WiFiProxy();

    /// Get the playerc wifi link data
    const playerc_wifi_link_t *GetLink(int aLink);

    /// Get how many links the device sees
    int GetLinkCount() const { return mDevice->link_count; };
    /// Get your current IP address
    char* GetOwnIP() const { return mDevice->ip; };
    /// Get the IP address from a particular link
    char* GetLinkIP(int index)  const { return (char*) mDevice->links[index].ip; };
    /// Get the MAC address from a particular link
    char* GetLinkMAC(int index)  const { return (char*) mDevice->links[index].mac; };
    /// Get the ESSID from a particular link
    char* GetLinkESSID(int index)  const { return (char*)mDevice->links[index].essid; };
    /// Get the frequency from a particular link
    double GetLinkFreq(int index)  const {return mDevice->links[index].freq;};
    /// Get the connection mode from a particular link
    int GetLinkMode(int index)  const { return mDevice->links[index].mode; };
    /// Get whether a particular link is encrypted
    int GetLinkEncrypt(int index)  const {return mDevice->links[index].encrypt; };
    /// Get the quality of a particular link
    int GetLinkQuality(int index)  const { return mDevice->links[index].qual; };
    /// Get the signal level of a particular link
    int GetLinkLevel(int index)  const {return mDevice->links[index].level; };
    /// Get the noise level of a particular link
    int GetLinkNoise(int index)  const {return mDevice->links[index].noise; }	;

};

/**
The @p WSNProxy class is used to control a @ref interface_wsn device. */
class PLAYERCC_EXPORT WSNProxy : public ClientProxy
{

  private:

    void Subscribe(uint32_t aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_wsn_t *mDevice;

  public:
    /// Constructor
    WSNProxy(PlayerClient *aPc, uint32_t aIndex=0);
    /// Destructor
    ~WSNProxy();

    /// Get the node's type
    uint32_t GetNodeType    () const { return GetVar(mDevice->node_type);      };
    /// Get the node's ID
    uint32_t GetNodeID      () const { return GetVar(mDevice->node_id);        };
    /// Get the node's parent ID
    uint32_t GetNodeParentID() const { return GetVar(mDevice->node_parent_id); };

    /// Get a WSN node packet
    player_wsn_node_data_t
       GetNodeDataPacket() const { return GetVar(mDevice->data_packet);    };

    /// Set a WSN device State
    void SetDevState(int nodeID, int groupID, int devNr, int value);
    /// Set WSN device Power
    void Power(int nodeID, int groupID, int value);
    /// Set WSN device type
    void DataType(int value);
    /// Set WSN device frequency
    void DataFreq(int nodeID, int groupID, float frequency);
};

/** @} (proxies)*/
}

namespace std
{
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const player_point_2d_t& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const player_pose2d_t& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const player_pose3d_t& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const player_bbox2d_t& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const player_bbox3d_t& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const player_segment_t& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const player_extent2d_t& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const playerc_device_info_t& c);

  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::ClientProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::ActArrayProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::AioProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::AudioProxy& a);
  //PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::BlinkenLightProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::BlobfinderProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::BumperProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::CameraProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::DioProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::FiducialProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::GpsProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::GripperProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::ImuProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::IrProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::LaserProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::LimbProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::LinuxjoystickProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::LocalizeProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::LogProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::MapProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::OpaqueProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::PlannerProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::Position1dProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::Position2dProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::Position3dProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::PowerProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::PtzProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream &os, const PlayerCc::RangerProxy &c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::SimulationProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::SonarProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::SpeechProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::SpeechRecognitionProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::StereoProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::VectorMapProxy& c);
  //PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::WafeformProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::WiFiProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::RFIDProxy& c);
  PLAYERCC_EXPORT std::ostream& operator << (std::ostream& os, const PlayerCc::WSNProxy& c);
}

#endif

