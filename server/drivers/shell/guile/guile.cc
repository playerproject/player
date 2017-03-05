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
/////////////////////////////////////////////////////////////////////////////
//
// Desc: Scripting engine for Player Server
// Author: Paul Osmialowski
// Date: 12 Mar 2011
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_guile guile
 * @brief Scripting engine for Player Server

Scripting engine for Player Server.
Note that full description of this driver would be a book size (and maybe one
day such a book will be written). Experienced Player user who is also familiar
with C++ and Scheme programming languages (being familiar with quasiquote and
unquote helps significantly) can figure out how this scripting engine can be used
just from examining provided examples and this driver source code. Due to the
nature of libguile (embedded Scheme interpreter), this driver is not threaded
and is completely message-driven. It is good idea to keep it in separate Player
instance.
On each message arrival a function defined as follows will be executed:

(define fname (lambda (link hdr data env)
  (your-code)
))

Such a function can be considered as a think-act part of sense-think-act loop.
Using 'scriptfile' or 'script' configuration file option, function body
can be provided.

The 'env' parameter is the value returned by previous call of such a function.
At first call, 'env' parameter is the value returned by initialization code
(see 'globals' configuration file option). If there's no initialization code,
the initial value for env is an empty list.

Since this is not threaded driver, requests should be handled carefully. In most
cases it is sufficient just to forward them. Following code will send ACK to
request sender (here called 'input') and forward request message to another
receiver (here called 'output'):

(cond
  ((player-match-message hdr player-msgtype-req -1 input)
    (player-publish-ack link input (player-hdr-subtype hdr))
    (player-putmsg-timestamped
      link
      output
      player-msgtype-req
      (player-hdr-subtype hdr)
      data
      (player-hdr-timestamp hdr)
    )
  )
)

This approach is not the best one since the reply sent back by final request
receiver is not forwarded to original sender (which always gets ACK). However,
this example shows how requests can be handled anyway.
Another approach is to use player-forwardreq function:

(cond
  ((player-match-message hdr player-msgtype-req -1 input)
   (player-forwardreq link output hdr data)
  )
)

@par Compile-time dependencies

- guile 1.8.x

@par Provides

- various interfaces (at least one)

@par Requires

- various interfaces

@par Configuration requests

- none

@par Configuration file options

- keys (string tuple)
  - default value: NONE (certain value must be provided explicitely)
  - variable names for provided/required interfaces
  - note that they are global for all guile driver instances in one Player server
  - keys are held in static array - it is ok, since this driver is NOT threaded
- globals (string tuple)
  - default value: '()
  - global definitions, custom initialization code (Scheme)
  - each string in this tuple is one line of Scheme code
- scriptfile (string)
  - default value: ""
  - file with Scheme code executed on each message arrival
- script (string tuple)
  - default value: NONE
  - if scriptfile is not given, this tuple should contain lines of Scheme code
  - each string is one line of Scheme code
- fname (string)
  - default value: NONE
  - if given, message processing function will have this explicit name;
    this name is global and should be unique
  - if not given, some unique name will be assigned

@par Example

@verbatim

driver
(
  name "guile"
  keys ["in" "out" "ranges"]
  requires ["out::6665:position2d:0" "ranges::6665:ranger:0"]
  provides ["in:::position2d:10"]
  globals ["(define start-env '())" "start-env"]
  scriptfile ""
  script ["(display '(alive))" "(newline)" "env"]
  alwayson 1
)

@endverbatim

Some bigger example (a drunk robot - 'velcmd' driver tells to go straight
ahead at speed 0.4m/s, 'guile' driver makes it to turn left/right randomly;
see how random number generator is initialized in 'globals'):

@verbatim

driver
(
  name "velcmd"
  provides ["opaque:0"]
  requires ["position2d:0"]
  px 0.4
  py 0.0
  pa 0.0
  alwayson 1
)

driver
(
  name "guile"
  fname "some"
  keys ["input" "output" "sensor"]
  provides ["input:::position2d:0"]
  requires ["output:127.0.0.1:6672:position2d:0" "sensor:127.0.0.1:6672:bumper:0"]
  globals [
    "(define get-time (lambda () (round (/ (get-internal-real-time) internal-time-units-per-second))))"
    "(let ((time (gettimeofday))) (set! *random-state* (seed->random-state (+ (car time) (cdr time)))))"
    "`(,(get-time) 0.0 0.0 0.0 1.0)"
  ]
  script [
    "(let*"
    "  ("
    "    (warn (cond"
    "               ((player-match-message hdr player-msgtype-data 1 sensor) (not (zero? (apply + (player-read-datapack-elems 'player_bumper_data 'bumpers data 0 (player-read-datapack 'player_bumper_data 'bumpers_count data))))))"
    "               ((player-match-message hdr player-msgtype-data 1 output) (not (zero? (player-read-datapack 'player_position2d_data 'stall data))))"
    "               (else #f)"
    "    ))"
    "    (new-px (cond"
    "                 ((player-match-message hdr player-msgtype-cmd 1 input) (player-read-datapack 'player_pose2d 'px (player-read-datapack 'player_position2d_cmd_vel 'vel data)))"
    "                 (else (cadr env))"
    "    ))"
    "    (timedout (> (abs (- (get-time) (car env))) 0))"
    "    (change (or warn timedout))"
    "    (direction (cond ((and warn timedout) (* -1.0 (car (cddddr env)))) (else (car (cddddr env)))))"
    "    (new-time (cond (change (get-time)) (else (car env))))"
    "    (px (cond (change new-px) (else (caddr env))))"
    "    (pa (cond (change (- (random:uniform) 0.5)) (else (cadddr env))))"
    "  )"
    "  (cond"
    "       ((player-match-message hdr player-msgtype-req -1 input) (player-forwardreq link output hdr data))"
    "       (else"
    "            (player-putmsg link output player-msgtype-cmd 1"
    "              (player-create-datapack link 'player_position2d_cmd_vel"
    "                `((vel . ,(player-create-datapack link 'player_pose2d `((px . ,(* px direction)) (py . 0.0) (pa . ,pa))))"
    "                  (state . ,player-enable)"
    "                 )"
    "              )"
    "            )"
    "       )"
    "  )"
    "  `(,new-time ,new-px ,px ,pa ,direction)"
    ")"
  ]
)

@endverbatim

More sophisticated example of drunk robot controller - this time a robot
does not like non-black blobs (from blobfinder device) - it tries to go
back until those blobs couldn't be detected (it may cause a robot to hit
the wall and block). This is important example as it shows how to deal with
data array with no size explicitely defined that holds compound data
(blob data), see how map function is used:

@verbatim

driver
(
  name "velcmd"
  provides ["opaque:0"]
  requires ["position2d:0"]
  px 0.4
  py 0.0
  pa 0.0
  alwayson 1
)

driver
(
  name "guile"
  fname "some"
  keys ["input" "output" "sensor" "blobs"]
  provides ["input:::position2d:0"]
  requires ["output:127.0.0.1:6665:position2d:0" "sensor:127.0.0.1:6665:bumper:0" "blobs:127.0.0.1:6665:blobfinder:0"]
  globals [
    "(define get-time (lambda () (round (/ (get-internal-real-time) internal-time-units-per-second))))"
    "(let ((time (gettimeofday))) (set! *random-state* (seed->random-state (+ (car time) (cdr time)))))"
    "`(,(get-time) 0.0 0.0 0.0 1.0)"
  ]
  script [
    "(let*"
    "  ("
    "    (warn (cond"
    "               ((player-match-message hdr player-msgtype-data 1 sensor) (not (zero? (apply + (player-read-datapack-elems 'player_bumper_data 'bumpers data 0 (player-read-datapack 'player_bumper_data 'bumpers_count data))))))"
    "               ((player-match-message hdr player-msgtype-data 1 output) (not (zero? (player-read-datapack 'player_position2d_data 'stall data))))"
    "               (else #f)"
    "    ))"
    "    (new-px (cond"
    "                 ((player-match-message hdr player-msgtype-cmd 1 input) (player-read-datapack 'player_pose2d 'px (player-read-datapack 'player_position2d_cmd_vel 'vel data)))"
    "                 (else (cadr env))"
    "    ))"
    "    (timedout (> (abs (- (get-time) (car env))) 0))"
    "    (change (or warn timedout))"
    "    (blob-colors (lambda (blob) (player-read-datapack 'player_blobfinder_blob 'color blob)))"
    "    (alien-blobs (cond"
    "                      ((player-match-message hdr player-msgtype-data 1 blobs)"
    "                        (not (zero? (apply + (map blob-colors (player-read-datapack-elems 'player_blobfinder_data 'blobs data 0 (player-read-datapack 'player_blobfinder_data 'blobs_count data))))))"
    "                      )"
    "                      (else #f)"
    "    ))"
    "    (direction (cond"
    "                    (alien-blobs -1.0)"
    "                    (else (cond ((and warn timedout) (* -1.0 (car (cddddr env)))) (else (car (cddddr env)))))"
    "    ))"
    "    (new-time (cond (change (get-time)) (else (car env))))"
    "    (px (cond (change new-px) (else (caddr env))))"
    "    (pa (cond (change (- (random:uniform) 0.5)) (else (cadddr env))))"
    "  )"
    "  (cond"
    "       ((player-match-message hdr player-msgtype-req -1 input) (player-forwardreq link output hdr data))"
    "       (else"
    "            (player-putmsg link output player-msgtype-cmd 1"
    "              (player-create-datapack link 'player_position2d_cmd_vel"
    "                `((vel . ,(player-create-datapack link 'player_pose2d `((px . ,(* px direction)) (py . 0.0) (pa . ,pa))))"
    "                  (state . ,player-enable)"
    "                 )"
    "              )"
    "            )"
    "       )"
    "  )"
    "  `(,new-time ,new-px ,px ,pa ,direction)"
    ")"
  ]
)

@endverbatim

@author Paul Osmialowski

*/
/** @} */

#include <libplayercore/playercore.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <libguile.h>

#define MAX_ADDR 20
#define MAX_KEYS 100
#define MAX_OBJS 128

#define BIGBUFFSIZE 4194304

#define PLAYER_FIELD_CHAR       1
#define PLAYER_FIELD_SHORT      2
#define PLAYER_FIELD_INT        4
#define PLAYER_FIELD_INT8       7
#define PLAYER_FIELD_UINT8      8
#define PLAYER_FIELD_INT16     15
#define PLAYER_FIELD_UINT16    16
#define PLAYER_FIELD_INT32     31
#define PLAYER_FIELD_UINT32    32
#define PLAYER_FIELD_INT64     63
#define PLAYER_FIELD_UINT64    64
#define PLAYER_FIELD_FLOAT     70
#define PLAYER_FIELD_DOUBLE    80
#define PLAYER_FIELD_COMPOUND 100

#define RQ_QUEUE_LEN 10

class Guile : public Driver
{
public:
  Guile(ConfigFile * cf, int section);
  virtual ~Guile();
  virtual int Setup();
  virtual int Shutdown();
  virtual int ProcessMessage(QueuePointer & resp_queue,
                             player_msghdr * hdr,
                             void * data);
private:
  struct Link
  {
    QueuePointer & q;
    Guile * d;
  };
  struct Addr
  {
    player_devaddr_t addr;
    Device * dev;
    player_msghdr_t rq_hdrs[RQ_QUEUE_LEN];
    QueuePointer rq_ptrs[RQ_QUEUE_LEN];
    player_devaddr_t rq_addr[RQ_QUEUE_LEN];
    Device * rq_dev[RQ_QUEUE_LEN];
    void * payloads[RQ_QUEUE_LEN];
    int rq[RQ_QUEUE_LEN];
    int last_rq;
  } provided[MAX_ADDR], required[MAX_ADDR];
  struct FieldDesc
  {
    int type;
    size_t size;
    void * ptr;
    int isptr;
    size_t array;
  };
  size_t num_provided;
  size_t num_required;
  void * objs[MAX_OBJS];
  SCM env;
  SCM fun;
  char fname[30];
  static int str_cat(size_t size, char * dst, const char * src);
  static void * getField(const char * structure, const char * field, void * data, struct Guile::FieldDesc * desc, int offset = 0);
  static void setter(void * field, SCM data, int type, size_t size, int offset = 0);
  static void * getptr(SCM ptrlist);
  static SCM mkptr(void * ptr);
  static void scm_player_init();
  static SCM scm_player_match_message(SCM header, SCM msg_type, SCM msg_subtype, SCM key);
  static SCM scm_player_hdr_type(SCM header);
  static SCM scm_player_hdr_subtype(SCM header);
  static SCM scm_player_hdr_timestamp(SCM header);
  static SCM scm_player_publish(SCM link, SCM key, SCM msg_type, SCM msg_subtype, SCM data);
  static SCM scm_player_publish_timestamped(SCM link, SCM key, SCM msg_type, SCM msg_subtype, SCM data, SCM timestamp);
  static SCM scm_player_publish_ack(SCM link, SCM key, SCM msg_subtype);
  static SCM scm_player_publish_nack(SCM link, SCM key, SCM msg_subtype);
  static SCM scm_player_putmsg(SCM link, SCM key, SCM msg_type, SCM msg_suptype, SCM data);
  static SCM scm_player_putmsg_timestamped(SCM link, SCM key, SCM msg_type, SCM msg_suptype, SCM data, SCM timestamp);
  static SCM scm_player_create_datapack(SCM link, SCM type_name, SCM pairlist);
  static SCM scm_player_read_datapack(SCM type_name, SCM field_name, SCM data);
  static SCM scm_player_read_datapack_elem(SCM type_name, SCM field_name, SCM data, SCM offset);
  static SCM scm_player_read_datapack_elems(SCM type_name, SCM field_name, SCM data, SCM offset, SCM count);
  static SCM scm_player_forwardreq(SCM link, SCM key, SCM hdr, SCM data);

#define GET_PTR(type, value) (reinterpret_cast<type *>(Guile::getptr(value)))
#define GET_INT(value) (static_cast<int>(scm_num2double(value, 0, NULL)))
#define GET_UINT(value) (static_cast<unsigned int>(scm_num2double(value, 0, NULL)))
#define GET_INTLL(value) (static_cast<long long>(scm_num2double(value, 0, NULL)))
#define GET_UINTLL(value) (static_cast<unsigned long long>(scm_num2double(value, 0, NULL)))
#define GET_DBL(value) (scm_num2double(value, 0, NULL))
#define SCM_FUNCTION(fun) (reinterpret_cast<SCM (*) ()>(fun))
#define SCM_LISTP(var) (SCM_NFALSEP(scm_list_p(var)))
#define SCM_LIST_APPEND(list, elem) ((list) = scm_append(scm_list_2((list), scm_list_1(elem))))
#define SCM_LIST_LENGTH(list) (scm_num2int(scm_length(list), 0, NULL))
};

void * Guile::getField(const char * structure, const char * field, void * data, struct Guile::FieldDesc * desc, int offset)
{
#define BUILD_THIS
#include <reflection.cc>
#undef BUILD_THIS
}

#define SCM_FIELD_VALUE(data, type, size, offset) (\
  ((type) == (PLAYER_FIELD_CHAR)) ? (scm_int2num((reinterpret_cast<char *>(data))[offset])) : \
  ((type) == (PLAYER_FIELD_SHORT)) ? (scm_int2num((reinterpret_cast<short *>(data))[offset])) : \
  ((type) == (PLAYER_FIELD_INT)) ? (scm_int2num((reinterpret_cast<int *>(data))[offset])) : \
  ((type) == (PLAYER_FIELD_INT8)) ? (scm_int2num((reinterpret_cast<int8_t *>(data))[offset])) : \
  ((type) == (PLAYER_FIELD_UINT8)) ? (scm_uint2num((reinterpret_cast<uint8_t *>(data))[offset])) : \
  ((type) == (PLAYER_FIELD_INT16)) ? (scm_int2num((reinterpret_cast<int16_t *>(data))[offset])) : \
  ((type) == (PLAYER_FIELD_UINT16)) ? (scm_uint2num((reinterpret_cast<uint16_t *>(data))[offset])) : \
  ((type) == (PLAYER_FIELD_INT32)) ? (scm_int2num((reinterpret_cast<int32_t *>(data))[offset])) : \
  ((type) == (PLAYER_FIELD_UINT32)) ? (scm_uint2num((reinterpret_cast<uint32_t *>(data))[offset])) : \
  ((type) == (PLAYER_FIELD_INT64)) ? (scm_long2num(static_cast<long>((reinterpret_cast<int64_t *>(data))[offset]))) : \
  ((type) == (PLAYER_FIELD_UINT64)) ? (scm_ulong2num(static_cast<unsigned long>((reinterpret_cast<uint64_t *>(data))[offset]))) : \
  ((type) == (PLAYER_FIELD_FLOAT)) ? (scm_double2num((reinterpret_cast<float *>(data))[offset])) : \
  ((type) == (PLAYER_FIELD_DOUBLE)) ? (scm_double2num((reinterpret_cast<double *>(data))[offset])) : \
  ((type) == (PLAYER_FIELD_COMPOUND)) ? (Guile::mkptr(reinterpret_cast<void *>(&((reinterpret_cast<unsigned char *>(data))[(offset) * (size)])))) : \
  SCM_EOL)

void Guile::setter(void * field, SCM data, int type, size_t size, int offset)
{
  void * ptr;

  assert(field);
  switch (type)
  {
  case PLAYER_FIELD_CHAR:
    (reinterpret_cast<char *>(field))[offset] = GET_INT(data);
    break;
  case PLAYER_FIELD_SHORT:
    (reinterpret_cast<short *>(field))[offset] = GET_INT(data);
    break;
  case PLAYER_FIELD_INT:
    (reinterpret_cast<int *>(field))[offset] = GET_INT(data);
    break;
  case PLAYER_FIELD_INT8:
    (reinterpret_cast<int8_t *>(field))[offset] = GET_INT(data);
    break;
  case PLAYER_FIELD_UINT8:
    (reinterpret_cast<uint8_t *>(field))[offset] = GET_UINT(data);
    break;
  case PLAYER_FIELD_INT16:
    (reinterpret_cast<int16_t *>(field))[offset] = GET_INT(data);
    break;
  case PLAYER_FIELD_UINT16:
    (reinterpret_cast<uint16_t *>(field))[offset] = GET_UINT(data);
    break;
  case PLAYER_FIELD_INT32:
    (reinterpret_cast<int32_t *>(field))[offset] = GET_INT(data);
    break;
  case PLAYER_FIELD_UINT32:
    (reinterpret_cast<uint32_t *>(field))[offset] = GET_UINT(data);
    break;
  case PLAYER_FIELD_INT64:
    (reinterpret_cast<int64_t *>(field))[offset] = GET_INTLL(data);
    break;
  case PLAYER_FIELD_UINT64:
    (reinterpret_cast<uint64_t *>(field))[offset] = GET_UINTLL(data);
    break;
  case PLAYER_FIELD_FLOAT:
    (reinterpret_cast<float *>(field))[offset] = GET_DBL(data);
    break;
  case PLAYER_FIELD_DOUBLE:
    (reinterpret_cast<double *>(field))[offset] = GET_DBL(data);
    break;
  case PLAYER_FIELD_COMPOUND:
    ptr = Guile::getptr(data);
    assert(ptr);
    memcpy(reinterpret_cast<void *>(&((reinterpret_cast<unsigned char *>(field))[size * offset])), ptr, size);
    break;
  default:
    assert(!"Internal error!");
  }
}

SCM Guile::scm_player_create_datapack(SCM link, SCM type_name, SCM pairlist)
{
  struct Guile::Link * lnk = GET_PTR(struct Guile::Link, link);
  int i, n, count;
  struct Guile::FieldDesc desc;
  SCM pair;
  SCM data;
  void * ptr = NULL;
  void * array;
  int array_count;
  const char * field_name = NULL;

  assert(lnk);
  assert(lnk->d);
  assert(SCM_LISTP(pairlist));
  count = SCM_LIST_LENGTH(pairlist);
  if (!(count > 0)) return SCM_EOL;
  ptr = Guile::getField(SCM_SYMBOL_CHARS(type_name), NULL, NULL, &desc, 0);
  assert(ptr);
  for (i = 0; i < count; i++)
  {
    pair = scm_list_ref(pairlist, scm_int2num(i));
    field_name = SCM_SYMBOL_CHARS(SCM_CAR(pair));
    assert(field_name);
    data = SCM_CDR(pair);
    Guile::getField(SCM_SYMBOL_CHARS(type_name), field_name, ptr, &desc, 0);
    assert(desc.ptr);
    assert(desc.type);
    assert(desc.size > 0);
    if (desc.isptr)
    {
      assert(SCM_LISTP(data));
      array_count = SCM_LIST_LENGTH(data);
      if (!array_count)
      {
        (reinterpret_cast<void **>(desc.ptr))[0] = NULL;
      } else
      {
        assert(array_count > 0);
        array = malloc(desc.size * array_count);
        assert(array);
        for (n = 0; n < MAX_OBJS; n++)
        {
          if (!(lnk->d->objs[n])) lnk->d->objs[n] = array;
          assert(lnk->d->objs[n]);
          break;
        }
        if (n >= MAX_OBJS)
        {
          free(array);
          free(ptr);
          assert(!"Not enough memory slots");
          return SCM_EOL;
        }
        for (n = 0; n < array_count; n++) Guile::setter(array, scm_list_ref(data, scm_int2num(n)), desc.type, desc.size, n);
        (reinterpret_cast<void **>(desc.ptr))[0] = array;
      }
    } else if (desc.array > 0)
    {
      assert(SCM_LISTP(data));
      assert(SCM_LIST_LENGTH(data) == static_cast<int>(desc.array));
      for (n = 0; n < static_cast<int>(desc.array); n++) Guile::setter(desc.ptr, scm_list_ref(data, scm_int2num(n)), desc.type, desc.size, n);
    } else Guile::setter(desc.ptr, data, desc.type, desc.size, 0);
  }
  for (i = 0; i < MAX_OBJS; i++)
  {
    if (!(lnk->d->objs[i])) lnk->d->objs[i] = ptr;
    assert(lnk->d->objs[i]);
    return mkptr(lnk->d->objs[i]);
  }
  free(ptr);
  assert(!"Not enough memory slots");
  return SCM_EOL;
}

SCM Guile::scm_player_read_datapack(SCM type_name, SCM field_name, SCM data)
{
  struct Guile::FieldDesc desc;
  void * gain;
  int i;
  SCM results = SCM_EOL;

  gain = Guile::getField(SCM_SYMBOL_CHARS(type_name), SCM_SYMBOL_CHARS(field_name), Guile::getptr(data), &desc, 0);
  assert(gain);
  assert(!(desc.isptr));
  if ((desc.array) > 0)
  {
    for (i = 0; i < static_cast<int>(desc.array); i++) SCM_LIST_APPEND(results, SCM_FIELD_VALUE(gain, desc.type, desc.size, i));
    return results;
  }
  return SCM_FIELD_VALUE(gain, desc.type, desc.size, 0);
}

SCM Guile::scm_player_read_datapack_elem(SCM type_name, SCM field_name, SCM data, SCM offset)
{
  struct Guile::FieldDesc desc;
  void * gain;

  gain = Guile::getField(SCM_SYMBOL_CHARS(type_name), SCM_SYMBOL_CHARS(field_name), Guile::getptr(data), &desc, GET_INT(offset));
  assert(gain);
  return SCM_FIELD_VALUE(gain, desc.type, desc.size, 0);
}

SCM Guile::scm_player_read_datapack_elems(SCM type_name, SCM field_name, SCM data, SCM offset, SCM count)
{
  struct Guile::FieldDesc desc;
  void * gain;
  int i;
  SCM results = SCM_EOL;

  if (!count) return SCM_EOL;
  gain = Guile::getField(SCM_SYMBOL_CHARS(type_name), SCM_SYMBOL_CHARS(field_name), Guile::getptr(data), &desc, GET_INT(offset));
  if (!gain) return SCM_EOL;
  for (i = 0; i < GET_INT(count); i++) SCM_LIST_APPEND(results, SCM_FIELD_VALUE(gain, desc.type, desc.size, i));
  return results;
}

int Guile::str_cat(size_t size, char * dst, const char * src)
{
  if ((strlen(dst) + strlen(src)) >= size) return -1;
  for (; dst[0]; dst++);
  snprintf(dst, (size - strlen(dst)), "%s", src);
  return 0;
}

SCM Guile::mkptr(void * ptr)
{
  unsigned char * chrptr = reinterpret_cast<unsigned char *>(&ptr);
  SCM ptrlist = SCM_EOL;
  size_t i;

  for (i = 0; i < sizeof ptr; i++) SCM_LIST_APPEND(ptrlist, scm_uint2num(chrptr[i]));
  return ptrlist;
}

void * Guile::getptr(SCM ptrlist)
{
  void * ptr = NULL;
  unsigned char * chrptr = reinterpret_cast<unsigned char *>(&ptr);
  size_t i;

  assert(SCM_LISTP(ptrlist));
  assert(SCM_LIST_LENGTH(ptrlist) == (sizeof ptr));
  for (i = 0; i < sizeof ptr; i++) chrptr[i] = GET_UINT(scm_list_ref(ptrlist, scm_int2num(i)));
  return ptr;
}

SCM Guile::scm_player_match_message(SCM header, SCM msg_type, SCM msg_subtype, SCM key)
{
  struct Guile::Addr * addr = GET_PTR(struct Guile::Addr, key);
  player_msghdr * hdr = GET_PTR(player_msghdr, header);

  assert(addr);
  assert(hdr);
  return SCM_BOOL(Message::MatchMessage(hdr, GET_INT(msg_type), GET_INT(msg_subtype), addr->addr));
}

SCM Guile::scm_player_hdr_type(SCM header)
{
  player_msghdr * hdr = GET_PTR(player_msghdr, header);

  assert(hdr);
  return scm_uint2num(hdr->type);
}

SCM Guile::scm_player_hdr_subtype(SCM header)
{
  player_msghdr * hdr = GET_PTR(player_msghdr, header);

  assert(hdr);
  return scm_uint2num(hdr->subtype);
}

SCM Guile::scm_player_hdr_timestamp(SCM header)
{
  player_msghdr * hdr = GET_PTR(player_msghdr, header);

  assert(hdr);
  return scm_uint2num(hdr->timestamp);
}

SCM Guile::scm_player_publish(SCM link, SCM key, SCM msg_type, SCM msg_subtype, SCM data)
{
  struct Guile::Link * lnk = GET_PTR(struct Guile::Link, link);
  struct Guile::Addr * addr = GET_PTR(struct Guile::Addr, key);

  assert(lnk);
  assert(lnk->d);
  assert(addr);
  lnk->d->Publish(addr->addr, lnk->q, GET_INT(msg_type), GET_INT(msg_subtype), Guile::getptr(data), 0, NULL, true);
  return SCM_EOL;
}

SCM Guile::scm_player_publish_timestamped(SCM link, SCM key, SCM msg_type, SCM msg_subtype, SCM data, SCM timestamp)
{
  struct Guile::Link * lnk = GET_PTR(struct Guile::Link, link);
  struct Guile::Addr * addr = GET_PTR(struct Guile::Addr, key);
  double t = GET_DBL(timestamp);

  assert(lnk);
  assert(lnk->d);
  assert(addr);
  lnk->d->Publish(addr->addr, lnk->q, GET_INT(msg_type), GET_INT(msg_subtype), Guile::getptr(data), 0, &t, true);
  return SCM_EOL;

}

SCM Guile::scm_player_publish_ack(SCM link, SCM key, SCM msg_subtype)
{
  struct Guile::Link * lnk = GET_PTR(struct Guile::Link, link);
  struct Guile::Addr * addr = GET_PTR(struct Guile::Addr, key);

  assert(lnk);
  assert(lnk->d);
  assert(addr);
  lnk->d->Publish(addr->addr, lnk->q, PLAYER_MSGTYPE_RESP_ACK, GET_INT(msg_subtype));
  return SCM_EOL;
}

SCM Guile::scm_player_publish_nack(SCM link, SCM key, SCM msg_subtype)
{
  struct Guile::Link * lnk = GET_PTR(struct Guile::Link, link);
  struct Guile::Addr * addr = GET_PTR(struct Guile::Addr, key);

  assert(lnk);
  assert(lnk->d);
  assert(addr);
  lnk->d->Publish(addr->addr, lnk->q, PLAYER_MSGTYPE_RESP_NACK, GET_INT(msg_subtype));
  return SCM_EOL;
}

SCM Guile::scm_player_putmsg(SCM link, SCM key, SCM msg_type, SCM msg_subtype, SCM data)
{
  struct Guile::Link * lnk = GET_PTR(struct Guile::Link, link);
  struct Guile::Addr * addr = GET_PTR(struct Guile::Addr, key);

  assert(lnk);
  assert(lnk->d);
  assert(addr);
  assert(addr->dev);
  addr->dev->PutMsg(lnk->d->InQueue, GET_INT(msg_type), GET_INT(msg_subtype), Guile::getptr(data), 0, NULL);
  return SCM_EOL;
}

SCM Guile::scm_player_putmsg_timestamped(SCM link, SCM key, SCM msg_type, SCM msg_subtype, SCM data, SCM timestamp)
{
  struct Guile::Link * lnk = GET_PTR(struct Guile::Link, link);
  struct Guile::Addr * addr = GET_PTR(struct Guile::Addr, key);
  double t = GET_DBL(timestamp);

  assert(lnk);
  assert(lnk->d);
  assert(addr);
  assert(addr->dev);
  addr->dev->PutMsg(lnk->d->InQueue, GET_INT(msg_type), GET_INT(msg_subtype), Guile::getptr(data), 0, &t);
  return SCM_EOL;
}

SCM Guile::scm_player_forwardreq(SCM link, SCM key, SCM hdr, SCM data)
{
  struct Guile::Link * lnk = GET_PTR(struct Guile::Link, link);
  struct Guile::Addr * addr = GET_PTR(struct Guile::Addr, key);
  player_msghdr_t * header = GET_PTR(player_msghdr_t, hdr);
  player_msghdr_t newhdr;
  void * pload = NULL;
  int i, n;

  assert(lnk);
  assert(lnk->d);
  assert(addr);
  assert(addr->dev);
  assert(header);
  for (i = 0; i < RQ_QUEUE_LEN; i++) if (!(addr->rq[i]))
  {
    addr->rq_hdrs[i] = *header;
    addr->rq_ptrs[i] = lnk->q;
    addr->rq_addr[i] = addr->addr;
    addr->rq_dev[i] = addr->dev;
    assert(addr->rq_dev[i]);
    if ((header->size) > 0)
    {
      pload = Guile::getptr(data);
      assert(pload);
      addr->payloads[i] = malloc(header->size);
      assert(addr->payloads[i]);
      memcpy(addr->payloads[i], pload, header->size);
    } else addr->payloads[i] = NULL;
    addr->rq[i] = !0;
    break;
  }
  assert(i < RQ_QUEUE_LEN);
  n = -1;
  for (i = 0; i < RQ_QUEUE_LEN; i++) if (addr->rq[i]) n = i;
  assert(n >= 0);
  if (!n)
  {
    newhdr = addr->rq_hdrs[n];
    newhdr.addr = addr->rq_addr[n];
    if ((newhdr.size) > 0) assert(addr->payloads[n]);
    assert(addr->rq_dev[n]);
    addr->rq_dev[n]->PutMsg(lnk->d->InQueue, &newhdr, addr->payloads[n], true); // copy = true
    addr->last_rq = n;
  }
  return SCM_EOL;
}

void Guile::scm_player_init()
{
  struct
  {
    const char * name;
    int code;
  } codenames[] =
  {
    { "player-player-code",		PLAYER_PLAYER_CODE },		// the server itself
    { "player-power-code",		PLAYER_POWER_CODE },		// power subsystem
    { "player-gripper-code",		PLAYER_GRIPPER_CODE },		// gripper
    { "player-position2d-code",		PLAYER_POSITION2D_CODE },	// device that moves about in the plane
    { "player-blobfinder-code",		PLAYER_BLOBFINDER_CODE },	// visual blobfinder
    { "player-ptz-code",		PLAYER_PTZ_CODE },		// pan-tilt-zoom unit
    { "player-fiducial-code",		PLAYER_FIDUCIAL_CODE },		// fiducial detector
    { "player-speech-code",		PLAYER_SPEECH_CODE },		// speech I/O
    { "player-bumper-code",		PLAYER_BUMPER_CODE },		// bumper array
    { "player-dio-code",		PLAYER_DIO_CODE },		// digital I/O
    { "player-aio-code",		PLAYER_AIO_CODE },		// analog I/O
    { "player-localize-code",		PLAYER_LOCALIZE_CODE },		// localization
    { "player-position3d-code",		PLAYER_POSITION3D_CODE },	// 3-D position
    { "player-simulation-code",		PLAYER_SIMULATION_CODE },	// simulators
    { "player-camera-code",		PLAYER_CAMERA_CODE },		// camera device
    { "player-map-code",		PLAYER_MAP_CODE },		// get a map
    { "player-planner-code",		PLAYER_PLANNER_CODE },		// 2D motion planner
    { "player-joystick-code",		PLAYER_JOYSTICK_CODE },		// joytstick
    { "player-opaque-code",		PLAYER_OPAQUE_CODE },		// plugin interface
    { "player-position1d-code",         PLAYER_POSITION1D_CODE },       // 1-D position
    { "player-graphics2d-code",         PLAYER_GRAPHICS2D_CODE },       // graphics2D interface
    { "player-actarray-code",           PLAYER_ACTARRAY_CODE },         // actarray interface
    { "player-ranger-code",             PLAYER_RANGER_CODE },           // ranger interface
    { NULL, 0 }
  };
  int i;

  scm_c_define("player-null",                           Guile::mkptr(NULL));
  scm_c_define("player-camera-format-mono8",            scm_int2num(PLAYER_CAMERA_FORMAT_MONO8));
  scm_c_define("player-camera-format-mono16",           scm_int2num(PLAYER_CAMERA_FORMAT_MONO16));
  scm_c_define("player-camera-format-rgb565",           scm_int2num(PLAYER_CAMERA_FORMAT_RGB565));
  scm_c_define("player-camera-format-rgb888",           scm_int2num(PLAYER_CAMERA_FORMAT_RGB888));
  scm_c_define("player-camera-compress-raw",            scm_int2num(PLAYER_CAMERA_COMPRESS_RAW));
  scm_c_define("player-camera-compress-jpeg",           scm_int2num(PLAYER_CAMERA_COMPRESS_JPEG));
  scm_c_define("player-cell-empty",                     scm_int2num(-1));
  scm_c_define("player-cell-unknown",                   scm_int2num(0));
  scm_c_define("player-cell-occupied",                  scm_int2num(1));
  scm_c_define("player-enable",                         scm_int2num(1));
  scm_c_define("player-disable",                        scm_int2num(0));
  scm_c_define("player-gripper-state-open",             scm_int2num(PLAYER_GRIPPER_STATE_OPEN));
  scm_c_define("player-gripper-state-closed",           scm_int2num(PLAYER_GRIPPER_STATE_CLOSED));
  scm_c_define("player-gripper-state-moving",           scm_int2num(PLAYER_GRIPPER_STATE_MOVING));
  scm_c_define("player-gripper-state-error",            scm_int2num(PLAYER_GRIPPER_STATE_ERROR));
  scm_c_define("player-msgtype-data",                   scm_int2num(PLAYER_MSGTYPE_DATA));
  scm_c_define("player-msgtype-cmd",                    scm_int2num(PLAYER_MSGTYPE_CMD));
  scm_c_define("player-msgtype-req",                    scm_int2num(PLAYER_MSGTYPE_REQ));
  scm_c_define("player-msgtype-resp-ack",               scm_int2num(PLAYER_MSGTYPE_RESP_ACK));
  scm_c_define("player-msgtype-synch",                  scm_int2num(PLAYER_MSGTYPE_SYNCH));
  scm_c_define("player-msgtype-resp-nack",              scm_int2num(PLAYER_MSGTYPE_RESP_NACK));
  scm_c_define("player-actarray-type-linear",           scm_int2num(PLAYER_ACTARRAY_TYPE_LINEAR));
  scm_c_define("player-actarray-type-rotary",           scm_int2num(PLAYER_ACTARRAY_TYPE_ROTARY));
  scm_c_define("player-actarray-actstate-idle",         scm_int2num(PLAYER_ACTARRAY_ACTSTATE_IDLE));
  scm_c_define("player-actarray-actstate-moving",       scm_int2num(PLAYER_ACTARRAY_ACTSTATE_MOVING));
  scm_c_define("player-actarray-actstate-braked",       scm_int2num(PLAYER_ACTARRAY_ACTSTATE_BRAKED));
  scm_c_define("player-actarray-actstate-stalled",      scm_int2num(PLAYER_ACTARRAY_ACTSTATE_STALLED));
  scm_c_define("player-draw-mode-points",               scm_int2num(PLAYER_DRAW_POINTS));
  scm_c_define("player-draw-mode-lines",                scm_int2num(PLAYER_DRAW_LINES));
  scm_c_define("player-draw-mode-line-strip",           scm_int2num(PLAYER_DRAW_LINE_STRIP));
  scm_c_define("player-draw-mode-line-loop",            scm_int2num(PLAYER_DRAW_LINE_LOOP));
  scm_c_define("player-draw-mode-triangles",            scm_int2num(PLAYER_DRAW_TRIANGLES));
  scm_c_define("player-draw-mode-triangle-strip",       scm_int2num(PLAYER_DRAW_TRIANGLE_STRIP));
  scm_c_define("player-draw-mode-triangle-fan",         scm_int2num(PLAYER_DRAW_TRIANGLE_FAN));
  scm_c_define("player-draw-mode-quads",                scm_int2num(PLAYER_DRAW_QUADS));
  scm_c_define("player-draw-mode-quad-strip",           scm_int2num(PLAYER_DRAW_QUAD_STRIP));
  scm_c_define("player-draw-mode-polygon",              scm_int2num(PLAYER_DRAW_POLYGON));
  scm_c_define("player-ptz-velocity-control",           scm_int2num(PLAYER_PTZ_VELOCITY_CONTROL));
  scm_c_define("player-ptz-position-control",           scm_int2num(PLAYER_PTZ_POSITION_CONTROL));
  for (i = 0; codenames[i].name; i++) scm_c_define(codenames[i].name, scm_int2num(codenames[i].code));
  scm_c_define_gsubr("player-match-message", 4, 0, 0, SCM_FUNCTION(Guile::scm_player_match_message));
  scm_c_define_gsubr("player-hdr-type", 1, 0, 0, SCM_FUNCTION(Guile::scm_player_hdr_type));
  scm_c_define_gsubr("player-hdr-subtype", 1, 0, 0, SCM_FUNCTION(Guile::scm_player_hdr_subtype));
  scm_c_define_gsubr("player-hdr-timestamp", 1, 0, 0, SCM_FUNCTION(Guile::scm_player_hdr_timestamp));
  scm_c_define_gsubr("player-publish", 5, 0, 0, SCM_FUNCTION(Guile::scm_player_publish));
  scm_c_define_gsubr("player-publish-timestamped", 6, 0, 0, SCM_FUNCTION(Guile::scm_player_publish_timestamped));
  scm_c_define_gsubr("player-publish-ack", 3, 0, 0, SCM_FUNCTION(Guile::scm_player_publish_ack));
  scm_c_define_gsubr("player-publish-nack", 3, 0, 0, SCM_FUNCTION(Guile::scm_player_publish_nack));
  scm_c_define_gsubr("player-putmsg", 5, 0, 0, SCM_FUNCTION(Guile::scm_player_putmsg));
  scm_c_define_gsubr("player-putmsg-timestamped", 6, 0, 0, SCM_FUNCTION(Guile::scm_player_putmsg_timestamped));
  scm_c_define_gsubr("player-forwardreq", 4, 0, 0, SCM_FUNCTION(Guile::scm_player_forwardreq));
  scm_c_define_gsubr("player-create-datapack", 3, 0, 0, SCM_FUNCTION(Guile::scm_player_create_datapack));
  scm_c_define_gsubr("player-read-datapack", 3, 0, 0, SCM_FUNCTION(Guile::scm_player_read_datapack));
  scm_c_define_gsubr("player-read-datapack-elem", 4, 0, 0, SCM_FUNCTION(Guile::scm_player_read_datapack_elem));
  scm_c_define_gsubr("player-read-datapack-elems", 5, 0, 0, SCM_FUNCTION(Guile::scm_player_read_datapack_elems));
}

Guile::Guile(ConfigFile * cf, int section)
  : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  int i, j, n, rnum;
  const char * key;
  const char * pkeys[MAX_ADDR];
  const char * rkeys[MAX_ADDR];
  char * bigbuffer;
  char linebuff[4096];
  FILE * f;
  static int init_guile = 0; // this is not a threaded driver, so we can safely use static variable here
  static const char * keys[MAX_KEYS];
  static size_t global_keys = 0;
  static int ids = 0;

  for (i = 0; i < MAX_ADDR; i++)
  {
    memset(&(this->provided[i].addr), 0, sizeof this->provided[i].addr);
    memset(&(this->required[i].addr), 0, sizeof this->required[i].addr);
    this->provided[i].dev = NULL;
    this->required[i].dev = NULL;
    memset(&(this->provided[i].rq_hdrs), 0, sizeof this->provided[i].rq_hdrs);
    memset(&(this->required[i].rq_hdrs), 0, sizeof this->required[i].rq_hdrs);
    memset(&(this->provided[i].rq_addr), 0, sizeof this->provided[i].rq_addr);
    memset(&(this->required[i].rq_addr), 0, sizeof this->required[i].rq_addr);
    for (j = 0; j < RQ_QUEUE_LEN; j++)
    {
      this->provided[i].payloads[j] = NULL;
      this->required[i].payloads[j] = NULL;
      this->provided[i].rq[j] = 0;
      this->required[i].rq[j] = 0;
      this->provided[i].rq_dev[j] = NULL;
      this->required[i].rq_dev[j] = NULL;
    }
    this->provided[i].last_rq = -1;
    this->required[i].last_rq = -1;
    pkeys[i] = NULL;
    rkeys[i] = NULL;
  }
  this->num_provided = 0;
  this->num_required = 0;
  for (i = 0; i < MAX_OBJS; i++) this->objs[i] = NULL;
  this->env = SCM_EOL;
  this->fun = SCM_EOL;
  this->fname[0] = 0;
  snprintf(this->fname, sizeof this->fname, "processor-%d", ids++);
  key = cf->ReadString(section, "fname", "");
  if (key)
  {
    if (strlen(key) > 0)
    {
      snprintf(this->fname, sizeof this->fname, "%s", key);
    }
  }
  key = this->fname;
  if (!key)
  {
    PLAYER_ERROR("NULL name");
    this->SetError(-1);
    return;
  }
  if (!(strlen(key) > 0))
  {
    PLAYER_ERROR("Name not given");
    this->SetError(-1);
    return;
  }
  if (global_keys >= MAX_KEYS)
  {
    PLAYER_ERROR("Too many names in use");
    this->SetError(-1);
    return;
  }
  for (j = 0; j < static_cast<int>(global_keys); j++)
  {
    if (!strcmp(key, keys[j]))
    {
      PLAYER_ERROR1("Name [%s] already in use", key);
      this->SetError(-1);
      return;
    }
  }
  keys[global_keys] = key;
  global_keys++;
  n = cf->GetTupleCount(section, "keys");
  if ((n <= 0) || (n > MAX_ADDR))
  {
    PLAYER_ERROR("Invalid number of keys");
    this->SetError(-1);
    return;
  }
  rnum = cf->GetTupleCount(section, "requires");
  if (rnum < 0)
  {
    this->SetError(-1);
    return;
  }
  for (i = 0; i < n; i++)
  {
    key = cf->ReadTupleString(section, "keys", i, "");
    if (!key)
    {
      PLAYER_ERROR("NULL key");
      this->SetError(-1);
      return;
    }
    if (!(strlen(key) > 0))
    {
      PLAYER_ERROR("Key name not given");
      this->SetError(-1);
      return;
    }
    if (global_keys >= MAX_KEYS)
    {
      PLAYER_ERROR("Too many keys in use");
      this->SetError(-1);
      return;
    }
    for (j = 0; j < static_cast<int>(global_keys); j++)
    {
      if (!strcmp(key, keys[j]))
      {
        PLAYER_ERROR1("Key [%s] already in use", key);
        this->SetError(-1);
        return;
      }
    }
    keys[global_keys] = key;
    global_keys++;
    if (cf->ReadDeviceAddr(&(this->provided[this->num_provided].addr), section, "provides",
                           -1, -1, key))
    {
      if (cf->ReadDeviceAddr(&(this->required[this->num_required].addr), section, "requires",
                           -1, -1, key))
      {
        PLAYER_ERROR1("%s: device not provided nor required", key);
        this->SetError(-1);
        return;
      }
      rkeys[this->num_required] = key;
      key = NULL;
      this->num_required++;
    } else
    {
      if (rnum > 0) if (!(cf->ReadDeviceAddr(&(this->required[this->num_required].addr), section, "requires",
                                             -1, -1, key)))
      {
        PLAYER_ERROR1("One key [%s] should not be used for both provided and required device", key);
        this->SetError(-1);
        return;
      }
      if (this->AddInterface(this->provided[this->num_provided].addr))
      {
        PLAYER_ERROR1("%s: cannot add interface", key);
        this->SetError(-1);
        return;
      }
      pkeys[this->num_provided] = key;
      key = NULL;
      this->num_provided++;
    }
  }
  if (n != static_cast<int>((this->num_required) + (this->num_provided)))
  {
    PLAYER_ERROR("Internal error");
    this->SetError(-1);
    return;
  }
  if (!init_guile)
  {
    init_guile = !0;
    scm_init_guile();
    Guile::scm_player_init();
  }
  for (i = 0; i < static_cast<int>(this->num_required); i++) scm_c_define(rkeys[i], Guile::mkptr(reinterpret_cast<void *>(&(this->required[i]))));
  for (i = 0; i < static_cast<int>(this->num_provided); i++) scm_c_define(pkeys[i], Guile::mkptr(reinterpret_cast<void *>(&(this->provided[i]))));
  n = cf->GetTupleCount(section, "globals");
  if (n > 0)
  {
    for (i = 0; i < n; i++)
    {
      key = cf->ReadTupleString(section, "globals", i, "");
      if (key)
      {
        if (strlen(key) > 0) this->env = scm_c_eval_string(key);
      }
    }
  } else
  {
    this->env = scm_c_eval_string("(quote ())");
  }
  bigbuffer = reinterpret_cast<char *>(malloc(BIGBUFFSIZE));
  if (!bigbuffer)
  {
    PLAYER_ERROR("Out of memory");
    this->SetError(-1);
    return;
  }
  bigbuffer[0] = 0;
  snprintf(linebuff, sizeof linebuff, "(define %s (lambda (link hdr data env) ", this->fname);
  if (Guile::str_cat(BIGBUFFSIZE, bigbuffer, linebuff))
  {
    PLAYER_ERROR("Internal error");
    free(bigbuffer);
    this->SetError(-1);
    return;
  }
  key = cf->ReadString(section, "scriptfile", "");
  if (!key) key = ""; // just in case
  assert(key);
  if (strlen(key) > 0)
  {
    f = fopen(key, "rt");
    if (!f)
    {
      PLAYER_ERROR1("cannot open file %s", key);
      free(bigbuffer);
      this->SetError(-1);
      return;
    }
    do
    {
      linebuff[0] = 0;
      fgets(linebuff, sizeof linebuff, f);
      if (strlen(linebuff) > 0)
      {
        if (Guile::str_cat(BIGBUFFSIZE, bigbuffer, linebuff))
        {
          PLAYER_ERROR("cannot process script file");
          fclose(f);
          free(bigbuffer);
          this->SetError(-1);
          return;
        }
      }
    } while (!feof(f));
    fclose(f);
  } else
  {
    n = cf->GetTupleCount(section, "script");
    if (!(n > 0))
    {
      PLAYER_ERROR("empty script");
      free(bigbuffer);
      this->SetError(-1);
      return;
    }
    for (i = 0; i < n; i++)
    {
      key = cf->ReadTupleString(section, "script", i, "");
      if (key)
      {
        if (strlen(key) > 0)
        {
          if (Guile::str_cat(BIGBUFFSIZE, bigbuffer, key))
          {
            PLAYER_ERROR("cannot process script");
            free(bigbuffer);
            this->SetError(-1);
            return;
          }
        }
      }
    }
  }
  snprintf(linebuff, sizeof linebuff, ")) %s", this->fname);
  if (Guile::str_cat(BIGBUFFSIZE, bigbuffer, linebuff))
  {
    PLAYER_ERROR("Internal error");
    free(bigbuffer);
    this->SetError(-1);
    return;
  }
  this->fun = scm_c_eval_string(bigbuffer);
  free(bigbuffer);
}

Guile::~Guile()
{
  int i, j;

  for (i = 0; i < MAX_OBJS; i++)
  {
    if (this->objs[i])
    {
      free(this->objs[i]);
      this->objs[i] = NULL;
    }
  }
  for (i = 0; i < MAX_ADDR; i++)
  {
    for (j = 0; j < RQ_QUEUE_LEN; j++)
    {
      if (this->provided[i].payloads[j])
      {
        free(this->provided[i].payloads[j]);
        this->provided[i].payloads[j] = NULL;
      }
      if (this->required[i].payloads[j])
      {
        free(this->required[i].payloads[j]);
        this->required[i].payloads[j] = NULL;
      }
    }
  }
}

int Guile::Setup()
{
  int i, j;

  for (i = 0; i < MAX_ADDR; i++)
  {
    memset(&(this->provided[i].rq_hdrs), 0, sizeof this->provided[i].rq_hdrs);
    memset(&(this->required[i].rq_hdrs), 0, sizeof this->required[i].rq_hdrs);
    memset(&(this->provided[i].rq_addr), 0, sizeof this->provided[i].rq_addr);
    memset(&(this->required[i].rq_addr), 0, sizeof this->required[i].rq_addr);
    for (j = 0; j < RQ_QUEUE_LEN; j++)
    {
      this->provided[i].payloads[j] = NULL;
      this->required[i].payloads[j] = NULL;
      this->provided[i].rq[j] = 0;
      this->required[i].rq[j] = 0;
      this->provided[i].rq_dev[j] = NULL;
      this->required[i].rq_dev[j] = NULL;
    }
    this->provided[i].last_rq = -1;
    this->required[i].last_rq = -1;
  }
  for (i = 0; i < static_cast<int>(this->num_required); i++)
  {
    this->required[i].dev = deviceTable->GetDevice(this->required[i].addr);
    if (!(this->required[i].dev))
    {
      PLAYER_ERROR1("unable to locate required device %d", i);
      for (j = 0; j < i; j++)
      {
        if (this->required[j].dev) this->required[j].dev->Unsubscribe(this->InQueue);
        this->required[j].dev = NULL;
      }
      return -1;
    }
    if (this->required[i].dev->Subscribe(this->InQueue))
    {
      PLAYER_ERROR1("unable to subscribe required device %d", i);
      for (j = 0; j < i; j++)
      {
        if (this->required[j].dev) this->required[j].dev->Unsubscribe(this->InQueue);
        this->required[j].dev = NULL;
      }
      return -1;
    }
  }
  return 0;
}

int Guile::Shutdown()
{
  int i, j;

  for (i = 0; i < static_cast<int>(this->num_required); i++)
  {
    if (this->required[i].dev) this->required[i].dev->Unsubscribe(this->InQueue);
    this->required[i].dev = NULL;
  }
  for (i = 0; i < MAX_ADDR; i++)
  {
    for (j = 0; j < RQ_QUEUE_LEN; j++)
    {
      if (this->provided[i].payloads[j])
      {
        free(this->provided[i].payloads[j]);
        this->provided[i].payloads[j] = NULL;
      }
      if (this->required[i].payloads[j])
      {
        free(this->required[i].payloads[j]);
        this->required[i].payloads[j] = NULL;
      }
      this->provided[i].rq[j] = 0;
      this->required[i].rq[j] = 0;
    }
  }
  return 0;
}

int Guile::ProcessMessage(QueuePointer & resp_queue, player_msghdr * hdr, void * data)
{
  struct Guile::Link link = { resp_queue, this };
  QueuePointer null;
  player_msghdr_t newhdr;
  SCM retval;
  int i, j;

  if (!hdr)
  {
    PLAYER_ERROR("NULL header");
    return -1;
  }
  for (i = 0; i < MAX_ADDR; i++)
  {
    if (!((this->required[i].last_rq) < 0))
    {
      if ((Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_ACK, -1, this->required[i].addr)) || (Message::MatchMessage(hdr, PLAYER_MSGTYPE_RESP_NACK, -1, this->required[i].addr)))
      {
        assert((this->required[i].last_rq) >= 0);
        assert((hdr->subtype) == (this->required[i].rq_hdrs[this->required[i].last_rq].subtype));
        this->Publish(this->required[i].rq_hdrs[this->required[i].last_rq].addr, this->required[i].rq_ptrs[this->required[i].last_rq], hdr->type, hdr->subtype, data, 0, &(hdr->timestamp), true); // copy = true
        memset(&(this->required[i].rq_addr[this->required[i].last_rq]), 0, sizeof this->required[i].rq_addr[this->required[i].last_rq]);
        this->required[i].rq_dev[this->required[i].last_rq] = NULL;
        this->required[i].rq_ptrs[this->required[i].last_rq] = null;
        assert(this->required[i].rq[this->required[i].last_rq]);
        if (this->required[i].payloads[this->required[i].last_rq]) free(this->required[i].payloads[this->required[i].last_rq]);
        this->required[i].payloads[this->required[i].last_rq] = NULL;
        this->required[i].rq[this->required[i].last_rq] = 0;
        this->required[i].last_rq = -1;
        for (j = 0; j < RQ_QUEUE_LEN; j++) if (this->required[i].rq[j])
        {
          newhdr = this->required[i].rq_hdrs[j];
          newhdr.addr = this->required[i].rq_addr[j];
          if ((newhdr.size) > 0) assert(this->required[i].payloads[j]);
          assert(this->required[i].rq_dev[j]);
          this->required[i].rq_dev[j]->PutMsg(this->InQueue, &newhdr, this->required[i].payloads[j], true); // copy = true;
          this->required[i].last_rq = i;
          break;
        }
        return 0;
      }
    }
  }
  retval = scm_call_4(this->fun, Guile::mkptr(reinterpret_cast<void *>(&link)), Guile::mkptr(reinterpret_cast<void *>(hdr)), Guile::mkptr(data), this->env);
  for (i = 0; i < MAX_OBJS; i++)
  {
    if (this->objs[i])
    {
      free(this->objs[i]);
      this->objs[i] = NULL;
    }
  }
  if (SCM_LISTP(retval)) if (!(SCM_LIST_LENGTH(retval) > 0)) return -1;
  this->env = retval;
  return 0;
}

Driver * Guile_Init(ConfigFile * cf, int section)
{
  return reinterpret_cast<Driver *>(new Guile(cf, section));
}

void guile_Register(DriverTable * table)
{
  table->AddDriver("guile", Guile_Init);
}
