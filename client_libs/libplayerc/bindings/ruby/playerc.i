
%module playerc

%{
#include "playerc_wrap.h"

%}

%include "typemaps.i"


// For playerc_simulation_get_pose2d()
%apply double *OUTPUT { double *x, double *y, double *a };


/******************************************

     C to RUBY  (Ruby reads)
 $1 C types
 $result Ruby types

******************************************/



// C to RUBY (ruby reads) basic types
//TODO: the unsigned long long stuff is correct?
%typemap(out) uint8_t
{
  $result = UINT2NUM((unsigned char) $1);
}

%typemap(out) uint16_t
{
  $result = UINT2NUM((unsigned long) $1);
}

%typemap(out) uint32_t
{
  $result = UINT2NUM((long) (unsigned long long) $1);
}





// Provide array access
%typemap(out) double [ANY]
{
  VALUE ary;
  int i;
  ary =rb_ary_new2($1_dim0);
  for (i = 0; i < $1_dim0; i++)
  {
    rb_ary_push(ary, rb_float_new((double)$1[i])); 
  }
  $result = ary;
}

%typemap(out) float [ANY]
{
  VALUE ary;
  int i;
  ary =rb_ary_new2($1_dim0);
  for (i = 0; i < $1_dim0; i++)
  {
    rb_ary_push(ary, rb_float_new((float)$1[i])); 
  }
  $result = ary;
}



%typemap(out) uint8_t [ANY]
{
  VALUE ary;
  int i;
  ary =rb_ary_new2($1_dim0);
  for (i = 0; i < $1_dim0; i++)
  {
    rb_ary_push(ary, INT2FIX($1[i]));
  }
  $result = ary;
}


// Provide array access doubly-dimensioned arrays
%typemap(out) double [ANY] [ANY]
{
    VALUE ary;
    int i,j;
    ary = rb_ary_new2($1_dim0);
    for (i = 0; i < $1_dim0; i++)
    {
        inner_ary =  rb_ary_new2($1_dim0);
        for (j = 0; j < $1_dim1; j++)
        {
            rb_ary_push(inner_ary, rb_float_new((double)$1[i][j])); 
        }
        rb_ary_push(ary, inner_ary);
    }
    $result = ary;
}



// Other stuff access

/* playerc_rfidtag_t is an array of:
typedef struct
{
    uint32_t type;
    uint32_t guid_count;
    uint8_t *guid;
}  playerc_rfidtag_t;

TODO: surely this is not the best interface to this data ever thought ...
// Provide array access to the RFID tags
// We will return an array of hashes. Each hash contains the RFID type, and the RFID ID.
//  The RFID ID is a string containing the unprocessed bytes of the RFID tag.
//  rfid.tags[i]["guid"].encode('hex') ?  to see the regular Hex-ASCII representation.
*/
%typemap(out) playerc_rfidtag_t [ANY]
{
  int i;
  $result = rb_ary_new2($1_dim0);  //we will return an Array

  //At this level, we dont get the ammount of tags, 
  //so we just produce a long list, of size PLAYER_RFID_MAX_TAGS
  //(this is the size of the array of playerc_rfidtag_t contained in the RFID data packet)
  for (i = 0; i != $1_dim0; ++i)
  {
    VALUE hash = rb_hash_new();
    
    const unsigned int blength=PLAYERC_RFID_MAX_GUID;
    char buffer[blength];
    memset(buffer, 0, blength );

    //result is of type playerc_rfidtag_t
    unsigned int j;    
    unsigned int guid_count=$1[i].guid_count;

    //copy the bytes into the buffer
    for (j=0 ; j != guid_count ; ++j) {
	buffer[j]=$1[i].guid[j];
    }

    //generate a string from the buffer
    VALUE guid = rb_str_new(buffer, guid_count);
    //generate an Int from the tag type
    VALUE type = UINT2NUM((long) (unsigned long long) $1[i].type);

    //set the previous objects into the tuple
    rb_hash_aset(hash, "type", type);
    rb_hash_aset(hash, "guid", guid);

    rb_ary_push ($result, hash);
  }
  //$result is the array that gets returned automatically at the end of this function
}





// Catch-all rule to converts arrays of structures to arrays of proxies for
// the underlying structs 

%typemap(out) SWIGTYPE [ANY]
{
  VALUE ary;
  VALUE obj;
  int i;
  ary =rb_ary_new2($1_dim0);
  for (i = 0; i < $1_dim0; i++)
  {
    obj = SWIG_NewPointerObj($1 + i, $1_descriptor, 0);
    rb_ary_push(ary, obj);
  }
  $result = ary;
}


/******************************************

     RUBY to C (Ruby writes)
 $input Ruby types
 $1 C types

******************************************/



%typemap(in) uint8_t
{
  $1 = NUM2CHR($input);
}


%typemap(in) uint16_t
{
  
  $1 = (unsigned short) NUM2UINT($input);
}


%typemap(in) uint32_t
{
  
  $1 = NUM2UINT($input);
}





// Provide array (write) access
%typemap(in) double [ANY] (double temp[$1_dim0])
{
  //TODO check if the input is a correct Ruby array
  //TODO: NUM2DBL and friends can return errors?
  int i;
  if (RARRAY_LEN($input) != $1_dim0) 
  {
    SWIG_exception( SWIG_TypeError, "Size mismatch on the array" );
    return 0;
  }

  for (i = 0; i < $1_dim0; i++)
  {
    temp[i] = (double) NUM2DBL( rb_ary_entry($input, i) );
  }
  $1 = temp;
}


%typemap(in) float [ANY] (float temp[$1_dim0])
{
  //TODO check if the input is a correct Ruby array
  int i;
  if (RARRAY_LEN($input) != $1_dim0) 
  {
    SWIG_exception( SWIG_TypeError, "Size mismatch on the array" );
    return 0;
  }

  for (i = 0; i < $1_dim0; i++)
  {
    temp[i] = (float) NUM2DBL( rb_ary_entry($input, i) );
  }
  $1 = temp;
}



%typemap(in) uint8_t data[]
{
//TODO check if the input is a correct Ruby array
  int i;
  int size;
  
  size = RARRAY_LEN($input);
  $1 = (uint8_t*) malloc (size * sizeof (uint8_t));
  
  for (i = 0; i < size; i++)
  {
    $1[i] = (uint8_t) NUM2CHR( rb_ary_entry($input, i) );
  }
}



/*// typemap to free the array created in the previous typemap
%typemap(freearg) uint8_t data[]
{
  if ($input) free ((uint8_t*) $input);
}*/

/*
// typemap for passing points into the graphics2d interface
// in C is an array of structures, where each structure has two doubles: px and py
// in Ruby an array of arrays is OK?
%typemap(in) player_point_2d_t pts[]
{
//TODO: check we have a double array
//check it has doubles
  int i;
  int size;
  VALUE *structure;
  
  size = RARRAY_LEN($input);

  $1 = (player_point_2d_t*) malloc (size * sizeof (player_point_2d_t));
  for (i = 0; i < size; i++)
  {
    structure = rb_ary_entry($input, i);
    $1[i][0] = (double) NUM2DBL( rb_ary_entry(structure, 0) );
    $1[i][1] = (double) NUM2DBL( rb_ary_entry(structure, 1) );
  }

}

*/




// Include Player header so we can pick up some constants and generate
// wrapper code for structs
%include "libplayercore/player.h"
%include "libplayercore/player_interfaces.h"

//TODO: service discovering, needs wrapping
//%include "libplayersd/playersd.h"



// Use this for object-oriented bindings;
// e.g., client.connect(...)
// This file is created by running ../parse_header.py
%include "playerc_wrap.i"

