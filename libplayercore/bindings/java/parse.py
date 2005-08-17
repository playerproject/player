#!/usr/bin/env python

import re
import string
import sys

USAGE = 'USAGE: parse.y <player.h> <playercore_casts.i>'

if __name__ == '__main__':

  if len(sys.argv) != 3:
    print USAGE
    sys.exit(-1)

  infilename = sys.argv[1]
  outfilename = sys.argv[2]

  # Read in the entire file
  infile = open(infilename, 'r')
  instream = infile.read()
  infile.close()

  outfile = open(outfilename, 'w+')

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

  contentspattern = re.compile('.*\{\s*(.*?)\s*\}', re.MULTILINE | re.DOTALL)
  declpattern = re.compile('\s*([^;]*?;)', re.MULTILINE)
  typepattern = re.compile('\s*\S+')
  variablepattern = re.compile('\s*([^,;]+?)\s*[,;]')
  #arraypattern = re.compile('\[\s*(\w*?)\s*\]')
  arraypattern = re.compile('\[(.*?)\]')

  outfile.write('%inline\n%{\n\n')

  for s in structs:
    # extract type of struct
    split = string.split(s)
    typename = split[-1]

    outfile.write(typename + '* buf_to_' + typename + '(unsigned char* buf)\n')
    outfile.write('{\n')
    outfile.write('  return((' + typename + '*)(buf));\n')
    outfile.write('}\n')

  outfile.write('\n%}\n')

  outfile.close()
