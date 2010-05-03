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
/////////////////////////////////////////////////////////////////////////////
//
// Desc: Driver for storing log data into PostgreSQL database
// Author: Paul Osmialowski
// Date: 12 Mar 2010
//
/////////////////////////////////////////////////////////////////////////////

/** @ingroup drivers */
/** @{ */
/** @defgroup driver_postlog postlog
 * @brief Driver for storing log data into PostgreSQL database

The postlog driver is for storing log data into given tables of given PostgreSQL
database.

@par Compile-time dependencies

- libpqxx

@par Provides

- @ref interface_log : can be used to turn logging on/off
- (optional) list of devices to provide (see "Configuration file options" below)

@par Requires

- list of devices to subscribe (see "Configuration file options" below)
  (can be empty only if there are provided interfaces for logging commands)

@par Configuration requests

- PLAYER_LOG_REQ_SET_WRITE_STATE
- PLAYER_LOG_REQ_GET_STATE

@par Configuration file options

- tables (string tuple)
  - Default: empty tuple
  - Names of tables in the database, data (or commands) obtained from each
    subscribed/provided device will be written into given table; these names
    are also used as key names for distinguishing devices in 'Requires' and
    'Provides' lists.
- init_state (integer)
  - Default: 1
  - Initial write state (1 - ON, 0 - OFF).
  - Write state can be changed by PLAYER_LOG_REQ_SET_WRITE_STATE request.
- wait_for_all
  - Default: 0
  - If set to non-zero, this driver will be waiting for complete set of data and commands,
    it means that data (and commands) will be stored right after receiving them from all
    provided and subscribed interfaces; data or commands from one interface received more
    frequently will be dropped.
  - Devices that send many subtypes of data to one subscribed interface may make
    confusions here.
- dbname (string)
  - Default: "postlog"
  - The name of the Postgresql database to connect to.
- host (string)
  - Default: "127.0.0.1"
  - The name of the database host.
- user (string)
  - Default: "postgres"
  - The name of the database user.
- port (string)
  - Default: "5432"
  - The port of the database.
- password (string)
  - Default: ""
  - The password for the database user.
- sequence (string)
  - Default: "postlog_seq"
  - Sequence name for primary keys (see "database prerequisites").

@par Database prerequisites

Before using this driver, you need to have database created in your PostgreSQL server.
You can create it using UNIX shell by typing such command:

createdb postlog

('postlog' is example database name; depending on database server policies, you may
be asked for database user password after entering this; if you do not have database
user account, ask database administrator to create one for you).

Then you can enter database shell by typing such command:

psql postlog

Now you can enter SQL commands. First you need to create incremental sequence that will
be used as primary key ('postlog_seq' is example sequence name):

CREATE SEQUENCE postlog_seq START 1 INCREMENT 1 MINVALUE 1;

Then you have to create table for each subscribed (or provided) device. Although record
structure for each table depends on interface type, first fields are common
for each table:

id INT4 PRIMARY KEY,
index INT2 NOT NULL,
unixtime BIGINT NOT NULL,
hdrtime DOUBLE PRECISION NOT NULL,
globaltime DOUBLE PRECISION NOT NULL,
subtype VARCHAR(50) NOT NULL

Additionally, you can add PostgreSQL date and time fields that will be filled
automatically:

date DATE DEFAULT CURRENT_DATE NOT NULL,
time TIME(4) DEFAULT CURRENT_TIME(4)

Time from four different sources (system clock, data header, Player global clock,
SQL server clock) will give you whole image of this infrastructure time dependencies.

Currently supported interfaces are:

- position1d
  - for commands:
  CREATE TABLE table1 (
    id INT4 PRIMARY KEY,
    index INT2 NOT NULL,
    date DATE DEFAULT CURRENT_DATE NOT NULL,
    time TIME(4) DEFAULT CURRENT_TIME(4),
    unixtime BIGINT NOT NULL,
    hdrtime DOUBLE PRECISION NOT NULL,
    globaltime DOUBLE PRECISION NOT NULL,
    subtype VARCHAR(50) NOT NULL,
    state BOOLEAN DEFAULT NULL,
    pos DOUBLE PRECISION DEFAULT NULL,
    vel DOUBLE PRECISION DEFAULT NULL
  );
  - for data:
  CREATE TABLE table1 (
    id INT4 PRIMARY KEY,
    index INT2 NOT NULL,
    date DATE DEFAULT CURRENT_DATE NOT NULL,
    time TIME(4) DEFAULT CURRENT_TIME(4),
    unixtime BIGINT NOT NULL,
    hdrtime DOUBLE PRECISION NOT NULL,
    globaltime DOUBLE PRECISION NOT NULL,
    subtype VARCHAR(50) NOT NULL,
    pos DOUBLE PRECISION DEFAULT NULL,
    vel DOUBLE PRECISION DEFAULT NULL,
    stall BOOLEAN DEFAULT NULL,
    status INTEGER DEFAULT NULL
  );

- position2d
  - for commands:
  CREATE TABLE table1 (
    id INT4 PRIMARY KEY,
    index INT2 NOT NULL,
    date DATE DEFAULT CURRENT_DATE NOT NULL,
    time TIME(4) DEFAULT CURRENT_TIME(4),
    unixtime BIGINT NOT NULL,
    hdrtime DOUBLE PRECISION NOT NULL,
    globaltime DOUBLE PRECISION NOT NULL,
    subtype VARCHAR(50) NOT NULL,
    state BOOLEAN DEFAULT NULL,
    px DOUBLE PRECISION DEFAULT NULL,
    py DOUBLE PRECISION DEFAULT NULL,
    pa DOUBLE PRECISION DEFAULT NULL,
    vx DOUBLE PRECISION DEFAULT NULL,
    vy DOUBLE PRECISION DEFAULT NULL,
    va DOUBLE PRECISION DEFAULT NULL,
    velocity DOUBLE PRECISION DEFAULT NULL,
    angle DOUBLE PRECISION DEFAULT NULL
  );
  - for data:
  CREATE TABLE table1 (
    id INT4 PRIMARY KEY,
    index INT2 NOT NULL,
    date DATE DEFAULT CURRENT_DATE NOT NULL,
    time TIME(4) DEFAULT CURRENT_TIME(4),
    unixtime BIGINT NOT NULL,
    hdrtime DOUBLE PRECISION NOT NULL,
    globaltime DOUBLE PRECISION NOT NULL,
    subtype VARCHAR(50) NOT NULL,
    px DOUBLE PRECISION DEFAULT NULL,
    py DOUBLE PRECISION DEFAULT NULL,
    pa DOUBLE PRECISION DEFAULT NULL,
    vx DOUBLE PRECISION DEFAULT NULL,
    vy DOUBLE PRECISION DEFAULT NULL,
    va DOUBLE PRECISION DEFAULT NULL,
    stall BOOLEAN DEFAULT NULL
  );

- position3d
  - for commands:
  CREATE TABLE table1 (
    id INT4 PRIMARY KEY,
    index INT2 NOT NULL,
    date DATE DEFAULT CURRENT_DATE NOT NULL,
    time TIME(4) DEFAULT CURRENT_TIME(4),
    unixtime BIGINT NOT NULL,
    hdrtime DOUBLE PRECISION NOT NULL,
    globaltime DOUBLE PRECISION NOT NULL,
    subtype VARCHAR(50) NOT NULL,
    state BOOLEAN DEFAULT NULL,
    px DOUBLE PRECISION DEFAULT NULL,
    py DOUBLE PRECISION DEFAULT NULL,
    pz DOUBLE PRECISION DEFAULT NULL,
    proll DOUBLE PRECISION DEFAULT NULL,
    ppitch DOUBLE PRECISION DEFAULT NULL,
    pyaw DOUBLE PRECISION DEFAULT NULL,
    vx DOUBLE PRECISION DEFAULT NULL,
    vy DOUBLE PRECISION DEFAULT NULL,
    vz DOUBLE PRECISION DEFAULT NULL,
    vroll DOUBLE PRECISION DEFAULT NULL,
    vpitch DOUBLE PRECISION DEFAULT NULL,
    vyaw DOUBLE PRECISION DEFAULT NULL
  );
  - for data:
  CREATE TABLE table1 (
    id INT4 PRIMARY KEY,
    index INT2 NOT NULL,
    date DATE DEFAULT CURRENT_DATE NOT NULL,
    time TIME(4) DEFAULT CURRENT_TIME(4),
    unixtime BIGINT NOT NULL,
    hdrtime DOUBLE PRECISION NOT NULL,
    globaltime DOUBLE PRECISION NOT NULL,
    subtype VARCHAR(50) NOT NULL,
    px DOUBLE PRECISION DEFAULT NULL,
    py DOUBLE PRECISION DEFAULT NULL,
    pz DOUBLE PRECISION DEFAULT NULL,
    proll DOUBLE PRECISION DEFAULT NULL,
    ppitch DOUBLE PRECISION DEFAULT NULL,
    pyaw DOUBLE PRECISION DEFAULT NULL,
    vx DOUBLE PRECISION DEFAULT NULL,
    vy DOUBLE PRECISION DEFAULT NULL,
    vz DOUBLE PRECISION DEFAULT NULL,
    vroll DOUBLE PRECISION DEFAULT NULL,
    vpitch DOUBLE PRECISION DEFAULT NULL,
    vyaw DOUBLE PRECISION DEFAULT NULL,
    stall BOOLEAN DEFAULT NULL
  );

- aio
  - for commands:
  CREATE TABLE table1 (
    id INT4 PRIMARY KEY,
    index INT2 NOT NULL,
    date DATE DEFAULT CURRENT_DATE NOT NULL,
    time TIME(4) DEFAULT CURRENT_TIME(4),
    unixtime BIGINT NOT NULL,
    hdrtime DOUBLE PRECISION NOT NULL,
    globaltime DOUBLE PRECISION NOT NULL,
    subtype VARCHAR(50) NOT NULL,
    io_id INTEGER DEFAULT NULL,
    voltage DOUBLE PRECISION DEFAULT NULL
  );
  - for data:
  CREATE TABLE table1 (
    id INT4 PRIMARY KEY,
    index INT2 NOT NULL,
    date DATE DEFAULT CURRENT_DATE NOT NULL,
    time TIME(4) DEFAULT CURRENT_TIME(4),
    unixtime BIGINT NOT NULL,
    hdrtime DOUBLE PRECISION NOT NULL,
    globaltime DOUBLE PRECISION NOT NULL,
    subtype VARCHAR(50) NOT NULL,
    count INTEGER NOT NULL,
    v0 DOUBLE PRECISION DEFAULT NULL,
    v1 DOUBLE PRECISION DEFAULT NULL,
    ...
  );
  Add as many v* fields as needed.

- bumper
  - for data:
  CREATE TABLE table1 (
    id INT4 PRIMARY KEY,
    index INT2 NOT NULL,
    date DATE DEFAULT CURRENT_DATE NOT NULL,
    time TIME(4) DEFAULT CURRENT_TIME(4),
    unixtime BIGINT NOT NULL,
    hdrtime DOUBLE PRECISION NOT NULL,
    globaltime DOUBLE PRECISION NOT NULL,
    subtype VARCHAR(50) NOT NULL,
    count INTEGER NOT NULL,
    bumper0 BOOLEAN DEFAULT NULL,
    bumper1 BOOLEAN DEFAULT NULL,
    ...
  );
  Add as many bumper* fields as needed.

- dio
  - for both commands and data:
  CREATE TABLE table1 (
    id INT4 PRIMARY KEY,
    index INT2 NOT NULL,
    date DATE DEFAULT CURRENT_DATE NOT NULL,
    time TIME(4) DEFAULT CURRENT_TIME(4),
    unixtime BIGINT NOT NULL,
    hdrtime DOUBLE PRECISION NOT NULL,
    globaltime DOUBLE PRECISION NOT NULL,
    subtype VARCHAR(50) NOT NULL,
    count INTEGER DEFAULT NULL,
    d31 BOOLEAN DEFAULT NULL,
    d30 BOOLEAN DEFAULT NULL,
    d29 BOOLEAN DEFAULT NULL,
    d28 BOOLEAN DEFAULT NULL,
    d27 BOOLEAN DEFAULT NULL,
    d26 BOOLEAN DEFAULT NULL,
    d25 BOOLEAN DEFAULT NULL,
    d24 BOOLEAN DEFAULT NULL,
    d23 BOOLEAN DEFAULT NULL,
    d22 BOOLEAN DEFAULT NULL,
    d21 BOOLEAN DEFAULT NULL,
    d20 BOOLEAN DEFAULT NULL,
    d19 BOOLEAN DEFAULT NULL,
    d18 BOOLEAN DEFAULT NULL,
    d17 BOOLEAN DEFAULT NULL,
    d16 BOOLEAN DEFAULT NULL,
    d15 BOOLEAN DEFAULT NULL,
    d14 BOOLEAN DEFAULT NULL,
    d13 BOOLEAN DEFAULT NULL,
    d12 BOOLEAN DEFAULT NULL,
    d11 BOOLEAN DEFAULT NULL,
    d10 BOOLEAN DEFAULT NULL,
    d9 BOOLEAN DEFAULT NULL,
    d8 BOOLEAN DEFAULT NULL,
    d7 BOOLEAN DEFAULT NULL,
    d6 BOOLEAN DEFAULT NULL,
    d5 BOOLEAN DEFAULT NULL,
    d4 BOOLEAN DEFAULT NULL,
    d3 BOOLEAN DEFAULT NULL,
    d2 BOOLEAN DEFAULT NULL,
    d1 BOOLEAN DEFAULT NULL,
    d0 BOOLEAN DEFAULT NULL
  );
  The 'count' value here tells how many bits (starting from d0 on the right)
  were actually set by sender.

- gripper
  - for commands:
  CREATE TABLE table1 (
    id INT4 PRIMARY KEY,
    index INT2 NOT NULL,
    date DATE DEFAULT CURRENT_DATE NOT NULL,
    time TIME(4) DEFAULT CURRENT_TIME(4),
    unixtime BIGINT NOT NULL,
    hdrtime DOUBLE PRECISION NOT NULL,
    globaltime DOUBLE PRECISION NOT NULL,
    subtype VARCHAR(50) NOT NULL
  );
  - for data:
  CREATE TABLE table1 (
    id INT4 PRIMARY KEY,
    index INT2 NOT NULL,
    date DATE DEFAULT CURRENT_DATE NOT NULL,
    time TIME(4) DEFAULT CURRENT_TIME(4),
    unixtime BIGINT NOT NULL,
    hdrtime DOUBLE PRECISION NOT NULL,
    globaltime DOUBLE PRECISION NOT NULL,
    subtype VARCHAR(50) NOT NULL,
    state INTEGER DEFAULT NULL,
    beams INTEGER DEFAULT NULL,
    stored INTEGER DEFAULT NULL
  );

- ptz
  - for commands:
  CREATE TABLE table1 (
    id INT4 PRIMARY KEY,
    index INT2 NOT NULL,
    date DATE DEFAULT CURRENT_DATE NOT NULL,
    time TIME(4) DEFAULT CURRENT_TIME(4),
    unixtime BIGINT NOT NULL,
    hdrtime DOUBLE PRECISION NOT NULL,
    globaltime DOUBLE PRECISION NOT NULL,
    subtype VARCHAR(50) NOT NULL,
    pan DOUBLE PRECISION DEFAULT NULL,
    tilt DOUBLE PRECISION DEFAULT NULL,
    zoom DOUBLE PRECISION DEFAULT NULL,
    panspeed DOUBLE PRECISION DEFAULT NULL,
    tiltspeed DOUBLE PRECISION DEFAULT NULL
  );
  - for data:
  CREATE TABLE table1 (
    id INT4 PRIMARY KEY,
    index INT2 NOT NULL,
    date DATE DEFAULT CURRENT_DATE NOT NULL,
    time TIME(4) DEFAULT CURRENT_TIME(4),
    unixtime BIGINT NOT NULL,
    hdrtime DOUBLE PRECISION NOT NULL,
    globaltime DOUBLE PRECISION NOT NULL,
    subtype VARCHAR(50) NOT NULL,
    pan DOUBLE PRECISION DEFAULT NULL,
    tilt DOUBLE PRECISION DEFAULT NULL,
    zoom DOUBLE PRECISION DEFAULT NULL,
    panspeed DOUBLE PRECISION DEFAULT NULL,
    tiltspeed DOUBLE PRECISION DEFAULT NULL,
    status INTEGER DEFAULT NULL
  );

- ranger
  - for data:
  CREATE TABLE table1 (
    id INT4 PRIMARY KEY,
    index INT2 NOT NULL,
    date DATE DEFAULT CURRENT_DATE NOT NULL,
    time TIME(4) DEFAULT CURRENT_TIME(4),
    unixtime BIGINT NOT NULL,
    hdrtime DOUBLE PRECISION NOT NULL,
    globaltime DOUBLE PRECISION NOT NULL,
    subtype VARCHAR(50) NOT NULL,
    count INTEGER DEFAULT NULL,
    r0 DOUBLE PRECISION DEFAULT NULL,
    r1 DOUBLE PRECISION DEFAULT NULL,
    ...
  );
  Add as many r* fields as needed.

- speech
  - for commands:
  CREATE TABLE table1 (
    id INT4 PRIMARY KEY,
    index INT2 NOT NULL,
    date DATE DEFAULT CURRENT_DATE NOT NULL,
    time TIME(4) DEFAULT CURRENT_TIME(4),
    unixtime BIGINT NOT NULL,
    hdrtime DOUBLE PRECISION NOT NULL,
    globaltime DOUBLE PRECISION NOT NULL,
    subtype VARCHAR(50) NOT NULL,
    string_count INTEGER DEFAULT NULL,
    phrase VARCHAR(....) DEFAULT NULL
  );
  Set the maximal lenght of the phraze as needed (in VARCHAR parenthesis).
  The 'string_count' value tells what is the actual length of the phraze.

@par Example

@verbatim

driver
(
  name "postlog"
  tables ["table1" "table2"]
  requires ["table1::6665:position2d:0"]
  provides ["log:0" "table2:::position2d:10"]
  wait_for_all 1
  alwayson 1
)
driver
(
  name "cmdsplitter"
  provides ["position2d:0"]
  devices 2
  requires ["1::6665:position2d:0" "0:::position2d:10"]
)

@endverbatim

This is not a threaded driver, it is good idea to keep it in separate Player instance.

@author Paul Osmialowski

*/
/** @} */

#include <libplayercore/playercore.h>
#include <stddef.h>
#include <libpq-fe.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#define MAX_ADDR 20
#define MAX_PARAMS 400
#define BUFSIZE 50
#define QUERY_SIZE 8192

#if defined (WIN32)
  #define snprintf _snprintf
#endif

class Postlog : public Driver
{
public:
  Postlog(ConfigFile * cf, int section);
  virtual ~Postlog();
  virtual int Setup();
  virtual int Shutdown();
  virtual int ProcessMessage(QueuePointer & resp_queue,
                             player_msghdr * hdr,
                             void * data);
private:
  int isConnected() const;
  int storeData(int getId, const char * table, const void * data, double timestamp, uint16_t interf, uint16_t index, uint8_t type, uint8_t subtype);
  void rollback() const;
  static int nparams(char * dst, size_t dstsize, const char * param, int offset, size_t num);
  PGconn * conn;
  player_devaddr_t provided_log_addr;
  player_devaddr_t provided[MAX_ADDR];
  player_devaddr_t required[MAX_ADDR];
  Device * required_devs[MAX_ADDR];
  const char * ptables[MAX_ADDR];
  const char * rtables[MAX_ADDR];
  int pstored[MAX_ADDR];
  int rstored[MAX_ADDR];
  size_t num_provided;
  size_t num_required;
  int init_state;
  int state;
  int wait_for_all;
  const char * dbname;
  const char * host;
  const char * user;
  const char * port;
  const char * password;
  const char * sequence;
  char * query;
  int id;
  char * buf[MAX_PARAMS];
};

Driver * Postlog_Init(ConfigFile * cf, int section)
{
  return reinterpret_cast<Driver *>(new Postlog(cf, section));
}

void postlog_Register(DriverTable *table)
{
  table->AddDriver("postlog", Postlog_Init);
}

Postlog::Postlog(ConfigFile * cf, int section)
  : Driver(cf, section, true, PLAYER_MSGQUEUE_DEFAULT_MAXLEN)
{
  int i, n, rnum;
  const char * table;

  this->conn = NULL;
  memset(&(this->provided_log_addr), 0, sizeof(player_devaddr_t));
  memset(this->provided, 0, sizeof this->provided);
  memset(this->required, 0, sizeof this->required);
  for (i = 0; i < MAX_ADDR; i++)
  {
    this->required_devs[i] = NULL;
    this->ptables[i] = NULL;
    this->rtables[i] = NULL;
    this->pstored[i] = 0;
    this->rstored[i] = 0;
  }
  this->num_provided = 0;
  this->num_required = 0;
  this->init_state = 0;
  this->state = 0;
  this->wait_for_all = 0;
  this->dbname = NULL;
  this->host = NULL;
  this->user = NULL;
  this->port = NULL;
  this->password = NULL;
  this->sequence = NULL;
  this->query = NULL;
  this->id = 0;
  for (i = 0; i < MAX_PARAMS; i++) this->buf[i] = NULL;
  if (cf->ReadDeviceAddr(&(this->provided_log_addr), section, "provides",
                         PLAYER_LOG_CODE, -1, NULL))
  {
    this->SetError(-1);
    return;
  }
  if (this->AddInterface(this->provided_log_addr))
  {
    this->SetError(-1);
    return;
  }
  rnum = cf->GetTupleCount(section, "requires");
  if (rnum < 0)
  {
    this->SetError(-1);
    return;
  }
  n = cf->GetTupleCount(section, "tables");
  if ((n <= 0) || (n > MAX_ADDR))
  {
    PLAYER_ERROR("Invalid number of tables");
    this->SetError(-1);
    return;
  }
  for (i = 0; i < n; i++)
  {
    table = cf->ReadTupleString(section, "tables", i, "");
    if (!table)
    {
      PLAYER_ERROR("NULL table");
      this->SetError(-1);
      return;
    }
    if (!(strlen(table) > 0))
    {
      PLAYER_ERROR("Table name not given");
      this->SetError(-1);
      return;
    }
    if (cf->ReadDeviceAddr(&(this->provided[this->num_provided]), section, "provides",
                           -1, -1, table))
    {
      if (cf->ReadDeviceAddr(&(this->required[this->num_required]), section, "requires",
                           -1, -1, table))
      {
        PLAYER_ERROR1("%s: device not provided nor required", table);
        this->SetError(-1);
        return;
      }
      this->rtables[this->num_required] = table;
      table = NULL;
      this->num_required++;
    } else
    {
      if (rnum > 0) if (!(cf->ReadDeviceAddr(&(this->required[this->num_required]), section, "requires",
                                             -1, -1, table)))
      {
        PLAYER_ERROR1("One table name [%s] should not be used for both provided and required device", table);
        this->SetError(-1);
        return;
      }
      if (this->AddInterface(this->provided[this->num_provided]))
      {
        PLAYER_ERROR1("%s: cannot add interface", table);
        this->SetError(-1);
        return;
      }
      this->ptables[this->num_provided] = table;
      table = NULL;
      this->num_provided++;
    }
  }
  if (n != static_cast<int>((this->num_required) + (this->num_provided)))
  {
    PLAYER_ERROR("Internal error");
    this->SetError(-1);
    return;
  }
  this->init_state = cf->ReadInt(section, "init_state", 1);
  this->wait_for_all = cf->ReadInt(section, "wait_for_all", 0);
  this->dbname = cf->ReadString(section, "dbname", "postlog");
  if (!(this->dbname))
  {
    PLAYER_ERROR("NULL dbname");
    this->SetError(-1);
    return;
  }
  if (!(strlen(this->dbname) > 0))
  {
    PLAYER_ERROR("Empty dbname");
    this->SetError(-1);
    return;
  }
  this->host = cf->ReadString(section, "host", "127.0.0.1");
  if (!(this->host))
  {
    PLAYER_ERROR("NULL host");
    this->SetError(-1);
    return;
  }
  this->user = cf->ReadString(section, "user", "postgres");
  if (!(this->user))
  {
    PLAYER_ERROR("NULL user");
    this->SetError(-1);
    return;
  }
  this->port = cf->ReadString(section, "port", "5432");
  if (!(this->port))
  {
    PLAYER_ERROR("NULL port");
    this->SetError(-1);
    return;
  }
  this->password = cf->ReadString(section, "password", "");
  if (!(this->password))
  {
    PLAYER_ERROR("NULL password");
    this->SetError(-1);
    return;
  }
  this->sequence = cf->ReadString(section, "sequence", "postlog_seq");
  if (!(this->sequence))
  {
    PLAYER_ERROR("NULL sequence");
    this->SetError(-1);
    return;
  }
  this->query = reinterpret_cast<char *>(malloc(QUERY_SIZE));
  if (!(this->query))
  {
    PLAYER_ERROR("Out of memory");
    this->SetError(-1);
    return;
  }
  for (i = 0; i < MAX_PARAMS; i++)
  {
    this->buf[i] = reinterpret_cast<char *>(malloc(BUFSIZE));
    if (!(this->buf[i]))
    {
      PLAYER_ERROR("Out of memory");
      for (i = 0; i < MAX_PARAMS; i++)
      {
        if (this->buf[i]) free(this->buf[i]);
        this->buf[i] = NULL;
      }
      if (this->query) free(this->query);
      this->query = NULL;
      this->SetError(-1);
      return;
    }
  }
}

Postlog::~Postlog()
{
  int i;

  if (this->isConnected()) PQfinish(this->conn);
  this->conn = NULL;
  if (this->query) free(this->query);
  this->query = NULL;
  for (i = 0; i < MAX_PARAMS; i++)
  {
    if (this->buf[i]) free(this->buf[i]);
    this->buf[i] = NULL;
  }
}

int Postlog::isConnected() const
{
  return this->conn && (PQstatus(this->conn) != CONNECTION_BAD);
}

int Postlog::Setup()
{
  int i, j;

  for (i = 0; i < MAX_ADDR; i++)
  {
    this->pstored[i] = 0;
    this->rstored[i] = 0;
  }
  for (i = 0; i < static_cast<int>(this->num_required); i++)
  {
    this->required_devs[i] = deviceTable->GetDevice(this->required[i]);
    if (!(this->required_devs[i]))
    {
      PLAYER_ERROR1("%s: unable to locate suitable device", rtables[i]);
      for (j = 0; j < i; j++)
      {
        if (this->required_devs[j]) this->required_devs[j]->Unsubscribe(this->InQueue);
        this->required_devs[j] = NULL;
      }
      return -1;
    }
    if (this->required_devs[i]->Subscribe(this->InQueue))
    {
      PLAYER_ERROR1("%s: unable to subscribe device", rtables[i]);
      for (j = 0; j < i; j++)
      {
        if (this->required_devs[j]) this->required_devs[j]->Unsubscribe(this->InQueue);
        this->required_devs[j] = NULL;
      }
      return -1;
    }
  }
  this->conn = PQsetdbLogin(this->host, this->port, NULL, NULL, this->dbname, this->user, this->password);
  if (!(this->conn)) return -1;
  if (PQstatus(this->conn) == CONNECTION_BAD)
  {
    this->conn = NULL;
    return -1;
  }
  this->id = 0;
  this->state = this->init_state;
  return 0;
}

int Postlog::Shutdown()
{
  int i;

  for (i = 0; i < static_cast<int>(this->num_required); i++)
  {
    if (this->required_devs[i]) this->required_devs[i]->Unsubscribe(this->InQueue);
    this->required_devs[i] = NULL;
  }
  if (this->isConnected()) PQfinish(this->conn);
  this->conn = NULL;
  for (i = 0; i < MAX_ADDR; i++)
  {
    this->pstored[i] = 0;
    this->rstored[i] = 0;
  }
  return 0;
}

int Postlog::ProcessMessage(QueuePointer & resp_queue,
                            player_msghdr * hdr,
                            void * data)
{
  int i, j;
  int getId;
  player_log_set_write_state_t * write_state;
  player_log_get_state_t get_state;

  if (!hdr)
  {
    PLAYER_ERROR("NULL header");
    return -1;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_LOG_REQ_SET_WRITE_STATE, this->provided_log_addr))
  {
    if (!data)
    {
      PLAYER_ERROR("NULL data");
      return -1;
    }
    write_state = reinterpret_cast<player_log_set_write_state_t *>(data);
    if (!write_state)
    {
      PLAYER_ERROR("Internal error: unexpected NULL");
      return -1;
    }
    this->state = write_state->state;
    this->Publish(this->provided_log_addr, resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK, PLAYER_LOG_REQ_SET_WRITE_STATE);
    return 0;
  }
  if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, PLAYER_LOG_REQ_GET_STATE, this->provided_log_addr))
  {
    get_state.type = PLAYER_LOG_TYPE_WRITE;
    get_state.state = static_cast<uint8_t>(this->state);
    this->Publish(this->provided_log_addr,
                  resp_queue,
                  PLAYER_MSGTYPE_RESP_ACK,
                  PLAYER_LOG_REQ_GET_STATE,
                  reinterpret_cast<void *>(&get_state), 0, NULL, true); // copy = true, do not dispose data placed on local stack!
    return 0;
  }
  getId = !0;
  if ((this->wait_for_all) && (this->id))
  {
    getId = 0;
    i = !0;
    for (j = 0; j < static_cast<int>(this->num_provided); j++) if (!(this->pstored[j]))
    {
      i = 0;
      break;
    }
    if (i) for (j = 0; j < static_cast<int>(this->num_required); j++) if (!(this->rstored[j]))
    {
      i = 0;
      break;
    }
    if (i)
    {
      for (j = 0; j < static_cast<int>(this->num_provided); j++) (this->pstored[j]) = 0;
      for (j = 0; j < static_cast<int>(this->num_required); j++) (this->rstored[j]) = 0;
      getId = !0;
    }
  }
  for (i = 0; i < static_cast<int>(this->num_provided); i++)
  {
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_REQ, -1, this->provided[i]))
    {
      this->Publish(this->provided[i], resp_queue,
                    PLAYER_MSGTYPE_RESP_NACK, hdr->subtype);
      return 0;
    }
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_CMD, -1, this->provided[i]))
    {
      if ((hdr->type) != PLAYER_MSGTYPE_CMD)
      {
        PLAYER_ERROR("Internal error in command header!");
        return -1;
      }
      if ((this->wait_for_all) && (this->pstored[i])) return 0;
      if (storeData(getId, this->ptables[i], data, hdr->timestamp, this->provided[i].interf, this->provided[i].index, hdr->type, hdr->subtype))
      {
        PLAYER_ERROR("Cannot store command");
        return -1;
      }
      this->pstored[i] = !0;
      return 0;
    }
  }
  for (i = 0; i < static_cast<int>(this->num_required); i++)
  {
    if (Message::MatchMessage(hdr, PLAYER_MSGTYPE_DATA, -1, this->required[i]))
    {
      if ((hdr->type) != PLAYER_MSGTYPE_DATA)
      {
        PLAYER_ERROR("Internal error in data header!");
        return -1;
      }
      if (!data)
      {
        PLAYER_ERROR("NULL data");
        return -1;
      }
      if ((this->wait_for_all) && (this->rstored[i])) return 0;
      if (storeData(getId, this->rtables[i], data, hdr->timestamp, this->required[i].interf, this->required[i].index, hdr->type, hdr->subtype))
      {
        PLAYER_ERROR("Cannot store command");
        return -1;
      }
      this->rstored[i] = !0;
      return 0;
    }
  }
  return -1;
}

void Postlog::rollback() const
{
  PGresult * res;

  if (!(this->isConnected())) return;
  res = PQexec(this->conn, "ROLLBACK;");
  if (!res)
  {
    PLAYER_ERROR("Couldn't rollback transaction");
    return;
  }
  if (PQresultStatus(res) != PGRES_COMMAND_OK) PLAYER_ERROR1("%s", PQresultErrorMessage(res));
  PQclear(res);
}

int Postlog::nparams(char * dst, size_t dstsize, const char * param, int offset, size_t num)
{
  int i;
  char * helper1;
  char * helper2;

  helper1 = reinterpret_cast<char *>(malloc(dstsize));
  if (!helper1)
  {
    PLAYER_ERROR("Out of memory");
    return -1;
  }
  helper2 = reinterpret_cast<char *>(malloc(dstsize));
  if (!helper2)
  {
    PLAYER_ERROR("Out of memory");
    free(helper1);
    return -1;
  }
  dst[0] = '\0';
  for (i = 0; i < static_cast<int>(num); i++)
  {
    snprintf(helper1, dstsize, ", %s%d", param, i + offset);
    snprintf(helper2, dstsize, "%s%s", dst, helper1);
    snprintf(dst, dstsize, "%s", helper2);
  }
  free(helper1);
  free(helper2);
  return 0;
}

int Postlog::storeData(int getId, const char * table, const void * data, double timestamp, uint16_t interf, uint16_t index, uint8_t type, uint8_t subtype)
{
  int n;
  uint32_t u;
  char qparams1[2048];
  char qparams2[2048];
  PGresult * res;
  double t;
  const char * params[MAX_PARAMS];
  size_t num_params;
  size_t count;
  char idbuf[20];
  char indbuf[8];
  char countbuf[8];
  char utimebuf[20];
  char htimebuf[30];
  char gtimebuf[30];
  const player_position1d_cmd_vel_t * position1d_cmd_vel;
  const player_position1d_cmd_pos_t * position1d_cmd_pos;
  const player_position1d_data_t * position1d_data;
  const player_position2d_cmd_vel_t * position2d_cmd_vel;
  const player_position2d_cmd_pos_t * position2d_cmd_pos;
  const player_position2d_cmd_car_t * position2d_cmd_car;
  const player_position2d_cmd_vel_head_t * position2d_cmd_vel_head;
  const player_position2d_data_t * position2d_data;
  const player_position3d_cmd_vel * position3d_cmd_vel;
  const player_position3d_cmd_pos * position3d_cmd_pos;
  const player_position3d_data_t * position3d_data;
  const player_bumper_data_t * bumper_data;
  const player_aio_cmd_t * aio_cmd;
  const player_aio_data_t * aio_data;
  const player_dio_cmd_t * dio_cmd;
  const player_dio_data_t * dio_data;
  const player_gripper_data_t * gripper_data;
  const player_ptz_cmd_t * ptz_cmd;
  const player_ptz_data_t * ptz_data;
  const player_ranger_data_range_t * ranger_data_range;
  const player_ranger_data_rangestamped_t * ranger_data_rangestamped;
  const player_ranger_data_intns_t * ranger_data_intns;
  const player_ranger_data_intnsstamped_t * ranger_data_intnsstamped;
  const player_speech_cmd_t * speech_cmd;

  if (!(this->state)) return 0;
  if (!(this->isConnected())) return -1;
  if (!table) return -1;
  if (getId)
  {
    snprintf(this->query, QUERY_SIZE, "SELECT NEXTVAL('%s');", this->sequence);
    res = PQexec(this->conn, this->query);
    if (!res)
    {
      PLAYER_ERROR("Cannot get sequence nextval (NULL returned)");
      return -1;
    }
    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
      PLAYER_ERROR("Cannot get sequence nextval (not PGRES_TUPLES_OK)");
      PQclear(res);
      return -1;
    }
    if (PQntuples(res) != 1)
    {
      PLAYER_ERROR("Cannot get sequence nextval (wrong number of returned tuples)");
      PQclear(res);
      return -1;
    }
    if (PQbinaryTuples(res))
    {
      PLAYER_ERROR("Cannot get sequence nextval (wrong type of returned data)");
      PQclear(res);
      return -1;
    }
    this->id = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);
    if ((this->id) <= 0)
    {
      PLAYER_ERROR("Cannot get sequence nextval (value <= 0)");
      return -1;
    }
  }
  if ((this->id) <= 0)
  {
    PLAYER_ERROR("No ID given");
    return -1;
  }
  num_params = 0;
  snprintf(idbuf, sizeof idbuf, "%d", this->id);
  params[num_params++] = idbuf;
  snprintf(indbuf, sizeof indbuf, "%u", index);
  params[num_params++] = indbuf;
  snprintf(utimebuf, sizeof utimebuf, "%lld", static_cast<long long>(time(NULL)));
  params[num_params++] = utimebuf;
  snprintf(htimebuf, sizeof htimebuf, "%.7f", timestamp);
  params[num_params++] = htimebuf;
  GlobalTime->GetTimeDouble(&t);
  snprintf(gtimebuf, sizeof gtimebuf, "%.7f", t);
  params[num_params++] = gtimebuf;
  res = PQexec(this->conn, "BEGIN TRANSACTION;");
  if (!res)
  {
    PLAYER_ERROR("Cannot begin transaction");
    return -1;
  }
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    PLAYER_ERROR1("%s", PQresultErrorMessage(res));
    PQclear(res);
    return -1;
  }
  PQclear(res);
  res = NULL;
  this->query[0] = '\0';
  switch (type)
  {
  case PLAYER_MSGTYPE_CMD:
    switch (interf)
    {
    case PLAYER_POSITION1D_CODE:
      switch (subtype)
      {
      case PLAYER_POSITION1D_CMD_VEL:
        position1d_cmd_vel = reinterpret_cast<const player_position1d_cmd_vel_t *>(data);
        if (!position1d_cmd_vel)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, state, vel) VALUES ($1, $2, $3, $4, $5, $6, $7, $8);", table);
        params[num_params++] = "PLAYER_POSITION1D_CMD_VEL";
        params[num_params++] = (position1d_cmd_vel->state) ? "TRUE" : "FALSE";
        snprintf(this->buf[0], BUFSIZE, "%.7f", position1d_cmd_vel->vel);
        params[num_params++] = this->buf[0];
        if (num_params != 8)
        {
          PLAYER_ERROR("PLAYER_POSITION1D_CMD_VEL: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      case PLAYER_POSITION1D_CMD_POS:
        position1d_cmd_pos = reinterpret_cast<const player_position1d_cmd_pos_t *>(data);
        if (!position1d_cmd_pos)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, state, pos, vel) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9);", table);
        params[num_params++] = "PLAYER_POSITION1D_CMD_POS";
        params[num_params++] = (position1d_cmd_pos->state) ? "TRUE" : "FALSE";
        snprintf(this->buf[0], BUFSIZE, "%.7f", position1d_cmd_pos->pos);
        params[num_params++] = this->buf[0];
        snprintf(this->buf[1], BUFSIZE, "%.7f", position1d_cmd_pos->vel);
        params[num_params++] = this->buf[1];
        if (num_params != 9)
        {
          PLAYER_ERROR("PLAYER_POSITION1D_CMD_POS: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      default:
        PLAYER_ERROR("Unknown position1d command");
        this->rollback();
        return -1;
      }
      break;
    case PLAYER_POSITION2D_CODE:
      switch (subtype)
      {
      case PLAYER_POSITION2D_CMD_VEL:
        position2d_cmd_vel = reinterpret_cast<const player_position2d_cmd_vel_t *>(data);
        if (!position2d_cmd_vel)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, state, vx, vy, va) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10);", table);
        params[num_params++] = "PLAYER_POSITION2D_CMD_VEL";
        params[num_params++] = (position2d_cmd_vel->state) ? "TRUE" : "FALSE";
        snprintf(this->buf[0], BUFSIZE, "%.7f", position2d_cmd_vel->vel.px);
        params[num_params++] = this->buf[0];
        snprintf(this->buf[1], BUFSIZE, "%.7f", position2d_cmd_vel->vel.py);
        params[num_params++] = this->buf[1];
        snprintf(this->buf[2], BUFSIZE, "%.7f", position2d_cmd_vel->vel.pa);
        params[num_params++] = this->buf[2];
        if (num_params != 10)
        {
          PLAYER_ERROR("PLAYER_POSITION2D_CMD_VEL: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      case PLAYER_POSITION2D_CMD_POS:
        position2d_cmd_pos = reinterpret_cast<const player_position2d_cmd_pos_t *>(data);
        if (!position2d_cmd_pos)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, state, px, py, pa, vx, vy, va) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13);", table);
        params[num_params++] = "PLAYER_POSITION2D_CMD_POS";
        params[num_params++] = (position2d_cmd_pos->state) ? "TRUE" : "FALSE";
        snprintf(this->buf[0], BUFSIZE, "%.7f", position2d_cmd_pos->pos.px);
        params[num_params++] = this->buf[0];
        snprintf(this->buf[1], BUFSIZE, "%.7f", position2d_cmd_pos->pos.py);
        params[num_params++] = this->buf[1];
        snprintf(this->buf[2], BUFSIZE, "%.7f", position2d_cmd_pos->pos.pa);
        params[num_params++] = this->buf[2];
        snprintf(this->buf[3], BUFSIZE, "%.7f", position2d_cmd_pos->vel.px);
        params[num_params++] = this->buf[3];
        snprintf(this->buf[4], BUFSIZE, "%.7f", position2d_cmd_pos->vel.py);
        params[num_params++] = this->buf[4];
        snprintf(this->buf[5], BUFSIZE, "%.7f", position2d_cmd_pos->vel.pa);
        params[num_params++] = this->buf[5];
        if (num_params != 13)
        {
          PLAYER_ERROR("PLAYER_POSITION2D_CMD_POS: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      case PLAYER_POSITION2D_CMD_CAR:
        position2d_cmd_car = reinterpret_cast<const player_position2d_cmd_car_t *>(data);
        if (!position2d_cmd_car)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, velocity, angle) VALUES ($1, $2, $3, $4, $5, $6, $7, $8);", table);
        params[num_params++] = "PLAYER_POSITION2D_CMD_CAR";
        snprintf(this->buf[0], BUFSIZE, "%.7f", position2d_cmd_car->velocity);
        params[num_params++] = this->buf[0];
        snprintf(this->buf[1], BUFSIZE, "%.7f", position2d_cmd_car->angle);
        params[num_params++] = this->buf[1];
        if (num_params != 8)
        {
          PLAYER_ERROR("PLAYER_POSITION2D_CMD_CAR: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      case PLAYER_POSITION2D_CMD_VEL_HEAD:
        position2d_cmd_vel_head = reinterpret_cast<const player_position2d_cmd_vel_head_t *>(data);
        if (!position2d_cmd_vel_head)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, velocity, angle) VALUES ($1, $2, $3, $4, $5, $6, $7, $8);", table);
        params[num_params++] = "PLAYER_POSITION2D_CMD_VEL_HEAD";
        snprintf(this->buf[0], BUFSIZE, "%.7f", position2d_cmd_vel_head->velocity);
        params[num_params++] = this->buf[0];
        snprintf(this->buf[1], BUFSIZE, "%.7f", position2d_cmd_vel_head->angle);
        params[num_params++] = this->buf[1];
        if (num_params != 8)
        {
          PLAYER_ERROR("PLAYER_POSITION2D_CMD_VEL_HEAD: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      default:
        PLAYER_ERROR("Unknown position2d command");
        this->rollback();
        return -1;
      }
      break;
    case PLAYER_POSITION3D_CODE:
      switch (subtype)
      {
      case PLAYER_POSITION3D_CMD_SET_VEL:
        position3d_cmd_vel = reinterpret_cast<const player_position3d_cmd_vel_t *>(data);
        if (!position3d_cmd_vel)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, state, vx, vy, vz, vroll, vpitch, vyaw) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13);", table);
        params[num_params++] = "PLAYER_POSITION3D_CMD_SET_VEL";
        params[num_params++] = (position3d_cmd_vel->state) ? "TRUE" : "FALSE";
        snprintf(this->buf[0], BUFSIZE, "%.7f", position3d_cmd_vel->vel.px);
        params[num_params++] = this->buf[0];
        snprintf(this->buf[1], BUFSIZE, "%.7f", position3d_cmd_vel->vel.py);
        params[num_params++] = this->buf[1];
        snprintf(this->buf[2], BUFSIZE, "%.7f", position3d_cmd_vel->vel.pz);
        params[num_params++] = this->buf[2];
        snprintf(this->buf[3], BUFSIZE, "%.7f", position3d_cmd_vel->vel.proll);
        params[num_params++] = this->buf[3];
        snprintf(this->buf[4], BUFSIZE, "%.7f", position3d_cmd_vel->vel.ppitch);
        params[num_params++] = this->buf[4];
        snprintf(this->buf[5], BUFSIZE, "%.7f", position3d_cmd_vel->vel.pyaw);
        params[num_params++] = this->buf[5];
        if (num_params != 13)
        {
          PLAYER_ERROR("PLAYER_POSITION3D_CMD_SET_VEL: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      case PLAYER_POSITION3D_CMD_SET_POS:
        position3d_cmd_pos = reinterpret_cast<const player_position3d_cmd_pos_t *>(data);
        if (!position3d_cmd_pos)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, state, px, py, pz, proll ppitch, pyaw, vx, vz, va, vroll, vpitch, vyaw) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18, $19);", table);
        params[num_params++] = "PLAYER_POSITION3D_CMD_SET_POS";
        params[num_params++] = (position3d_cmd_pos->state) ? "TRUE" : "FALSE";
        snprintf(this->buf[0], BUFSIZE, "%.7f", position3d_cmd_pos->pos.px);
        params[num_params++] = this->buf[0];
        snprintf(this->buf[1], BUFSIZE, "%.7f", position3d_cmd_pos->pos.py);
        params[num_params++] = this->buf[1];
        snprintf(this->buf[2], BUFSIZE, "%.7f", position3d_cmd_pos->pos.pz);
        params[num_params++] = this->buf[2];
        snprintf(this->buf[3], BUFSIZE, "%.7f", position3d_cmd_pos->pos.proll);
        params[num_params++] = this->buf[3];
        snprintf(this->buf[4], BUFSIZE, "%.7f", position3d_cmd_pos->pos.ppitch);
        params[num_params++] = this->buf[4];
        snprintf(this->buf[5], BUFSIZE, "%.7f", position3d_cmd_pos->pos.pyaw);
        params[num_params++] = this->buf[5];
        snprintf(this->buf[6], BUFSIZE, "%.7f", position3d_cmd_pos->vel.px);
        params[num_params++] = this->buf[6];
        snprintf(this->buf[7], BUFSIZE, "%.7f", position3d_cmd_pos->vel.py);
        params[num_params++] = this->buf[7];
        snprintf(this->buf[8], BUFSIZE, "%.7f", position3d_cmd_pos->vel.pz);
        params[num_params++] = this->buf[8];
        snprintf(this->buf[9], BUFSIZE, "%.7f", position3d_cmd_pos->vel.proll);
        params[num_params++] = this->buf[9];
        snprintf(this->buf[10], BUFSIZE, "%.7f", position3d_cmd_pos->vel.ppitch);
        params[num_params++] = this->buf[10];
        snprintf(this->buf[11], BUFSIZE, "%.7f", position3d_cmd_pos->vel.pyaw);
        params[num_params++] = this->buf[11];
        if (num_params != 19)
        {
          PLAYER_ERROR("PLAYER_POSITION3D_CMD_SET_POS: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      default:
        PLAYER_ERROR("Unknown position3d command");
        this->rollback();
        return -1;
      }
      break;
    case PLAYER_AIO_CODE:
      switch (subtype)
      {
      case PLAYER_AIO_CMD_STATE:
        aio_cmd = reinterpret_cast<const player_aio_cmd_t *>(data);
        if (!aio_cmd)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, io_id, voltage) VALUES ($1, $2, $3, $4, $5, $6, $7, $8);", table);
        params[num_params++] = "PLAYER_AIO_CMD_STATE";
        snprintf(this->buf[0], BUFSIZE, "%u", aio_cmd->id);
        params[num_params++] = this->buf[0];
        snprintf(this->buf[1], BUFSIZE, "%.7f", aio_cmd->voltage);
        params[num_params++] = this->buf[1];
        if (num_params != 8)
        {
          PLAYER_ERROR("PLAYER_AIO_CMD_STATE: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      default:
        PLAYER_ERROR("Unknown aio command");
        this->rollback();
        return -1;
      }
      break;
    case PLAYER_DIO_CODE:
      switch (subtype)
      {
      case PLAYER_DIO_CMD_VALUES:
        dio_cmd = reinterpret_cast<const player_dio_cmd_t *>(data);
        if (!dio_cmd)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, count, d31, d30, d29, d28, d27, d26, d25, d24, d23, d22, d21, d20, d19, d18, d17, d16, d15, d14, d13, d12, d11, d10, d9, d8, d7, d6, d5, d4, d3, d2, d1, d0) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18, $19, $20, $21, $22, $23, $24, $25, $26, $27, $28, $29, $30, $31, $32, $33, $34, $35, $36, $37, $38, $39);", table);
        params[num_params++] = "PLAYER_DIO_CMD_VALUES";
        snprintf(countbuf, sizeof countbuf, "%u", dio_cmd->count);
        params[num_params++] = countbuf;
        u = dio_cmd->digout;
        for (n = 31; n >= 0; n--)
        {
          snprintf(this->buf[n], BUFSIZE, "%u", u & 1);
          u >>= 1;
        }
        for (n = 0; n < 32; n++) params[num_params++] = this->buf[n];
        if (num_params != 39)
        {
          PLAYER_ERROR("PLAYER_DIO_CMD_VALUES: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      default:
        PLAYER_ERROR("Unknown dio command");
        this->rollback();
        return -1;
      }
      break;
    case PLAYER_GRIPPER_CODE:
      snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype) VALUES ($1, $2, $3, $4, $5, $6);", table);
      switch (subtype)
      {
      case PLAYER_GRIPPER_CMD_OPEN:
        params[num_params++] = "PLAYER_GRIPPER_CMD_OPEN";
        break;
      case PLAYER_GRIPPER_CMD_CLOSE:
        params[num_params++] = "PLAYER_GRIPPER_CMD_CLOSE";
        break;
      case PLAYER_GRIPPER_CMD_STOP:
        params[num_params++] = "PLAYER_GRIPPER_CMD_STOP";
        break;
      case PLAYER_GRIPPER_CMD_STORE:
        params[num_params++] = "PLAYER_GRIPPER_CMD_STORE";
        break;
      case PLAYER_GRIPPER_CMD_RETRIEVE:
        params[num_params++] = "PLAYER_GRIPPER_CMD_RETRIEVE";
        break;
      default:
        PLAYER_ERROR("Unknown gripper command");
        this->rollback();
        return -1;
      }
      if (num_params != 6)
      {
        PLAYER_ERROR("PLAYER_GRIPPER_CMD_*: Internal error: invalid number of params");
        this->rollback();
        return -1;
      }
      break;
    case PLAYER_PTZ_CODE:
      switch (subtype)
      {
      case PLAYER_PTZ_CMD_STATE:
        ptz_cmd = reinterpret_cast<const player_ptz_cmd_t *>(data);
        if (!ptz_cmd)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, pan, tilt, zoom, panspeed, tiltspeed) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11);", table);
        params[num_params++] = "PLAYER_PTZ_CMD_STATE";
        snprintf(this->buf[0], BUFSIZE, "%.7f", ptz_cmd->pan);
        params[num_params++] = this->buf[0];
        snprintf(this->buf[1], BUFSIZE, "%.7f", ptz_cmd->tilt);
        params[num_params++] = this->buf[1];
        snprintf(this->buf[2], BUFSIZE, "%.7f", ptz_cmd->zoom);
        params[num_params++] = this->buf[2];
        snprintf(this->buf[3], BUFSIZE, "%.7f", ptz_cmd->panspeed);
        params[num_params++] = this->buf[3];
        snprintf(this->buf[4], BUFSIZE, "%.7f", ptz_cmd->tiltspeed);
        params[num_params++] = this->buf[4];
        if (num_params != 11)
        {
          PLAYER_ERROR("PLAYER_PTZ_CMD_STATE: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      default:
        PLAYER_ERROR("Unknown ptz command");
        this->rollback();
        return -1;
      }
      break;
    case PLAYER_SPEECH_CODE:
      switch (subtype)
      {
      case PLAYER_SPEECH_CMD_SAY:
        speech_cmd = reinterpret_cast<const player_speech_cmd_t *>(data);
        if (!speech_cmd)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, string_count, phrase) VALUES ($1, $2, $3, $4, $5, $6, $7, $8);", table);
        params[num_params++] = "PLAYER_SPEECH_CMD_SAY";
        snprintf(countbuf, sizeof countbuf, "%u", speech_cmd->string_count);
        params[num_params++] = countbuf;
        snprintf(this->buf[0], BUFSIZE, "%s", speech_cmd->string);
        params[num_params++] = this->buf[0];
        if (num_params != 8)
        {
          PLAYER_ERROR("PLAYER_SPEECH_CMD_SAY: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      default:
        PLAYER_ERROR("Unknown speech command");
        this->rollback();
        return -1;
      }
      break;
    default:
      PLAYER_ERROR("Command for unknown interface");
      this->rollback();
      return -1;
    }
    break;
  case PLAYER_MSGTYPE_DATA:
    switch (interf)
    {
    case PLAYER_POSITION1D_CODE:
      switch (subtype)
      {
      case PLAYER_POSITION1D_DATA_STATE:
        position1d_data = reinterpret_cast<const player_position1d_data_t *>(data);
        if (!position1d_data)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, pos, vel, stall, status) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10);", table);
        params[num_params++] = "PLAYER_POSITION1D_DATA_STATE";
        snprintf(this->buf[0], BUFSIZE, "%.7f", position1d_data->pos);
        params[num_params++] = this->buf[0];
        snprintf(this->buf[1], BUFSIZE, "%.7f", position1d_data->vel);
        params[num_params++] = this->buf[1];
        params[num_params++] = (position1d_data->stall) ? "TRUE" : "FALSE";
        snprintf(this->buf[2], BUFSIZE, "%d", static_cast<signed>(position1d_data->status));
        params[num_params++] = this->buf[2];
        if (num_params != 10)
        {
          PLAYER_ERROR("PLAYER_POSITION1D_DATA_STATE: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      default:
        PLAYER_ERROR("Unknown position1d data");
        this->rollback();
        return -1;
      }
      break;
    case PLAYER_POSITION2D_CODE:
      switch (subtype)
      {
      case PLAYER_POSITION2D_DATA_STATE:
        position2d_data = reinterpret_cast<const player_position2d_data_t *>(data);
        if (!position2d_data)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, px, py, pa, vx, vy, va, stall) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13);", table);
        params[num_params++] = "PLAYER_POSITION2D_DATA_STATE";
        snprintf(this->buf[0], BUFSIZE, "%.7f", position2d_data->pos.px);
        params[num_params++] = this->buf[0];
        snprintf(this->buf[1], BUFSIZE, "%.7f", position2d_data->pos.py);
        params[num_params++] = this->buf[1];
        snprintf(this->buf[2], BUFSIZE, "%.7f", position2d_data->pos.pa);
        params[num_params++] = this->buf[2];
        snprintf(this->buf[3], BUFSIZE, "%.7f", position2d_data->vel.px);
        params[num_params++] = this->buf[3];
        snprintf(this->buf[4], BUFSIZE, "%.7f", position2d_data->vel.py);
        params[num_params++] = this->buf[4];
        snprintf(this->buf[5], BUFSIZE, "%.7f", position2d_data->vel.pa);
        params[num_params++] = this->buf[5];
        params[num_params++] = (position2d_data->stall) ? "TRUE" : "FALSE";
        if (num_params != 13)
        {
          PLAYER_ERROR("PLAYER_POSITION2D_DATA_STATE: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      default:
        PLAYER_ERROR("Unknown position2d data");
        this->rollback();
        return -1;
      }
      break;
    case PLAYER_POSITION3D_CODE:
      switch (subtype)
      {
      case PLAYER_POSITION3D_DATA_STATE:
        position3d_data = reinterpret_cast<const player_position3d_data_t *>(data);
        if (!position3d_data)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, px, py, pz, proll, ppitch, pyaw, vx, vy, vz, vroll, vpitch, vyaw, stall) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18, $19);", table);
        params[num_params++] = "PLAYER_POSITION3D_DATA_STATE";
        snprintf(this->buf[0], BUFSIZE, "%.7f", position3d_data->pos.px);
        params[num_params++] = this->buf[0];
        snprintf(this->buf[1], BUFSIZE, "%.7f", position3d_data->pos.py);
        params[num_params++] = this->buf[1];
        snprintf(this->buf[2], BUFSIZE, "%.7f", position3d_data->pos.pz);
        params[num_params++] = this->buf[2];
        snprintf(this->buf[3], BUFSIZE, "%.7f", position3d_data->pos.proll);
        params[num_params++] = this->buf[3];
        snprintf(this->buf[4], BUFSIZE, "%.7f", position3d_data->pos.ppitch);
        params[num_params++] = this->buf[4];
        snprintf(this->buf[5], BUFSIZE, "%.7f", position3d_data->pos.pyaw);
        params[num_params++] = this->buf[5];
        snprintf(this->buf[6], BUFSIZE, "%.7f", position3d_data->vel.px);
        params[num_params++] = this->buf[6];
        snprintf(this->buf[7], BUFSIZE, "%.7f", position3d_data->vel.py);
        params[num_params++] = this->buf[7];
        snprintf(this->buf[8], BUFSIZE, "%.7f", position3d_data->vel.pz);
        params[num_params++] = this->buf[8];
        snprintf(this->buf[9], BUFSIZE, "%.7f", position3d_data->vel.proll);
        params[num_params++] = this->buf[9];
        snprintf(this->buf[10], BUFSIZE, "%.7f", position3d_data->vel.ppitch);
        params[num_params++] = this->buf[10];
        snprintf(this->buf[11], BUFSIZE, "%.7f", position3d_data->vel.pyaw);
        params[num_params++] = this->buf[11];
        params[num_params++] = (position3d_data->stall) ? "TRUE" : "FALSE";
        if (num_params != 19)
        {
          PLAYER_ERROR("PLAYER_POSITION3D_DATA_STATE: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      default:
        PLAYER_ERROR("Unknown position3d data");
        this->rollback();
        return -1;
      }
      break;
    case PLAYER_AIO_CODE:
      switch (subtype)
      {
      case PLAYER_AIO_DATA_STATE:
        aio_data = reinterpret_cast<const player_aio_data_t *>(data);
        if (!aio_data)
        {
          this->rollback();
          return -1;
        }
        count = aio_data->voltages_count;
        if ((count + 7) > MAX_PARAMS)
        {
          PLAYER_ERROR("Too many aio readings");
          this->rollback();
          return -1;
        }
        if (Postlog::nparams(qparams1, sizeof qparams1, "v", 0, count))
        {
          this->rollback();
          return -1;
        }
        if (Postlog::nparams(qparams2, sizeof qparams2, "$", 8, count))
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, count%s) VALUES ($1, $2, $3, $4, $5, $6, $7%s);", table, qparams1, qparams2);
        params[num_params++] = "PLAYER_AIO_DATA_STATE";
        snprintf(countbuf, sizeof countbuf, "%zu", count);
        params[num_params++] = countbuf;
        for (n = 0; n < static_cast<int>(count); n++)
        {
          snprintf(this->buf[n], BUFSIZE, "%.7f", aio_data->voltages[n]);
          params[num_params++] = this->buf[n];
        }
        if (num_params != static_cast<size_t>(count + 7))
        {
          PLAYER_ERROR("PLAYER_AIO_DATA_STATE: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      default:
        PLAYER_ERROR("Unknown aio data");
        this->rollback();
        return -1;
      }
      break;
    case PLAYER_BUMPER_CODE:
      switch (subtype)
      {
      case PLAYER_BUMPER_DATA_STATE:
        bumper_data = reinterpret_cast<const player_bumper_data_t *>(data);
        if (!bumper_data)
        {
          this->rollback();
          return -1;
        }
        count = bumper_data->bumpers_count;
        if ((count + 7) > MAX_PARAMS)
        {
          PLAYER_ERROR("Too many bumper readings");
          this->rollback();
          return -1;
        }
        if (Postlog::nparams(qparams1, sizeof qparams1, "bumper", 0, count))
        {
          this->rollback();
          return -1;
        }
        if (Postlog::nparams(qparams2, sizeof qparams2, "$", 8, count))
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, count%s) VALUES ($1, $2, $3, $4, $5, $6, $7%s);", table, qparams1, qparams2);
        params[num_params++] = "PLAYER_BUMPER_DATA_STATE";
        snprintf(countbuf, sizeof countbuf, "%zu", count);
        params[num_params++] = countbuf;
        for (n = 0; n < static_cast<int>(count); n++)
        {
          params[num_params++] = (bumper_data->bumpers[n]) ? "TRUE" : "FALSE";
        }
        if (num_params != static_cast<size_t>(count + 7))
        {
          PLAYER_ERROR("PLAYER_BUMPER_DATA_STATE: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      default:
        PLAYER_ERROR("Unknown bumper data");
        this->rollback();
        return -1;
      }
      break;
    case PLAYER_DIO_CODE:
      switch (subtype)
      {
      case PLAYER_DIO_DATA_VALUES:
        dio_data = reinterpret_cast<const player_dio_data_t *>(data);
        if (!dio_data)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, count, d31, d30, d29, d28, d27, d26, d25, d24, d23, d22, d21, d20, d19, d18, d17, d16, d15, d14, d13, d12, d11, d10, d9, d8, d7, d6, d5, d4, d3, d2, d1, d0) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12, $13, $14, $15, $16, $17, $18, $19, $20, $21, $22, $23, $24, $25, $26, $27, $28, $29, $30, $31, $32, $33, $34, $35, $36, $37, $38, $39);", table);
        params[num_params++] = "PLAYER_DIO_DATA_VALUES";
        snprintf(countbuf, sizeof countbuf, "%u", dio_data->count);
        params[num_params++] = countbuf;
        u = dio_data->bits;
        for (n = 31; n >= 0; n--)
        {
          snprintf(this->buf[n], BUFSIZE, "%u", u & 1);
          u >>= 1;
        }
        for (n = 0; n < 32; n++) params[num_params++] = this->buf[n];
        if (num_params != 39)
        {
          PLAYER_ERROR("PLAYER_DIO_DATA_VALUES: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      default:
        PLAYER_ERROR("Unknown dio data");
        this->rollback();
        return -1;
      }
      break;
    case PLAYER_GRIPPER_CODE:
      switch (subtype)
      {
      case PLAYER_GRIPPER_DATA_STATE:
        gripper_data = reinterpret_cast<const player_gripper_data_t *>(data);
        if (!gripper_data)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, state, beams, stored) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9);", table);
        params[num_params++] = "PLAYER_GRIPPER_DATA_STATE";
        snprintf(this->buf[0], BUFSIZE, "%u", gripper_data->state);
        params[num_params++] = this->buf[0];
        snprintf(this->buf[1], BUFSIZE, "%u", gripper_data->beams);
        params[num_params++] = this->buf[1];
        snprintf(this->buf[2], BUFSIZE, "%u", gripper_data->stored);
        params[num_params++] = this->buf[2];
        if (num_params != 9)
        {
          PLAYER_ERROR("PLAYER_GRIPPER_DATA_STATE: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      default:
        PLAYER_ERROR("Unknown gripper data");
        this->rollback();
        return -1;
      }
      break;
    case PLAYER_PTZ_CODE:
      switch (subtype)
      {
      case PLAYER_PTZ_DATA_STATE:
        ptz_data = reinterpret_cast<const player_ptz_data_t *>(data);
        if (!ptz_data)
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, pan, tilt, zoom, panspeed, tiltspeed, status) VALUES ($1, $2, $3, $4, $5, $6, $7, $8, $9, $10, $11, $12);", table);
        params[num_params++] = "PLAYER_PTZ_DATA_STATE";
        snprintf(this->buf[0], BUFSIZE, "%.7f", ptz_data->pan);
        params[num_params++] = this->buf[0];
        snprintf(this->buf[1], BUFSIZE, "%.7f", ptz_data->tilt);
        params[num_params++] = this->buf[1];
        snprintf(this->buf[2], BUFSIZE, "%.7f", ptz_data->zoom);
        params[num_params++] = this->buf[2];
        snprintf(this->buf[3], BUFSIZE, "%.7f", ptz_data->panspeed);
        params[num_params++] = this->buf[3];
        snprintf(this->buf[4], BUFSIZE, "%.7f", ptz_data->tiltspeed);
        params[num_params++] = this->buf[4];
        snprintf(this->buf[5], BUFSIZE, "%d", static_cast<signed>(ptz_data->status));
        params[num_params++] = this->buf[5];
        if (num_params != 12)
        {
          PLAYER_ERROR("PLAYER_PTZ_DATA_STATE: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      default:
        PLAYER_ERROR("Unknown ptz data");
        this->rollback();
        return -1;
      }
      break;
    case PLAYER_RANGER_CODE:
      switch (subtype)
      {
      case PLAYER_RANGER_DATA_RANGE:
        ranger_data_range = reinterpret_cast<const player_ranger_data_range_t *>(data);
        if (!ranger_data_range)
        {
          this->rollback();
          return -1;
        }
        count = ranger_data_range->ranges_count;
        if ((count + 7) > MAX_PARAMS)
        {
          PLAYER_ERROR("Too many ranger readings");
          this->rollback();
          return -1;
        }
        if (Postlog::nparams(qparams1, sizeof qparams1, "r", 0, count))
        {
          this->rollback();
          return -1;
        }
        if (Postlog::nparams(qparams2, sizeof qparams2, "$", 8, count))
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, count%s) VALUES ($1, $2, $3, $4, $5, $6, $7%s);", table, qparams1, qparams2);
        params[num_params++] = "PLAYER_RANGER_DATA_RANGE";
        snprintf(countbuf, sizeof countbuf, "%zu", count);
        params[num_params++] = countbuf;
        for (n = 0; n < static_cast<int>(count); n++)
        {
          snprintf(this->buf[n], BUFSIZE, "%.7f", ranger_data_range->ranges[n]);
          params[num_params++] = this->buf[n];
        }
        if (num_params != static_cast<size_t>(count + 7))
        {
          PLAYER_ERROR("PLAYER_RANGER_DATA_RANGE: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      case PLAYER_RANGER_DATA_RANGESTAMPED:
        ranger_data_rangestamped = reinterpret_cast<const player_ranger_data_rangestamped_t *>(data);
        if (!ranger_data_rangestamped)
        {
          this->rollback();
          return -1;
        }
        count = ranger_data_rangestamped->data.ranges_count;
        if ((count + 7) > MAX_PARAMS)
        {
          PLAYER_ERROR("Too many ranger readings");
          this->rollback();
          return -1;
        }
        if (Postlog::nparams(qparams1, sizeof qparams1, "r", 0, count))
        {
          this->rollback();
          return -1;
        }
        if (Postlog::nparams(qparams2, sizeof qparams2, "$", 8, count))
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, count%s) VALUES ($1, $2, $3, $4, $5, $6, $7%s);", table, qparams1, qparams2);
        params[num_params++] = "PLAYER_RANGER_DATA_RANGESTAMPED";
        snprintf(countbuf, sizeof countbuf, "%zu", count);
        params[num_params++] = countbuf;
        for (n = 0; n < static_cast<int>(count); n++)
        {
          snprintf(this->buf[n], BUFSIZE, "%.7f", ranger_data_rangestamped->data.ranges[n]);
          params[num_params++] = this->buf[n];
        }
        if (num_params != static_cast<size_t>(count + 7))
        {
          PLAYER_ERROR("PLAYER_RANGER_DATA_RANGESTAMPED: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      case PLAYER_RANGER_DATA_INTNS:
        ranger_data_intns = reinterpret_cast<const player_ranger_data_intns_t *>(data);
        if (!ranger_data_intns)
        {
          this->rollback();
          return -1;
        }
        count = ranger_data_intns->intensities_count;
        if ((count + 7) > MAX_PARAMS)
        {
          PLAYER_ERROR("Too many intensities");
          this->rollback();
          return -1;
        }
        if (Postlog::nparams(qparams1, sizeof qparams1, "r", 0, count))
        {
          this->rollback();
          return -1;
        }
        if (Postlog::nparams(qparams2, sizeof qparams2, "$", 8, count))
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, count%s) VALUES ($1, $2, $3, $4, $5, $6, $7%s);", table, qparams1, qparams2);
        params[num_params++] = "PLAYER_RANGER_DATA_INTNS";
        snprintf(countbuf, sizeof countbuf, "%zu", count);
        params[num_params++] = countbuf;
        for (n = 0; n < static_cast<int>(count); n++)
        {
          snprintf(this->buf[n], BUFSIZE, "%.7f", ranger_data_intns->intensities[n]);
          params[num_params++] = this->buf[n];
        }
        if (num_params != static_cast<size_t>(count + 7))
        {
          PLAYER_ERROR("PLAYER_RANGER_DATA_INTNS: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      case PLAYER_RANGER_DATA_INTNSSTAMPED:
        ranger_data_intnsstamped = reinterpret_cast<const player_ranger_data_intnsstamped_t *>(data);
        if (!ranger_data_intnsstamped)
        {
          this->rollback();
          return -1;
        }
        count = ranger_data_intnsstamped->data.intensities_count;
        if ((count + 7) > MAX_PARAMS)
        {
          PLAYER_ERROR("Too many intensities");
          this->rollback();
          return -1;
        }
        if (Postlog::nparams(qparams1, sizeof qparams1, "r", 0, count))
        {
          this->rollback();
          return -1;
        }
        if (Postlog::nparams(qparams2, sizeof qparams2, "$", 8, count))
        {
          this->rollback();
          return -1;
        }
        snprintf(this->query, QUERY_SIZE, "INSERT INTO \"%s\" (id, index, unixtime, hdrtime, globaltime, subtype, count%s) VALUES ($1, $2, $3, $4, $5, $6, $7%s);", table, qparams1, qparams2);
        params[num_params++] = "PLAYER_RANGER_DATA_INTNSSTAMPED";
        snprintf(countbuf, sizeof countbuf, "%zu", count);
        params[num_params++] = countbuf;
        for (n = 0; n < static_cast<int>(count); n++)
        {
          snprintf(this->buf[n], BUFSIZE, "%.7f", ranger_data_intnsstamped->data.intensities[n]);
          params[num_params++] = this->buf[n];
        }
        if (num_params != static_cast<size_t>(count + 7))
        {
          PLAYER_ERROR("PLAYER_RANGER_DATA_INTNSSTAMPED: Internal error: invalid number of params");
          this->rollback();
          return -1;
        }
        break;
      default:
        PLAYER_ERROR("Unknown ranger data");
        this->rollback();
        return -1;
      }
      break;
    default:
      PLAYER_ERROR("Data from unknown interface");
      this->rollback();
      return -1;
    }
    break;
  default:
    PLAYER_ERROR("Unknown message type");
    this->rollback();
    return -1;
  }
  if (num_params < 6)
  {
    PLAYER_ERROR("Internal error: invalid number of params");
    this->rollback();
    return -1;
  }
  if (!(strlen(this->query) > 0))
  {
    PLAYER_ERROR("Internal error: empty query");
    this->rollback();
    return -1;
  }
  res = PQexecParams(this->conn, this->query, num_params, NULL, params, NULL, NULL, 0);
  if (!res)
  {
    PLAYER_ERROR("Couldn't insert new command to the database");
    this->rollback();
    return -1;
  }
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    PLAYER_ERROR1("%s", PQresultErrorMessage(res));
    PQclear(res);
    this->rollback();
    return -1;
  }
  PQclear(res);
  res = PQexec(this->conn, "COMMIT TRANSACTION;");
  if (!res)
  {
    PLAYER_ERROR("Couldn't commit transaction");
    this->rollback();
    return -1;
  }
  if (PQresultStatus(res) != PGRES_COMMAND_OK)
  {
    PLAYER_ERROR1("%s", PQresultErrorMessage(res));
    PQclear(res);
    return -1;
  }
  PQclear(res);
  res = NULL;
  return 0;
}
