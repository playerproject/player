/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey et al.
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */
///////////////////////////////////////////////////////////////////////////
//
// Desc: Canon Powershot digicam capture driver
// Author: Paul Osmialowski
// Date: 24 Jul 2010
//
///////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_powershot powershot
 * @brief Canon Powershot digicam capture driver

The powershot driver captures images from various Canon Powershot digicams.
Based on Capture tool http://capture.sourceforge.net (GNU GPL v2).

@par Compile-time dependencies

- libusb
- libptp2
- libexif

@par Provides

- @ref interface_camera

@par Requires

- none

@par Configuration requests

- none

@par Configuration file options

- repeat (integer)
  - Default: 0
  - If set to 1, keep on publishing any previously captured frame

- sleep_nsec (integer)
  - Default: 10000000 (=10ms which gives max 100 fps)
  - timespec value for nanosleep() (used when repeat = 1)

- init_commands (string tuple)
  - Default: NONE
  - See list of commands accepted by capture tool by Petr Danecek
    http://capture.sourceforge.net

- live_view (integer)
  - Default: 0
  - If set to 1, live view will be published (better framerate, worse quality)

@par Properties

- live_width (integer)
  - Default: 320
  - Image width when live_view is set to 1 (set this when 320 does not fit live image width)

- live_height (integer)
  - Default: 240
  - Image height when live_view is set to 1 (set this when 240 does not fit live image height)

@par Example

@verbatim
driver
(
  name "powershot"
  provides ["camera:0"]
  init_commands ["flash on" "size small"]
)
@endverbatim

@author Paul Osmialowski, Petr Danecek

*/
/** @} */

#include <time.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#if HAVE_STRINGS_H
#include <strings.h>
#endif
#include <assert.h>
#include <string.h>
#include <pthread.h>
#include <usb.h>
#include <libplayercore/playercore.h>
#include <libexif/exif-byte-order.h>
#include <libexif/exif-data-type.h>
#include <libexif/exif-ifd.h>
#include <libexif/exif-tag.h>
#include <libexif/exif-loader.h>
#include <libexif/exif-data.h>
#include <libexif/exif-content.h>
#include <libexif/exif-entry.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <libptp2/ptp.h>

#ifdef __cplusplus
}
#endif

#define IS_JPEG(ptr) ((((ptr)[0]) == 0xff) && (((ptr)[1]) == 0xd8))

#define MAX_BULK_SIZE 131072
#define MAX_SMALLREAD_SIZE 256

class Powershot : public ThreadedDriver
{
public:
  Powershot(ConfigFile * cf, int section);

  virtual ~Powershot();

  virtual int ProcessMessage(QueuePointer & resp_queue,
                             player_msghdr * hdr,
                             void * data);

private:
  virtual void Main();

  player_devaddr_t camera_addr;
  player_camera_data_t imgData;
  int needs_frame;
  int repeat;
  int sleep_nsec;
  int live_view;
  IntProperty live_width;
  IntProperty live_height;
  struct ptp_usb_t
  {
    usb_dev_handle * handle;
    int inep;
    int outep;
    int intep;
  };
  struct settings_t
  {
    struct usb_device * dev;
    PTPParams params;
    struct Powershot::ptp_usb_t ptp_usb;
  } settings;
  int started;
  struct allowed_t
  {
    int code;
    char string[40];
  };
  static struct allowed_t zoom_vals[];
  static struct allowed_t flash_vals[];
  static struct allowed_t macro_vals[];
  static struct allowed_t aperture_vals[];
  static struct allowed_t shutter_vals[];
  static struct allowed_t tvav_vals[];
  static struct allowed_t focuspoint_vals[];
  static struct allowed_t exposure_compensation_vals[];
  static struct allowed_t iso_speed_vals[];
  static struct allowed_t white_balance_vals[];
  static struct allowed_t photo_effect_vals[];
  static struct allowed_t light_metering_vals[];
  static struct allowed_t qual_vals[];
  static struct allowed_t size_vals[];
  static struct allowed_t focus_vals[];
  static struct cmd_t
  {
    char command[40];
    int ptpcode;
    int (* handler)(struct Powershot::settings_t *, struct Powershot::cmd_t *, char *);
    struct Powershot::allowed_t * params;
  } cmds[];
  static struct usb_device * find_device(int busn, int devn);
  static void find_endpoints(struct Powershot::settings_t * settings);
  static int ptp_usb_init(struct Powershot::settings_t * settings);
  static void ptp_usb_close(struct Powershot::settings_t * settings);
  static int magic_code(struct Powershot::settings_t * settings);
  static int usb_checkevent_wait(struct Powershot::settings_t * settings);
  static int ptp_checkevent(struct Powershot::settings_t * settings, PTPUSBEventContainer * fill_event);
  static short ptp_write_func(unsigned char * bytes, unsigned int size, void * data);
  static short ptp_read_func(unsigned char * bytes, unsigned int size, void * data);
  static short ptp_check_int_func(unsigned char * bytes, unsigned int size, void * data);
  static short ptp_check_int_fast_func(unsigned char * bytes, unsigned int size, void * data);
  static int set_prop8(struct Powershot::settings_t * settings, uint16_t prop, uint8_t val);
  static int set_prop16(struct Powershot::settings_t * settings, uint16_t prop, uint16_t val);
  static int get_prop16(struct Powershot::settings_t * settings, uint16_t prop, uint16_t * val);
  static int get_prop32(struct Powershot::settings_t * settings, uint16_t prop, uint32_t * val);
  static void do_nothing(void * data, const char * format, va_list args);
  static char * getarg(char * line, int * n);
  static int cmdcmp(char * cmd1, char * cmd2);
  static int exec_cmd(struct Powershot::settings_t * settings, char * arg);
  static int value_ok(struct Powershot::allowed_t * all, char * value);
  static int prop16_handler(struct Powershot::settings_t * settings, struct Powershot::cmd_t * cmd, char * args);
  static int prop8_handler(struct Powershot::settings_t * settings, struct Powershot::cmd_t * cmd, char * args);
  static int focuslock_handler(struct Powershot::settings_t * settings, struct Powershot::cmd_t * cmd, char * args);
  static int lock_focus(struct Powershot::settings_t * settings);
  static int unlock_focus(struct Powershot::settings_t * settings);
  static int getchanges(struct Powershot::settings_t * settings);
  static int image_size_from_exif(player_camera_data_t * camData);
};

struct Powershot::allowed_t Powershot::exposure_compensation_vals[] =
{
  { 0xff, "factory default" },
  { 8 , "+2" },
  { 11, "+1 2/3" },
  { 13, "+1 1/3" },
  { 16, "+1" },
  { 19, "+2/3" },
  { 21, "+1/3" },
  { 24,  "0" },
  { 27, "-1/3" },
  { 29, "-2/3" },
  { 32, "-1" },
  { 35, "-1 1/3" },
  { 37, "-1 2/3" },
  { 40, "-2" },
  { 0 , "" }
};

struct Powershot::allowed_t Powershot::iso_speed_vals[] =
{
  { 0xffff, "factory default"  },
  { 0x0040, "50" },
  { 0x0048, "100" },
  { 0x0050, "200" },
  { 0x0058, "400" },
  { 0x0000, "Auto" },
  { 0, "" }
};

struct Powershot::allowed_t Powershot::white_balance_vals[] =
{
  { 0, "Auto" },
  { 1, "Daylight" },
  { 2, "Cloudy" },
  { 3, "Tungsten" },
  { 4, "Fluorescent" },
  { 7, "Fluorescent H" },
  { 6, "Custom" },
  { 0, "" }
};

struct Powershot::allowed_t Powershot::photo_effect_vals[] =
{
  { 0, "off" },
  { 1, "Vivid" },
  { 2, "Neutral" },
  { 3, "Low sharpening" },
  { 4, "Sepia" },
  { 5, "Black & white" },
  { 0, "" }
};

struct Powershot::allowed_t Powershot::light_metering_vals[] =
{
  { 0, "center weighted(?)" },
  { 1, "spot" },
  { 3, "integral(?)" },
  { 0, "" }
};

struct Powershot::allowed_t Powershot::zoom_vals[] =
{
  { 0, "0" },
  { 1, "1" },
  { 2, "2" },
  { 3, "3" },
  { 4, "4" },
  { 5, "5" },
  { 6, "6" },
  { 7, "7" },
  { 8, "8" },
  { 9, "9" },
  { 10, "10" },
  { 11, "11" },
  { 12, "12" },
  { 13, "13" },
  { 14, "14" },
  { 15, "15" },
  { 0, "" }
};

struct Powershot::allowed_t Powershot::focus_vals[] =
{
  { 0, "unlock" },
  { 1, "lock" },
  { 0, "" }
};

struct Powershot::allowed_t Powershot::flash_vals[] =
{
  { 0, "off" },
  { 1, "auto" },
  { 2, "on" },
  { 5, "auto red eye" },
  { 6, "on red eye" },
  { 0, "" }
};

struct Powershot::allowed_t Powershot::tvav_vals[] =
{
  { 1, "program" },
  { 2, "shutter priority" },
  { 3, "aperture priority" },
  { 4, "manual" },
  { 0 , "" }
};

struct Powershot::allowed_t Powershot::focuspoint_vals[] =
{
  { 0x3003, "center" },
  { 0x3001, "auto" },
  { 0, "" }
};

struct Powershot::allowed_t Powershot::macro_vals[] =
{
  { 1, "off" },
  { 3, "on" },
  { 0, "" }
};

struct Powershot::allowed_t Powershot::shutter_vals[] =
{
  { 0     , "auto" },
  { 0x0018, "15\"" },
  { 0x001b, "13\"" },
  { 0x001d, "10\"" },
  { 0x0020, "8\"" },
  { 0x0023, "6\"" },
  { 0x0025, "5\"" },
  { 0x0028, "4\"" },
  { 0x002b, "3\"2" },
  { 0x002d, "2\"5" },
  { 0x0030, "2\"" },
  { 0x0033, "1\"6" },
  { 0x0035, "1\"3" },
  { 0x0038, "1\"" },
  { 0x003b, "0\"8" },
  { 0x003d, "0\"6" },
  { 0x0040, "0\"5" },
  { 0x0043, "0\"4" },
  { 0x0045, "0\"3" },
  { 0x0048, "1/4" },
  { 0x004b, "1/5" },
  { 0x004d, "1/6" },
  { 0x0050, "1/8" },
  { 0x0053, "1/10" },
  { 0x0055, "1/13" },
  { 0x0058, "1/15" },
  { 0x005b, "1/20" },
  { 0x005d, "1/25" },
  { 0x0060, "1/30" },
  { 0x0063, "1/40" },
  { 0x0065, "1/50" },
  { 0x0068, "1/60" },
  { 0x006b, "1/80" },
  { 0x006d, "1/100" },
  { 0x0070, "1/125" },
  { 0x0073, "1/160" },
  { 0x0075, "1/200" },
  { 0x0078, "1/250" },
  { 0x007b, "1/320" },
  { 0x007d, "1/400" },
  { 0x0080, "1/500" },
  { 0x0083, "1/640" },
  { 0x0085, "1/800" },
  { 0x0088, "1/1000" },
  { 0x008b, "1/1250" },
  { 0x008d, "1/1600" },
  { 0x0090, "1/2000" },
  { 0, "" }
};

struct Powershot::allowed_t Powershot::aperture_vals[] =
{
  { 0xffff, "auto" },
  { 0x0018, "2.0" },    /* Added on request for G5 */
  { 0x001B, "2.2" },    /* Added on request for G5 */
  { 0x001D, "2.5" },    /* Added on request for G5 */
  { 0x0020, "2.8" },
  { 0x0023, "3.2" },
  { 0x0025, "3.5" },
  { 0x0028, "4.0" },
  { 0x002b, "4.5" },
  { 0x0030, "5.6" },
  { 0x0033, "6.3" },
  { 0x0035, "7.1" },
  { 0x0038, "8.0" },
  { 0x002d, "5" },
  { 0 , "" }
};

struct Powershot::allowed_t Powershot::qual_vals[] =
{
  { 5, "superfine" },
  { 3, "fine" },
  { 2, "normal" },
  { 0 , "" }
};

struct Powershot::allowed_t Powershot::size_vals[] =
{
  { 0, "large" },
  { 1, "medium1" },
  { 3, "medium2" },
  { 7, "medium3" },
  { 2, "small" },
  { 0 , "" }
};

struct Powershot::cmd_t Powershot::cmds[] =
{
  { "zoom", PTP_DPC_CANON_Zoom, Powershot::prop16_handler, Powershot::zoom_vals },
  { "flash", PTP_DPC_CANON_FlashMode, Powershot::prop8_handler, Powershot::flash_vals },
  { "macro", PTP_DPC_CANON_MacroMode, Powershot::prop8_handler, Powershot::macro_vals },
  { "aperture", PTP_DPC_CANON_Aperture, Powershot::prop16_handler, Powershot::aperture_vals },
  { "shutter", PTP_DPC_CANON_ShutterSpeed, Powershot::prop16_handler, Powershot::shutter_vals },
  { "tv/av", PTP_DPC_CANON_TvAvSetting, Powershot::prop8_handler, Powershot::tvav_vals },
  { "focuspoint", PTP_DPC_CANON_FocusingPoint, Powershot::prop16_handler, Powershot::focuspoint_vals },
  { "ecomp", PTP_DPC_CANON_ExpCompensation, Powershot::prop8_handler, Powershot::exposure_compensation_vals },
  { "iso", PTP_DPC_CANON_ISOSpeed, Powershot::prop16_handler, Powershot::iso_speed_vals },
  { "white", PTP_DPC_CANON_WhiteBalance, Powershot::prop8_handler, Powershot::white_balance_vals },
  { "effect", PTP_DPC_CANON_PhotoEffect, Powershot::prop16_handler, Powershot::photo_effect_vals },
  { "metering", PTP_DPC_CANON_MeteringMode, Powershot::prop8_handler, Powershot::light_metering_vals },
  { "qual", PTP_DPC_CANON_ImageQuality, Powershot::prop8_handler, Powershot::qual_vals },
  { "size", PTP_DPC_CANON_ImageSize, Powershot::prop8_handler, Powershot::size_vals },
  { "focus", 0, Powershot::focuslock_handler, Powershot::focus_vals },
  { "", 0, NULL, NULL }
};

Powershot::Powershot(ConfigFile * cf, int section)
    : ThreadedDriver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN),
      live_width("live_width", 320, false),
      live_height("live_height", 240, false)
{
  int i, stat, opts;
  struct timespec ts;
  char cmd[80];
  const char * str;

  assert(PTP_RC_OK != 0); // logical assumptions verification
  this->RegisterProperty("live_width", &(this->live_width), cf, section);
  if ((this->live_width.GetValue()) <= 0)
  {
    PLAYER_ERROR("invalid live_widht value");
    this->SetError(-1);
    return;
  }
  this->RegisterProperty("live_height", &(this->live_height), cf, section);
  if ((this->live_height.GetValue()) <= 0)
  {
    PLAYER_ERROR("invalid live_height value");
    this->SetError(-1);
    return;
  }
  memset(&(this->camera_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->imgData), 0, sizeof this->imgData);
  memset(&(this->settings), 0, sizeof this->settings);
  this->imgData.image_count = 0;
  this->imgData.image = NULL;
  this->needs_frame = 0;
  this->repeat = 0;
  this->sleep_nsec = 0;
  this->live_view = 0;
  this->started = 0;
  if (cf->ReadDeviceAddr(&(this->camera_addr), section, "provides", PLAYER_CAMERA_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->camera_addr))
  {
    this->SetError(-1);
    return;
  }
  this->repeat = cf->ReadInt(section, "repeat", 0);
  this->sleep_nsec = cf->ReadInt(section, "sleep_nsec", 10000000);
  if ((this->sleep_nsec) < 0)
  {
    PLAYER_ERROR("Invalid sleep_nsec value");
    this->SetError(-1);
    return;
  }
  this->live_view = cf->ReadInt(section, "live_view", 0);
  if (this->live_view) this->repeat = 0;
  for (i = 0; i < 5; i++)
  {
    if (i)
    {
      ts.tv_sec = 1;
      ts.tv_nsec = 0;
      nanosleep(&ts, NULL);
    }
    this->settings.dev = Powershot::find_device(0, 0);
    if (!(this->settings.dev)) continue;
    Powershot::find_endpoints(&(this->settings));
    if (!(Powershot::ptp_usb_init(&(this->settings))))
    {
      PLAYER_WARN("could not init ptp_usb");
    }
    ts.tv_sec = 1;
    ts.tv_nsec = 0;
    nanosleep(&ts, NULL);
    if ((ptp_opensession(&(this->settings.params), 1)) != PTP_RC_OK)
    {
      PLAYER_WARN("could not open session");
      Powershot::ptp_usb_close(&(this->settings));
      continue;
    }
    break;
  }
  if (Powershot::magic_code(&(this->settings)) != PTP_RC_OK)
  {
    PLAYER_ERROR("No magic can be done");
    stat = Powershot::usb_checkevent_wait(&(this->settings));
    Powershot::ptp_checkevent(&(this->settings), NULL);
    Powershot::ptp_checkevent(&(this->settings), NULL);
    if (stat != PTP_RC_OK) Powershot::usb_checkevent_wait(&(this->settings));
    ptp_closesession(&(this->settings.params));
    Powershot::ptp_usb_close(&(this->settings));
    this->SetError(-1);
    return;
  }
  if (ptp_canon_startshootingmode(&(this->settings.params)) != PTP_RC_OK)
  {
    PLAYER_ERROR("Cannot start shooting mode");
    stat = Powershot::usb_checkevent_wait(&(this->settings));
    Powershot::ptp_checkevent(&(this->settings), NULL);
    Powershot::ptp_checkevent(&(this->settings), NULL);
    if (stat != PTP_RC_OK) Powershot::usb_checkevent_wait(&(this->settings));
    ptp_closesession(&(this->settings.params));
    Powershot::ptp_usb_close(&(this->settings));
    this->SetError(-1);
    return;
  }
  Powershot::usb_checkevent_wait(&(this->settings));
  Powershot::ptp_checkevent(&(this->settings), NULL);
  if (Powershot::set_prop16(&(this->settings), PTP_DPC_CANON_D029, 3) != PTP_RC_OK)
  {
    PLAYER_ERROR("Cannot set PTP_DPC_CANON_D029");
    if (ptp_canon_endshootingmode(&(this->settings.params)) != PTP_RC_OK) PLAYER_ERROR("endshootingmode cannot be done");
    stat = Powershot::usb_checkevent_wait(&(this->settings));
    Powershot::ptp_checkevent(&(this->settings), NULL);
    Powershot::ptp_checkevent(&(this->settings), NULL);
    if (stat != PTP_RC_OK) Powershot::usb_checkevent_wait(&(this->settings));
    ptp_closesession(&(this->settings.params));
    Powershot::ptp_usb_close(&(this->settings));
    this->SetError(-1);
    return;
  }
  this->started = !0;
  opts = cf->GetTupleCount(section, "init_commands");
  for (i = 0; i < opts; i++)
  {
    str = cf->ReadTupleString(section, "init_commands", i, NULL);
    if (!str)
    {
      PLAYER_ERROR("NULL init command");
      if (ptp_canon_endshootingmode(&(this->settings.params)) != PTP_RC_OK) PLAYER_ERROR("endshootingmode cannot be done");
      stat = Powershot::usb_checkevent_wait(&(this->settings));
      Powershot::ptp_checkevent(&(this->settings), NULL);
      Powershot::ptp_checkevent(&(this->settings), NULL);
      if (stat != PTP_RC_OK) Powershot::usb_checkevent_wait(&(this->settings));
      ptp_closesession(&(this->settings.params));
      Powershot::ptp_usb_close(&(this->settings));
      this->started = 0;
      this->SetError(-1);
      return;
    }
    snprintf(cmd, sizeof cmd, "%s", str);
    if (Powershot::exec_cmd(&(this->settings), cmd) != PTP_RC_OK)
    {
      PLAYER_ERROR1("Cannot execute command [%s]", cmd);
      if (ptp_canon_endshootingmode(&(this->settings.params)) != PTP_RC_OK) PLAYER_ERROR("endshootingmode cannot be done");
      stat = Powershot::usb_checkevent_wait(&(this->settings));
      Powershot::ptp_checkevent(&(this->settings), NULL);
      Powershot::ptp_checkevent(&(this->settings), NULL);
      if (stat != PTP_RC_OK) Powershot::usb_checkevent_wait(&(this->settings));
      ptp_closesession(&(this->settings.params));
      Powershot::ptp_usb_close(&(this->settings));
      this->started = 0;
      this->SetError(-1);
      return;
    }
  }
  if (this->live_view)
  {
    if (ptp_canon_reflectchanges(&(this->settings.params), 7) != PTP_RC_OK)
    {
      PLAYER_ERROR("Cannot reflect changes");
      if (ptp_canon_endshootingmode(&(this->settings.params)) != PTP_RC_OK) PLAYER_ERROR("endshootingmode cannot be done");
      stat = Powershot::usb_checkevent_wait(&(this->settings));
      Powershot::ptp_checkevent(&(this->settings), NULL);
      Powershot::ptp_checkevent(&(this->settings), NULL);
      if (stat != PTP_RC_OK) Powershot::usb_checkevent_wait(&(this->settings));
      ptp_closesession(&(this->settings.params));
      Powershot::ptp_usb_close(&(this->settings));
      this->started = 0;
      this->SetError(-1);
      return;
    }
    if (Powershot::getchanges(&(this->settings)) != PTP_RC_OK)
    {
      PLAYER_ERROR("Cannot get changes");
      if (ptp_canon_endshootingmode(&(this->settings.params)) != PTP_RC_OK) PLAYER_ERROR("endshootingmode cannot be done");
      stat = Powershot::usb_checkevent_wait(&(this->settings));
      Powershot::ptp_checkevent(&(this->settings), NULL);
      Powershot::ptp_checkevent(&(this->settings), NULL);
      if (stat != PTP_RC_OK) Powershot::usb_checkevent_wait(&(this->settings));
      ptp_closesession(&(this->settings.params));
      Powershot::ptp_usb_close(&(this->settings));
      this->started = 0;
      this->SetError(-1);
      return;
    }
    ptp_canon_viewfinderon(&(this->settings.params));
  }
}

Powershot::~Powershot()
{
  int stat;

  if (this->imgData.image) free(this->imgData.image);
  this->imgData.image_count = 0;
  this->imgData.image = NULL;
  if (this->started)
  {
    if (this->live_view) ptp_canon_viewfinderoff(&(this->settings.params));
    if (ptp_canon_endshootingmode(&(this->settings.params)) != PTP_RC_OK) PLAYER_ERROR("endshootingmode cannot be done");
    stat = Powershot::usb_checkevent_wait(&(this->settings));
    Powershot::ptp_checkevent(&(this->settings), NULL);
    Powershot::ptp_checkevent(&(this->settings), NULL);
    if (stat != PTP_RC_OK) Powershot::usb_checkevent_wait(&(this->settings));
    ptp_closesession(&(this->settings.params));
    Powershot::ptp_usb_close(&(this->settings));
    this->started = 0;
  }
}

void Powershot::do_nothing(void * data, const char * format, va_list args) { }

struct usb_device * Powershot::find_device(int busn, int devn)
{
  struct usb_bus * bus;
  struct usb_device * dev;
  int curbusn, curdevn;

  usb_init();
  usb_find_busses();
  usb_find_devices();
  for (bus = usb_get_busses(); bus; bus = bus->next)
  {
    for (dev = bus->devices; dev; dev = dev->next)
    {
      assert(dev);
      assert(dev->config);
      assert(dev->config->interface);
      assert(dev->config->interface->altsetting);
      if ((dev->config->interface->altsetting->bInterfaceClass) == USB_CLASS_PTP)
      {
        if ((dev->descriptor.bDeviceClass) != USB_CLASS_HUB)
        {
          curbusn = strtol(bus->dirname, NULL, 10);
          curdevn = strtol(dev->filename, NULL, 10);
          if (!devn)
          {
            if (!busn) return dev;
            if (curbusn == busn) return dev;
          } else
          {
            if ((!busn) && (curdevn == devn)) return dev;
            if ((curbusn == busn) && (curdevn == devn)) return dev;
          }
        }
      }
    }
  }
  return NULL;
}

void Powershot::find_endpoints(struct Powershot::settings_t * settings)
{
  int i, n;
  struct usb_endpoint_descriptor * ep;

  assert(settings);
  assert(settings->dev);
  assert(settings->dev->config);
  assert(settings->dev->config->interface);
  assert(settings->dev->config->interface->altsetting);
  ep = settings->dev->config->interface->altsetting->endpoint;
  n  = settings->dev->config->interface->altsetting->bNumEndpoints;

  for (i = 0; i < n; i++)
  {
    if (ep[i].bmAttributes == USB_ENDPOINT_TYPE_BULK) 
    {
      if (((ep[i].bEndpointAddress) & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_DIR_MASK)
      settings->ptp_usb.inep  = ep[i].bEndpointAddress;
      if (((ep[i].bEndpointAddress) & USB_ENDPOINT_DIR_MASK) == 0) settings->ptp_usb.outep = ep[i].bEndpointAddress;
    } else
    {
      if ((ep[i].bmAttributes) == USB_ENDPOINT_TYPE_INTERRUPT)
      {
        if (((ep[i].bEndpointAddress) & USB_ENDPOINT_DIR_MASK) == USB_ENDPOINT_DIR_MASK) settings->ptp_usb.intep = ep[i].bEndpointAddress;
      }
    }
  }
}

int Powershot::ptp_usb_init(struct Powershot::settings_t * settings)
{
  usb_dev_handle * device_handle;

  assert(settings);
  settings->params.write_func = Powershot::ptp_write_func;
  settings->params.read_func = Powershot::ptp_read_func;
  settings->params.check_int_func = Powershot::ptp_check_int_func;
  settings->params.check_int_fast_func = Powershot::ptp_check_int_fast_func;
  settings->params.error_func = Powershot::do_nothing;
  settings->params.debug_func = Powershot::do_nothing;
  settings->params.sendreq_func = ptp_usb_sendreq;
  settings->params.senddata_func = ptp_usb_senddata;
  settings->params.getresp_func = ptp_usb_getresp;
  settings->params.getdata_func = ptp_usb_getdata;
  settings->params.data = &(settings->ptp_usb);
  settings->params.transaction_id = 0;
  settings->params.byteorder = PTP_DL_LE;
  device_handle = usb_open(settings->dev);
  if (device_handle)
  {
    settings->ptp_usb.handle = device_handle;
    assert(settings->dev);
    assert(settings->dev->config);
    assert(settings->dev->config->interface);
    assert(settings->dev->config->interface->altsetting);
    usb_claim_interface(device_handle, settings->dev->config->interface->altsetting->bInterfaceNumber);
    return !0;
  }
  return 0;
}

int Powershot::magic_code(struct Powershot::settings_t * settings)
{
  uint32_t ui32;
  uint16_t ui16;
  int stat;

  assert(settings);
  Powershot::get_prop16(settings, 0xd045, &ui16);
  stat = Powershot::set_prop16(settings, 0xd045, 1);
  if (stat != PTP_RC_OK) return stat;
  stat = Powershot::get_prop32(settings, 0xd02e, &ui32);
  if (stat != PTP_RC_OK) return stat;
  stat = get_prop32(settings,0xd02f, &ui32);
  if (stat != PTP_RC_OK) return stat;
  stat = ptp_getdeviceinfo(&(settings->params), &(settings->params.deviceinfo));
  if (stat != PTP_RC_OK) return stat;
  stat = ptp_getdeviceinfo(&(settings->params), &(settings->params.deviceinfo));
  if (stat != PTP_RC_OK) return stat;
  stat = get_prop32(settings, 0xd02e, &ui32);
  if (stat != PTP_RC_OK) return stat;
  stat = get_prop32(settings, 0xd02f, &ui32);
  if (stat != PTP_RC_OK) return stat;
  stat = ptp_getdeviceinfo(&(settings->params), &(settings->params.deviceinfo));
  if (stat != PTP_RC_OK) return stat;
  stat = get_prop16(settings, 0xd045, &ui16);
  if (stat != PTP_RC_OK) return stat;
  stat = set_prop16(settings, 0xd045, 4);
  if (stat != PTP_RC_OK) return stat;
  return PTP_RC_OK;
}

int Powershot::usb_checkevent_wait(struct Powershot::settings_t * settings)
{
  int stat;
  PTPContainer evc;

  assert(settings);
  stat = ptp_usb_event_wait(&(settings->params), &evc);
  return stat;
}

int Powershot::ptp_checkevent(struct Powershot::settings_t * settings, PTPUSBEventContainer * fill_event)
{
  int isevent;
  PTPUSBEventContainer event;
  PTPUSBEventContainer * ev = (fill_event) ? fill_event : &event;

  assert(settings);
  assert(ev);
  if (ptp_canon_checkevent(&(settings->params), ev, &isevent) != PTP_RC_OK) return 0;
  if (!isevent) return 0;
  if (! ev->code && ev->type == PTP_USB_CONTAINER_UNDEFINED) return 0;
  return 1;
}

void Powershot::ptp_usb_close(struct Powershot::settings_t * settings)
{
  assert(settings);
  assert(settings->dev);
  assert(settings->dev->config);
  assert(settings->dev->config->interface);
  assert(settings->dev->config->interface->altsetting);
  usb_release_interface(settings->ptp_usb.handle, settings->dev->config->interface->altsetting->bInterfaceNumber);
}

short Powershot::ptp_write_func(unsigned char * bytes, unsigned int size, void * data)
{
  int result;
  struct Powershot::ptp_usb_t * ptp_usb;

  assert(bytes);
  assert(size > 0);
  assert(data);
  ptp_usb = reinterpret_cast<struct Powershot::ptp_usb_t *>(data);
  assert(ptp_usb);
  result = usb_bulk_write(ptp_usb->handle, ptp_usb->outep, reinterpret_cast<char *>(bytes), size, MAX_BULK_SIZE);
  if (result >= 0) return PTP_RC_OK;
  return PTP_ERROR_IO;
}

short Powershot::ptp_read_func(unsigned char * bytes, unsigned int size, void * data)
{
  int result;
  struct Powershot::ptp_usb_t * ptp_usb;

  assert(bytes);
  assert(size > 0);
  assert(data);
  ptp_usb = reinterpret_cast<struct Powershot::ptp_usb_t *>(data);
  assert(ptp_usb);
  result = usb_bulk_read(ptp_usb->handle, ptp_usb->inep, reinterpret_cast<char *>(bytes), size, MAX_BULK_SIZE);
  if (!result) result = usb_bulk_read(ptp_usb->handle, ptp_usb->inep, reinterpret_cast<char *>(bytes), size, MAX_BULK_SIZE);
  if (result >= 0) return PTP_RC_OK;
  return PTP_ERROR_IO;
}

short Powershot::ptp_check_int_func(unsigned char * bytes, unsigned int size, void * data)
{
  int count = 100;
  int result = 0;
  struct Powershot::ptp_usb_t * ptp_usb;

  assert(bytes);
  assert(size > 0);
  assert(data);
  ptp_usb = reinterpret_cast<struct Powershot::ptp_usb_t *>(data);
  assert(ptp_usb);
  while (!result && count)
  {
    result = usb_bulk_read(ptp_usb->handle, ptp_usb->intep, reinterpret_cast<char *>(bytes), size, MAX_SMALLREAD_SIZE);
    count--;
  }
  if (result >= 0)
  {
    do
    {
      bytes += result;
      size  -= result;
      if (size > 0) result = usb_bulk_read(ptp_usb->handle, ptp_usb->intep, reinterpret_cast<char *>(bytes), size, MAX_SMALLREAD_SIZE);
    } while ((result > 0) && (size > 0));
    return PTP_RC_OK;
  }
  return PTP_ERROR_IO;
}

short Powershot::ptp_check_int_fast_func(unsigned char * bytes, unsigned int size, void * data)
{
  int result;
  struct Powershot::ptp_usb_t * ptp_usb;

  assert(bytes);
  assert(size > 0);
  assert(data);
  ptp_usb = reinterpret_cast<struct Powershot::ptp_usb_t *>(data);
  assert(ptp_usb);
  result = usb_bulk_read(ptp_usb->handle, ptp_usb->intep, reinterpret_cast<char *>(bytes), size, MAX_SMALLREAD_SIZE);
  if (!result) result = usb_bulk_read(ptp_usb->handle, ptp_usb->intep, reinterpret_cast<char *>(bytes), size, MAX_SMALLREAD_SIZE);
  if (result >= 0)
  {
    do
    {
      bytes += result;
      size -= result;
      if (size > 0) result = usb_bulk_read(ptp_usb->handle, ptp_usb->intep, reinterpret_cast<char *>(bytes), size, MAX_SMALLREAD_SIZE);
    } while ((result > 0) && (size > 0));
    return PTP_RC_OK;
  }
  return PTP_ERROR_IO;
}

int Powershot::set_prop8(struct Powershot::settings_t * settings, uint16_t prop, uint8_t val)
{
  return ptp_setdevicepropvalue(&(settings->params), prop, &val, PTP_DTC_UINT8);
}

int Powershot::set_prop16(struct Powershot::settings_t * settings, uint16_t prop, uint16_t val)
{
  return ptp_setdevicepropvalue(&(settings->params), prop, &val, PTP_DTC_UINT16);
}

int Powershot::get_prop16(struct Powershot::settings_t * settings, uint16_t prop, uint16_t * val)
{
  uint16_t * p = NULL;

  uint32_t stat = ptp_getdevicepropvalue(&(settings->params), prop, reinterpret_cast<void **>(&p), PTP_DTC_UINT16);
  if (p && *p)
  {
    *val = *p;
    free(p);
  }
  return stat;
}

int Powershot::get_prop32(struct Powershot::settings_t * settings, uint16_t prop, uint32_t * val)
{
  uint32_t * p = NULL;

  uint32_t stat = ptp_getdevicepropvalue(&(settings->params), prop, reinterpret_cast<void **>(&p), PTP_DTC_UINT32);
  if (p && *p)
  {
    *val = *p;
    free(p);
  }
  return stat;
}

char * Powershot::getarg(char * line, int * n)
{
  char  * p = line;
  char  * s;

  assert(line);
  while (p && *p && ((*p) == ' ')) p++;
  if (! p || ! *p) return NULL;
  s = p;
  while (p && *p && !((*p) == ' ')) p++;
  if (p == s) return NULL;
  if (n) (*n) = (p - s);
  return s;
}

int Powershot::cmdcmp(char * cmd1, char * cmd2)
{
  int n1, n2;
  char * arg1 = Powershot::getarg(cmd1, &n1);
  char * arg2 = Powershot::getarg(cmd2, &n2);

  if (n1 != n2) return 1;
  if (!arg1) return 1;
  if (!arg2) return 1;
  assert(arg2);
  return strncasecmp(arg1, arg2, n1);
}

int Powershot::exec_cmd(struct Powershot::settings_t * settings, char * arg)
{
  int n;
  struct Powershot::cmd_t * c;

  assert(settings);
  arg = Powershot::getarg(arg, &n);
  if (!arg) return 0;
  c = Powershot::cmds;
  while (c->command[0])
  {
    if (!(Powershot::cmdcmp(arg, c->command)))
    {
      if (c->handler)
      {
        arg = Powershot::getarg(arg + n, &n);
        return c->handler(settings, c, arg);
      } else
      {
        PLAYER_ERROR1("Powershot configuration command cannot be executed [%s]", arg);
        return 0;
      }
    }
    c++;
    assert(c);
  }
  PLAYER_ERROR1("Powershot configuration command cannot be understood [%s]", arg);
  return 0;
}

int Powershot::value_ok(struct Powershot::allowed_t * all, char * value)
{
  int n;

  assert(value);
  value = getarg(value, &n);
  if (!value) return -1;
  while (all && (all->string[0]))
  {
    if (!strcasecmp(value, all->string)) return all->code;
    all++;
  }
  return -1;
}

int Powershot::prop16_handler(struct Powershot::settings_t * settings, struct Powershot::cmd_t * cmd, char * args)
{
  int prop;
  uint16_t value;
  PTPDevicePropDesc dpd;

  assert(settings);
  assert(cmd);
  assert(args);
  memset(&dpd, 0, sizeof dpd);
  if (!(Powershot::getarg(args, NULL)))
  {
    if (ptp_getdevicepropdesc(&(settings->params), cmd->ptpcode, &dpd) != PTP_RC_OK) return 0;
    ptp_free_devicepropdesc(&dpd);
    return PTP_RC_OK;
  }
  prop = Powershot::value_ok(cmd->params, args);
  if (prop == -1)
  {
    PLAYER_ERROR1("bad parameter \"%s\"", args);
    return 0;
  }
  value = prop;
  if (ptp_getdevicepropdesc(&(settings->params),cmd->ptpcode,&dpd) != PTP_RC_OK) return 0;
  ptp_free_devicepropdesc(&dpd);
  if (ptp_setdevicepropvalue(&(settings->params), cmd->ptpcode, &value, dpd.DataType) != PTP_RC_OK) return 0;
  if (ptp_getdevicepropdesc(&(settings->params), cmd->ptpcode, &dpd) != PTP_RC_OK) return 0;
  if (value != (*(reinterpret_cast<uint16_t *>(dpd.CurrentValue))))
  {
    ptp_free_devicepropdesc(&dpd);
    return 0;
  }
  ptp_free_devicepropdesc(&dpd);
  return PTP_RC_OK;
}

int Powershot::prop8_handler(struct Powershot::settings_t * settings, struct Powershot::cmd_t * cmd, char * args)
{
  int prop;
  PTPDevicePropDesc dpd;

  assert(settings);
  assert(cmd);
  assert(args);
  memset(&dpd, 0, sizeof dpd);
  if (!(Powershot::getarg(args, NULL)))
  {
    if (ptp_getdevicepropdesc(&(settings->params), cmd->ptpcode, &dpd) != PTP_RC_OK) return 0;
    ptp_free_devicepropdesc(&dpd);
    return PTP_RC_OK;
  }
  prop = Powershot::value_ok(cmd->params, args);
  if (prop == -1)
  {
    PLAYER_ERROR1("bad parameter \"%s\"", args);
    return 0;
  }
  if (Powershot::set_prop8(settings, cmd->ptpcode, prop) != PTP_RC_OK) return 0;
  return PTP_RC_OK;
}

int Powershot::focuslock_handler(struct Powershot::settings_t * settings, struct Powershot::cmd_t * cmd, char * args)
{
  int lock;

  assert(settings);
  assert(cmd);
  assert(args);
  lock = Powershot::value_ok(cmd->params, args);
  if (lock == -1)
  {
    PLAYER_ERROR1("bad parameter \"%s\"", args);
    return 0;
  }
  if (lock) Powershot::lock_focus(settings); else Powershot::unlock_focus(settings);
  return PTP_RC_OK;
}

int Powershot::lock_focus(struct Powershot::settings_t * settings)
{
  int stat;

  assert(settings);
  stat = ptp_canon_focuslock(&(settings->params));
  if (stat != PTP_RC_OK) return stat;
  stat = Powershot::getchanges(settings);
  if (stat != PTP_RC_OK) return stat;
  return PTP_RC_OK;
}

int Powershot::unlock_focus(struct Powershot::settings_t * settings)
{
  int stat;

  assert(settings);
  stat = ptp_canon_focusunlock(&(settings->params));
  if (stat != PTP_RC_OK) return stat;
  stat = Powershot::getchanges(settings);
  if (stat != PTP_RC_OK) return stat;
  return PTP_RC_OK;
}

int Powershot::getchanges(struct Powershot::settings_t * settings)
{
  uint16_t * props = NULL;
  uint32_t propnum;
  int stat;

  assert(settings);
  stat = ptp_canon_getchanges(&(settings->params), &props, &propnum);
  if (stat != PTP_RC_OK) return stat;
  if (props) free(props);
  return PTP_RC_OK;
}

int Powershot::image_size_from_exif(player_camera_data_t * camData)
{
  ExifLoader * el;
  ExifData * ed;
  ExifEntry * ee;
  struct timespec ts;
  int i;

  el = exif_loader_new();
  if (!el)
  {
    PLAYER_ERROR("cannot create new EXIF loader");
    return -1;
  }
  for (i = 0; i < 100; i++)
  {
    if (!(exif_loader_write(el, camData->image, camData->image_count))) break;
    ts.tv_sec = 0;
    ts.tv_nsec = 1000000;
    nanosleep(&ts, NULL);
  }
  if (i == 100)
  {
    PLAYER_ERROR("Cannot read EXIF data");
    exif_loader_unref(el);
    return -1;
  }
  ed = exif_loader_get_data(el);
  if (!ed)
  {
    PLAYER_ERROR("NULL EXIF data");
    exif_loader_unref(el);
    return -1;
  }
  if (exif_data_get_byte_order(ed) != EXIF_BYTE_ORDER_INTEL) exif_data_set_byte_order(ed, EXIF_BYTE_ORDER_INTEL);
  if (exif_data_get_byte_order(ed) != EXIF_BYTE_ORDER_INTEL)
  {
    PLAYER_ERROR("invalid EXIF data byte order");
    exif_data_unref(ed);
    exif_loader_unref(el);
    return -1;
  }
  ee = exif_data_get_entry(ed, exif_tag_from_name("PixelXDimension"));
  if (!ee)
  {
    PLAYER_ERROR("cannot get PixelXDimension EXIF entry");
    exif_data_unref(ed);
    exif_loader_unref(el);
    return -1;
  }
  if (strcmp(exif_format_get_name(ee->format), "Short") || (exif_format_get_size(ee->format) != 2))
  {
    PLAYER_ERROR("cannot handle this kind of EXIF data");
    exif_data_unref(ed);
    exif_loader_unref(el);
    return -1;
  }
  if (exif_format_get_size(ee->format) != ee->size)
  {
    PLAYER_ERROR("EXIF inconsistency");
    exif_data_unref(ed);
    exif_loader_unref(el);
    return -1;
  }
  camData->width = ((ee->data[1]) * 256) + (ee->data[0]); // Intel endiannes handled this way should also fit to big-endian machines
  ee = exif_data_get_entry(ed, exif_tag_from_name("PixelYDimension"));
  if (!ee)
  {
    PLAYER_ERROR("cannot get PixelYDimension EXIF entry");
    exif_data_unref(ed);
    exif_loader_unref(el);
    return -1;
  }
  if (strcmp(exif_format_get_name(ee->format), "Short") || (exif_format_get_size(ee->format) != 2))
  {
    PLAYER_ERROR("cannot handle this kind of EXIF data");
    exif_data_unref(ed);
    exif_loader_unref(el);
    return -1;
  }
  if (exif_format_get_size(ee->format) != ee->size)
  {
    PLAYER_ERROR("EXIF inconsistency");
    exif_data_unref(ed);
    exif_loader_unref(el);
    return -1;
  }
  camData->height = ((ee->data[1]) * 256) + (ee->data[0]); // Intel endiannes handled this way should also fit to big-endian machines
  exif_data_unref(ed);
  exif_loader_unref(el);
  PLAYER_WARN2("JPEG size %u x %u", camData->width, camData->height);
  return 0;
}

void Powershot::Main()
{
  char * image;
  uint8_t * img;
  int has_frame = 0;
  int stat;
  int nblocks, tail;
  unsigned int readnum;
  double frame_stamp = 0.0;
  int i, j;
  struct timespec ts;
  PTPUSBEventContainer event;
  uint32_t handle, size, dummy, pos;

  for (;;)
  {
    if (((this->repeat) && has_frame) || (this->live_view))
    {
      if ((this->sleep_nsec) > 0)
      {
        ts.tv_sec = 0;
        ts.tv_nsec = this->sleep_nsec;
        nanosleep(&ts, NULL);
      }
    } else this->InQueue->Wait();
    pthread_testcancel();
    ProcessMessages();
    if (this->live_view)
    {
      this->imgData.width = this->live_width.GetValue();
      this->imgData.height = this->live_height.GetValue();
      this->imgData.bpp = 24;
      this->imgData.format = PLAYER_CAMERA_FORMAT_RGB888;
      this->imgData.compression = PLAYER_CAMERA_COMPRESS_JPEG;
      this->imgData.fdiv = 0;
      if (this->imgData.image) free(this->imgData.image);
      this->imgData.image_count = 0;
      this->imgData.image = NULL;
      ptp_canon_getviewfinderimage(&(this->settings.params), reinterpret_cast<char **>(&(this->imgData.image)), &(this->imgData.image_count));
      if (!((this->imgData.image) && (this->imgData.image_count > 0)))
      {
        if (this->imgData.image) free(this->imgData.image);
        this->imgData.image_count = 0;
        this->imgData.image = NULL;
        continue;
      }
      if (!(IS_JPEG(this->imgData.image)))
      {
        PLAYER_ERROR("not a JPEG image");
        free(this->imgData.image);
        this->imgData.image_count = 0;
        this->imgData.image = NULL;
        continue;
      }
      assert(this->imgData.image);
      this->Publish(this->camera_addr,
                    PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE,
                    reinterpret_cast<void *>(&(this->imgData)), 0, NULL, true); // copy = true
      if (this->imgData.image) free(imgData.image);
      this->imgData.image_count = 0;
      this->imgData.image = NULL;
      continue;
    } else if (this->needs_frame)
    {
      this->needs_frame = 0;
      if (ptp_canon_initiatecaptureinmemory(&(this->settings.params)) != PTP_RC_OK)
      {
        PLAYER_ERROR("cannot capture frame (ptp_canon_initiatecaptureinmemory)");
        continue;
      }
      stat = Powershot::usb_checkevent_wait(&(this->settings));
      handle = 0;
      for (i = 0; i < 50; i++)
      {
        if (i)
        {
          ts.tv_sec = 0;
          ts.tv_nsec = 50000000;
          nanosleep(&ts, NULL);
        }
        if (!(Powershot::ptp_checkevent(&(this->settings), &event))) continue;
        if ((event.type != PTP_USB_CONTAINER_EVENT) || (event.code != PTP_EC_CANON_RequestObjectTransfer)) continue;
        handle = event.param1;
        Powershot::ptp_checkevent(&(this->settings), NULL);
        Powershot::ptp_checkevent(&(this->settings), NULL);
        break;
      }
      if (stat != PTP_RC_OK) stat = usb_checkevent_wait(&(this->settings));
      if (!(i < 50))
      {
        PLAYER_ERROR("capture timed out");
        continue;
      }
      size = 0;
      dummy = 0;
      stat = ptp_canon_getobjectsize(&(this->settings.params), handle, 0, &size, &dummy);
      if (stat != PTP_RC_OK)
      {
        ts.tv_sec = 0;
        ts.tv_nsec = 50000000;
        nanosleep(&ts, NULL);
        stat = ptp_canon_getobjectsize(&(this->settings.params), handle, 0, &size, &dummy);
        if (stat != PTP_RC_OK)
        {
          PLAYER_ERROR("cannot get image size");
          continue;
        }
      }
      if (!(size > 0))
      {
        PLAYER_ERROR("invalid image size");
        continue;
      }
      this->imgData.width = 0;
      this->imgData.height = 0;
      this->imgData.bpp = 24;
      this->imgData.format = PLAYER_CAMERA_FORMAT_RGB888;
      this->imgData.compression = PLAYER_CAMERA_COMPRESS_JPEG;
      this->imgData.fdiv = 0;
      if ((this->imgData.image_count) != size)
      {
        if (this->imgData.image) free(this->imgData.image);
        this->imgData.image_count = 0;
        this->imgData.image = NULL;
      }
      if (!(this->imgData.image))
      {
        assert(!(this->imgData.image_count));
        this->imgData.image_count = size;
        this->imgData.image = reinterpret_cast<uint8_t *>(malloc(this->imgData.image_count));
        if (!(this->imgData.image))
        {
          this->imgData.image_count = 0;
          PLAYER_ERROR("Out of memory");
          continue;
        }
      }
      assert(this->imgData.image);
      assert((this->imgData.image_count) == size);
      nblocks = size / 5000;
      tail = size % 5000;
      img = this->imgData.image;
      image = NULL;
      for (i = 0; i < nblocks; i++)
      {
        pos = (!i) ? 0 : ((!tail && i == nblocks - 1) ? 2 : 1);
        for (j = 0; j < 10; j++)
        {
          image = NULL;
          if (ptp_canon_getpartialobject(&(this->settings.params), handle, i * 5000, 5000, pos, &image, &readnum) == PTP_RC_OK) break;
        }
        if (j == 10)
        {
          if (image) free(image);
          image = NULL;
          break;
        }
        if (!image) break;
        if (readnum != 5000)
        {
          PLAYER_ERROR("wrong chunk size");
          free(image);
          image = NULL;
          break;
        }
        assert(img);
        memcpy(img, image, readnum);
        img += readnum;
        free(image);
        image = NULL;
      }
      assert(!image);
      if (i != nblocks)
      {
        PLAYER_ERROR("cannot get image (ptp_canon_getpartialobject)");
        if (this->imgData.image) free(this->imgData.image);
        this->imgData.image_count = 0;
        this->imgData.image = NULL;
        continue;
      }
      if (tail)
      {
        pos = (nblocks) ? 3 : 1;
        image = NULL;
        if (ptp_canon_getpartialobject(&(this->settings.params), handle, nblocks * 5000, 5000, pos, &image, &readnum) != PTP_RC_OK)
        {
          if (image) free(image);
          image = NULL;
          PLAYER_ERROR("cannot get image tail (ptp_canon_getpartialobject)");
          if (this->imgData.image) free(this->imgData.image);
          this->imgData.image_count = 0;
          this->imgData.image = NULL;
          continue;
        }
        if (!image)
        {
          PLAYER_ERROR("cannot get image tail (NULL image)");
          if (this->imgData.image) free(this->imgData.image);
          this->imgData.image_count = 0;
          this->imgData.image = NULL;
          continue;
        }
        if ((readnum < 1) || (readnum > 5000))
        {
          PLAYER_ERROR("cannot get image tail (wrong chunk size)");
          free(image);
          image = NULL;
          if (this->imgData.image) free(this->imgData.image);
          this->imgData.image_count = 0;
          this->imgData.image = NULL;
          continue;
        }
        assert(img);
        memcpy(img, image, readnum);
        free(image);
        image = NULL;
      }
      assert(!image);
      if (!(IS_JPEG(this->imgData.image)))
      {
        PLAYER_ERROR("not a JPEG image");
        if (this->imgData.image) free(this->imgData.image);
        this->imgData.image_count = 0;
        this->imgData.image = NULL;
        continue;
      }
      if (Powershot::image_size_from_exif(&(this->imgData)))
      {
        if (this->imgData.image) free(this->imgData.image);
        this->imgData.image_count = 0;
        this->imgData.image = NULL;
        continue;
      }
      has_frame = !0;
      GlobalTime->GetTimeDouble(&frame_stamp);
    }
    if (has_frame)
    {
      if ((!(this->imgData.width > 0)) || (!(this->imgData.height > 0)))
      {
        PLAYER_ERROR("zero sized image");
        if (this->imgData.image) free(imgData.image);
        this->imgData.image_count = 0;
        this->imgData.image = NULL;
        has_frame = 0;
        continue;
      }
      assert(this->imgData.image);
      this->Publish(this->camera_addr,
                    PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE,
                    reinterpret_cast<void *>(&(this->imgData)), 0, &frame_stamp, true); // copy = true
    }
    if (!(this->repeat)) has_frame = 0;
  }
}

int Powershot::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_camera_data_t img;

  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_CAMERA_REQ_GET_IMAGE, this->camera_addr))
  {
    memset(&img, 0, sizeof img);
    if (this->live_view)
    {
      img.width = this->live_width.GetValue();
      img.height = this->live_height.GetValue();
      img.bpp = 24;
      img.format = PLAYER_CAMERA_FORMAT_RGB888;
      img.compression = PLAYER_CAMERA_COMPRESS_JPEG;
      img.fdiv = 0;
      img.image_count = 0;
      img.image = NULL;
      ptp_canon_getviewfinderimage(&(this->settings.params), reinterpret_cast<char **>(&(img.image)), &(img.image_count));
      if (!((img.image) && (img.image_count > 0)))
      {
        if (img.image) free(img.image);
        return -1;
      }
      if (!(IS_JPEG(img.image)))
      {
        PLAYER_ERROR("not a JPEG image");
        free(img.image);
        return -1;
      }
      assert(img.image);
    } else
    {
      img.width = 0;
      img.height = 0;
      img.bpp = 24;
      img.format = PLAYER_CAMERA_FORMAT_RGB888;
      img.compression = PLAYER_CAMERA_COMPRESS_RAW;
      img.fdiv = 0;
      img.image_count = 0;
      img.image = NULL;
      this->needs_frame = !0;
    }
    this->Publish(this->camera_addr,
                  resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_CAMERA_REQ_GET_IMAGE,
                  reinterpret_cast<void *>(&img));
    if (img.image) free(img.image);
    return 0;
  }
  return -1;
}

Driver * Powershot_Init(ConfigFile * cf, int section)
{
  return (Driver *)(new Powershot(cf, section));
}

void powershot_Register(DriverTable * table)
{
  table->AddDriver("powershot", Powershot_Init);
}
