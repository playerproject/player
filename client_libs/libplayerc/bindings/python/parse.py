#!/bin/env python

import string
import sys


class DefineDef:
    pass

class StructDef:
    pass

class FunctionDef:
    pass

class ClassDef:
    pass



def parse_file(filename):
    """Parse the file to extract symbols."""

    cpp = Literal('#') + Word('_' + alphanums) + restOfLine 
    cpp_comment = Literal('//') + restOfLine
    extern_c = Literal('extern') + Literal('"C"') + Literal('{')

    # struct foo;
    forward = Literal('struct') + Word('_' + alphanums) + Literal(';')

    # typedef void (*fn) (...);
    typedef_fn = Literal('typedef') + Word('_' + alphanums) + \
                 Literal('(') + Word('*_' + alphanums) + Literal(')') + \
                 CharsNotIn(';') + Literal(';')

    # typedef struct foo {...} foo_t;
    typedef = Literal('typedef') + Literal('struct') + \
              ZeroOrMore(Word('_' + alphanums)).suppress() + Literal('{').suppress() + \
              CharsNotIn('}') + Literal('}').suppress() + \
              Word('_' + alphanums) + Literal(';').suppress()

    # void function(...);
    fn = Group(OneOrMore(Word('_' + alphas, '_' + alphanums) | Literal('*'))) + \
         Combine(Literal('(') + CharsNotIn(';')) + Literal(';').suppress()

    cpp = Group(cpp).setResultsName('cpp')
    typedef = Group(typedef).setResultsName('typedef')
    fn = Group(fn).setResultsName('function')

    #fn.setDebug()

    all = ZeroOrMore(cpp | typedef | fn)
    #all.ignore(fn)
    #all.ignore(typedef)
    #all.ignore(cpp)
    all.ignore(cStyleComment)
    all.ignore(cpp_comment)
    all.ignore(extern_c)
    all.ignore(forward)
    all.ignore(typedef_fn)
    
    tokens = all.parseFile(filename)

    # Suck out different entities.  This is horrible, but I cant get
    # the parser to do this right
    defines = []
    structs = []
    functions = []
    for group in tokens:
        if group[0] == 'typedef':
            struct = StructDef()
            struct.name = group[3]
            struct.body = group[2]
            structs += [struct]
        elif group[0] == '#':
            if group[1] == 'define':
                define = DefineDef()
                define.body = group[2]
                defines += [define]
        else:
            function = FunctionDef()
            function.name = group.asList()[0][-1]
            function.args = group.asList()[1]
            function.ret = string.join(group.asList()[0][0:-1], ' ')
            #print function.ret, function.name, function.args
            #print
            functions += [function]

    return (defines, structs, functions)



def write_swig(filename, defines, structs, functions):
    # Create swig .i file with shadow classes

    file = open(filename, 'w+')
    
    classes = {}
    sorted = []

    for struct in structs:
        tokens = string.split(struct.name, '_')
        classname = string.join(tokens[:-1], '_')
        nclass = ClassDef()
        nclass.index = structs.index(struct)
        nclass.name = classname
        nclass.body = struct.body
        nclass.methods = []
        classes[nclass.name] = nclass
        sorted += [nclass]

    for function in functions:
        tokens = string.split(function.name, '_')

        # Divide function name into class/method names
        classname = '%s_%s' % (tokens[0], tokens[1])
        methodname = string.join(tokens[2:], '_')
        methodret = function.ret
            
        # Handle connstructor
        if methodname == 'create':
            methodname = classname
            methodargs = function.args
            methodret = ''

        # Handle destructor
        elif methodname == 'destroy':
            methodname = '~' + classname
            methodargs = '(void)'
            methodret = ''

        # Handle regular methods (suck out self pointer)
        else:
            tokens = string.split(function.args, ',')
            if len(tokens) == 1:
                methodargs = '(void)'
            else:
                methodargs = '(' + string.join(tokens[1:], ',')
        
        nclass = classes.get(classname, None)
        if nclass:
            function.sname = methodname
            function.sargs = methodargs
            function.sret = methodret
            nclass.methods += [function]
        else:
            function.sname = None

    # Write out any pre-processor stuff
    for define in defines:
        file.write('#define %s\n' % (define.body))

    # Write out any 'naked' functions
    for function in functions:
        if function.sname != None:
            continue
        file.write('%s %s %s;\n' % (function.ret, function.name, function.args))

    # Write out the classes in the same order as the original file
    for nclass in sorted:

        file.write('%header\n%{\n')
        file.write('typedef %s_t %s;\n' % (nclass.name, nclass.name))
        file.write('#define new_%s %s_create\n' % (nclass.name, nclass.name))
        file.write('#define delete_%s %s_destroy\n' % (nclass.name, nclass.name))
        file.write('%}\n')
        file.write('typedef struct\n{\n%s\n' % nclass.body)
        file.write('  %extend\n  {\n')
        for method in nclass.methods:
            file.write('    %s %s%s;\n' % (method.sret, method.sname, method.sargs))
        file.write('  }\n')
        file.write('\n} %s;\n\n\n' % nclass.name)

    return



if __name__ == '__main__':

    infilename = sys.argv[1]
    outfilename = sys.argv[2]

    try:
        from pyparsing import *
    except:
        print 'warning: pyparsing not found; skipping build of [%s]' % outfile

    # Suck out functions and elements
    (defines, structs, functions) = parse_file(infilename)

    # Create swig .i file with shadow classes
    write_swig(outfilename, defines, structs, functions)
    

