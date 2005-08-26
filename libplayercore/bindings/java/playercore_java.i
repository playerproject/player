%module playercore_java

%include "typemaps.i"
%include "arrays_java.i"

// The stuff between the braces gets copied verbatim into the output
%{
#include <libplayercore/playercore.h>
#include <server/libplayerdrivers/driverregistry.h>
%}

// Tell SWIG what some standard types really are
typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long int64_t;
typedef unsigned long uint64_t;

// Include the auto-generated macros that allow the use of arrays of 
// non-primitive types (i.e., message structures)
%include playercore_arraysofclasses.i

%include <libplayercore/configfile.h>
%include <libplayercore/device.h>
%include <libplayercore/devicetable.h>
%include <libplayercore/driver.h>
%include <libplayercore/drivertable.h>
%include <libplayercore/error.h>
%include <libplayercore/globals.h>
%include <libplayercore/message.h>
%include <libplayercore/player.h>
%include <libplayercore/playercommon.h>
%include <libplayercore/playertime.h>

%include <server/libplayerdrivers/driverregistry.h>

// Include the auto-generated functions that cast between void* buffers and 
// pointers to message structures
%include playercore_casts.i

