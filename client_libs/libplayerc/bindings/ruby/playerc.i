/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2004
 *     Andrew Howard
 *  Copyright (C) 2009 - Ruby Modifications
 *     Jordi Polo
 *                      
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
 */
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

//used by fiducial id (signed as -1 can be used)
%typemap(out) int32_t
{
  $result = INT2NUM((long) (long long) $1);
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

    //set the previous objects into the hash
    rb_hash_aset(hash, "type", type);
    rb_hash_aset(hash, "guid", guid);

    rb_ary_push ($result, hash);
  }
  //$result is the array that gets returned automatically at the end of this function
}


// Provide array access doubly-dimensioned arrays
%typemap(out) player_point_2d_t [ANY] 
{
    VALUE ary;
    int i,j;
    ary = rb_ary_new2($1_dim0);

    for (i = 0; i < $1_dim0; i++)
    {
        inner_ary =  rb_ary_new2(2);
        rb_ary_push(inner_ary, $1[i].px);
        rb_ary_push(inner_ary, $1[i].py);
        rb_ary_push(ary, inner_ary);
    }
    $result = ary;
}



// Provide array access doubly-dimensioned arrays
%typemap(out) player_point_3d_t [ANY] 
{
    VALUE ary;
    int i,j;
    ary = rb_ary_new2($1_dim0);

    for (i = 0; i < $1_dim0; i++)
    {
        inner_ary =  rb_ary_new2(2);
        rb_ary_push(inner_ary, $1[i].px);
        rb_ary_push(inner_ary, $1[i].py);
        rb_ary_push(inner_ary, $1[i].pz);
        rb_ary_push(ary, inner_ary);

/*
change to hash when we know how to use hash_lookup
        hash = rb_hash_new();
        rb_hash_aset(hash, "px", $1[i].px);
        rb_hash_aset(hash, "py", $1[i].py);
        rb_hash_aset(hash, "pz", $1[i].pz);
        rb_ary_push(ary, hash);
*/

    }
    $result = ary;
}

/*
// Provide array access doubly-dimensioned arrays
// when I know how to use hash_lookup, change this to array of hashes  
%typemap(out) player_color_t [ANY] 
{
    VALUE ary;
    int i,j;
    ary = rb_ary_new2($1_dim0);
   $result = ary;
}


// typemap for tuples to colours
%typemap(in) player_color_t (player_color_t temp)
{
	// Check it is a tuple
	if (PyTuple_Check ($input))
	{
		// Check the tuple has four elements
		if (PyTuple_GET_SIZE ($input) != 4)
		{
			PyErr_SetString (PyExc_ValueError, "tuple must have 4 items");
			return NULL;
		}
		temp.alpha = PyInt_AsLong (PyTuple_GET_ITEM ($input, 0));
		temp.red = PyInt_AsLong (PyTuple_GET_ITEM ($input, 1));
		temp.green = PyInt_AsLong (PyTuple_GET_ITEM ($input, 2));
		temp.blue = PyInt_AsLong (PyTuple_GET_ITEM ($input, 3));
	}
	else
	{
		PyErr_SetString (PyExc_TypeError, "not a tuple");
		return NULL;
	}
}


*/


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
    rb_raise( rb_eRuntimeError, "Size mismatch on the array" );
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
    rb_raise( rb_eRuntimeError, "Size mismatch on the array" );
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


// Provide array of array (write) access
%typemap(in) double [ANY] [ANY] (double temp[$1_dim0][$1_dim1])
{
  //TODO check if the input is a correct Ruby array
  //TODO: NUM2DBL and friends can return errors?
  int i;
  VALUE row;

  if (RARRAY_LEN($input) != $1_dim0) 
  {
    rb_raise( rb_eRuntimeError, "Size mismatch on the array" );
    return 0;
  }

  for (i = 0; i < $1_dim0; i++)
  {
    row = rb_ary_entry ($input, i)
    for (j = 0;j< $1_dim1;j++)
      temp[i][j] = (double) NUM2DBL( rb_ary_entry(row, i) );
  }
  $1 = temp;
}




// typemap for passing points into the graphics2d interface
// in C is an array of structures, where each structure has two doubles: px and py
// in Ruby an array of arrays 
%typemap(in) player_point_2d_t pts[]
{
//TODO: check we have a double array
//check it has doubles
  int i;
  int size;
  VALUE structure;
  
  size = RARRAY_LEN($input);

  $1 = (player_point_2d_t*) malloc (size * sizeof (player_point_2d_t));
  for (i = 0; i < size; i++)
  {
    structure = rb_ary_entry($input, i);
    $1[i].px = (double) NUM2DBL( rb_ary_entry(structure, 0) );
    $1[i].py = (double) NUM2DBL( rb_ary_entry(structure, 1) );
//    $1[i].px = (double) NUM2DBL( rb_hash_lookup(structure, "px") );
//    $1[i].py = (double) NUM2DBL( rb_hash_lookup(structure, "py") );
  }

}

%typemap(in) player_point_3d_t pts[]
{
//TODO: check we have a double array
//check it has doubles
  int i;
  int size;
  VALUE structure;
  
  size = RARRAY_LEN($input);

  $1 = (player_point_3d_t*) malloc (size * sizeof (player_point_3d_t));
  for (i = 0; i < size; i++)
  {
    structure = rb_ary_entry($input, i);
    $1[i].px = (double) NUM2DBL( rb_ary_entry(structure, 0) );
    $1[i].py = (double) NUM2DBL( rb_ary_entry(structure, 1) );
    $1[i].pz = (double) NUM2DBL( rb_ary_entry(structure, 2) );

//    $1[i].px = (double) NUM2DBL( rb_hash_lookup(structure, "px") );
//    $1[i].py = (double) NUM2DBL( rb_hash_lookup(structure, "py") );
//    $1[i].pz = (double) NUM2DBL( rb_hash_lookup(structure, "pz") );
  }

}




// typemap to free the array created in the previous typemap
%typemap(freearg) player_point2d_t pts[]
{
	if ($input) free ((player_point2d_t*) $input);
}



/*

What is this structure in ruby?
// typemap for tuples to colours
%typemap(in) player_color_t (player_color_t temp)
{
	// Check it is a tuple
	if (PyTuple_Check ($input))
	{
		// Check the tuple has four elements
		if (PyTuple_GET_SIZE ($input) != 4)
		{
			PyErr_SetString (PyExc_ValueError, "tuple must have 4 items");
			return NULL;
		}
		temp.alpha = PyInt_AsLong (PyTuple_GET_ITEM ($input, 0));
		temp.red = PyInt_AsLong (PyTuple_GET_ITEM ($input, 1));
		temp.green = PyInt_AsLong (PyTuple_GET_ITEM ($input, 2));
		temp.blue = PyInt_AsLong (PyTuple_GET_ITEM ($input, 3));
	}
	else
	{
		PyErr_SetString (PyExc_TypeError, "not a tuple");
		return NULL;
	}
}

*/




// Include Player header so we can pick up some constants and generate
// wrapper code for structs
%include "libplayerinterface/player.h"
%include "libplayerinterface/player_interfaces.h"

//TODO: service discovering, needs wrapping
//%include "libplayersd/playersd.h"



// Use this for object-oriented bindings;
// e.g., client.connect(...)
// This file is created by running ../parse_header.py
%include "playerc_wrap.i"

