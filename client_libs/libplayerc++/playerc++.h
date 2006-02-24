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

/***************************************************************************
 * Desc: Player v2.0 C++ client
 * Authors: Brad Kratochvil, Toby Collett
 *
 * Date: 23 Sep 2005
 # CVS: $Id$
 **************************************************************************/


#ifndef PLAYERCC_H
#define PLAYERCC_H

#include <cmath>
#include <string>
#include <list>
#include <stdint.h>


#include "libplayerc/playerc.h"
#include "libplayerc++/playerc++config.h"
#include "libplayerc++/playerclient.h"
#include "libplayerc++/playererror.h"


#ifdef HAVE_BOOST_SIGNALS
  #include <boost/signal.hpp>
  #include <boost/bind.hpp>
#endif

#ifdef HAVE_BOOST_THREAD
  #include <boost/thread/mutex.hpp>
  #include <boost/thread/thread.hpp>
  #include <boost/thread/xtime.hpp>
#endif

namespace PlayerCc
{
/** @ingroup clientlibs 
    @defgroup player_clientlib_cplusplus libplayerc++
    @brief A C++ client library for the @ref util_player

The C++ library is built on a "service proxy" model in which the client
maintains local objects that are proxies for remote services.  This library
wraps the functionality of @ref player_clientlib_libplayerc with a more
friendly C++ API.

Be sure to see @ref cplusplus_example "this example".
 */

/** @ingroup player_clientlib_cplusplus
   @defgroup player_clientlib_utility Utility and error-handling functions
   @brief Helper functions when using the library.
*/
/** @{ */

// Since they are inline, these functions are as efficient as DEFINES,
// but now they have the benefit of type checking!

/// Convert radians to degrees
inline double rtod(double r)
{
  return r * 180.0 / M_PI;
}

/// Convert degrees to radians
inline double dtor(double r)
{
  return r * M_PI / 180.0;
}

/// Normalize angle to domain -pi, pi
inline double normalize(double z)
{
  return atan2(sin(z), cos(z));
}

/// Return the minimum of a, b
template<typename T>
inline T min(T a, T b)
{
  if (a < b)
    return a;
  else
    return b;
}

/// Return the maximum of a, b
template<typename T>
inline T max(T a, T b)
{
  if (a > b)
    return a;
  else
    return b;
}

/// Limit a value to the range of min, max
template<typename T>
inline T limit(T a, T min, T max)
{
  if (a < min)
    return min;
  else if (a > max)
    return max;
  else
    return a;
}

/** @} (utility)*/


/** @ingroup player_clientlib_cplusplus
 * @defgroup player_clientlib_cplusplus_core Core functionality
 * @brief Device-independent components

  The core of libplayerc++ is based around the @p PlayerClient and
  @p ClientProxy.
  */
/** @{ */

/** @brief The client proxy base class
 *
 * Base class for all proxy devices. Access to a device is provided by a
 * device-specific proxy class.  These classes all inherit from the @p
 * ClientProxy class which defines an interface for device proxies.  As such,
 * a few methods are common to all devices and we explain them here.
 *
 * Since the ConnectReadSignal() and DisconnectReadSignal() member functions
 * are based on the Boost signals library, they are conditionally available
 * depending on Boost's presence in the system.  See the @em configure script
 * for more information.
*/
class ClientProxy
{
  friend class PlayerClient;

  public:

#ifdef HAVE_BOOST_SIGNALS
    /// A connection type.  This is usefull when attaching signals to the
    /// ClientProxy because it allows for detatching the signals.
    typedef boost::signals::connection connection_t;

    // A scoped lock
    typedef boost::mutex::scoped_lock scoped_lock_t;

    // the function pointer type for read signals signal
    typedef boost::signal<void (void)> read_signal_t;
#else
    // if we're not using boost, just define them.
    typedef int connection_t;
    // we redefine boost::mustex::scoped_lock in playerclient.h
    typedef boost::mutex::scoped_lock scoped_lock_t;
    typedef int read_signal_t;
#endif

  protected:

    // The ClientProxy constructor
    // @attention Potected, so it can only be instantiated by other clients
    //
    // @throw PlayerError Throws a PlayerError if unable to connect to the client
    ClientProxy(PlayerClient* aPc, uint aIndex);

    // destructor will try to close access to the device
    virtual ~ClientProxy();

    // Subscribe to the proxy
    // This needs to be defined for every proxy.
    // @arg aIndex the index of the devce we want to connect to

    // I wish these could be pure virtual,
    // but they're used in the constructor/destructor
    virtual void Subscribe(uint aIndex) {};

    // Unsubscribe from the proxy
    // This needs to be defined for every proxy.
    virtual void Unsubscribe() {};

    // The controlling client object.
    PlayerClient* mPc;

    // A reference to the C client
    playerc_client_t* mClient;

    // contains convenience information about the device
    playerc_device_t *mInfo;

    // if set to true, the current data is "fresh"
    bool mFresh;

    // @brief Get a variable from the client
    // All Get functions need to use this when accessing data from the
    // c library to make sure the data access is thread safe.
    template<typename T>
    T GetVar(const T &aV) const
    { // these have to be defined here since they're templates
      scoped_lock_t lock(mPc->mMutex);
      T v = aV;
      return v;
    }

    // @brief Get a variable from the client by reference
    // All Get functions need to use this when accessing data from the
    // c library to make sure the data access is thread safe.  In this
    // case, a begin, end, and destination pointer must be given (similar
    // to C++ copy).  It is up to the user to ensure there is memory
    // allocated at aDest.
    template<typename T>
    void GetVarByRef(const T aBegin, const T aEnd, T aDest) const
    { // these have to be defined here since they're templates
      scoped_lock_t lock(mPc->mMutex);
      std::copy(aBegin, aEnd, aDest);
    }

  private:

    // The last time that data was read by this client in [s].
    double mLastTime;

    // A boost::signal which is used for our callbacks.
    // The signal will normally be of a type such as:
    // - boost::signal<void ()>
    // - boost::signal<void (T)>
    // where T can be any type.
    //
    // @attention we currently only use signals that return void because we
    // don't have checks to make sure a signal is registered.  If an empty
    // signal is called:
    //
    // @attention "Calling the function call operator may invoke undefined
    // behavior if no slots are connected to the signal, depending on the
    // combiner used. The default combiner is well-defined for zero slots when
    // the return type is void but is undefined when the return type is any
    // other type (because there is no way to synthesize a return value)."
    //
    read_signal_t mReadSignal;

    // Outputs the signal if there is new data
    void ReadSignal();

  public:

    ///  Returns true if we have received any data from the device.
    bool IsValid() const { return 0!=GetVar(mInfo->datatime); };

    /// Fresh is set to true on each new read.  It is up to the user to
    /// set it to false if the data has already been read.  This is most
    /// useful when used in conjunction with the PlayerMultiClient
    bool IsFresh() const { return GetVar(mFresh); };

    /// This states that the data in a client is currently not Fresh
    void NotFresh();

    ///  Returns the driver name
    ///  @todo GetDriverName isn't guarded by locks yet
    std::string GetDriverName() const { return mInfo->drivername; };

    /// Returns the received timestamp [s]
    double GetDataTime() const { return GetVar(mInfo->datatime); };

    /// Returns the received timestamp [s]
    double GetElapsedTime() const
      { return GetVar(mInfo->datatime) - GetVar(mInfo->lasttime); };

    /// Returns device index
    uint GetIndex() const { return GetVar(mInfo->addr.index); };

    /// Returns device interface
    uint GetInterface() const { return GetVar(mInfo->addr.interf); };

    /// Returns device interface
    std::string GetInterfaceStr() const
      { return playerc_lookup_name(GetVar(mInfo->addr.interf)); };

    /// Connect a signal to this proxy
    /// For more information check out @ref player_clientlib_multi
    template<typename T>
    connection_t ConnectReadSignal(T aSubscriber)
      {
#ifdef HAVE_BOOST_SIGNALS
        scoped_lock_t lock(mPc->mMutex);
        return mReadSignal.connect(aSubscriber);
#else
        return -1;
#endif
      }

    /// Disconnect a signal to this proxy
    void DisconnectReadSignal(connection_t aSubscriber)
      {
#ifdef HAVE_BOOST_SIGNALS
        scoped_lock_t lock(mPc->mMutex);
        aSubscriber.disconnect();
#endif
      }

};

/** @} (core) */

#if 0

/**
 * The @p SomethingProxy class is a template for adding new subclasses of
 * ClientProxy.  You need to have at least all of the following:
 */
class SomethingProxy : public ClientProxy
{

  private:

    // Subscribe
    void Subscribe(uint aIndex);
    // Unsubscribe
    void Unsubscribe();

    // libplayerc data structure
    playerc_something_t *mDevice;

  public:
    // Constructor
    SomethingProxy(PlayerClient *aPc, uint aIndex=0);
    // Destructor
    ~SomethingProxy();

};

#endif

/** @ingroup player_clientlib_cplusplus
 * @defgroup player_clientlib_cplusplus_proxies Proxies
 * @brief A proxy class is associated with each kind of device

  The proxies all inherit from @p ClientProxy and implement the functions
  from @ref player_clientlib_libplayerc.
*/
/** @{ */

// ==============================================================
//
// These are alphabetized, please keep them that way!!!
//
// ==============================================================

/**
The @p ActArrayProxy class is used to control a @ref interface_actarray
device.
 */
class ActArrayProxy : public ClientProxy
{
  private:

   void Subscribe(uint aIndex);
   void Unsubscribe();

   // libplayerc data structure
   playerc_actarray_t *mDevice;

  public:

    /// Default constructor
    ActArrayProxy(PlayerClient *aPc, uint aIndex=0);
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
    void SetSpeedConfig(uint aJoint, float aSpeed);

    /// Send an actuator to a position
    void MoveTo(uint aJoint, float aPos);
    /// Move an actuator at a speed
    void MoveAtSpeed(uint aJoint, float aSpeed);
    /// Send an actuator, or all actuators, home
    void MoveHome(int aJoint);

    /// Gets the number of actuators in the array
    uint GetCount(void) const { return GetVar(mDevice->actuators_count); }
    /// Accessor method for getting an actuator's data
    player_actarray_actuator_t GetActuatorData(uint aJoint) const;
    /// Same again for getting actuator geometry
    player_actarray_actuatorgeom_t GetActuatorGeom(uint aJoint) const;

    /// Actuator data access operator.
    ///    This operator provides an alternate way of access the actuator data.
    ///    For example, given a @p ActArrayProxy named @p ap, the following
    ///    expressions are equivalent: @p ap.GetActuatorData[0] and @p ap[0].
    player_actarray_actuator_t operator [](uint aJoint)
      { return(GetActuatorData(aJoint)); }
};

/**
The @p AioProxy class is used to read from a @ref interface_aio
(analog I/O) device.  */
class AioProxy : public ClientProxy
{
  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_aio_t *mDevice;

  public:

    AioProxy (PlayerClient *aPc, uint aIndex=0);
    ~AioProxy();

    /// Accessor function
    uint GetCount() const { return(GetVar(mDevice->voltages_count)); };

    /// Accessor function
    double GetVoltage(uint aIndex)  const
      { return(GetVar(mDevice->voltages[aIndex])); };

    /// Set the output voltage
    void SetVoltage(uint aIndex, double aVoltage);

    /// AioProxy data access operator.
    ///    This operator provides an alternate way of access the actuator data.
    ///    For example, given a @p AioProxy named @p bp, the following
    ///    expressions are equivalent: @p ap.GetVoltage(0) and @p ap[0].
    bool operator [](uint aIndex) const
      { return GetVoltage(aIndex); }

};

#if 0 // Not in libplayerc

/**
The @p AudioProxy class controls an @ref interface_audio device.
*/
class AudioProxy : public ClientProxy
{

  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_audio_t *mDevice;

  public:

    AudioProxy(PlayerClient *aPc, uint aIndex=0)
    ~AudioProxy();

    uint GetCount() const { return(GetVar(mDevice->count)); };

    double GetFrequency(uint aIndex) const
      { return(GetVar(mDevice->frequency[aIndex])); };
    double GetAmplitude(uint aIndex) const
      { return(GetVar(mDevice->amplitude[aIndex])); };

    // Play a fixed-frequency tone
    void PlayTone(uint aFreq, uint aAmp, uint aDur);
};

/**
The @p AudioDspProxy class controls an @ref interface_audiodsp device.

@todo can we make this a subclass of @ref AudioProxy?
*/
class AudioDspProxy : public ClientProxy
{
  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_audiodsp_t *mDevice;

  public:
    AudioDspProxy(PlayerClient *aPc, uint aIndex=0);
    ~AudioDspProxy

    /** Format code of each sample */
    uint SetFormat(int aFormat);

    /** Rate at which to sample (Hz) */
    uint SetRate(uint aRate);

    uint GetCount() const { return(GetVar(mDevice->count)); };

    double GetFrequency(uint aIndex) const
      { return(GetVar(mDevice->frequency[aIndex])); };
    double GetAmplitude(uint aIndex) const
      { return(GetVar(mDevice->amplitude[aIndex])); };

    void Configure(uint aChan, uint aRate, int16_t aFormat=0xFFFFFFFF );

    void RequestConfigure();

    /// Play a fixed-frequency tone
    void PlayTone(uint aFreq, uint aAmp, uint aDur);
    void PlayChirp(uint aFreq, uint aAmp, uint aDur,
                   const uint8_t aBitString, uint aBitStringLen);
    void Replay();

    /// Print the current data.
    void Print ();
};

/**
The @p AudioMixerProxy class controls an @ref interface_audiomixer device.
*/
class AudioMixerProxy : public ClientProxy
{
  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_audiodsp_t *mDevice;

  public:

    AudioMixerProxy (PlayerClient *aPc, uint aIndex=0);

    void GetConfigure();

    void SetMaster(uint aLeft, uint aRight);
    void SetPCM(uint aLeft, uint aRight);
    void SetLine(uint aLeft, uint aRight);
    void SetMic(uint aLeft, uint aRight);
    void SetIGain(uint aGain);
    void SetOGain(uint aGain);

};

/**
The @p BlinkenlightProxy class is used to enable and disable
a flashing indicator light, and to set its period, via a @ref
interface_blinkenlight device */
class BlinkenLightProxy : public ClientProxy
{
  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_blinkenlight_t *mDevice;

  public:
    /** Constructor.
        Leave the access field empty to start unconnected.
    */
    BlinkenLightProxy(PlayerClient *aPc, uint aIndex=0);
    ~BlinkenLightProxy();

    // true: indicator light enabled, false: disabled.
    bool GetEnable();

    /** The current period (one whole on/off cycle) of the blinking
        light. If the period is zero and the light is enabled, the light
        is on.
    */
    void SetPeriod(double aPeriod);

    /** Set the state of the indicator light. A period of zero means
        the light will be unblinkingly on or off. Returns 0 on
        success, else -1.
      */
    void SetEnable(bool aSet);
};

#endif

/**
The @p BlobfinderProxy class is used to control a  @ref
interface_blobfinder device.  It contains no methods.  The latest
color blob data is stored in @p blobs, a dynamically allocated 2-D array,
indexed by color channel.
*/
class BlobfinderProxy : public ClientProxy
{
  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_blobfinder_t *mDevice;

  public:
    /// default contsructor
    BlobfinderProxy(PlayerClient *aPc, uint aIndex=0);
    /// destructor
    ~BlobfinderProxy();

    /// returns the number of blobs
    uint GetCount() const { return GetVar(mDevice->blobs_count); };
    /// returns a blob
    playerc_blobfinder_blob_t GetBlob(uint aIndex) const
      { return GetVar(mDevice->blobs[aIndex]);};

    /// get the width of the image
    uint GetWidth() const { return GetVar(mDevice->width); };
    /// get the height of the image
    uint GetHeight() const { return GetVar(mDevice->height); };

    /// Blobfinder data access operator.
    ///    This operator provides an alternate way of access the actuator data.
    ///    For example, given a @p BlobfinderProxy named @p bp, the following
    ///    expressions are equivalent: @p bp.GetBlob[0] and @p bp[0].
    playerc_blobfinder_blob_t operator [](uint aIndex) const
      { return(GetBlob(aIndex)); }

/*
    /// Set the color to be tracked
    void SetTrackingColor(uint aReMin=0,   uint aReMax=255, uint aGrMin=0,
                          uint aGrMax=255, uint aBlMin=0,   uint aBlMax=255);
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
class BumperProxy : public ClientProxy
{

  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_bumper_t *mDevice;

  public:

    BumperProxy(PlayerClient *aPc, uint aIndex=0);
    ~BumperProxy();

    uint GetCount() const { return GetVar(mDevice->bumper_count); };

    /// Returns true if the specified bumper has been bumped, false otherwise.
    uint IsBumped(uint aIndex) const
      { return GetVar(mDevice->bumpers[aIndex]); };

    /// Returns true if any bumper has been bumped, false otherwise.
    bool IsAnyBumped();

    /// Requests the geometries of the bumpers.
    void RequestBumperConfig();

    /// Returns the number bumper poses
    uint GetPoseCount() const { return GetVar(mDevice->pose_count); };

    /// Returns a specific bumper pose
    player_bumper_define_t GetPose(uint aIndex) const
      { return GetVar(mDevice->poses[aIndex]); };

    /// BumperProxy data access operator.
    ///    This operator provides an alternate way of access the actuator data.
    ///    For example, given a @p BumperProxy named @p bp, the following
    ///    expressions are equivalent: @p bp.IsBumped[0] and @p bp[0].
    bool operator [](uint aIndex) const
      { return IsBumped(aIndex); }

};

/**
The @p CameraProxy class can be used to get images from a @ref
interface_camera device. */
class CameraProxy : public ClientProxy
{

  private:

    virtual void Subscribe(uint aIndex);
    virtual void Unsubscribe();

    // libplayerc data structure
    playerc_camera_t *mDevice;

    std::string mPrefix;
    int mFrameNo;

  public:

    /// Constructor
    CameraProxy (PlayerClient *aPc, uint aIndex=0);

    virtual ~CameraProxy();

    /// Save the frame
    void SaveFrame(const std::string aPrefix, uint aWidth=4);

    /// decompress the image
    void Decompress();

    /// Image color depth
    uint GetDepth() const { return GetVar(mDevice->bpp); };

    /// Image dimensions (pixels)
    uint GetWidth() const { return GetVar(mDevice->width); };

    /// Image dimensions (pixels)
    uint GetHeight() const { return GetVar(mDevice->height); };

    /// Image format (e.g., RGB888)
    uint GetFormat() const { return GetVar(mDevice->format); };

    /// Size of the image (bytes)
    uint GetImageSize() const { return GetVar(mDevice->image_count); };

    /// Image data
    void GetImage(uint8_t* aImage) const
      {
        return GetVarByRef(mDevice->image,
                           mDevice->image+GetVar(mDevice->image_count),
                           aImage);
      };

    /// What is the compression type
    uint GetCompression() const { return GetVar(mDevice->compression); };

};


/**
The @p DioProxy class is used to read from a @ref interface_dio
(digital I/O) device.
*/
class DioProxy : public ClientProxy
{
  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_dio_t *mDevice;

  public:
    /// constructor
    DioProxy(PlayerClient *aPc, uint aIndex=0);
    /// destructor
    ~DioProxy();

    /// The number of valid digital inputs.
    uint GetCount() const { return GetVar(mDevice->count); };

    /// A bitfield of the current digital inputs.
    uint32_t GetDigin() const { return GetVar(mDevice->digin); };

    /// Set the output to the bitfield aDigout
    void SetOutput(uint aCount, uint32_t aDigout);
};

#if 0 // not libplayerc yet


/**
The @p EnergyProxy class is used to read from an @ref
interface_energy device.
*/
class EnergyProxy : public ClientProxy
{
  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_energy_t *mDevice;

public:

    EnergyProxy(PlayerClient *aPc, uint aIndex=0);
    ~EnergyProxy();

    /** These members give the current amount of energy stored [Joules] */
    uint GetJoules() const { return GetVar(mDevice->joules); };
    /** the amount of energy current being consumed [Watts] */
    uint GetWatts() const { return GetVar(mDevice->watts); };
    /** The charging flag is true if we are currently charging, else false. */
    bool GetCharging() const { return GetVar(mDevice->charging); };
};

#endif

/**
The @p FiducialProxy class is used to control @ref
interface_fiducial devices.  The latest set of detected beacons
is stored in the @p beacons array.
*/
class FiducialProxy : public ClientProxy
{
  private:
    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_fiducial_t *mDevice;

  public:
    /// constructor
    FiducialProxy(PlayerClient *aPc, uint aIndex=0);
    /// destructor
    ~FiducialProxy();

    /// The number of beacons detected
    uint GetCount() const { return GetVar(mDevice->fiducials_count); };

    /// Get detected beacon description
    player_fiducial_item_t GetFiducialItem(uint aIndex) const
      { return GetVar(mDevice->fiducials[aIndex]);};

    /// The pose of the sensor
    player_pose_t GetSensorPose() const
      { return GetVar(mDevice->fiducial_geom.pose);};

    /// The size of the sensor
    player_bbox_t GetSensorSize() const
      { return GetVar(mDevice->fiducial_geom.size);};

    /// The size of the most recently detected fiducial
    player_bbox_t GetFiducialSize() const
      { return GetVar(mDevice->fiducial_geom.fiducial_size);};

    /// Get the sensor's geometry configuration
    void RequestGeometry();

    /// FiducialProxy data access operator.
    ///    This operator provides an alternate way of access the actuator data.
    ///    For example, given a @p FiducialProxy named @p fp, the following
    ///    expressions are equivalent: @p fp.GetFiducialItem[0] and @p fp[0].
    player_fiducial_item_t operator [](uint aIndex) const
      { return GetFiducialItem(aIndex); }
};

/**
The @p GpsProxy class is used to control a @ref interface_gps
device.  The latest pose data is stored in three class attributes.  */
class GpsProxy : public ClientProxy
{

  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_gps_t *mDevice;

  public:

    // Constructor
    GpsProxy(PlayerClient *aPc, uint aIndex=0);

    ~GpsProxy();

    /// Latitude and longitude, in degrees.
    double GetLatitude() const { return GetVar(mDevice->lat); };
    double GetLongitude() const { return GetVar(mDevice->lon); };

    /// Altitude, in meters.
    double GetAltitude() const { return GetVar(mDevice->alt); };

    /// Number of satellites in view.
    uint GetSatellites() const { return GetVar(mDevice->sat_count); };

    /// Fix quality
    uint GetQuality() const { return GetVar(mDevice->quality); };

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
class Graphics2dProxy : public ClientProxy
{

  private:

    // Subscribe
    void Subscribe(uint aIndex);
    // Unsubscribe
    void Unsubscribe();

    // libplayerc data structure
    playerc_graphics2d_t *mDevice;

  public:
    // Constructor
    Graphics2dProxy(PlayerClient *aPc, uint aIndex=0);
    // Destructor
    ~Graphics2dProxy();
    
    /// Set the current pen color
    void Color( player_color_t col );

    /// Set the current pen color
    void Color( uint8_t red,  uint8_t green,  uint8_t blue,  uint8_t alpha );

    /// Clear the drawing area
    void Clear( void );

    /// Draw a set of points
    void DrawPoints( player_point_2d_t pts[], int count );

    /// Draw a polygon defined by a set of points
    void DrawPolygon( player_point_2d_t pts[], int count, bool filled, player_color_t fill_color );
    
    /// Draw a line connecting  set of points
    void DrawPolyline( player_point_2d_t pts[], int count );
};

/**
The @p GripperProxy class is used to control a @ref
interface_gripper device.  The latest gripper data held in a
handful of class attributes.  A single method provides user control.
*/
class GripperProxy : public ClientProxy
{

  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_gripper_t *mDevice;

  public:

    /// constructor
    GripperProxy(PlayerClient *aPc, uint aIndex=0);
    /// destructor
    ~GripperProxy();

    ///
    uint GetState() const { return GetVar(mDevice->state); };
    ///
    uint GetBeams() const { return GetVar(mDevice->beams); };
    ///
    uint GetOuterBreakBeam() const
      { return GetVar(mDevice->outer_break_beam); };
    ///
    uint GetInnerBreakBeam() const
      { return GetVar(mDevice->inner_break_beam); };
    ///
    uint GetPaddlesOpen() const
      { return GetVar(mDevice->paddles_open); };
    ///
    uint GetPaddlesClosed() const
      { return GetVar(mDevice->paddles_closed); };
    ///
    uint GetPaddlesMoving() const
      { return GetVar(mDevice->paddles_moving); };
    ///
    uint GetGripperError() const
      { return GetVar(mDevice->gripper_error); };
    ///
    uint GetLiftUp() const { return GetVar(mDevice->lift_up); };
    ///
    uint GetLiftDown() const { return GetVar(mDevice->lift_down); };
    ///
    uint GetLiftMoving() const { return GetVar(mDevice->lift_moving); };
    ///
    uint GetLiftError() const { return GetVar(mDevice->lift_error); };

    /// Send a gripper command.  Look in the Player user manual for details
    /// on the command and argument.
    void SetGrip(uint8_t aCmd, uint8_t aArg=0);
};



/**
The @p IrProxy class is used to control an @ref interface_ir
device.
*/
class IrProxy : public ClientProxy
{

  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_ir_t *mDevice;

  public:

    /// Constructor
    IrProxy(PlayerClient *aPc, uint aIndex=0);
    /// destructor
    ~IrProxy();

    /// get the number of IR rangers
    uint GetCount() const { return GetVar(mDevice->ranges.ranges_count); };
    /// get the current range
    double GetRange(uint aIndex) const
      { return GetVar(mDevice->ranges.ranges[aIndex]); };
    /// get the current voltage
    double GetVoltage(uint aIndex) const
      { return GetVar(mDevice->ranges.voltages[aIndex]); };
    /// get the number of poses
    uint GetPoseCount() const { return GetVar(mDevice->poses.poses_count); };
    /// get a particular pose
    player_pose_t GetPose(uint aIndex) const
      {return GetVar(mDevice->poses.poses[aIndex]);};

    /// Request IR pose information
    void RequestGeom();

    /// Range access operator.
    /// This operator provides an alternate way of access the range data.
    /// For example, given a @p IrProxy named @p ip, the following
    /// expressions are equivalent: @p ip.GetRange[0] and @p ip[0].
    double operator [](uint aIndex) const
      { return GetRange(aIndex); }

};

/**
The @p LaserProxy class is used to control a @ref interface_laser
device.  The latest scan data is held in two arrays: @p ranges and @p
intensity.  The laser scan range, resolution and so on can be configured
using the Configure() method.  */
class LaserProxy : public ClientProxy
{
  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_laser_t *mDevice;

    double aMinLeft;
    double aMinRight;

    // local storage of config
    double min_angle, max_angle, scan_res, range_res;
    bool intensity;

  public:

    /// constructor
    LaserProxy(PlayerClient *aPc, uint aIndex=0);
    /// destructor
    ~LaserProxy();

    /// Number of points in scan
    uint GetCount() const { return GetVar(mDevice->scan_count); };

    /// Angular resolution of scan (radians)
    double GetScanRes() const { return GetVar(mDevice->scan_res); };

    /// Range resolution of scan (mm)
    double GetRangeRes() const { return GetVar(mDevice->range_res); };


    /// Scan range for the latest set of data (radians)
    double GetMaxAngle() const { return GetVar(mDevice->scan_start); };
    /// Scan range for the latest set of data (radians)
    double GetMinAngle() const { return GetVar(mDevice->scan_start) + GetVar(mDevice->scan_count)*GetVar(mDevice->scan_res); };

/*    /// Whether or not reflectance (i.e., intensity) values are being returned.
    bool IsIntensity() const { return GetVar(mDevice->intensity); };

    /// Scan data (polar): range (m) and bearing (radians)
    double GetScan(uint aIndex) const
      { return GetVar(mDevice->scan[aIndex]); };
*/
    /// Scan data (Cartesian): x,y (m)
    player_point_2d_t GetPoint(uint aIndex) const
      { return GetVar(mDevice->point[aIndex]); };


    /// get the range
    double GetRange(uint aIndex) const
      { return GetVar(mDevice->ranges[aIndex]); };

    /// get the intensity
    double GetIntensity(uint aIndex) const
      { return GetVar(mDevice->intensity[aIndex]); };

    /// Configure the laser scan pattern.  Angles @p min_angle and
    /// @p max_angle are measured in radians.
    /// @p scan_res is measured in units of @f$0.01^{\circ}@f$;
    /// valid values are: 25 (@f$0.25^{\circ}@f$), 50 (@f$0.5^{\circ}@f$) and
    /// @f$100 (1^{\circ}@f$).  @p range_res is measured in mm; valid values
    /// are: 1, 10, 100.  Set @p intensity to @p true to
    /// enable intensity measurements, or @p false to disable.
    void Configure(double aMinAngle,
                   double aMaxAngle,
                   uint aScanRes,
                   uint aRangeRes,
                   bool aIntensity);

    /// Get the current laser configuration; it is read into the
    /// relevant class attributes.
    void RequestConfigure();

    /// Get the laser's geometry; it is read into the
    /// relevant class attributes.
    void RequestGeom();

    /// Accessor for the pose (fill it in by calling RequestGeom)
    player_pose_t GetPose()
    {
      player_pose_t p;
      scoped_lock_t lock(mPc->mMutex);

      p.px = mDevice->pose[0];
      p.py = mDevice->pose[1];
      p.pa = mDevice->pose[2];
      return(p);
    }

    /// Accessor for the size (fill it in by calling RequestGeom)
    player_bbox_t GetSize()
    {
      player_bbox_t b;
      scoped_lock_t lock(mPc->mMutex);

      b.sl = mDevice->size[0];
      b.sw = mDevice->size[1];
      return(b);
    }

    ///
    double MinLeft () { return aMinLeft; }
    ///
    double MinRight () { return aMinRight; }

    /// Range access operator.  This operator provides an alternate
    /// way of access the range data.  For example, given an @p
    /// LaserProxy named @p lp, the following expressions are
    /// equivalent: @p lp.ranges[0], @p lp.Ranges(0),
    /// and @p lp[0].
    double operator [] (uint index) const
      { return GetRange(index);}

};

/**
The @p LimbProxy class is used to control a @ref interface_limb
device.
 */
class LimbProxy : public ClientProxy
{
  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

   // libplayerc data structure
    playerc_limb_t *mDevice;

  public:

    LimbProxy(PlayerClient *aPc, uint aIndex=0);
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
The @p LocalizeProxy class is used to control a @ref
interface_localize device, which can provide multiple pose
hypotheses for a robot.
*/
class LocalizeProxy : public ClientProxy
{

  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_localize_t *mDevice;

  public:

    /// constructor
    LocalizeProxy(PlayerClient *aPc, uint aIndex=0);
    /// destructor
    ~LocalizeProxy();

    /// Map dimensions (cells)
    // @todo should these be in a player_pose_t?
    uint GetMapSizeX() const { return GetVar(mDevice->map_size_x); };
    uint GetMapSizeY() const { return GetVar(mDevice->map_size_y); };

    // @todo should these be in a player_pose_t?
    uint GetMapTileX() const { return GetVar(mDevice->map_tile_x); };
    uint GetMapTileY() const { return GetVar(mDevice->map_tile_y); };

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
    uint GetPendingCount() const { return GetVar(mDevice->pending_count); };

    /// Number of possible poses
    uint GetHypothCount() const { return GetVar(mDevice->hypoth_count); };

    /// Array of possible poses.
    player_localize_hypoth_t GetHypoth(uint aIndex) const
      { return GetVar(mDevice->hypoths[aIndex]); };

    /// Set the current pose hypothesis (m, m, radians).
    void SetPose(double pose[3], double cov[3]);

    /// Get the number of localization hypoths.
    uint GetNumHypoths() const { return GetVar(mDevice->hypoth_count); };

    /// Get the number of particles (for particle filter-based localization
    /// systems).  Returns the number of particles.
    uint GetNumParticles() const { return GetVar(mDevice->num_particles); };
};

/**
The @p LogProxy proxy provides access to a @ref interface_log device.
*/
class LogProxy : public ClientProxy
{
  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_log_t *mDevice;

  public:
    /// Constructor
    LogProxy(PlayerClient *aPc, uint aIndex=0);

    /// Destructor
    ~LogProxy();

    /// What kind of log device is this? Either PLAYER_LOG_TYPE_READ or
    /// PLAYER_LOG_TYPE_WRITE. Call GetState() to fill it.
    int GetType() const { return GetVar(mDevice->type); };

    /// Is logging/playback enabled? Call GetState() to fill it.
    int GetState() const { return GetVar(mDevice->state); };

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
class MapProxy : public ClientProxy
{
  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_map_t *mDevice;

  public:
    /// constructor
    MapProxy(PlayerClient *aPc, uint aIndex=0);

    /// destructor
    ~MapProxy();

    /// Get the map and store it in the proxy
    void RequestMap();

    /// Return the index of the (x,y) item in the cell array
    int GetCellIndex(int x, int y) const
    { return y*GetWidth() + x; };

    /// Get the (x,y) cell
    unsigned char GetCell(int x, int y ) const
    { return GetVar(mDevice->cells[GetCellIndex(x,y)]); };

    /// Map resolution, m/cell
    double GetResolution() const { return GetVar(mDevice->resolution); };

    /// Map size, in cells
    ///    @todo should this be returned as a player_size_t?
    uint GetWidth() const { return GetVar(mDevice->width); };
    /// Map size, in cells
    /// @todo should this be returned as a player_size_t?
    uint GetHeight() const { return GetVar(mDevice->height); };

    double GetOriginX() const { return GetVar(mDevice->origin[0]); };
    double GetOriginY() const { return GetVar(mDevice->origin[1]); };

    /// Occupancy for each cell (empty = -1, unknown = 0, occupied = +1)
    void GetMap(int8_t* aMap) const
    {
      return GetVarByRef(reinterpret_cast<int8_t*>(mDevice->cells),
                         reinterpret_cast<int8_t*>(mDevice->cells+GetWidth()*GetHeight()),
                         aMap);
    };
};

#if 0
/**
The @p McomProxy class is used to exchange data with other clients
connected with the same server, through a set of named "channels" in
a @ref interface_mcom device.  For some useful (but optional)
type and constant definitions that you can use in your clients, see
playermcomtypes.h.
*/
class McomProxy : public ClientProxy
{
  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_mcom_t *mDevice;

    /** These members contain the results of the last command.
        Note: It's better to use the LastData() method. */
    player_mcom_data_t data;
    int type;
    char channel[MCOM_CHANNEL_LEN];

public:
    McomProxy(PlayerClient* pc, unsigned short index,
              unsigned char access = 'c') :
            ClientProxy(pc,PLAYER_MCOM_CODE,index,access){}

    /** Read and remove the most recent buffer in 'channel' with type 'type'.
        The result can be read with LastData() after the next call to
        PlayerClient::Read().
    */
    int Pop(int type, char channel[MCOM_CHANNEL_LEN]);

    /** Read the most recent buffer in 'channel' with type 'type'.
        The result can be read with LastData() after the next call to
        PlayerClient::Read().
        @return 0 if no error
        @return -1 on error, the channel does not exist, or the channel is empty.
    */
    int Read(int type, char channel[MCOM_CHANNEL_LEN]);

    /** Push a message 'dat' into channel 'channel' with message type 'type'. */
    int Push(int type, char channel[MCOM_CHANNEL_LEN], char dat[MCOM_DATA_LEN]);

    /** Clear all messages of type 'type' on channel 'channel' */
    int Clear(int type, char channel[MCOM_CHANNEL_LEN]);

  /** Set the capacity of the buffer using 'type' and 'channel' to 'cap'.
      Note that 'cap' is an unsigned char and must be < MCOM_N_BUFS */
  int SetCapacity(int type, char channel[MCOM_CHANNEL_LEN], unsigned char cap);

    /** Get the results of the last command (Pop or Read). Call
        PlayerClient::Read() before using.  */
    char* LastData() { return data.data; }

    /** Get the results of the last command (Pop or Read). Call
        PlayerClient::Read() before using.  */
    int LastMsgType() { return type; }

    /** Get the channel of the last command (Pop or Read). Call
        PlayerClient::Read() before using.  */
    char* LastChannel() { return channel; }

    void FillData(player_msghdr_t hdr, const char* buffer);
    void Print();
};

#endif

/**
The @p MotorProxy class is used to control a @ref interface_motor
device.  The latest motor data is contained in the attributes @p theta,
@p thetaspeed, etc. */
class MotorProxy : public ClientProxy
{
  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_motor_t *mDevice;

  public:

    /** Constructor. */
    MotorProxy(PlayerClient *aPc, uint aIndex=0);
    ~MotorProxy();

    /** Send a motor command for velocity control mode.
        Specify the angular speeds in rad/s.
        Returns: 0 if everything's ok, -1 otherwise.
    */
    void SetSpeed(double aSpeed);

    /** Send a motor command for position control mode.  Specify the
        desired angle of the motor in radians.
    */
    void GoTo(double aAngle);


    /** Enable/disable the motors.
        Set @p state to false to disable or true to enable.
        Be VERY careful with this method!  Your robot is likely to run across the
        room with the charger still attached.
    */
    void SetMotorEnable(bool aEnable);

    /** Select velocity control mode.
        The motor state is typically device independent.  This is where you
        can potentially choose between different types of control methods
        (PD, PID, etc)
    */
    void SelectVelocityControl(uint aMode);

    /** Reset odometry to (0,0,0).
        Returns: 0 if everything's ok, -1 otherwise.
    */
    void ResetOdometry();

    /** Sets the odometry to the pose @p (theta).
        Note that @p theta is in radians.
        Returns: 0 if OK, -1 else
    */
    void SetOdometry(double aAngle);

    /** Select position mode on the motor driver.
        Set @p mode for 0 for velocity mode, 1 for position mode.
        Returns: 0 if OK, -1 else
    */

    void SelectPositionMode(uint aMode);

    /** Set the PID parameters of the motor for use in velocity control mode
        Returns: 0 if OK, -1 else
    */

    void SetSpeedPID(double aKp, double aKi, double aKd);

    /** Set the PID parameters of the motor for use in position control mode
        Returns: 0 if OK, -1 else
    */

    void SetPositionPID(double aKp, double aKi, double aKd);

    /** Set the ramp profile of the motor when in position control mode
        spd is in rad/s and acc is in rad/s/s
        Returns: 0 if OK, -1 else
    */

    void SetPositionSpeedProfile(double aSpd, double aAcc);

    /// Accessor method
    double GetPos() const { return GetVar(mDevice->pt); };

    /// Accessor method
    double GetSpeed() const { return GetVar(mDevice->vt); };

    /// Accessor method
    uint GetStall() const { return GetVar(mDevice->stall); };

    /// Accessor method
    bool IsLimitMin() const
      { return GetVar(mDevice->limits) && PLAYER_MOTOR_LIMIT_MIN > 0; };

    /// Accessor method
    bool IsLimitCenter() const
      { return GetVar(mDevice->limits) && PLAYER_MOTOR_LIMIT_CENTER > 0; };

    /// Accessor method
    bool IsLimitMax() const
      { return GetVar(mDevice->limits) && PLAYER_MOTOR_LIMIT_MAX > 0; };

};

/**
The @p PlannerProxy proxy provides an interface to a 2D motion @ref
interface_planner. */
class PlannerProxy : public ClientProxy
{

  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_planner_t *mDevice;

  public:

    /// constructor
    PlannerProxy(PlayerClient *aPc, uint aIndex=0);
    /// destructor
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
    uint GetPathValid() const { return GetVar(mDevice->path_valid); };

    /// Have we arrived at the goal?
    uint GetPathDone() const { return GetVar(mDevice->path_done); };

    /// Current pose (m)
    double GetPx() const { return GetVar(mDevice->px); };
    /// Current pose (m)
    double GetPy() const { return GetVar(mDevice->py); };
    /// Current pose (m)
    double GetPz() const { return GetVar(mDevice->pa); };

    /// Goal location (radians)
    double GetGx() const { return GetVar(mDevice->gx); };
    /// Goal location (radians)
    double GetGy() const { return GetVar(mDevice->gy); };
    /// Goal location (radians)
    double GetGz() const { return GetVar(mDevice->ga); };

    /// Current waypoint location (m)
    double GetWx() const { return GetVar(mDevice->wx); };
    /// Current waypoint location (m)
    double GetWy() const { return GetVar(mDevice->wy); };
    /// Current waypoint location (m)
    double GetWz() const { return GetVar(mDevice->wa); };

    /// Current waypoint index (handy if you already have the list
    /// of waypoints). May be negative if there's no plan, or if
    /// the plan is done
    uint GetCurrentWaypoint() const
      { return GetVar(mDevice->curr_waypoint); };

    /// Number of waypoints in the plan
    uint GetWaypointCount() const
      { return GetVar(mDevice->waypoint_count); };

    // Get a waypoints in the current plan (m,m,radians).
    //uint GetWaypoint(uint aIndex) const
    //  { return GetVar(mDevice->waypoints[aIndex]); };

};

/**
The @p Position2dProxy class is used to control a @ref
interface_position2d device.  The latest position data is contained
in the attributes xpos, ypos, etc.  */
class Position2dProxy : public ClientProxy
{

  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_position2d_t *mDevice;

  public:

    /// constructor
    Position2dProxy(PlayerClient *aPc, uint aIndex=0);
    /// destructor
    ~Position2dProxy();

    /// Send a motor command for velocity control mode.
    /// Specify the forward, sideways, and angular speeds in m/sec, m/sec,
    /// and radians/sec, respectively.
    void SetSpeed(double aXSpeed, double aYSpeed, double aYawSpeed);

    /// Same as the previous SetSpeed(), but doesn't take the yspeed speed
    /// (so use this one for non-holonomic robots).
    void SetSpeed(double aXSpeed, double aYawSpeed)
        { return SetSpeed(aXSpeed, 0, aYawSpeed);}

    /// Send a motor command for position control mode.  Specify the
    /// desired pose of the robot in m, m, radians.
    void GoTo(double aX, double aY, double aYaw);

    /// Sets command for carlike robot
    void SetCarlike(double aXSpeed, double aDriveAngle);

    /// Get the device's geometry; it is read into the
    /// relevant class attributes.
    void RequestGeom();

    /// Accessor for the pose (fill it in by calling RequestGeom)
    player_pose_t GetPose()
    {
      player_pose_t p;
      scoped_lock_t lock(mPc->mMutex);
      p.px = mDevice->pose[0];
      p.py = mDevice->pose[1];
      p.pa = mDevice->pose[2];
      return(p);
    }

    /// Accessor for the size (fill it in by calling RequestGeom)
    player_bbox_t GetSize()
    {
      player_bbox_t b;
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

    /// Select velocity control mode.
    ///
    /// For the the p2os_position driver, set @p mode to 0 for direct wheel
    /// velocity control (default), or 1 for separate translational and
    /// rotational control.
    ///
    /// For the reb_position driver: 0 is direct velocity control, 1 is for
    /// velocity-based heading PD controller (uses DoDesiredHeading()).
    //void SelectVelocityControl(unsigned char mode);

    /// Reset odometry to (0,0,0).
    void ResetOdometry();

    /// Select position mode
    /// Set @p mode for 0 for velocity mode, 1 for position mode.
    //void SelectPositionMode(unsigned char mode);

    /// Sets the odometry to the pose @p (x, y, yaw).
    /// Note that @p x and @p y are in m and @p yaw is in radians.
    void SetOdometry(double aX, double aY, double aYaw);

    /// Set PID terms
    //void SetSpeedPID(double kp, double ki, double kd);

    /// Set PID terms
    //void SetPositionPID(double kp, double ki, double kd);

    /// Set speed ramping profile
    /// spd rad/s, acc rad/s/s
    //void SetPositionSpeedProfile(double spd, double acc);

    ///
    // void DoStraightLine(double m);

    ///
    //void DoRotation(double yawspeed);

    ///
    //void DoDesiredHeading(double yaw, double xspeed, double yawspeed);

    ///
    //void SetStatus(uint8_t cmd, uint16_t value);

    ///
    //void PlatformShutdown();

    /// Accessor method
    double  GetXPos() const { return GetVar(mDevice->px); };

    /// Accessor method
    double  GetYPos() const { return GetVar(mDevice->py); };

    /// Accessor method
    double GetYaw() const { return GetVar(mDevice->pa); };

    /// Accessor method
    double  GetXSpeed() const { return GetVar(mDevice->vx); };

    /// Accessor method
    double  GetYSpeed() const { return GetVar(mDevice->vy); };

    /// Accessor method
    double  GetYawSpeed() const { return GetVar(mDevice->va); };

    /// Accessor method
    bool GetStall() const { return GetVar(mDevice->stall); };

};

/**

The @p Position3dProxy class is used to control
a interface_position3d device.  The latest position data is
contained in the attributes xpos, ypos, etc.
*/
class Position3dProxy : public ClientProxy
{

  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_position3d_t *mDevice;

  public:

    /// constructor
    Position3dProxy(PlayerClient *aPc, uint aIndex=0);
    /// destructor
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


    /// Send a motor command for position control mode.  Specify the
    /// desired pose of the robot in m, m, m, rad, rad, rad.
    void GoTo(double aX, double aY, double aZ,
              double aRoll, double aPitch, double aYaw);

    /// Enable/disable the motors.
    /// Set @p state to 0 to disable or 1 to enable.
    /// @attention Be VERY careful with this method!  Your robot is likely
    /// to run across the room with the charger still attached.
    void SetMotorEnable(bool aEnable);

    /// Select velocity control mode.
    /// This is driver dependent.
    //void SelectVelocityControl(unsigned char mode);

    /// Reset odometry to (0,0,0).
//    void ResetOdometry() {SetOdometry(0,0,0);};

    /// Sets the odometry to the pose @p (x, y, z, roll, pitch, yaw).
    /// Note that @p x, @p y, and @p z are in m and @p roll,
    /// @p pitch, and @p yaw are in radians.
//    void SetOdometry(double aX, double aY, double aZ,
//                     double aRoll, double aPitch, double aYaw);

    /// Select position mode
    /// Set @p mode for 0 for velocity mode, 1 for position mode.
    //void SelectPositionMode(unsigned char mode);

    ///
    //void SetSpeedPID(double kp, double ki, double kd);

    ///
    //void SetPositionPID(double kp, double ki, double kd);

    /// Sets the ramp profile for position based control
    /// spd rad/s, acc rad/s/s
    //void SetPositionSpeedProfile(double spd, double acc);

    /// Accessor method
    double  GetXPos() const { return GetVar(mDevice->pos_x); };

    /// Accessor method
    double  GetYPos() const { return GetVar(mDevice->pos_y); };

    /// Accessor method
    double  GetZPos() const { return GetVar(mDevice->pos_z); };

    /// Accessor method
    double  GetRoll() const { return GetVar(mDevice->pos_roll); };

    /// Accessor method
    double  GetPitch() const { return GetVar(mDevice->pos_pitch); };

    /// Accessor method
    double  GetYaw() const { return GetVar(mDevice->pos_yaw); };

    /// Accessor method
    double  GetXSpeed() const { return GetVar(mDevice->vel_x); };

    /// Accessor method
    double  GetYSpeed() const { return GetVar(mDevice->vel_y); };

    /// Accessor method
    double  GetZSpeed() const { return GetVar(mDevice->vel_z); };

    /// Accessor method
    double  GetRollSpeed() const { return GetVar(mDevice->vel_roll); };

    /// Accessor method
    double  GetPitchSpeed() const { return GetVar(mDevice->vel_pitch); };

    /// Accessor method
    double  GetYawSpeed() const { return GetVar(mDevice->vel_yaw); };

    /// Accessor method
    bool GetStall () const { return GetVar(mDevice->stall); };
};

/**
The @p PowerProxy class controls a @ref interface_power device. */
class PowerProxy : public ClientProxy
{
  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_power_t *mDevice;

  public:
    /// constructor
    PowerProxy(PlayerClient *aPc, uint aIndex=0);
    /// destructor
    ~PowerProxy();

    /// Returns the current charge.
    double GetCharge() const { return GetVar(mDevice->charge); };

};

/**

The @p PtzProxy class is used to control a @ref interface_ptz
device.  The state of the camera can be read from the pan, tilt, zoom
attributes and changed using the SetCam() method.
*/
class PtzProxy : public ClientProxy
{

  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_ptz_t *mDevice;

  public:
    /// constructor
    PtzProxy(PlayerClient *aPc, uint aIndex=0);
    // destructor
    ~PtzProxy();

  public:

    /// Change the camera state.
    /// Specify the new @p pan, @p tilt, and @p zoom values
    /// (all degrees).
    void SetCam(double aPan, double aTilt, double aZoom);

    /// Specify new target velocities
    void SetSpeed(double aPanSpeed=0, double aTiltSpeed=0, double aZoomSpeed=0);

    /// Select new control mode.  Use either PLAYER_PTZ_POSITION_CONTROL
    /// or PLAYER_PTZ_VELOCITY_CONTROL.
    //void SelectControlMode(uint aMode);

    /// Return Pan (rad)
    double GetPan() const { return GetVar(mDevice->pan); };
    /// Return Tilt (rad)
    double GetTilt() const { return GetVar(mDevice->tilt); };
    /// Return Zoom
    double GetZoom() const { return GetVar(mDevice->zoom); };

};

/**
The @p SimulationProxy proxy provides access to a
@ref interface_simulation device.
*/
class SimulationProxy : public ClientProxy
{
  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_simulation_t *mDevice;

  public:
    /// constructor
    SimulationProxy(PlayerClient *aPc, uint aIndex=0);
    /// destructor
    ~SimulationProxy();

  /// set the 2D pose of an object in the simulator, identified by the
  /// std::string. Returns 0 on success, else a non-zero error code.
  void SetPose2d(char* identifier, double x, double y, double a);

  /// get the pose of an object in the simulator, identified by the
  /// std::string Returns 0 on success, else a non-zero error code.
  void GetPose2d(char* identifier, double& x, double& y, double& a);
};


/**
The @p SonarProxy class is used to control a @ref interface_sonar
device.  The most recent sonar range measuremts can be read from the
range attribute, or using the the [] operator.
*/
class SonarProxy : public ClientProxy
{
  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_sonar_t *mDevice;

  public:
    /// constructor
    SonarProxy(PlayerClient *aPc, uint aIndex=0);
    /// destructor
    ~SonarProxy();

    /// return the sensor count
    uint GetCount() const { return GetVar(mDevice->scan_count); };

    /// return a particular scan value
    double GetScan(uint aIndex) const
      { return GetVar(mDevice->scan[aIndex]); };
    /// This operator provides an alternate way of access the scan data.
    /// For example, SonarProxy[0] == SonarProxy.GetRange(0)
    double operator [] (uint aIndex) const { return GetScan(aIndex); }

    /// Number of valid sonar poses
    uint GetPoseCount() const { return GetVar(mDevice->pose_count); };

    /// Sonar poses (m,m,radians)
    player_pose_t GetPose(uint aIndex) const
      { return GetVar(mDevice->poses[aIndex]); };

    /// Enable/disable the sonars.
    /// Set @p state to 1 to enable, 0 to disable.
    /// Note that when sonars are disabled the client will still receive sonar
    /// data, but the ranges will always be the last value read from the sonars
    /// before they were disabled.
    //void SetEnable(bool aEnable);

    /// Request the sonar geometry.
    void RequestGeom();
};

#if 0
/**
The @p SoundProxy class is used to control a @ref interface_sound
device, which allows you to play pre-recorded sound files on a robot.
*/
class SoundProxy : public ClientProxy
{

  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_sound_t *mDevice;

  public:
    // Constructor
    SoundProxy(PlayerClient *aPc, uint aIndex=0);

    ~SoundProxy();

    /** Play the sound indicated by the index. */
    void Play(int aIndex);
};
#endif

/**
The @p SpeechProxy class is used to control a @ref interface_speech
device.  Use the say method to send things to say.
*/
class SpeechProxy : public ClientProxy
{

  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_speech_t *mDevice;

  public:
    /// constructor
    SpeechProxy(PlayerClient *aPc, uint aIndex=0);
    /// constructor
    ~SpeechProxy();

    /// Send a phrase to say.
    /// The phrase is an ASCII std::string.
    void Say(std::string aStr);
};

#if 0
/**
The @p SpeechRecognition proxy provides access to a @ref interface_speech_recognition device.
*/
class SpeechRecognitionProxy : public ClientProxy
{

  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_speech_t *mDevice;

  public:
    // Constructor
    SpeechRecognitionProxy(PlayerClient *aPc, uint aIndex=0);

    ~SpeechRecognitionProxy();

  std::string GetText();

};
#endif

#if 0
/**
The @p TruthProxy gets and sets the @e true pose of a @ref
interface_truth device [worldfile tag: truth()]. This may be
different from the pose returned by a device such as GPS or Position. If
you want to log what happened in an experiment, this is the device to use.

Setting the position of a truth device moves its parent, so you
can put a truth device on robot and teleport it around the place.
*/
class TruthProxy : public ClientProxy
{

  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_truth_t *mDevice;

  public:
    /// constructor
    TruthProxy(PlayerClient *aPc, uint aIndex=0);
    /// destructor
    ~TruthProxy();

    /// Returns x (m)
    double GetX() const { return GetVar(mDevice->pos[0]); };
    /// Returns y (m)
    double GetY() const { return GetVar(mDevice->pos[1]); };
    /// Returns z (m)
    double GetZ() const { return GetVar(mDevice->pos[2]); };
    /// Returns roll (rad)
    double GetRoll() const { return GetVar(mDevice->rot[0]); };
    /// Returns pitch (rad)
    double GetPitch() const { return GetVar(mDevice->rot[1]); };
    /// Returns yaw (rad)
    double GetYaw() const { return GetVar(mDevice->rot[2]); };

    /// Query Player about the current pose - requests the pose from the
    /// server, then fills in values for the arguments. Usually you'll
    /// just read the class attributes but this function allows you
    /// to get pose direct from the server if you need too.
    void GetPose(double *px, double *py, double *pz,
                 double *rx, double *ry, double *rz);

    /// Request a change in pose.
    void SetPose(double px, double py, double pz,
                 double rx, double ry, double rz);

    /// ???
    void SetPoseOnRoot(double px, double py, double pz,
                       double rx, double ry, double rz);

    /// Request the value returned by a fiducialfinder (and possibly a
    /// foofinser, depending on its mode), when detecting this
    /// object.
    uint GetFiducialID();

    /// Set the value returned by a fiducialfinder (and possibly a
    /// foofinser, depending on its mode), when detecting this
    /// object.
    void SetFiducialID(int16_t id);

};
/**
The @p WaveformProxy class is used to read raw digital waveforms from
a @ref interface_waveform device.  */
class WaveformProxy : public ClientProxy
{

  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_waveform_t *mDevice;

  public:
    // Constructor
    WaveformProxy(PlayerClient *aPc, uint aIndex=0);

    ~WaveformProxy();

    uint GetCount() const { return GetVar(mDevice->data_count); };

    /// sample rate in bits per second
    uint GetBitRate() const { return GetVar(mDevice->rate); };

    /// sample depth in bits
    uint GetDepth() const { return GetVar(mDevice->depth); };

    /// the data is buffered here for playback
    void GetImage(uint8_t* aBuffer) const
    {
      return GetVarByRef(mDevice->data,
                         mDevice->data+GetCount(),
                         aBuffer);
    };

    // dsp file descriptor
    //uint GetFd() const { return GetVar(mDevice->fd); };

    // set up the DSP to the current bitrate and depth
    void ConfigureDSP(uint aBitRate, uint aDepth);

    // open the sound device
    void OpenDSPforWrite();

    // Play the waveform through the DSP
    void Play();
};
#endif

/**
The @p WiFiProxy class controls a @ref interface_wifi device.  */
class WiFiProxy: public ClientProxy
{

  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_wifi_t *mDevice;

  public:
    /// constructor
    WiFiProxy(PlayerClient *aPc, uint aIndex=0);
    /// destructor
    ~WiFiProxy();

#if 0

    int GetLinkQuality(char/// ip = NULL);
    int GetLevel(char/// ip = NULL);
    int GetLeveldBm(char/// ip = NULL) { return GetLevel(ip) - 0x100; }
    int GetNoise(char/// ip = NULL);
    int GetNoisedBm(char/// ip = NULL) { return GetNoise(ip) - 0x100; }

    uint16_t GetMaxLinkQuality() { return maxqual; }
    uint8_t GetMode() { return op_mode; }

    int GetBitrate();

    char/// GetMAC(char *buf, int len);

    char/// GetIP(char *buf, int len);
    char/// GetAP(char *buf, int len);

    int AddSpyHost(char *address);
    int RemoveSpyHost(char *address);

  private:
    int GetLinkIndex(char *ip);

    /// The current wifi data.
    int link_count;
    player_wifi_link_t links[PLAYER_WIFI_MAX_LINKS];
    uint32_t throughput;
    uint8_t op_mode;
    int32_t bitrate;
    uint16_t qual_type, maxqual, maxlevel, maxnoise;

    char access_point[32];
#endif
};
/** 
The @p RFIDProxy class is used to control a  @ref interface_rfid device. */
class RFIDProxy : public ClientProxy
{

  private:

    void Subscribe(uint aIndex);
    void Unsubscribe();

    // libplayerc data structure
    playerc_rfid_t *mDevice;

  public:
    /// constructor
    RFIDProxy(PlayerClient *aPc, uint aIndex=0);
    /// destructor
    ~RFIDProxy();

    /// returns the number of RFID tags
    uint GetTagsCount() const { return GetVar(mDevice->tags_count); };
    /// returns a RFID tag
    playerc_rfidtag_t GetRFIDTag(uint aIndex) const
      { return GetVar(mDevice->tags[aIndex]);};

    /// RFID data access operator.
    ///    This operator provides an alternate way of access the actuator data.
    ///    For example, given a @p RFIDProxy named @p rp, the following
    ///    expressions are equivalent: @p rp.GetRFIDTag[0] and @p rp[0].
    playerc_rfidtag_t operator [](uint aIndex) const
      { return(GetRFIDTag(aIndex)); }
};
/** @} (proxies)*/

} // namespace PlayerCc


/** @addtogroup player_clientlib_utility Utility and error-handling functions

 @{

 */

namespace std
{
  std::ostream& operator << (std::ostream& os, const player_point_2d_t& c);
  std::ostream& operator << (std::ostream& os, const player_pose_t& c);
  std::ostream& operator << (std::ostream& os, const player_pose3d_t& c);
  std::ostream& operator << (std::ostream& os, const player_bbox_t& c);
  std::ostream& operator << (std::ostream& os, const player_segment_t& c);
  std::ostream& operator << (std::ostream& os, const playerc_device_info_t& c);

  std::ostream& operator << (std::ostream& os, const PlayerCc::ClientProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::ActArrayProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::AioProxy& c);
  //std::ostream& operator << (std::ostream& os, const PlayerCc::AudioDspProxy& c);
  //std::ostream& operator << (std::ostream& os, const PlayerCc::AudioMixerProxy& c);
  //std::ostream& operator << (std::ostream& os, const PlayerCc::BlinkenLightProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::BlobfinderProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::BumperProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::CameraProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::DioProxy& c);
  //std::ostream& operator << (std::ostream& os, const PlayerCc::EnergyProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::FiducialProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::GpsProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::GripperProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::IrProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::LaserProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::LimbProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::LocalizeProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::LogProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::MapProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::MotorProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::PlannerProxy& c);
  //std::ostream& operator << (std::ostream& os, const PlayerCc::Position1dProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::Position2dProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::Position3dProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::PowerProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::PtzProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::SimulationProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::SonarProxy& c);
  //std::ostream& operator << (std::ostream& os, const PlayerCc::SoundProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::SpeechProxy& c);
  //std::ostream& operator << (std::ostream& os, const PlayerCc::SpeechRecognitionProxy& c);
  //std::ostream& operator << (std::ostream& os, const PlayerCc::WafeformProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::WiFiProxy& c);
  std::ostream& operator << (std::ostream& os, const PlayerCc::RFIDProxy& c);
  //std::ostream& operator << (std::ostream& os, const PlayerCc::TruthProxy& c);
}
/** @} (utility) */



#endif

