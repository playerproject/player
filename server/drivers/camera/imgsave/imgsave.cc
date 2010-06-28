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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_imgsave imgsave
 * @brief image storage dispatcher

The imgsave driver stores received images into a directory tree in which paths form
such a pattern: key/date/hour, where key is the given key name of camera device.
By default images are written in human readable format called xyRGB (where each line
is formed by five integers: x, y, R, G, B of a pixel), alternatively, JPEG compressed
files can be stored. When desired, this driver can inprint image informations at the
top of stored and published images (key, date, time, number of current image in given
second). This driver publishes all images that are stored (if needed, JPEG compressed).

@par Compile-time dependencies

- none

@par Provides

- @ref interface_camera

@par Requires

- @ref interface_camera

@par Configuration requests

- none

@par Configuration file options

- key (string)
  - Default: "Unnamed"
  - The name of camera device (any name you like for it)
- jpeg (integer)
  - Default: 0
  - If set to 1, images that are stored and published are JPEG compressed
- jpeg_quality (float)
  - Default: 0.8
  - JPEG quality
- print (integer)
  - Default: 0
  - If set to 1, images are decorated at the top with status info
    (key, date, time, number of current image in given second)
- sleep_nsec (integer)
  - Default: 10000
  - timespec value for additional nanosleep()

@par Properties

- none

@par Example

@verbatim
driver
(
  name "imgsave"
  requires ["camera:0"]
  provides ["camera:10"]
  key "vault"
  jpeg 1
  print 1
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <pthread.h>
#include <math.h>
#include <libplayercore/playercore.h>
#include <config.h>
#if HAVE_JPEG
  #include <libplayerjpeg/playerjpeg.h>
#endif
#include "videofont.h"

#define QUEUE_LEN 1
#define MAX_KEY_LEN 15
#define MAX_STAMP_LEN 255
#define EPS 0.00001

class ImgSave : public ThreadedDriver
{
  public: ImgSave(ConfigFile * cf, int section);
  public: virtual ~ImgSave();

  public: virtual int MainSetup();
  public: virtual void MainQuit();

  // This method will be invoked on each incoming message
  public: virtual int ProcessMessage(QueuePointer & resp_queue,
                                     player_msghdr * hdr,
                                     void * data);

  private: virtual void Main();

  // Input camera device
  private: player_devaddr_t camera_provided_addr, camera_id;
  private: Device * camera;
  private: char key[MAX_KEY_LEN + 1];
  private: int jpeg;
  private: double jpeg_quality;
  private: int print;
  private: int sleep_nsec;
  private: unsigned char * font;
  private: char stamp[MAX_STAMP_LEN + 1];
  private: time_t last_time;
  private: int last_num;
  private: double tstamp;
  private: static void txtwrite(int x, int y, unsigned char forecolor, unsigned char backcolor, const char * msg, const unsigned char * _fnt, unsigned char * img, size_t imgwidth, size_t imgheight);
};

Driver * ImgSave_Init(ConfigFile * cf, int section)
{
  return reinterpret_cast<Driver *>(new ImgSave(cf, section));
}

void imgsave_Register(DriverTable *table)
{
  table->AddDriver("imgsave", ImgSave_Init);
}

ImgSave::ImgSave(ConfigFile * cf, int section)
  : ThreadedDriver(cf, section, true, QUEUE_LEN)
{
  static const unsigned char _fnt[] = VIDEOFONT; /* static: I don't want huge array to stick on local stack */
  const char * _key;
  int i;

  memset(&(this->camera_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->camera_id), 0, sizeof(player_devaddr_t));
  this->camera = NULL;
  this->key[0] = '\0';
  this->jpeg = 0;
  this->jpeg_quality = 0.0;
  this->print = 0;
  this->sleep_nsec = 0;
  this->font = NULL;
  this->stamp[0] = '\0';
  this->last_time = 0;
  this->last_num = 0;
  this->tstamp = 0;
  if (cf->ReadDeviceAddr(&(this->camera_provided_addr), section, "provides", PLAYER_CAMERA_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->camera_provided_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->camera_id), section, "requires", PLAYER_CAMERA_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);
    return;
  }
  _key = cf->ReadString(section, "key", "Unnamed");
  if (!_key)
  {
    this->SetError(-1);
    return;
  }
  if (!((strlen(_key) > 0) && (strlen(_key) <= MAX_KEY_LEN)))
  {
    this->SetError(-1);
    return;
  }
  for (i = 0; i < MAX_KEY_LEN; i++)
  {
    if (!(_key[i])) break;
    this->key[i] =  (((_key[i] >= '0') && (_key[i] <= '9')) \
                  || ((_key[i] >= 'A') && (_key[i] <= 'Z')) \
                  || ((_key[i] >= 'a') && (_key[i] <= 'z')) \
                  ||  (_key[i] == '.') \
                  ||  (_key[i] == '-')) \
                  ? _key[i] : '_';
  }
  if (!i)
  {
    this->SetError(-1);
    return;
  }
  this->key[i] = '\0';
  this->jpeg = cf->ReadInt(section, "jpeg", 0);
  this->jpeg_quality = cf->ReadFloat(section, "jpeg_quality", 0.8);
  if (((this->jpeg_quality) < EPS) || ((this->jpeg_quality) > 1.0))
  {
    this->SetError(-1);
    return;
  }
  this->print = cf->ReadInt(section, "print", 0);
  this->sleep_nsec = cf->ReadInt(section, "sleep_nsec", 10000);
  if ((this->sleep_nsec) < 0)
  {
    this->SetError(-1);
    return;
  }
  if (this->print)
  {
    this->font = reinterpret_cast<unsigned char *>(malloc(sizeof _fnt));
    if (!(this->font))
    {
      this->SetError(-1);
      return;
    }
    for (i = 0; i < static_cast<int>(sizeof _fnt); i++) this->font[i] = _fnt[i];
  }
}

ImgSave::~ImgSave()
{
  if (this->font) free(this->font);
  this->font = NULL;
}

int ImgSave::MainSetup()
{
  if (Device::MatchDeviceAddress(this->camera_id, this->camera_provided_addr))
  {
    PLAYER_ERROR("attempt to subscribe to self");
    return -1;
  }
  this->camera = deviceTable->GetDevice(this->camera_id);
  if (!this->camera)
  {
    PLAYER_ERROR("unable to locate suitable camera device");
    return -1;
  }
  if (this->camera->Subscribe(this->InQueue) != 0)
  {
    PLAYER_ERROR("unable to subscribe to camera device");
    this->camera = NULL;
    return -1;
  }
  return 0;
}

void ImgSave::MainQuit()
{
  if (this->camera) this->camera->Unsubscribe(this->InQueue);
  this->camera = NULL;
}

void ImgSave::Main()
{
  struct timespec tspec;

  snprintf(this->stamp, sizeof this->stamp, "**Start**");
  this->last_time = time(NULL);
  this->last_num = 0;
  for (;;)
  {
    this->InQueue->Wait();
    pthread_testcancel();
    this->ProcessMessages();
    pthread_testcancel();
    tspec.tv_sec = 0;
    tspec.tv_nsec = this->sleep_nsec;
    if (tspec.tv_nsec > 0)
    {
      nanosleep(&tspec, NULL);
      pthread_testcancel();
    }
  }
}

int ImgSave::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  player_camera_data_t * output;
  int i, j;
  int save = 0;
  size_t jpegsize = 0;
  size_t bufsize = 0;
  unsigned char * buffer = NULL;
  unsigned char * jbuffer = NULL;
  unsigned char * ptr, * ptr1;
  player_camera_data_t * rawdata;
  char dname[MAX_STAMP_LEN + 1];
  char fname[MAX_STAMP_LEN + 1];
  char cmd[MAX_STAMP_LEN + 1];
  FILE * f;
  struct tm t;
  time_t tt = time(NULL);

  assert(hdr);
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, this->camera_id))
  {
    assert(data);
    rawdata = reinterpret_cast<player_camera_data_t *>(data);
    if ((static_cast<int>(rawdata->width) <= 0) || (static_cast<int>(rawdata->height) <= 0))
    {
      return -1;
    } else
    {
      bufsize = rawdata->width * rawdata->height * 3;
      buffer = reinterpret_cast<unsigned char *>(malloc(bufsize));
      if (!buffer)
      {
        PLAYER_ERROR("Out of memory");
        return -1;
      }
      if (this->jpeg)
      {
        jbuffer = reinterpret_cast<unsigned char *>(malloc(bufsize));
        if (!jbuffer)
        {
          PLAYER_ERROR("Out of memory");
          if (buffer) free(buffer);
          buffer = NULL;
          return -1;
        }
      } else jbuffer = NULL;
      switch (rawdata->compression)
      {
      case PLAYER_CAMERA_COMPRESS_RAW:
        switch (rawdata->bpp)
        {
        case 8:
          ptr = buffer;
          ptr1 = reinterpret_cast<unsigned char *>(rawdata->image);
          assert(ptr);
          assert(ptr1);
          for (i = 0; i < static_cast<int>(rawdata->height); i++)
          {
            for (j = 0; j < static_cast<int>(rawdata->width); j++)
            {
              ptr[0] = *ptr1;
              ptr[1] = *ptr1;
              ptr[2] = *ptr1;
              ptr += 3; ptr1++;
            }
          }
          break;
        case 24:
          assert(buffer);
          assert(rawdata->image);
          memcpy(buffer, rawdata->image, bufsize);
          break;
        case 32:
          ptr = buffer;
          ptr1 = reinterpret_cast<unsigned char *>(rawdata->image);
          assert(ptr);
          assert(ptr1);
          for (i = 0; i < static_cast<int>(rawdata->height); i++)
          {
            for (j = 0; j < static_cast<int>(rawdata->width); j++)
            {
              ptr[0] = ptr1[0];
              ptr[1] = ptr1[1];
              ptr[2] = ptr1[2];
              ptr += 3; ptr1 += 4;
            }
          }
          break;
        default:
          PLAYER_WARN("unsupported image depth (not good)");
          if (buffer) free(buffer);
          buffer = NULL;
          if (jbuffer) free(jbuffer);
          jbuffer = NULL;
          return -1;
        }
        break;
#if HAVE_JPEG
      case PLAYER_CAMERA_COMPRESS_JPEG:
        if ((this->jpeg) && (!(this->print)))
        {
          assert(jbuffer);
          assert(rawdata->image);
          jpegsize = rawdata->image_count;
          memcpy(jbuffer, rawdata->image, jpegsize);
        } else
        {
          assert(buffer);
          assert(rawdata->image);
          jpeg_decompress(buffer, bufsize, rawdata->image, rawdata->image_count);
        }
        break;
#endif
      default:
        PLAYER_WARN("unsupported compression scheme (not good)");
        if (buffer) free(buffer);
        buffer = NULL;
        if (jbuffer) free(jbuffer);
        jbuffer = NULL;
        return -1;
      }
      if (this->print)
      {
        assert(buffer);
        txtwrite(0, 8, 255, 0, this->stamp, this->font, buffer, rawdata->width, rawdata->height);
      }
      save = ((this->tstamp) != (hdr->timestamp)) ? (!0) : 0;
      if (save)
      {
        assert((localtime_r(&tt, &t)) == (&t));
        if (tt != (this->last_time))
        {
          this->last_time = tt;
          this->last_num = 0;
        }
        snprintf(dname, sizeof dname, "%s/%d.%.2d.%.2d/%.2d", this->key, (t.tm_year) + 1900, (t.tm_mon) + 1, t.tm_mday, t.tm_hour);
        snprintf(fname, sizeof fname, "%s/%.2d.%.2d.%.2d-%.2d.%s", dname, t.tm_hour, t.tm_min, t.tm_sec, this->last_num, (this->jpeg) ? "jpg" : "txt");
        snprintf(this->stamp, sizeof this->stamp, "%s  %d.%.2d.%.2d %.2d:%.2d:%.2d-%.2d", this->key, (t.tm_year) + 1900, (t.tm_mon) + 1, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec, this->last_num);
        this->last_num++;
      }
      if (this->print)
      {
        assert(buffer);
        txtwrite(0, 0, 0, 255, this->stamp, this->font, buffer, rawdata->width, rawdata->height);
      }
      if (this->jpeg)
      {
        assert(jbuffer);
        if ((this->print) || ((rawdata->compression) == PLAYER_CAMERA_COMPRESS_RAW))
        {
          assert(buffer);
#if HAVE_JPEG
          jpegsize = jpeg_compress(reinterpret_cast<char *>(jbuffer), reinterpret_cast<char *>(buffer), rawdata->width, rawdata->height, bufsize, static_cast<int>((this->jpeg_quality) * 100.0));
#else
          PLAYER_ERROR("No JPEG compression supported");
          if (buffer) free(buffer);
          buffer = NULL;
          if (jbuffer) free(jbuffer);
          jbuffer = NULL;
          return -1;
#endif
        } else
        {
          assert((rawdata->compression) == PLAYER_CAMERA_COMPRESS_JPEG);
        }
        free(buffer);
        buffer = NULL;
        assert(jpegsize > 0);
      } else
      {
        assert(buffer);
        assert(!jbuffer);
      }
      if (save)
      {
        f = fopen(fname, (this->jpeg) ? "wb" : "w");
        if (!f)
        {
          snprintf(cmd, sizeof cmd, "mkdir -p %s", dname);
          system(cmd);
          f = fopen(fname, (this->jpeg) ? "wb" : "w");
        }
        if (!f)
        {
          PLAYER_ERROR1("Cannot open file in order to write [%s]", fname);
        } else
        {
          if (this->jpeg)
          {
            assert(jbuffer);
            assert(jpegsize > 0);
            fwrite(jbuffer, jpegsize, 1, f);
          } else
          {
            assert(buffer);
            ptr1 = buffer;
            for (i = 0; i < static_cast<int>(rawdata->height); i++)
            {
              for (j = 0; j < static_cast<int>(rawdata->width); j++)
              {
                fprintf(f, "%d %d %u %u %u\n", j, i, ptr1[0], ptr1[1], ptr1[2]);
                ptr1 += 3;
              }
            }
          }
          fclose(f);
        }
      }
      output = reinterpret_cast<player_camera_data_t *>(malloc(sizeof(player_camera_data_t)));
      if (!output)
      {
        PLAYER_ERROR("Out of memory");
        if (buffer) free(buffer);
        buffer = NULL;
        if (jbuffer) free(jbuffer);
        jbuffer = NULL;
        return -1;
      }
      memset(output, 0, sizeof(player_camera_data_t));
      output->bpp = 24;
      output->compression = (this->jpeg) ? PLAYER_CAMERA_COMPRESS_JPEG : PLAYER_CAMERA_COMPRESS_RAW;
      output->format = PLAYER_CAMERA_FORMAT_RGB888;
      output->fdiv = rawdata->fdiv;
      output->width = rawdata->width;
      output->height = rawdata->height;
      output->image_count = (this->jpeg) ? jpegsize : bufsize;
      output->image = (this->jpeg) ? jbuffer : buffer;
      assert(output->image);
      this->tstamp = hdr->timestamp;
      this->Publish(this->camera_provided_addr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, reinterpret_cast<void *>(output), 0, &(this->tstamp), false); // copy = false
      // I assume that Publish() (with copy = false) freed those output data!
      output = NULL;
    }
    return 0;
  }
  return -1;
}

void ImgSave::txtwrite(int x, int y, unsigned char forecolor, unsigned char backcolor, const char * msg, const unsigned char * _fnt, unsigned char * img, size_t imgwidth, size_t imgheight)
{
  int i, l, c, xi;
  const unsigned char * lettershape;
  size_t linewidth = imgwidth * 3;
  unsigned char * line;
  unsigned char * col;
  unsigned char color;

  assert(msg);
  assert(_fnt);
  assert(img);
  for (i = 0; msg[i]; i++) if ((msg[i]) > ' ')
  {
    lettershape = _fnt + (msg[i] * 8);
    xi = x + (i * 8);
    assert(lettershape);
    for (l = 0; l < 8; l++)
    {
      if ((y + l) >= static_cast<int>(imgheight)) break;
      line = img + ((y + l) * linewidth);
      assert(line);
      for (c = 0; c < 8; c++)
      {
        if ((xi + c) >= static_cast<int>(imgwidth)) break;
        col = line + ((xi + c) * 3);
        assert(col);
        color = (((lettershape[l]) << c) & 128) ? forecolor : backcolor;
        col[0] = color;
        col[1] = color;
        col[2] = color;
      }
    }
  }
}

