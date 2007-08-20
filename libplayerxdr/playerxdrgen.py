#!/usr/bin/env python

#TODO:
#  - Add an option to specify whether we're building libplayerxdr (whose
#    header gets installed for general use, has copyright boilerplate,
#    etc.) or a user XDR lib

import re
import string
import sys

USAGE = 'USAGE: playerxdrgen.y [-distro] <interface-spec.h> [<extra_interface-spec.h>] <pack.c> <pack.h>'

if __name__ == '__main__':

  if len(sys.argv) < 4:
    print USAGE
    sys.exit(-1)

  distro = 0

  idx = 1
  if sys.argv[1] == '-distro':
    if len(sys.argv) < 5:
      print USAGE
      sys.exit(-1)
    distro = 1
    idx += 1

  infilenames = [sys.argv[idx],]
  idx += 1
  sourcefilename = sys.argv[idx]
  idx += 1
  headerfilename = sys.argv[idx]
  idx += 1
  if len(sys.argv) > idx:
    for opt in sys.argv[idx:]:
      infilenames.append(opt)
      print "processeing extra file ", opt


  # Read in the entire file
  instream = ""
  for f in infilenames:
    infile = open(f, 'r')
    instream += infile.read()
    infile.close()

  sourcefile = open(sourcefilename, 'w+')
  headerfile = open(headerfilename, 'w+')

  # strip C++-style comments
  pattern = re.compile('//.*')
  instream = pattern.sub('', instream)

  # strip C-style comments
  pattern = re.compile('/\*.*?\*/', re.MULTILINE | re.DOTALL)
  instream = pattern.sub('', instream)

  # strip blank lines
  pattern = re.compile('^\s*?\n', re.MULTILINE)
  instream = pattern.sub('', instream)

  # find structs
  pattern = re.compile('typedef\s+struct\s+player_\w+[^}]+\}[^;]+',
                   re.MULTILINE)
  structs = pattern.findall(instream)

  print 'Found ' + `len(structs)` + ' struct(s)'

  if distro:
    headerfile.write(
'''/** @ingroup libplayerxdr
 @{ */\n\n''')
    headerfile.write('#ifndef _PLAYERXDR_PACK_H_\n')
    headerfile.write('#define _PLAYERXDR_PACK_H_\n\n')
    headerfile.write('#include <rpc/types.h>\n')
    headerfile.write('#include <rpc/xdr.h>\n\n')
    headerfile.write('#include <libplayercore/player.h>\n\n')
    headerfile.write('#include <libplayerxdr/functiontable.h>\n\n')
    headerfile.write('#ifdef __cplusplus\nextern "C" {\n#endif\n\n')
    headerfile.write('#ifndef XDR_ENCODE\n')
    headerfile.write('  #define XDR_ENCODE 0\n')
    headerfile.write('#endif\n')
    headerfile.write('#ifndef XDR_DECODE\n')
    headerfile.write('  #define XDR_DECODE 1\n')
    headerfile.write('#endif\n')
    headerfile.write('#define PLAYERXDR_ENCODE XDR_ENCODE\n')
    headerfile.write('#define PLAYERXDR_DECODE XDR_DECODE\n\n')
    headerfile.write('#define PLAYERXDR_MSGHDR_SIZE 40\n\n')
    headerfile.write('#define PLAYERXDR_MAX_MESSAGE_SIZE (4*PLAYER_MAX_MESSAGE_SIZE)\n\n')

    sourcefile.write('#include <libplayerxdr/' + headerfilename + '>\n')
    sourcefile.write('#include <string.h>\n\n')
    sourcefile.write('#include <stdlib.h>\n\n')
  else:
    ifndefsymbol = '_'
    for i in range(0,len(string.split(infilename,'.')[0])):
      ifndefsymbol += string.capitalize(infilename[i])
    ifndefsymbol += '_'
    headerfile.write('#ifndef ' + ifndefsymbol + '\n\n')
    headerfile.write('#include <libplayerxdr/playerxdr.h>\n\n')
    headerfile.write('#include "' + infilename + '"\n\n')
    headerfile.write('#ifdef __cplusplus\nextern "C" {\n#endif\n\n')
    sourcefile.write('#include <rpc/types.h>\n')
    sourcefile.write('#include <rpc/xdr.h>\n\n')
    sourcefile.write('#include "' + headerfilename + '"\n')
    sourcefile.write('#include <string.h>')
    sourcefile.write('#include <stdlib.h>\n\n')

  contentspattern = re.compile('.*\{\s*(.*?)\s*\}', re.MULTILINE | re.DOTALL)
  declpattern = re.compile('\s*([^;]*?;)', re.MULTILINE)
  typepattern = re.compile('\s*\S+')
  variablepattern = re.compile('\s*([^,;]+?)\s*[,;]')
  #arraypattern = re.compile('\[\s*(\w*?)\s*\]')
  arraypattern = re.compile('\[(.*?)\]')
#  pointerpattern = re.compile('[A-Za-z0-9_]+\*|\*[A-Za-z0-9_]+')
  pointerpattern = re.compile('\*')

  hasdynamic = []    # A list of types that contain dynamic data. During pass 1,
                     # if a type is found to have dynamic data it will be added
                     # to this list. Then, during other passes for this and
                     # other types, necessary deep copy/clean up functions can
                     # be created and called.
  for s in structs:
    # extract prefix for packing function name and type of struct
    split = string.split(s)
    prefix = split[2]
    typename = split[-1]

    # pick out the contents of the struct
    varpart = contentspattern.findall(s)
    if len(varpart) != 1:
      print 'skipping nested / empty struct ' + typename
      continue

    # First time through this loop, we write an xdr_ function for the
    # struct; this function will be used internally if the struct appears
    # inside another struct.
    #
    # Second time through, we write a _pack function for external use.
    # Third time through, we write a _dpcpy function if needed.
    # Fourth time through, we write a _cleanup function if needed.
    for i in range(0,4):

      if i == 0:
        headerfile.write('int xdr_' + typename + '(XDR* xdrs, ' + typename +
                         '* msg);\n')
        sourcefile.write('int\nxdr_' + typename + '(XDR* xdrs, ' + typename +
                         '* msg)\n{\n')
      elif i == 1:
        headerfile.write('int ' + prefix + '_pack(void* buf, size_t buflen, ' +
                         typename + '* msg, int op);\n')
        sourcefile.write('int\n' + prefix + '_pack(void* buf, size_t buflen, ' +
                         typename + '* msg, int op)\n{\n')
        sourcefile.write('  XDR xdrs;\n')
        sourcefile.write('  int len;\n')
        sourcefile.write('  if(!buflen)\n')
        sourcefile.write('    return(0);\n')
        sourcefile.write('  xdrmem_create(&xdrs, buf, buflen, op);\n')
      elif i == 2:
        # If type is not in hasdynamic, not going to write a function so may as well just continue with the next struct
        if typename not in hasdynamic:
          headerfile.write('#define ' + typename + '_dpcpy NULL\n')
          continue
        headerfile.write('unsigned int ' + typename + '_dpcpy(const ' + typename + '* src, ' + typename + '* dest);\n')
        sourcefile.write('unsigned int\n' + typename + '_dpcpy(const ' + typename + '* src, ' + typename + '* dest)\n{\n')
        #sourcefile.write('  printf ("' + typename + '_dpcpy starting\\n"); fflush(NULL);\n')
        sourcefile.write('  unsigned ii;\nif(src == NULL)\n    return(0);\n')
        sourcefile.write('  unsigned int size = 0;\n')
      else:
        # If type is not in hasdynamic, not going to write a function so may as well just continue with the next struct
        if typename not in hasdynamic:
          headerfile.write('#define ' + typename + '_cleanup NULL\n')
          continue
        headerfile.write('void ' + typename + '_cleanup(' + typename + '* msg);\n')
        sourcefile.write('void\n' + typename + '_cleanup(' + typename + '* msg)\n{\n')
        #sourcefile.write('  printf("' + typename + '_cleanup starting\\n"); fflush(NULL);\n')
        sourcefile.write('  unsigned ii;\nif(msg == NULL)\n    return;\n')

      varlist = []

      # separate the variable declarations
      decls = declpattern.findall(varpart[0])
      for dstring in decls:
        # find the type and variable names in this declaration
        typestring = typepattern.findall(dstring)[0]
        dstring = typepattern.sub('', dstring, 1)
        vars = variablepattern.findall(dstring)

        # Check for pointer in type part
        numstars = len (pointerpattern.findall (typestring))
        if numstars > 0:
          typestring = typestring[:-1 * numstars]
        # Do some name mangling for common types
        if typestring == 'long long':
          xdr_proc = 'xdr_longlong_t'
        elif typestring == 'int64_t':
          xdr_proc = 'xdr_longlong_t'
        elif typestring == 'uint64_t':
          xdr_proc = 'xdr_u_long'
        elif typestring == 'int32_t':
          xdr_proc = 'xdr_int'
        elif typestring == 'uint32_t':
          xdr_proc = 'xdr_u_int'
        elif typestring == 'int16_t':
          xdr_proc = 'xdr_short'
        elif typestring == 'uint16_t':
          xdr_proc = 'xdr_u_short'
        elif typestring == 'int8_t' or typestring == 'char':
          xdr_proc = 'xdr_char'
        elif typestring == 'uint8_t':
          xdr_proc = 'xdr_u_char'
        elif typestring == 'bool_t':
          xdr_proc = 'xdr_bool'
        else:
          # rely on a previous declaration of an xdr_ proc for this type
          xdr_proc = 'xdr_' + typestring
          # Check if this type has dynamic data, if it does then the current struct will to
          if typestring in hasdynamic and typename not in hasdynamic:
            hasdynamic.append (typename)

        # iterate through each variable
        for varstring in vars:
          # is it an array or a scalar?
          array = False
          arraysize = arraypattern.findall(varstring)
          pointersize = pointerpattern.findall(varstring)
          if len(arraysize) > 0:
              array = True
              arraysize = arraysize[0]
              varstring = arraypattern.sub('', varstring)
          elif len(pointersize) > 0 and numstars > 0:  # This checks for things like "uint8_t* *data"
            raise 'Illegal pointer declaration in struct\n' + s
          elif len(pointersize) > 0 or numstars > 0:
            array = True
            arraysize = ''
            varstring = pointerpattern.sub('', varstring)

          if array:
            pointervar = varstring + '_p'
            countvar = varstring + '_count'
            if arraysize == '':  # Handle a dynamically allocated array
              # Make a note that this structure has dynamic data (need to know for passes 3 and 4, and for other messages)
              if typename not in hasdynamic:
                hasdynamic.append (typename)
              # Check for a matching count variable, because compulsory for dynamic arrays
              if countvar not in varlist:
                raise 'Missing count var "' + countvar + '" in\n' + s
              # First put in a check to see if unpacking, and if so allocate memory for the array
              if i == 0:
                sourcefile.write('  if(xdrs->x_op == XDR_DECODE)\n  {\n')
                sourcefile.write('    if((msg->' + varstring + ' = malloc(msg->' + countvar + '*sizeof(' + typestring + '))) == NULL)\n      return(0);\n')
                #sourcefile.write('    memset(msg->' + varstring + ', 0, msg->' + countvar + '*sizeof(' + typestring + '));\n')
                sourcefile.write('  }\n')
              if i == 1:
                sourcefile.write('  if(op == PLAYERXDR_DECODE)\n  {\n')
                sourcefile.write('    if((msg->' + varstring + ' = malloc(msg->' + countvar + '*sizeof(' + typestring + '))) == NULL)\n      return(-1);\n')
                #sourcefile.write('    memset(msg->' + varstring + ', 0, msg->' + countvar + '*sizeof(' + typestring + '));\n')
                sourcefile.write('  }\n')
              # Write the lines to (un)pack/convert
              # Is it an array of bytes?  If so, then we'll encode
              # it a packed opaque bytestring, rather than an array of 4-byte-aligned
              # chars.
              if xdr_proc == 'xdr_u_char' or xdr_proc == 'xdr_char':
                if i == 0:
                  sourcefile.write('  {\n')
                  sourcefile.write('    ' + typestring + '* ' + pointervar + ' = msg->' + varstring + ';\n')
                  sourcefile.write('    if(xdr_bytes(xdrs, (char**)&' + pointervar + ', &msg->' + countvar + ', msg->' + countvar + ') != 1)\n      return(0);\n')
                  sourcefile.write('  }\n')
                elif i == 1:
                  sourcefile.write('  {\n')
                  sourcefile.write('    ' + typestring + '* ' + pointervar + ' = msg->' + varstring + ';\n')
                  sourcefile.write('    if(xdr_bytes(&xdrs, (char**)&' + pointervar + ', &msg->' + countvar + ', msg->' + countvar + ') != 1)\n      return(-1);\n')
                  sourcefile.write('  }\n')
              else:
                if i == 0:
                  sourcefile.write('  {\n')
                  sourcefile.write('    ' + typestring + '* ' + pointervar + ' = msg->' + varstring + ';\n')
                  sourcefile.write('    if(xdr_array(xdrs, (char**)&' + pointervar + ', &msg->' + countvar + ', msg->' + countvar +', sizeof(' + typestring + '), (xdrproc_t)' + xdr_proc + ') != 1)\n      return(0);\n')
                  sourcefile.write('  }\n')
                elif i == 1:
                  sourcefile.write('  {\n')
                  sourcefile.write('    ' + typestring + '* ' + pointervar + ' = msg->' + varstring + ';\n')
                  sourcefile.write('    if(xdr_array(&xdrs, (char**)&' + pointervar + ', &msg->' + countvar + ', msg->' + countvar +', sizeof(' + typestring + '), (xdrproc_t)' + xdr_proc + ') != 1)\n      return(-1);\n')
                  sourcefile.write('  }\n')
              if i == 2:
                #sourcefile.write('  printf ("deep copying, src = %p, dest = %p\\n", src, dest);\n')
                sourcefile.write('  if(src->' + varstring + ' != NULL && src->' + countvar + ' > 0)\n  {\n')
                #sourcefile.write('    printf ("deep copying, src->' + varstring + ' = %p, dest->' + varstring + ' = %p\\n", src->' + varstring + ', dest->' + varstring + ');\n')
                sourcefile.write('    if((dest->' + varstring + ' = malloc(src->' + countvar + '*sizeof(' + typestring + '))) == NULL)\n      return(0);\n')
                sourcefile.write('    memcpy(dest->' + varstring + ', src->' + varstring + ', src->' + countvar + '*sizeof(' + typestring + '));\n')
                sourcefile.write('    size += src->' + countvar + '*sizeof(' + typestring + ');\n  }\n')
                sourcefile.write('  else\n    dest->' + varstring + ' = NULL;\n')
                if typestring in hasdynamic:    # Need to deep copy embedded structures
                  sourcefile.write('  for(ii = 0; ii < src->' + countvar + '; ii++)\n  {\n')
                  sourcefile.write('    size += ' + typestring + '_dpcpy(&(src->' + varstring + '[ii]), &(dest->' + varstring + '[ii]));\n  }\n')
              elif i == 3:
                sourcefile.write('  if(msg->' + varstring + ' == NULL)\n    return;\n')
                if typestring in hasdynamic:    # Need to clean up embedded structures
                  sourcefile.write('  for(ii = 0; ii < msg->' + countvar + '; ii++)\n  {\n')
                  sourcefile.write('    ' + typestring + '_cleanup(&(msg->' + varstring + '[ii]));\n  }\n')
                sourcefile.write('  free(msg->' + varstring + ');\n')
            else:           # Handle a static array
              # Was a _count variable declared? If so, we'll encode as a
              # variable-length array (with xdr_array); otherwise we'll
              # do it fixed-length (with xdr_vector).  xdr_array is picky; we
              # have to declare a pointer to the array, then pass in the
              # address of this pointer.  Passing the address of the array
              # does NOT work.
              if countvar in varlist:

                # Is it an array of bytes?  If so, then we'll encode
                # it a packed opaque bytestring, rather than an array of 4-byte-aligned
                # chars.
                if xdr_proc == 'xdr_u_char' or xdr_proc == 'xdr_char':
                  if i == 0:
                    sourcefile.write('  {\n')
                    sourcefile.write('    ' + typestring + '* ' + pointervar +
                                    ' = msg->' + varstring + ';\n')
                    sourcefile.write('    if(xdr_bytes(xdrs, (char**)&' + pointervar +
                                    ', &msg->' + countvar +
                                    ', ' + arraysize + ') != 1)\n      return(0);\n')
                    sourcefile.write('  }\n')
                  elif i == 1:
                    sourcefile.write('  {\n')
                    sourcefile.write('    ' + typestring + '* ' + pointervar +
                                    ' = msg->' + varstring + ';\n')
                    sourcefile.write('    if(xdr_bytes(&xdrs, (char**)&' + pointervar +
                                    ', &msg->' + countvar +
                                    ', ' + arraysize + ') != 1)\n      return(-1);\n')
                    sourcefile.write('  }\n')
                else:
                  if i == 0:
                    sourcefile.write('  {\n')
                    sourcefile.write('    ' + typestring + '* ' + pointervar +
                                    ' = msg->' + varstring + ';\n')
                    sourcefile.write('    if(xdr_array(xdrs, (char**)&' + pointervar +
                                    ', &msg->' + countvar +
                                    ', ' + arraysize + ', sizeof(' + typestring + '), (xdrproc_t)' +
                                    xdr_proc + ') != 1)\n      return(0);\n')
                    sourcefile.write('  }\n')
                  elif i == 1:
                    sourcefile.write('  {\n')
                    sourcefile.write('    ' + typestring + '* ' + pointervar +
                                    ' = msg->' + varstring + ';\n')
                    sourcefile.write('    if(xdr_array(&xdrs, (char**)&' + pointervar +
                                    ', &msg->' + countvar +
                                    ', ' + arraysize + ', sizeof(' + typestring + '), (xdrproc_t)' +
                                    xdr_proc + ') != 1)\n      return(-1);\n')
                    sourcefile.write('  }\n')
                  if typestring in hasdynamic:    # Need to deep copy/clean up embedded structures
                    if i == 2:
                      sourcefile.write('  for(ii = 0; ii < src->' + countvar + '; ii++)\n  {\n')
                      sourcefile.write('    size += ' + typestring + '_dpcpy(&(src->' + varstring + '[ii]), &(dest->' + varstring + '[ii]));\n  }\n')
                    elif i == 3:
                      sourcefile.write('  for(ii = 0; ii < msg->' + countvar + '; ii++)\n  {\n')
                      sourcefile.write('    ' + typestring + '_cleanup(&(msg->' + varstring + '[ii]));\n  }\n')
              else:
                # Is it an array of bytes?  If so, then we'll encode
                # it a packed opaque bytestring, rather than an array of 4-byte-aligned
                # chars.
                if xdr_proc == 'xdr_u_char' or xdr_proc == 'xdr_char':
                  if i == 0:
                    sourcefile.write('  if(xdr_opaque(xdrs, (char*)&msg->' +
                                    varstring + ', ' + arraysize + ') != 1)\n    return(0);\n')
                  elif i == 1:
                    sourcefile.write('  if(xdr_opaque(&xdrs, (char*)&msg->' +
                                    varstring + ', ' + arraysize + ') != 1)\n    return(-1);\n')
                else:
                  if i == 0:
                    sourcefile.write('  if(xdr_vector(xdrs, (char*)&msg->' +
                                    varstring + ', ' + arraysize +
                                    ', sizeof(' + typestring + '), (xdrproc_t)' +
                                    xdr_proc + ') != 1)\n    return(0);\n')
                  elif i == 1:
                    sourcefile.write('  if(xdr_vector(&xdrs, (char*)&msg->' +
                                    varstring + ', ' + arraysize +
                                    ', sizeof(' + typestring + '), (xdrproc_t)' +
                                    xdr_proc + ') != 1)\n    return(-1);\n')
                  if typestring in hasdynamic:    # Need to deep copy/clean up embedded structures
                    if i == 2:
                      sourcefile.write('  for(ii = 0; ii < ' + arraysize + '; ii++)\n  {\n')
                      sourcefile.write('    size += ' + typestring + '_dpcpy(&(src->' + varstring + '[ii]), &(dest->' + varstring + '[ii]));\n  }\n')
                    elif i == 3:
                      sourcefile.write('  for(ii = 0; ii < ' + arraysize + '; ii++)\n  {\n')
                      sourcefile.write('    ' + typestring + '_cleanup(&(msg->' + varstring + '[ii]_);\n  }\n')
          else:
            if i == 0:
              sourcefile.write('  if(' + xdr_proc + '(xdrs,&msg->' +
                               varstring + ') != 1)\n    return(0);\n')
            elif i == 1:
              sourcefile.write('  if(' + xdr_proc + '(&xdrs,&msg->' +
                               varstring + ') != 1)\n    return(-1);\n')
            if typestring in hasdynamic:    # Need to deep copy/clean up embedded structures
              if i == 2:
                sourcefile.write('  size += ' + typestring + '_dpcpy(&src->' + varstring + ', &dest->' + varstring + ');\n')
              elif i == 3:
                sourcefile.write('  ' + typestring + '_cleanup(&msg->' + varstring + ');\n')

          varlist.append(varstring)
      if i == 0:
        sourcefile.write('  return(1);\n}\n\n')
      elif i == 1:
        sourcefile.write('  if(op == PLAYERXDR_ENCODE)\n')
        sourcefile.write('    len = xdr_getpos(&xdrs);\n')
        sourcefile.write('  else\n')
        sourcefile.write('    len = sizeof(' + typename + ');\n')
        sourcefile.write('  xdr_destroy(&xdrs);\n')
        sourcefile.write('  return(len);\n')
        sourcefile.write('}\n\n')
      elif i == 2:
        #sourcefile.write('  printf ("' + typename + '_dpcpy done, size = %d\\n", size); fflush(NULL);\n')
        sourcefile.write('  return(size);\n}\n\n')
      else:
        #sourcefile.write('  printf("' + typename + '_cleanup done\\n"); fflush(NULL);\n')
        sourcefile.write('}\n\n')

  headerfile.write('\n#ifdef __cplusplus\n}\n#endif\n\n')
  headerfile.write('#endif\n')
  if distro:
    headerfile.write('/** @} */\n')

  sourcefile.close()
  headerfile.close()

