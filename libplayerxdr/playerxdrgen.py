#!/usr/bin/env python

import re
import string
import sys

USAGE = 'USAGE: parse.y <interface-spec.h> <pack.c> <pack.h>'

if __name__ == '__main__':

  if len(sys.argv) != 4:
    print USAGE
    sys.exit(-1)

  infilename = sys.argv[1]
  sourcefilename = sys.argv[2]
  headerfilename = sys.argv[3]

  # Read in the entire file
  infile = open(infilename, 'r')
  instream = infile.read()
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

  headerfile.write(
'''/** @defgroup libplayerxdr libplayerxdr
 * @brief Player XDR library
 *
 * This library provides functions for translating between player C message
 * structs and their XDR representations.  For each 'struct player_foo',
 * there is a single function of the following name and form:
 *    int player_foo_pack(void* buf, size_t buflen, player_foo_t* msg, int op);
 * This function will either pack (encode to XDR) or unpack (decode from
 * XDR), depending on the last argument.
 *
 * Params:
 *  - buf: The XDR-encoded buffer that is being encoded / decoded.
 *  - buflen: Size of buf, in bytes.
 *  - msg: Pointer to the C struct that is being encoded / decoded.
 *  - op: Either PLAYERXDR_ENCODE or PLAYERXDR_DECODE
 *
 * Returns: On success, the length of the XDR-encoded buffer, and -1
 *          otherwise (e.g., the buffer was not large enough).
 *
 * The caller is responsible for allocating enough space to buf.
 */
 /** @ingroup libplayerxdr
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

  sourcefile.write('#include <libplayerxdr/' + headerfilename + '>\n\n')

  contentspattern = re.compile('.*\{\s*(.*?)\s*\}', re.MULTILINE | re.DOTALL)
  declpattern = re.compile('\s*([^;]*?;)', re.MULTILINE)
  typepattern = re.compile('\s*\S+')
  variablepattern = re.compile('\s*([^,;]+?)\s*[,;]')
  #arraypattern = re.compile('\[\s*(\w*?)\s*\]')
  arraypattern = re.compile('\[(.*?)\]')

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
    for i in range(0,2):

      if i == 0:
        headerfile.write('int xdr_' + typename + '(XDR* xdrs, ' + typename + 
                         '* msg);\n')
        sourcefile.write('int\nxdr_' + typename + '(XDR* xdrs, ' + typename + 
                         '* msg)\n{\n')
      else:
        headerfile.write('int ' + prefix + '_pack(void* buf, size_t buflen, ' +
                         typename + '* msg, int op);\n')
        sourcefile.write('int\n' + prefix + '_pack(void* buf, size_t buflen, ' +
                         typename + '* msg, int op)\n{\n')
        sourcefile.write('  XDR xdrs;\n')
        sourcefile.write('  int len;\n')
        sourcefile.write('  if(!buflen)\n')
        sourcefile.write('    return(0);\n')
        sourcefile.write('  xdrmem_create(&xdrs, buf, buflen, op);\n')

      varlist = []
  
      # separate the variable declarations
      decls = declpattern.finditer(varpart[0])
      for d in decls:
        # find the type and variable names in this declaration
        dstring = d.string[d.start(1):d.end(1)]
        type = typepattern.findall(dstring)[0]
        dstring = typepattern.sub('', dstring, 1)
        vars = variablepattern.finditer(dstring)
  
        # Do some name mangling for common types
        if type == 'int64_t':
          xdr_proc = 'xdr_long'
        elif type == 'uint64_t':
          xdr_proc = 'xdr_u_long'
        elif type == 'int32_t':
          xdr_proc = 'xdr_int'
        elif type == 'uint32_t':
          xdr_proc = 'xdr_u_int'
        elif type == 'int16_t':
          xdr_proc = 'xdr_short'
        elif type == 'uint16_t':
          xdr_proc = 'xdr_u_short'
        elif type == 'int8_t' or type == 'char':
          xdr_proc = 'xdr_char'
        elif type == 'uint8_t':
          xdr_proc = 'xdr_u_char'
        elif type == 'bool_t':
          xdr_proc = 'xdr_bool'
        else:
          # rely on a previous declaration of an xdr_ proc for this type
          xdr_proc = 'xdr_' + type
  
        # iterate through each variable
        for var in vars:
          varstring = var.string[var.start(1):var.end(1)]
          # is it an array or a scalar?
          arraysize = arraypattern.findall(varstring)
          if len(arraysize) > 0:
            arraysize = arraysize[0]
            varstring = arraypattern.sub('', varstring)
            pointervar = varstring + '_p'
            countvar = varstring + '_count'

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
                  sourcefile.write('    ' + type + '* ' + pointervar +
                                   ' = msg->' + varstring + ';\n')
                  sourcefile.write('    if(xdr_bytes(xdrs, (char**)&' + pointervar + 
                                   ', &msg->' + countvar +
                                   ', ' + arraysize + ') != 1)\n      return(0);\n')
                  sourcefile.write('  }\n')
                else:
                  sourcefile.write('  {\n')
                  sourcefile.write('    ' + type + '* ' + pointervar +
                                   ' = msg->' + varstring + ';\n')
                  sourcefile.write('    if(xdr_bytes(&xdrs, (char**)&' + pointervar + 
                                   ', &msg->' + countvar +
                                   ', ' + arraysize + ') != 1)\n      return(-1);\n')
                  sourcefile.write('  }\n')
              else:
                if i == 0:
                  sourcefile.write('  {\n')
                  sourcefile.write('    ' + type + '* ' + pointervar +
                                   ' = msg->' + varstring + ';\n')
                  sourcefile.write('    if(xdr_array(xdrs, (char**)&' + pointervar + 
                                   ', &msg->' + countvar +
                                   ', ' + arraysize + ', sizeof(' + type + '), (xdrproc_t)' + 
                                   xdr_proc + ') != 1)\n      return(0);\n')
                  sourcefile.write('  }\n')
                else:
                  sourcefile.write('  {\n')
                  sourcefile.write('    ' + type + '* ' + pointervar +
                                   ' = msg->' + varstring + ';\n')
                  sourcefile.write('    if(xdr_array(&xdrs, (char**)&' + pointervar + 
                                   ', &msg->' + countvar +
                                   ', ' + arraysize +  ', sizeof(' + type + '), (xdrproc_t)' + 
                                   xdr_proc + ') != 1)\n      return(-1);\n')
                  sourcefile.write('  }\n')
            else:
              # Is it an array of bytes?  If so, then we'll encode
              # it a packed opaque bytestring, rather than an array of 4-byte-aligned
              # chars.
              if xdr_proc == 'xdr_u_char' or xdr_proc == 'xdr_char':
                if i == 0:
                  sourcefile.write('  if(xdr_opaque(xdrs, (char*)&msg->' +
                                   varstring + ', ' + arraysize + ') != 1)\n    return(0);\n')
                else:
                  sourcefile.write('  if(xdr_opaque(&xdrs, (char*)&msg->' +
                                   varstring + ', ' + arraysize + ') != 1)\n    return(-1);\n')
              else:
                if i == 0:
                  sourcefile.write('  if(xdr_vector(xdrs, (char*)&msg->' +
                                   varstring + ', ' + arraysize +
                                   ', sizeof(' + type + '), (xdrproc_t)' +
                                   xdr_proc + ') != 1)\n    return(0);\n')
                else:
                  sourcefile.write('  if(xdr_vector(&xdrs, (char*)&msg->' +
                                   varstring + ', ' + arraysize +
                                   ', sizeof(' + type + '), (xdrproc_t)' +
                                   xdr_proc + ') != 1)\n    return(-1);\n')
          else:
            if i == 0:
              sourcefile.write('  if(' + xdr_proc + '(xdrs,&msg->' + 
                               varstring + ') != 1)\n    return(0);\n')
            else:
              sourcefile.write('  if(' + xdr_proc + '(&xdrs,&msg->' + 
                               varstring + ') != 1)\n    return(-1);\n')

          varlist.append(varstring)
      if i == 0:
        sourcefile.write('  return(1);\n}\n\n')
      else:
        sourcefile.write('  if(op == PLAYERXDR_ENCODE)\n')
        sourcefile.write('    len = xdr_getpos(&xdrs);\n')
        sourcefile.write('  else\n')
        sourcefile.write('    len = sizeof(' + typename + ');\n')
        sourcefile.write('  xdr_destroy(&xdrs);\n')
        sourcefile.write('  return(len);\n')
        sourcefile.write('}\n\n')

  headerfile.write('\n#ifdef __cplusplus\n}\n#endif\n\n')
  headerfile.write('#endif\n')
  headerfile.write('/** @} */\n')

  sourcefile.close()
  headerfile.close()

