#!/usr/bin/python

import os.path
import sys
import string

true = 1
false = 0


class Block:
    """Class for holding info about documentation blocks."""

    def __init__(self):
        self.type = None
        self.brief = ''
        self.desc = ''
        self.code = ''
        return

    def __str__(self):
        return '%s %s %s' % (self.brief, self.desc, self.code)


def parsefile(filename):
    """Parse out the classes, methods, etc with their associated descritpions."""

    srcfile = open(filename, 'r')

    state = 0
    blocks = []

    while true:
        line = srcfile.readline()
        if not line:
            break

        stripped = string.strip(line)

        if state == 0:
            if stripped[:3] == '/**':
                state = 1
                block = Block()
                block.brief = string.strip(stripped[3:])
            elif stripped[:3] == '///':
                state = 2
                block = Block()
                block.brief = string.strip(stripped[3:])
        elif state == 1:
            if stripped[:2] == '*/':
                state = 2
            else:
                block.desc = block.desc + stripped + '\n'
        elif state == 2:
            if not stripped:
                state = 0
                blocks.append(block)
            else:
                block.code = block.code + stripped + '\n'

    #for block in blocks:
    #    print block

    return blocks


def guessblock(block):
    """Guess some of the fields in the doc block."""

    tokens = string.split(block.code)

    if tokens[0] == 'class':
        block.type = 'class'
        block.classname = tokens[1]
    elif string.find(block.code, '(') > 0:
        block.type = 'method'
    else:
        block.type = 'data'

    # Do some munging of the block code, like getting rid of excess stuff.
    if string.find(block.code, '{') >= 0:
        block.code = block.code[0:string.find(block.code, '{')]
    if string.find(block.code, ';') >= 0:
        block.code = block.code[0:string.find(block.code, ';')]

    # Get rid of leading access modifiers and inline code
    if string.find(block.code, ':') >= 0:
        l = block.code[0:string.find(block.code, ':')]
        r = block.code[string.find(block.code, ':') + 1:]
        if l == 'public':
            block.code = r
        else:
            block.code = l

    # Strip leading and trailing spaces
    block.code = string.strip(block.code)

    return


def make_tex(filename, blocks):
    """Generate latex from the doc blocks."""

    texfile = open(filename, 'w+')

    for block in blocks:
        guessblock(block)

    for block in blocks:
        if block.type == 'class':
            make_class(texfile, block)

    texfile.write('\\subsection*{Attributes}')
    for block in blocks:            
        if block.type == 'data':
            make_data(texfile, block)
            
    texfile.write('\\subsection*{Methods}')
    for block in blocks:
        if block.type == 'method':
            make_method(texfile, block)

    return


def make_class(file, block):
    """Generate a class entry."""

    file.write('\\section{class %s}\n\n' % block.classname);
    file.write('%s\n' % block.brief)
    file.write('%s\n' % block.desc)
    return


def make_method(file, block):
    """Generate method blocks."""

    file.write('\\begin{verbatim}\n')
    file.write(block.code)
    file.write('\\end{verbatim}\n')
    file.write('\\vspace*{-0.5em}\n')
    file.write('%s ' % block.brief)
    file.write('%s\n' % block.desc)
    file.write('\\vspace*{0.5em}\n')

    return


def make_data(file, block):
    """Generate data blocks."""

    file.write('\\begin{verbatim}\n')
    file.write(block.code)
    file.write('\\end{verbatim}\n')
    file.write('\\vspace*{-0.5em}\n')
    file.write('%s ' % block.brief)
    file.write('%s\n' % block.desc)
    file.write('\\vspace*{0.5em}\n')

    return


if __name__ == '__main__':

    filelist = sys.argv[1:]

    for file in filelist:
        blocks = parsefile(file)
        make_tex(os.path.basename(file) + '.tex', blocks)
