#!/usr/bin/env python

import os.path
import re
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
        self.name = None
        self.desc = ''
        self.code = []
        return

    def __str__(self):
        return '<Type>\n%s\n<Name>\n%s\n<Desc>\n%s\n<Code>\n%s\n' % \
               (self.type, self.name, self.desc, self.code)


def print_error(msg, filename, line):
    """Print a parser errors."""

    sys.stderr.write('error (%s) in file %s:%d\n' % (msg, filename, line))
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


    blocks = []

    index = 0
    while index < len(section):

        line = section[index]
        stripped = string.strip(line)
        tokens = string.split(stripped)

        # Parse C-style comments
        if tokens and tokens[0] == '/**':

            # Parse the comment bit
            block = Block()
            index = parse_comment_c(section, index, block)

            # Parse the code bit
            if block:
                index = parse_code(section, index, block)
                blocks.append(block)

        index += 1
            
    return blocks


def parse_comment_c(section, index, block):
    """Parse a C-style comment."""

    while index < len(section):
        line = section[index]
        stripped = string.strip(line)

        text = stripped

        if text[:3] == '/**':
            text = text[3:]
        if text[-2:] == '*/':
            text = text[:-2]

        block.desc += string.strip(text) + ' '
        index += 1
        
        if stripped[-2:] == '*/':
            break

    return index


def parse_code(section, index, block):
    """Parse a code block."""

    while index < len(section):
        line = section[index]
        stripped = string.strip(line)

        if not stripped:
            break
        if stripped[:3] == '/**':
            index -= 1
            break
        if stripped[:3] == '///':
            index -= 1
            break

        code = stripped
        if code[-1:] == '{':
            code = code[:-1]
        #if code[-1:] == ';':
        #    code = code[:-1]

        if code:
            block.code.append(code)
            #if block.code:
            #    block.code += '\\\\\n'
            #block.code += code

        if stripped[-1:] == '{':
            break

        index += 1

    return index



def guess_block(section, block):
    """Guess some of the fields in the doc block."""

    if block.code:
        code = block.code[0]
        tokens = string.split(code)

        if len(tokens) >= 2 and tokens[0] == 'class':
            block.type = 'class'
            block.name = tokens[1]

        elif len(tokens) >= 3 and tokens[:2] == ['typedef', 'struct']:
            block.type = 'struct'
            block.name = tokens[2]

        elif len(tokens) >= 1 and tokens[0] == '#define':
            block.type = 'define'

        elif string.find(code, '(') > 0:
            block.type = 'method'

        elif tokens:
            block.type = 'data'        

    # Look for headings
    if block.type == None:
        m = re.compile('\[.*\]').search(block.desc)
        if m:
            block.name = block.desc[m.start()+1:m.end()-1]
            block.desc = block.desc[0:m.start()] + block.desc[m.end():]

    # Do some munging of the block description, like escaping _.
    block.desc = string.replace(block.desc, '\_', '_')
    block.desc = string.replace(block.desc, '_', '\_')

    # Do some munging of the block code, like getting rid of excess stuff.
    #if string.find(block.code, '{') >= 0:
    #    block.code = block.code[0:string.find(block.code, '{')]
    #if string.find(block.code, ';') >= 0:
    #    block.code = block.code[0:string.find(block.code, ';')]

    # Get rid of leading access modifiers and inline code
    #if string.find(block.code, ':') >= 0:
    #    l = block.code[0:string.find(block.code, ':')]
    #    r = block.code[string.find(block.code, ':') + 1:]
    #    if l == 'public':
    #        block.code = r
    #    else:
    #        block.code = l

    # Strip leading and trailing spaces
    #block.code = string.strip(block.code)

    return


def make_tex(filename, section, blocks):
    """Generate latex from the doc blocks."""

    texfile = open(filename, 'w+')

    make_section(texfile, section)

    index = 0
    while index < len(blocks):
        block = blocks[index]

        if block.type == None:
            make_text(texfile, section, block)
            index += 1
        elif block.type == 'define':
            index = make_defines(texfile, section, blocks, index)
        elif block.type == 'struct':
            index = make_struct(texfile, section, blocks, index)
        else:
            index += 1

    return


def make_section(file, section):
    """Generate a section entry."""

    file.write('\\section{%s}\n\n' % section.name);
    return


def make_text(file, section, block):
    """Generate a plain text entry."""

    if block.name:
        file.write('\\subsection*{%s}\n' % block.name)
    file.write('%s\n' % block.desc)
    return 


def make_defines(file, section, blocks, index):

    file.write('\\begin{center}')
    file.write('\\begin{footnotesize}')
    file.write('\\begin{tabularx}{\\columnwidth}{XX}\n')
    file.write('\\hline\n')
    file.write('Value & Meaning \\\\\n')
    file.write('\\hline\n')

    while index < len(blocks):
        block = blocks[index]
    
        if block.type == 'define':
            for line in block.code:
                file.write('\\verb+%s+ ' % line)
            file.write('& %s \\\\ \n' % block.desc)
            index += 1
        else:
            break

    file.write('\\hline\n')
    file.write('\\end{tabularx}\n')
    file.write('\\end{footnotesize}')
    file.write('\\end{center}')
    return index


def make_struct(file, section, blocks, index):
    """Generate a class entry."""

    block = blocks[index]

    file.write('\n\n\\vspace{1em}\\noindent \\verb+%s+ : %s' % (block.name + '_t', block.desc));
    
    index += 1

    file.write('\\begin{center}')    
    file.write('\\begin{footnotesize}')
    file.write('\\begin{tabularx}{\\columnwidth}{XX}\n')
    file.write('\\hline\n')
    file.write('Value & Meaning \\\\\n')
    file.write('\\hline\n')

    while index < len(blocks):
        block = blocks[index]
    
        if block.type == 'data':
            for line in block.code:
                file.write('\\verb+%s+ ' % line)
            file.write('& %s \\\\ \n' % block.desc)
            index += 1
        else:
            break

    file.write('\\hline\n')
    file.write('\\end{tabularx}\n')
    file.write('\\end{footnotesize}')
    file.write('\\end{center}')
    return index


def make_class(file, section, block):
    """Generate a class entry."""

    file.write('\\section{%s}\n\n' % section.name);
    file.write('%s\n' % block.desc)
    return


def make_method(file, section, block):
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


def make_data(file, section, block):
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

        for block in blocks:
            guess_block(section, block)
            print block
        
        make_tex(section.name + '.tex', section, blocks)
