#!/usr/bin/env python

import os.path
import sys
import string

true = 1
false = 0


class Section:
    """Class holding info about a section."""

    def __init__(self, name):
        self.name = name
        self.lines = []
        return
    

class Block:
    """Class for holding info about documentation blocks."""

    def __init__(self):
        self.type = None
        self.code = ''
        self.desc = ''
        return

    def __str__(self):
        return 'Type: %s\nCode: %s\nDesc: %s\n\n' % (self.type, self.code, self.desc)


def print_error(msg, filename, line):
    """Print a parser errors."""

    sys.stderr.write('error (%d) in file %s:%d\n' % (msg, filename, line))
    return


def print_error_block(msg, section, block):

    sys.stderr.write('error (%s) in section: %s\n%s' % (msg, section.name, str(block)))
    return
    

def parse_file(filename):
    """Parse out sections from a file."""

    file = open(filename, 'r')
    count = 0
    section = None
    sections = []

    for line in file:
        tokens = string.split(line)
        count += 1

        # Parse section breaks
        if len(tokens) == 4 and tokens[:3] == ['**', 'begin', 'section']:
            if section:
                print_error('missing end section', filename, count)
                return
            name = tokens[3]
            section = Section(name)
        elif len(tokens) == 3 and tokens[:3] == ['**', 'end', 'section']:
            sections.append(section)
            section = None

        if section:
            section.lines.append(line)

    return sections


def parse_section(section):
    """Parse out the classes, methods, etc with their associated descriptions."""

    state = 0
    blocks = []

    for line in section:
        stripped = string.strip(line)

        if state == 0:
            if stripped[:3] == '/**':
                if stripped[-2:] == '*/':
                    block = Block()
                    block.desc = string.strip(stripped[3:-2])
                    blocks.append(block)
                else:
                    state = 1
                    block = Block()
                    block.desc = string.strip(stripped[3:])
            elif stripped[:3] == '///':
                state = 2
                block = Block()
                block.desc = string.strip(stripped[3:])
        elif state == 1:
            if stripped[-2:] == '*/':
                block.desc = block.desc + ' ' + stripped[:-2]
                state = 2
            else:
                block.desc = block.desc + ' ' + stripped
        elif state == 2:
            if not stripped:
                state = 0
                blocks.append(block)
            else:
                block.code = block.code + stripped

    #for block in blocks:
    #    print block

    return blocks


def guessblock(section, block):
    """Guess some of the fields in the doc block."""

    tokens = string.split(block.code)

    # Do some munging of the block description, like escaping _.
    block.desc = string.replace(block.desc, '\_', '_')
    block.desc = string.replace(block.desc, '_', '\_')

    if len(tokens) > 0:
        if tokens[0] == 'class':
            block.type = 'class'
            block.classname = tokens[1]
        elif string.find(block.code, '(') > 0:
            block.type = 'method'
        else:
            block.type = 'data'
    else:
        print_error_block('unparsable block', section, block)

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


def make_tex(filename, section, blocks):
    """Generate latex from the doc blocks."""

    texfile = open(filename, 'w+')

    for block in blocks:
        guessblock(section, block)

    for block in blocks:
        print block

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
    file.write('%s\n' % block.desc)
    return


def make_method(file, block):
    """Generate method blocks."""

    #file.write('\\begin{small}')
    file.write('\\begin{verbatim}\n')
    file.write(block.code)
    file.write('\\end{verbatim}\n')
    #file.write('\\end{small}')
    file.write('\\vspace*{-0.5em}\n')
    file.write('%s\n' % block.desc)
    file.write('\\vspace*{0.5em}\n')

    return


def make_data(file, block):
    """Generate data blocks."""

    file.write('\\begin{verbatim}\n')
    file.write(block.code)
    file.write('\\end{verbatim}\n')
    file.write('\\vspace*{-0.5em}\n')
    file.write('%s\n' % block.desc)
    file.write('\\vspace*{0.5em}\n')

    return


if __name__ == '__main__':

    filelist = sys.argv[1:]

    # Split input files into sections
    sections = []
    for file in filelist:
        sections += parse_file(file)

    # Parse each section to generate
    for section in sections:
        blocks = parse_section(section.lines)
        make_tex(section.name + '.tex', section, blocks)
