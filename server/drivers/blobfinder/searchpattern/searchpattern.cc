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
/** @defgroup driver_searchpattern searchpattern
 * @brief Pattern finder

The searchpattern driver searches for given patern in the camera image.

@par Compile-time dependencies

- none

@par Provides

- @ref interface_blobfinder
- (optionally) @ref interface_camera (thresholded image)

@par Requires

- @ref interface_camera

@par Configuration requests

- none

@par Configuration file options

- patterns (string array)
  - Default: Nothing! Explicit settings required.
  - Each string should contain one s-expression (a LISP-style list)
    which define one pattern; first element of a list is a 8-digit hex color
    value (0x prefixed): whenever given pattern is found it will be denoted by
    a blob of this color.
- debug (integer)
  - Default: 0
  - If it is set to non-zero, debug messages will be printed

@par Properties

- threshold (integer)
  - Default: 112
  - Valid values: 0..255
  - Luminance threshold used during thresholding
    see http://en.wikipedia.org/wiki/Thresholding_%28image_processing%29
- min_blob_pixels (integer)
  - Default: 16
  - Valid values: greater than 0
  - Minimal number of pixel for a blob to be considered as blob
    (used for noise elimination).
- sleep_nsec (integer)
  - Default: 10000
  - timespec value for additional nanosleep()

@par Example

@verbatim
driver
(
  name "searchpattern"
  provides ["blobfinder:0"]
  requires ["camera:0"]
  patterns ["(0x00ff0000 (black (white (black) (black (white)))))" "(0x0000ff00 (black (white) (white (black))))"]
  threshold 112
  min_blob_pixels 16
  debug 1
)
@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <libplayercore/playercore.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <config.h>
#if HAVE_JPEG
  #include <libplayerjpeg/playerjpeg.h>
#endif

#define EPS 0.000001

#define QUEUE_LEN 1
#define MAX_PATTERNS 10
#define MAX_DESCRIPTIONS (MAX_PATTERNS * 10)

#define CHECK_TOKEN(str, token) (!(strncmp((str), (token), strlen(token))))
#define EAT_TOKEN(str, token, tmp) \
do \
{ \
  for (tmp = (token); (*(tmp)); (tmp)++, (str)++) \
  { \
    if ((*str) != (*tmp)) \
    { \
      str = NULL; \
      break; \
    } \
  } \
} while (0)
#define IS_HEX_DIGIT(chr) ((((chr) >= '0') && ((chr) <= '9')) || (((chr) >= 'A') && ((chr) <= 'F')) || (((chr) >= 'a') && ((chr) <= 'f')))

#define NO_PARENT -1
#define ERR_TOO_MANY_BLACKS -1
#define ERR_TOO_MANY_WHITES -2
#define ERR_TOO_MANY_PIXELS -3

class Searchpattern: public ThreadedDriver
{
  public:
    // Constructor; need that
    Searchpattern(ConfigFile * cf, int section);

    virtual ~Searchpattern();

    // Must implement the following methods.
    virtual int MainSetup();
    virtual void MainQuit();
    virtual void Main();

    // This method will be invoked on each incoming message
    virtual int ProcessMessage(QueuePointer & resp_queue,
                               player_msghdr * hdr,
                               void * data);
  private:
    player_devaddr_t blobfinder_provided_addr, camera_provided_addr;
    player_devaddr_t camera_id;
    Device * camera;
    int publish_timg;
    uint32_t colors[MAX_PATTERNS];
    struct pattern_description
    {
      int parent_id;
      int internals;
      unsigned char color;
    } descriptions[MAX_DESCRIPTIONS];
    struct blob_struct
    {
      int minx, miny;
      int maxx, maxy;
      size_t pixels;
      int internals;
      int parent;
      unsigned char color;
      unsigned char in_use;
    } blobs[256];
    size_t descNum;
    int numpatterns;
    int debug;
    int * _stack;
    int _sp;
    size_t _stack_size;
    size_t max_blob_pixels;
    int * blob_x;
    int * blob_y;
    unsigned char * buffer;
    size_t bufsize;
    size_t numresults;
    int * results;
    IntProperty threshold;
    IntProperty min_blob_pixels;
    IntProperty sleep_nsec;
    int find_pattern(const struct pattern_description * pattern, int id, int blob_parent) const;
    int searchpattern(unsigned char * area, int width, int height, int min_blob_pixels, const struct pattern_description * pattern, int * results);
#define SEARCH_STACK_GUARD -1
#define SEARCH_STACK_PUSH(value) do \
{ \
    if ((this->_sp) == static_cast<int>(this->_stack_size)) PLAYER_ERROR("Stack overflow"); \
    else this->_stack[(this->_sp)++] = (value); \
} while (0)
#define SEARCH_STACK_POP() ((this->_sp) ? this->_stack[--(this->_sp)] : SEARCH_STACK_GUARD)
#define SEARCH_STACK_WIPE() (this->_sp = 0)
#define CHECK_PIXEL(line, x, y, blobnum) do \
{ \
    switch ((line)[x]) \
    { \
    case 0: \
    case 255: \
        if (this->blobs[blobnum].color == ((line)[x])) \
        { \
            SEARCH_STACK_PUSH(x); \
            SEARCH_STACK_PUSH(y); \
            ((line)[x]) = (blobnum); \
        } \
        break; \
    default: \
        if (((line)[x]) != (blobnum)) \
        { \
            if ((this->blobs[blobnum].parent) == NO_PARENT) \
            { \
                this->blobs[blobnum].parent = ((line)[x]); \
                this->blobs[this->blobs[blobnum].parent].internals++; \
            } else if (((line)[x]) != (this->blobs[blobnum].parent)) PLAYER_ERROR3("Internal error (multiple parents? %d %d) (color %d)", this->blobs[blobnum].parent, (line)[x], this->blobs[blobnum].color); \
        } \
    } \
} while (0)
};

Searchpattern::Searchpattern(ConfigFile * cf, int section)
    : ThreadedDriver(cf, section, true, QUEUE_LEN),
    threshold("threshold", 112, false),
    min_blob_pixels("min_blob_pixels", 16, false),
    sleep_nsec("sleep_nsec", 10000, false)
{
  const char * str;
  const char * tmp;
  char hexbuf[9];
  int i, j, k, parent;

  memset(&(this->blobfinder_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->camera_provided_addr), 0, sizeof(player_devaddr_t));
  memset(&(this->camera_id), 0, sizeof(player_devaddr_t));
  this->camera = NULL;
  this->publish_timg = 0;
  memset(this->colors, 0, sizeof this->colors);
  memset(this->descriptions, 0, sizeof this->descriptions);
  memset(this->blobs, 0, sizeof this->blobs);
  this->descNum = 0;
  this->numpatterns = 0;
  this->debug = 0;
  this->_stack = NULL;
  this->_sp = 0;
  this->_stack_size = 0;
  this->max_blob_pixels = 0;
  this->blob_x = NULL;
  this->blob_y = NULL;
  this->buffer = NULL;
  this->bufsize = 0;
  this->numresults = 0;
  this->results = NULL;
  if (cf->ReadDeviceAddr(&(this->blobfinder_provided_addr), section, "provides",
                           PLAYER_BLOBFINDER_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->blobfinder_provided_addr))
  {
    this->SetError(-1);
    return;
  }
  if (cf->ReadDeviceAddr(&(this->camera_provided_addr), section, "provides",
                           PLAYER_CAMERA_CODE, -1, NULL))
  {
    this->publish_timg = 0;
  } else
  {
    if (this->AddInterface(this->camera_provided_addr))
    {
      this->SetError(-1);
      return;
    }
    this->publish_timg = !0;
  }
  if (cf->ReadDeviceAddr(&(this->camera_id), section, "requires", PLAYER_CAMERA_CODE, -1, NULL) != 0)
  {
    this->SetError(-1);
    return;
  }
  this->debug = cf->ReadInt(section, "debug", 0);
  this->numpatterns = cf->GetTupleCount(section, "patterns");
  if (!((this->numpatterns) > 0))
  {
    PLAYER_ERROR("No patterns given");
    this->SetError(-1);
    return;
  }
  if ((this->numpatterns) > MAX_PATTERNS)
  {
    PLAYER_ERROR("Too many paterns given");
    this->SetError(-1);
    return;
  }
  j = (this->numpatterns);
  for (i = 0; i < (this->numpatterns); i++)
  {
    str = cf->ReadTupleString(section, "patterns", i, "");
    if (!str)
    {
      PLAYER_ERROR1("NULL pattern %d", i);
      this->SetError(-1);
      return;
    }
    if (!(strlen(str) > 0))
    {
      PLAYER_ERROR1("Empty pattern %d", i);
      this->SetError(-1);
      return;
    }
    if ((*str) != '(')
    {
      PLAYER_ERROR1("Syntax error in pattern %d (1)", i);
      this->SetError(-1);
      return;
    }
    str++;
    if ((*str) != '0')
    {
      PLAYER_ERROR1("Syntax error in pattern %d (2)", i);
      this->SetError(-1);
      return;
    }
    str++;
    if ((*str) != 'x')
    {
      PLAYER_ERROR1("Syntax error in pattern %d (3)", i);
      this->SetError(-1);
      return;
    }
    str++;
    for (k = 0; k < 8; k++, str++)
    {
      if (!IS_HEX_DIGIT(*str))
      {
        PLAYER_ERROR1("Syntax error in pattern %d (4)", i);
        this->SetError(-1);
        return;
      }
      hexbuf[k] = (*str);
    }
    hexbuf[8] = '\0';
    sscanf(hexbuf, "%x", &(this->colors[i]));
    if ((*str) != ' ')
    {
      PLAYER_ERROR1("Syntax error in pattern %d (5)", i);
      this->SetError(-1);
      return;
    }
    str++;
    if ((*str) != '(')
    {
      PLAYER_ERROR1("Syntax error in pattern %d (6)", i);
      this->SetError(-1);
      return;
    }
    str++;
    this->descriptions[i].parent_id = NO_PARENT;
    this->descriptions[i].internals = 0;
    if (CHECK_TOKEN(str, "black"))
    {
      this->descriptions[i].color = 0;
      EAT_TOKEN(str, "black", tmp);
    } else if (CHECK_TOKEN(str, "white"))
    {
      this->descriptions[i].color = 255;
      EAT_TOKEN(str, "white", tmp);
    } else
    {
      PLAYER_ERROR1("Syntax error in pattern %d (7)", i);
      this->SetError(-1);
      return;
    }
    if (!str)
    {
      PLAYER_ERROR("Internal error within the parser");
      this->SetError(-1);
      return;
    }
    parent = i;
    for (;;)
    {
      if ((*str) == ')')
      {
        str++;
        if ((this->descriptions[parent].parent_id) == NO_PARENT) break;
        parent = (this->descriptions[parent].parent_id);
        continue;
      }
      if ((*str) != ' ')
      {
        PLAYER_ERROR1("Syntax error in pattern %d (8)", i);
        this->SetError(-1);
        return;
      }
      str++;
      if ((*str) != '(')
      {
        PLAYER_ERROR1("Syntax error in pattern %d (9)", i);
        this->SetError(-1);
        return;
      }
      str++;
      if (j >= MAX_DESCRIPTIONS)
      {
        PLAYER_ERROR("Pattern set too complex");
        this->SetError(-1);
        return;
      }
      this->descriptions[parent].internals++;
      this->descriptions[j].parent_id = parent;
      this->descriptions[j].internals = 0;
      if (CHECK_TOKEN(str, "black"))
      {
        this->descriptions[j].color = 0;
        EAT_TOKEN(str, "black", tmp);
      } else if (CHECK_TOKEN(str, "white"))
      {
        this->descriptions[j].color = 255;
        EAT_TOKEN(str, "white", tmp);
      } else
      {
        PLAYER_ERROR1("Syntax error in pattern %d (10)", i);
        this->SetError(-1);
        return;
      }
      if (!str)
      {
        PLAYER_ERROR("Internal error within the parser");
        this->SetError(-1);
        return;
      }
      parent = j;
      j++;
    }
    if ((*str) != ')')
    {
      PLAYER_ERROR1("Syntax error in pattern %d (11)", i);
      this->SetError(-1);
      return;
    }
    str++;
    if (*str)
    {
      PLAYER_ERROR1("Syntax error in pattern %d (12)", i);
      this->SetError(-1);
      return;
    }
  }
  if (this->debug)
  {
    PLAYER_WARN2("patterns: %d, descriptions = %d", this->numpatterns, j);
    for (i = 0; i < (this->numpatterns); i++)
    {
      PLAYER_WARN4("%d: key: 0x%08x internals: %d color: %s", i, this->colors[i], this->descriptions[i].internals, (this->descriptions[i].color) ? "white" : "black");
      if ((this->descriptions[i].parent_id) != NO_PARENT)
      {
        PLAYER_ERROR1("Pattern integrity check failed for pattern %i", i);
        this->SetError(-1);
        return;
      }
    }
    for (; i < j; i++)
    {
      PLAYER_WARN4("%d: parent: %d internals: %d color: %s", i, this->descriptions[i].parent_id, this->descriptions[i].internals, (this->descriptions[i].color) ? "white" : "black");
      if ((this->descriptions[i].parent_id) == NO_PARENT)
      {
        PLAYER_ERROR1("Pattern integrity check failed for pattern %i", i);
        this->SetError(-1);
        return;
      }
    }
  }
  this->descNum = j;
  if (!(this->RegisterProperty("threshold", &(this->threshold), cf, section)))
  {
    PLAYER_ERROR("Cannot register 'threshold' property");
    this->SetError(-1);
    return;
  }
  if (((this->threshold.GetValue()) < 0) || ((this->threshold.GetValue()) > 255))
  {
    PLAYER_ERROR("Invalid threshold value");
    this->SetError(-1);
    return;
  }
  if (!(this->RegisterProperty("min_blob_pixels", &(this->min_blob_pixels), cf, section)))
  {
    PLAYER_ERROR("Cannot register 'min_blob_pixels' property");
    this->SetError(-1);
    return;
  }
  if ((this->min_blob_pixels.GetValue()) <= 0)
  {
    PLAYER_ERROR("Invalid min_blob_pixels value");
    this->SetError(-1);
    return;
  }
  if (!(this->RegisterProperty("sleep_nsec", &(this->sleep_nsec), cf, section)))
  {
    this->SetError(-1);
    return;
  }
  if ((this->sleep_nsec.GetValue()) < 0)
  {
    this->SetError(-1);
    return;
  }
  this->numresults = ((this->numpatterns) * 4);
  this->results = reinterpret_cast<int *>(malloc((this->numresults) * sizeof(int)));
  if (!(this->results))
  {
    PLAYER_ERROR("Out of memory");
    this->SetError(-1);
    return;
  }
  memset(this->results, 0, sizeof((this->numresults) * sizeof(int)));
}

Searchpattern::~Searchpattern()
{
  if (this->results) free(this->results);
  this->results = NULL;
  if (this->_stack) free(this->_stack);
  this->_stack = NULL;
  if (this->blob_x) free(this->blob_x);
  this->blob_x = NULL;
  if (this->blob_y) free(this->blob_y);
  this->blob_y = NULL;
  if (this->buffer) free(this->buffer);
  this->buffer = NULL;
}

////////////////////////////////////////////////////////////////////////////////
// Set up the device.  Return 0 if things go well, and -1 otherwise.
int Searchpattern::MainSetup()
{
  if (this->publish_timg)
  {
    if (Device::MatchDeviceAddress(this->camera_id, this->camera_provided_addr))
    {
      PLAYER_ERROR("attempt to subscribe to self");
      return -1;
    }
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

////////////////////////////////////////////////////////////////////////////////
// Shutdown the device
void Searchpattern::MainQuit()
{
  if (this->camera) this->camera->Unsubscribe(this->InQueue);
  this->camera = NULL;
}

void Searchpattern::Main()
{
  struct timespec tspec;

  for (;;)
  {
    this->InQueue->Wait();
    pthread_testcancel();
    this->ProcessMessages();
    pthread_testcancel();
    tspec.tv_sec = 0;
    tspec.tv_nsec = this->sleep_nsec.GetValue();
    if (tspec.tv_nsec > 0)
    {
      nanosleep(&tspec, NULL);
      pthread_testcancel();
    }
  }
}

int Searchpattern::ProcessMessage(QueuePointer & resp_queue,
                                  player_msghdr * hdr,
                                  void * data)
{
  int i, j, found;
  size_t area_width, area_height;
  double lum;
  size_t new_size = 0;
  size_t new_buf_size = 0;
  size_t ns = 0;
  player_camera_data_t * rawdata = NULL;
  player_camera_data_t * timg_output = NULL;
  player_blobfinder_data_t * output = NULL;
  unsigned char * ptr = NULL;
  unsigned char * ptr1 = NULL;
  unsigned char thresh = static_cast<unsigned char>(this->threshold.GetValue());

  if (!hdr)
  {
    PLAYER_ERROR("NULL header");
    return -1;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, this->camera_id))
  {
    if (!data)
    {
      PLAYER_ERROR("NULL data");
      return -1;
    }
    rawdata = reinterpret_cast<player_camera_data_t *>(data);
    if ((static_cast<int>(rawdata->width) <= 0) || (static_cast<int>(rawdata->height) <= 0))
    {
      return -1;
    } else
    {
      new_size = rawdata->width * rawdata->height;
      new_buf_size = new_size * 3;
      if ((this->bufsize) != new_buf_size)
      {
        if (this->_stack) free(this->_stack);
        this->_stack = NULL;
        if (this->blob_x) free(this->blob_x);
        this->blob_x = NULL;
        if (this->blob_y) free(this->blob_y);
        this->blob_y = NULL;
        if (this->buffer) free(this->buffer);
        this->buffer = NULL;
        this->bufsize = 0;
        this->_stack_size = 0;
        this->_sp = 0;
        this->max_blob_pixels = 0;
      }
      if (!(this->buffer))
      {
        this->bufsize = 0;
        this->buffer = reinterpret_cast<unsigned char *>(malloc(new_buf_size));
        if (!(this->buffer))
        {
          PLAYER_ERROR("Out of memory");
          return -1;
        }
        this->bufsize = new_buf_size;
      }
      if (!(this->_stack))
      {
        this->_stack_size = 0;
        this->_sp = 0;
        ns = (new_size * 2);
        this->_stack = reinterpret_cast<int *>(malloc(ns * sizeof(int)));
        if (!(this->_stack))
        {
          PLAYER_ERROR("Out of memory");
          return -1;
        }
        this->_stack_size = ns;
        this->_sp = 0;
      }
      if (!(this->blob_x))
      {
        this->blob_x = reinterpret_cast<int *>(malloc(new_size * sizeof(int)));
        if (!(this->blob_x))
        {
          PLAYER_ERROR("Out of memory");
          return -1;
        }
      }
      if (!(this->blob_y))
      {
        this->blob_y = reinterpret_cast<int *>(malloc(new_size * sizeof(int)));
        if (!(this->blob_y))
        {
          PLAYER_ERROR("Out of memory");
          return -1;
        }
      }
      if ((this->blob_x) && (this->blob_y)) this->max_blob_pixels = new_size;
      else
      {
        PLAYER_ERROR("Internal error");
        return -1;
      }
      switch (rawdata->compression)
      {
      case PLAYER_CAMERA_COMPRESS_RAW:
        switch (rawdata->bpp)
        {
        case 8:
          ptr = this->buffer;
          ptr1 = reinterpret_cast<unsigned char *>(rawdata->image);
          for (i = 0; i < static_cast<int>(rawdata->height); i++)
          {
            for (j = 0; j < static_cast<int>(rawdata->width); j++)
            {
              (*ptr) = ((*ptr1) >= thresh) ? 255 : 0;
              ptr++; ptr1++;
            }
          }
          break;
        case 24:
          ptr = this->buffer;
          ptr1 = reinterpret_cast<unsigned char *>(rawdata->image);
          for (i = 0; i < static_cast<int>(rawdata->height); i++)
          {
            for (j = 0; j < static_cast<int>(rawdata->width); j++)
            {
              lum = (0.299 * static_cast<double>(ptr1[0])) + (0.587 * static_cast<double>(ptr1[1])) + (0.114 * static_cast<double>(ptr1[2]));
              if (lum < EPS) lum = 0.0;
              if (lum > (255.0 - EPS)) lum = 255.0;
              (*ptr) = static_cast<unsigned char>(lum);
              (*ptr) = ((*ptr) >= thresh) ? 255 : 0;
              ptr++; ptr1 += 3;
            }
          }
          break;
        case 32:
          ptr = this->buffer;
          ptr1 = reinterpret_cast<unsigned char *>(rawdata->image);
          for (i = 0; i < static_cast<int>(rawdata->height); i++)
          {
            for (j = 0; j < static_cast<int>(rawdata->width); j++)
            {
              lum = (0.299 * static_cast<double>(ptr1[0])) + (0.587 * static_cast<double>(ptr1[1])) + (0.114 * static_cast<double>(ptr1[2]));
              if (lum < EPS) lum = 0.0;
              if (lum > (255.0 - EPS)) lum = 255.0;
              (*ptr) = static_cast<unsigned char>(lum);
              (*ptr) = ((*ptr) >= thresh) ? 255 : 0;
              ptr++; ptr1 += 4;
            }
          }
          break;
        default:
          PLAYER_WARN("unsupported image depth (not good)");
          return -1;
        }
        break;
#if HAVE_JPEG
      case PLAYER_CAMERA_COMPRESS_JPEG:
        jpeg_decompress(this->buffer, this->bufsize, rawdata->image, rawdata->image_count);
        ptr = this->buffer;
        ptr1 = this->buffer;
        for (i = 0; i < static_cast<int>(rawdata->height); i++)
        {
          for (j = 0; j < static_cast<int>(rawdata->width); j++)
          {
            lum = (0.299 * static_cast<double>(ptr1[0])) + (0.587 * static_cast<double>(ptr1[1])) + (0.114 * static_cast<double>(ptr1[2]));
            if (lum < EPS) lum = 0.0;
            if (lum > (255.0 - EPS)) lum = 255.0;
            (*ptr) = static_cast<unsigned char>(lum);
            (*ptr) = ((*ptr) >= thresh) ? 255 : 0;
            ptr++; ptr1 += 3;
          }
        }
        break;
#endif
      default:
        PLAYER_WARN("unsupported compression scheme (not good)");
        return -1;
      }
      if (this->publish_timg)
      {
        timg_output = reinterpret_cast<player_camera_data_t *>(malloc(sizeof(player_camera_data_t)));
        if (!timg_output)
        {
          PLAYER_ERROR("Out of memory");
          return -1;
        }
        memset(timg_output, 0, sizeof(player_camera_data_t));
        timg_output->bpp = 8;
        timg_output->compression = PLAYER_CAMERA_COMPRESS_RAW;
        timg_output->format = PLAYER_CAMERA_FORMAT_MONO8;
        timg_output->fdiv = rawdata->fdiv;
        timg_output->width = rawdata->width;
        timg_output->height = rawdata->height;
        timg_output->image_count = (rawdata->width * rawdata->height);
        timg_output->image = reinterpret_cast<uint8_t *>(malloc(timg_output->image_count));
        if (!(timg_output->image))
        {
          free(timg_output);
          PLAYER_ERROR("Out of memory");
          return -1;
        }
        memcpy(timg_output->image, this->buffer, timg_output->image_count);
        this->Publish(this->camera_provided_addr, PLAYER_MSGTYPE_DATA, PLAYER_CAMERA_DATA_STATE, reinterpret_cast<void *>(timg_output), 0, &(hdr->timestamp), false); // copy = false
        // I assume that Publish() (with copy = false) freed those output data!
        timg_output = NULL;
      }
      found = searchpattern(this->buffer, rawdata->width, rawdata->height, this->min_blob_pixels.GetValue(), this->descriptions, this->results);
      if (found <= 0)
      {
        for (i = 0; i < static_cast<int>(this->numresults); i++) results[i] = -1;
      }
      if (this->debug)
      {
        switch (found)
        {
        case 0:
          PLAYER_WARN("Found nothing");
          break;
        case ERR_TOO_MANY_BLACKS:
          PLAYER_ERROR("Searchpattern error: Too many blacks");
          break;
        case ERR_TOO_MANY_WHITES:
          PLAYER_ERROR("Searchpattern error: Too many whites");
          break;
        case ERR_TOO_MANY_PIXELS:
          PLAYER_ERROR("Searchpattern error: Too many whites");
          break;
        default:
          if (found < 0)
          {
            PLAYER_ERROR1("Unknown error code %d", found);
          } else PLAYER_WARN1("Found %d", found);
        }
        for (i = 0; i < (this->numpatterns); i++)
        {
          PLAYER_WARN5("%d. %d %d - %d %d", i + 1, results[i * 4], results[(i * 4) + 1], results[(i * 4) + 2], results[(i * 4) + 3]);
        }
        PLAYER_WARN("==============");
      }
      if (found < 0) found = 0;
      output = reinterpret_cast<player_blobfinder_data_t *>(malloc(sizeof(player_blobfinder_data_t)));
      if (!output)
      {
        PLAYER_ERROR("Out of memory");
        return -1;
      }
      memset(output, 0, sizeof(player_blobfinder_data_t));
      output->width = (rawdata->width);
      output->height = (rawdata->height);
      output->blobs_count = found;
      output->blobs = reinterpret_cast<player_blobfinder_blob_t *>(malloc(found * sizeof(player_blobfinder_blob_t)));
      if (!(output->blobs))
      {
        free(output);
        PLAYER_ERROR("Out of memory");
        return -1;
      }
      j = 0;
      for (i = 0; i < (this->numpatterns); i++)
      {
        if (!((results[i * 4] < 0) || (results[(i * 4) + 1] < 0) || (results[(i * 4) + 2] < 0) || (results[(i * 4) + 3] < 0)))
        {
          if (j >= found)
          {
            PLAYER_ERROR("Internal error: unexpected number of results");
            return -1;
          }
          area_width = static_cast<size_t>(results[(i * 4) + 1] - results[i * 4]);
          area_height = static_cast<size_t>(results[(i * 4) + 3] - results[(i * 4) + 2]);
          memset(&(output->blobs[j]), 0, sizeof output->blobs[j]);
          output->blobs[j].id = i;
          output->blobs[j].color = this->colors[i];
          output->blobs[j].area = (area_width * area_height);
          output->blobs[j].x = ((static_cast<unsigned>(results[i * 4])) + (area_width >> 1));
          output->blobs[j].y = ((static_cast<unsigned>(results[(i * 4) + 2])) + (area_height >> 1));
          output->blobs[j].left = results[i * 4];
          output->blobs[j].right = results[(i * 4) + 1];
          output->blobs[j].top = results[(i * 4) + 2];
          output->blobs[j].bottom = results[(i * 4) + 3];
          output->blobs[j].range = 0;
          j++;
        }
      }
      this->Publish(this->blobfinder_provided_addr, PLAYER_MSGTYPE_DATA, PLAYER_BLOBFINDER_DATA_BLOBS, reinterpret_cast<void *>(output), 0, &(hdr->timestamp), false); // copy = false
      // I assume that Publish() (with copy = false) freed those output data!
      output = NULL;
    }
    return 0;
  }
  return -1;
}

int Searchpattern::find_pattern(const struct pattern_description * pattern, int id, int blob_parent) const
{
  int f, i, j, k;

  f = 0;
  for (i = 1; i < 255; i++) if ((this->blobs[i].in_use) && ((this->blobs[i].internals) == (pattern[id].internals)))
  {
    if (((pattern[id].parent_id) == NO_PARENT) && ((this->blobs[i].color) != (pattern[id].color))) continue;
    if ((blob_parent != NO_PARENT) && ((this->blobs[i].parent) != blob_parent)) continue;
    for (j = 0, k = 0; k < (pattern[id].internals); j++) if ((pattern[j].parent_id) == id)
    {
      k++;
      if ((this->find_pattern(pattern, j, i)) <= 0) return f;
    }
    if (f) return -1;
    f = i;
  }
  return f;
}

int Searchpattern::searchpattern(unsigned char * area, int width, int height, int min_blob_pixels, const struct pattern_description * pattern, int * results)
{
  int i, j, k, x, y, mustcheck = 0;
  unsigned char blobnum = 0;
  int blackblobs = 0;
  int whiteblobs = 0;
  unsigned char * line;
  unsigned char * current_line;

  for (i = 0; i < 256; i++) (this->blobs[i].in_use) = 0;
  line = area + ((height - 1) * width);
  for (i = 0; i < width; i++)
  {
    area[i] = 0;
    line[i] = 0;
  }
  for (i = 1; i < (height - 1); i++)
  {
    line = area + (i * width);
    line[0] = 0;
    line[width - 1] = 0;
  }
  for (i = 0; i < height; i++)
  {
    current_line = area + (i * width);
    for (j = 0; j < width; j++)
    {
      switch (current_line[j])
      {
      case 0:
        blobnum = blackblobs + 1;
        if (blobnum >= 128)
        {
          PLAYER_ERROR("Too many black blobs");
          return ERR_TOO_MANY_BLACKS;
        }
        break;
      case 255:
        blobnum = 255 - (whiteblobs + 1);
        if (blobnum <= 127)
        {
          PLAYER_ERROR("Too many white blobs");
          return ERR_TOO_MANY_WHITES;
        }
        break;
      default:
        blobnum = 0;
      }
      if (blobnum)
      {
        this->blobs[blobnum].pixels = 0;
        this->blobs[blobnum].minx = j;
        this->blobs[blobnum].maxx = j;
        this->blobs[blobnum].miny = i;
        this->blobs[blobnum].maxy = i;
        this->blobs[blobnum].parent = NO_PARENT;
        this->blobs[blobnum].internals = 0;
        this->blobs[blobnum].color = current_line[j];
        mustcheck = !0;
        while (blobnum)
        {
          if ((current_line[j]) != (this->blobs[blobnum].color)) PLAYER_ERROR("Internal error, something has changed");
          SEARCH_STACK_WIPE();
          SEARCH_STACK_PUSH(j);
          SEARCH_STACK_PUSH(i);
          current_line[j] = blobnum;
          for (;;)
          {
            y = SEARCH_STACK_POP();
            x = SEARCH_STACK_POP();
            if ((x < 0) || (y < 0)) break;
            line = area + (y * width);
            if (y > 0)
            {
              line -= width; y--;
              if (x > 0)
              {
                x--;
                CHECK_PIXEL(line, x, y, blobnum);
                x++;
              }
              CHECK_PIXEL(line, x, y, blobnum);
              if (x < (width - 1))
              {
                x++;
                CHECK_PIXEL(line, x, y, blobnum);
                x--;
              }
              line += width; y++;
            }
            if (y < (height - 1))
            {
              line += width; y++;
              if (x > 0)
              {
                x--;
                CHECK_PIXEL(line, x, y, blobnum);
                x++;
              }
              CHECK_PIXEL(line, x, y, blobnum);
              if (x < (width - 1))
              {
                x++;
                CHECK_PIXEL(line, x, y, blobnum);
                x--;
              }
              line -= width; y--;
            }
            if (x > 0)
            {
              x--;
              CHECK_PIXEL(line, x, y, blobnum);
              x++;
            }
            if (x < (width - 1))
            {
              x++;
              CHECK_PIXEL(line, x, y, blobnum);
              x--;
            }
            if (x < (this->blobs[blobnum].minx)) this->blobs[blobnum].minx = x;
            if (x > (this->blobs[blobnum].maxx)) this->blobs[blobnum].maxx = x;
            if (y < (this->blobs[blobnum].miny)) this->blobs[blobnum].miny = y;
            if (y > (this->blobs[blobnum].maxy)) this->blobs[blobnum].maxy = y;
            this->blob_x[this->blobs[blobnum].pixels] = x;
            this->blob_y[this->blobs[blobnum].pixels] = y;
            (this->blobs[blobnum].pixels)++;
            if ((this->blobs[blobnum].pixels) >= (this->max_blob_pixels))
            {
              PLAYER_ERROR("Blob too big");
              return ERR_TOO_MANY_PIXELS;
            }
          }
          if (mustcheck)
          {
            mustcheck = 0;
            if (((this->blobs[blobnum].pixels) < static_cast<size_t>(min_blob_pixels)) && ((this->blobs[blobnum].parent) > 0))
            {
              for (k = 0; k < static_cast<int>(this->blobs[blobnum].pixels); k++)
              {
                area[((this->blob_y[k]) * width) + (this->blob_x[k])] = this->blobs[this->blobs[blobnum].parent].color;
              }
              this->blobs[this->blobs[blobnum].parent].internals--;
              blobnum = (this->blobs[blobnum].parent);
            } else
            {
              if (!(this->blobs[blobnum].color)) blackblobs++; else whiteblobs++;
              this->blobs[blobnum].in_use = !0;
              blobnum = 0;
            }
          } else blobnum = 0;
        }
      }
    }
  }
  k = 0;
  for (i = 0; (pattern[i].parent_id) == NO_PARENT; i++)
  {
    results[i * 4] = -1;
    results[(i * 4) + 1] = -1;
    results[(i * 4) + 2] = -1;
    results[(i * 4) + 3] = -1;
    j = (this->find_pattern(pattern, i, NO_PARENT));
    switch (j)
    {
    case -1:
      PLAYER_ERROR1("Too many occurences of pattern %d", i);
      break;
    case 0:
      break;
    default:
      results[i * 4] = (this->blobs[j].minx);
      results[(i * 4) + 1] = (this->blobs[j].maxx);
      results[(i * 4) + 2] = (this->blobs[j].miny);
      results[(i * 4) + 3] = (this->blobs[j].maxy);
      k++;
    }
  }
  return k;
}

Driver * Searchpattern_Init(ConfigFile * cf, int section)
{
  // Create and return a new instance of this driver
  return reinterpret_cast<Driver *>(new Searchpattern(cf, section));
}

////////////////////////////////////////////////////////////////////////////////
// a driver registration function
void searchpattern_Register(DriverTable* table)
{
  table->AddDriver("searchpattern", Searchpattern_Init);
}
