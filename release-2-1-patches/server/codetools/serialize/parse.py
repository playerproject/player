#!/usr/bin/env python

# TODO:
#  - Handle nested structs (make an xdrproc for each struct + a wrapper for
#    it; then each xdrproc can just call the right proc for each of its
#    member)
#
#  - Handle variable-length arrays (I keep getting segfaults)
#
#  - Handle multi-dimensional arrays (e.g., player_sonar_geom_t::poses)


import re
import string
import sys

USAGE = 'USAGE: parse.y <player.h> <pack.c> <pack.h>'

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

  headerfile.write('#include <rpc/types.h>\n')
  headerfile.write('#include <rpc/xdr.h>\n\n')
  headerfile.write('#include <player.h>\n\n')
  headerfile.write('#ifdef __cplusplus\nextern "C" {\n#endif\n\n')
  headerfile.write('#define PLAYER_ENCODE XDR_ENCODE\n')
  headerfile.write('#define PLAYER_DECODE XDR_DECODE\n\n')

  sourcefile.write('#include <' + headerfilename + '>\n\n')

  contentspattern = re.compile('.*\{\s*(.*?)\s*\}', re.MULTILINE | re.DOTALL)
  declpattern = re.compile('\s*([^;]*?;)', re.MULTILINE)
  typepattern = re.compile('\s*\S+')
  variablepattern = re.compile('\s*([^,;]+?)\s*[,;]')
  arraypattern = re.compile('\[\s*(\w*?)\s*\]')

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

    headerfile.write('int ' + prefix + '_pack(void* buf, size_t buflen, ' +
                     typename + '* msg, int op);\n')
    sourcefile.write('int\n' + prefix + '_pack(void* buf, size_t buflen, ' +
                     typename + '* msg, int op)\n{\n')
    sourcefile.write('  XDR xdrs;\n')
    sourcefile.write('  size_t len;\n')
    sourcefile.write('  len = 0;\n')
    sourcefile.write('  xdrmem_create(&xdrs, buf, buflen, op);\n')

    # separate the variable declarations
    decls = declpattern.finditer(varpart[0])
    for d in decls:
      # find the type and variable names in this declaration
      dstring = d.string[d.start(1):d.end(1)]
      type = typepattern.findall(dstring)[0]
      dstring = typepattern.sub('', dstring, 1)
      vars = variablepattern.finditer(dstring)

      if type == 'int64_t' or \
         type == 'uint64_t' or \
         type == 'int32_t' or \
         type == 'uint32_t' or \
         type == 'int16_t' or \
         type == 'uint16_t' or \
         type == 'int8_t' or \
         type == 'uint8_t':
        xdr_proc = 'xdr_' + type
      else:
        print 'skipping ' + typename + '::' + dstring + '(unknown type ' + type + ')'
        continue

      # iterate through each variable
      for var in vars:
        varstring = var.string[var.start(1):var.end(1)]
        # is it an array or a scalar?
        arraysize = arraypattern.findall(varstring)
        if len(arraysize) > 0:
          arraysize = arraysize[0]
          varstring = arraypattern.sub('', varstring)
          sourcefile.write('  {\n')
          sourcefile.write('    xdr_vector(&xdrs, (char*)&msg->' + varstring + ', ' + arraysize + ', sizeof(' + type + '), (xdrproc_t)' + xdr_proc + ');\n')
          sourcefile.write('  }\n')
        else:
          sourcefile.write('  ' + xdr_proc + '(&xdrs,&msg->' + varstring + ');\n')

    sourcefile.write('  xdr_destroy(&xdrs);\n')
    sourcefile.write('  return(len);\n}\n\n')

  headerfile.write('\n#ifdef __cplusplus\n}\n#endif\n\n')

  sourcefile.close()
  headerfile.close()

