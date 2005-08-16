%module playerxdr_java

%include "arrays_java.i"

// The stuff between the braces gets copied verbatim into the output
%{
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <libplayercore/player.h>
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

%apply jbyteArray { void* buf }
%typemap(in) void *buf {
  $1 = (void*)((*jenv)->GetByteArrayElements(jenv, $input, NULL));
}
%typemap(freearg) void *buf {
  (*jenv)->ReleaseByteArrayElements(jenv, $input, $1, 0);
}

// Process player.h to make wrappers for all the message structs defined
// therein
%include <libplayercore/player.h>

// Process playerxdr.h to make wrappers for all the wrapper functions
// defined therein
%include <libplayerxdr/playerxdr.h>
