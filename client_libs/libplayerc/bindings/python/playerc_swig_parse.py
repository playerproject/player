#!/usr/bin/env python
#/*
# *  Player - One Hell of a Robot Server
# *  Copyright (C) 2004
# *     Andrew Howard
# *                      
# *
# *  This library is free software; you can redistribute it and/or
# *  modify it under the terms of the GNU Lesser General Public
# *  License as published by the Free Software Foundation; either
# *  version 2.1 of the License, or (at your option) any later version.
# *
# *  This library is distributed in the hope that it will be useful,
# *  but WITHOUT ANY WARRANTY; without even the implied warranty of
# *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# *  Lesser General Public License for more details.
# *
# *  You should have received a copy of the GNU Lesser General Public
# *  License along with this library; if not, write to the Free Software
# *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
# */
 
import re
import string
import sys

# Global array of PlayerStruct objects (defined below) to hold information about
# structure definitions.
structures=[]

# Accessor functions to allow access to dynamic arrays (e.g. playerc_laser_t.ranges).
# accessor_header goes in playerc_wrap.h, accessor_interface goes in playerc_wrap.i.
accessor_header="""
typedef struct
{
    TYPE *actual;
} TYPEArray;
"""

accessor_interface="""
typedef struct
{
    TYPE *actual;

    %extend {
        TYPE __getitem__(int index) {return $self->actual[index];}
        void __setitem__(int index,TYPE value) {$self->actual[index]=value;}
    }
} TYPEArray;
"""

# Class for holding information about a detected structure, including a header
# section for preprocessor directives and typedefs and a 'members' section for
# external functions that become members.
class PlayerStruct:
    name=''
    header=""
    members=""
    
    def __init__(self,initName):
        self.name=initName
        
    def __eq__(self,other):
        if isinstance(other,str):
            return self.name==other
        return NotImplemented

# Called for every function that matches the regex in main.  If successful,
# the function will be removed, temporarily stored in a 'PlayerStruct' class,
# and later reconstituted as a member function.
def memberize(match):
    # Used to remove the first parameter from a list of function parameters.
    # The first parameter to a potential member function is a pointer to the structure
    # to work with, which is not necessary after the transformation.
    lastParams=re.compile('\(\s*\w+?.*?,\s*(?P<last>.*)\)',re.DOTALL)
    funcName=match.group('name')
    # The start of the loop checks chunks of the function name for a recognizable structure
    # name.  e.g. playerc_some_class_do_some_action will try playerc_some, playerc_some_class
    # and so on until a match is found.
    objName="playerc"
    for i in range(1,funcName.count("_")):
        objName=funcName[:funcName.find("_",len(objName)+1)]
        action=funcName[len(objName)+1:]
        # Once a match is found, transform the external function to a member function.
        if objName in structures:
            struct=structures[structures.index(objName)]
            # e.g. playerc_client_create
            if action=="create":
                # add the function to a structure
                struct.members+="\t\t" + objName + " " + match.group('params') + ";\n"
                # add a preprocessor directive to ensure swig picks up the old library
                # function name as a constructor.
                struct.header+="\t#define new_" + objName + " " + funcName + "\n"
            # e.g. playerc_client_destroy
            elif action == "destroy":
                # add the destructor to the structure.  No destructors take parameters
                # or return a value.
                struct.members+="\t\tvoid destroy(void);\n"
                struct.header+="\t#define del_" + objName + " " + funcName + "\n"
            # e.g. playerc_client_connect
            else:
                # set 'last' to every parameter in the parameter list except the first.
                # if no match, there are less than two parameters, in which case the
                # transformed function will have no parameters (void).
                lm=lastParams.match(match.group('params'))
                if lm:
                    last=lm.group('last')
                else:
                    last="void"
                # add the function prototype to the structure.
                struct.members+="\t\t" + match.group('retval') + " " + action + " (%s);\n" % last
                # functions that are not constructors or destructors already match the required SWIG pattern,
                # so no preprocessor directive is required.
            # Since the function is a member function and will now have a prototype
            # inside an 'extend' clause in the structure definition, remove the external
            # reference by substituting with ''
            return ''
    # If the function is not a member function of a structure, take no action.
    return match.group()

# This function is called for every structure.  It attempts to find dynamically
# allocated arrays of simple types (which are not subscriptable in python with
# standard binding) and wrap them in an accessor structure defined at the top of this file.
def accessorize(match):
    # 'text' is the new code for the matched structure (to be returned)
    text=match.group('decl')+"{"
    # 'body' is everything within the {}s of the structure definition
    body=match.group('body');
    # 'arrayMember' is a regex designed to match certain member definitions,
    # e.g. 'double *ranges;' (playerc_laser_t)
    arrayMember=re.compile(r"\b(?P<type>int|double|float)\s*[*]\s*(?P<name>\w+);",re.MULTILINE)
    
    # performs a substitution on the member definitions described above,
    # e.g. 'double *ranges;' becomes 'doubleArray ranges;',
    # and appends the modified struct body to the code.
    text+=arrayMember.sub("\g<type>Array \g<name>;",body)
    
    # terminate the modified structure and add the 'footer', usually just the end
    # of the typedef.
    text+="}"+match.group('footer')
    return text;

# Called for every structure in the input stream.  Modifies the code block to include
# an appropriate 'header' section with typedefs and preprocessor directives,
# and adds new member function prototypes in a SWIG 'extend' block at the end of the
# structure.
def genifacestruct(match):
    # ensure the structure has previously been scanned (should always be the case)
    if match.group('name') in structures:
        # retrieve the data about the matched structure
        struct=structures[structures.index(match.group('name'))]
        # add the header block and a typedef to permit the change from structname_t to structname
        # (for clarity in python)
        text="%header\n%{\n"+struct.header+"\ttypedef " + struct.name+"_t "+struct.name+";\n%}\n\n"
        # add the structure declaration and the body, omitting a closing brace
        text+=match.group('decl')+"{"+match.group('body')
        # add the extension block with member function protoypes, plus a closing brace.
        text+="\t%extend\n\t{\n"+struct.members+"\t}\n}"
        # add the remainder of the original structure definition (usually just the typedef name)
        text+=match.group('footer')
        return text
    # if no match, print a statement to that effect and return the original structure definition.
    # should never happen.
    print "No match on %s" % match.group('name')
    return match.group() 

# Used to remove _t from structure references.
# Had to do this due to ambiguity with typedef statements and callback function prototypes.
# e.g.   typedef void (*playerc_putmsg_fn_t) (void *device, char *header, char *data);
# also   typedef player_blobfinder_blob_t playerc_blobfinder_blob_t;
# This function ensures that only structures defined in playerc.h are modified.
def underscore(match):
    if match.group('name') in structures:
        return match.group('name')
    return match.group()

if __name__ == '__main__':

    # should always be playerc.h
    infilename = sys.argv[1]
    # should always be 'playerc_wrap'
    outfilename = sys.argv[2]

    # Read in the entire file
    file = open(infilename, 'r')
    ifaceStream = file.read()
    file.close()

    # Pattern to remove all double-spacing (used later)
    blank=re.compile('([ \t\f\v\r]*\n){3,}',re.MULTILINE)

    # Remove all comments (multi- and single-line).
    comment=re.compile('((/\*.*?\*/)|(//.*?$))', re.DOTALL|re.MULTILINE)
    ifaceStream=comment.sub('',ifaceStream)

    # Pattern to recognise structure definitions
    struct=re.compile("""
        (?P<decl>typedef\s+struct\s*\w*\s*)  # Declaration
        {(?P<body>.*?)}(?P<footer>\s*        # Body
        (?P<name>playerc_\w+?)(_t)?          # Name
        \s*;)
    """, re.DOTALL|re.VERBOSE)
    
    # Match and replace all structure definitions with versions that have
    # accessor structures for dynamically allocated arrays using the 'accessorize'
    # function above.
    ifaceStream=struct.sub(accessorize,ifaceStream)
    
    # remove all double-spacing (pattern defined above)
    ifaceStream=blank.sub('\n\n',ifaceStream)

    # output to new header file.  It is necessary to modify the original header file to
    # allow for data types of structure members to be changed.  Pointers are modified
    # to structures containing a pointer, to allow the member to be subscriptable in python.
    # see comments on the 'accessorize' function above.  
    file=open("%s.h" % outfilename,"w")
    # write accessor structure definitions for arrays of ints, doubles and floats.
    for type in ["int","double","float"]:
        file.write(accessor_header.replace("TYPE",type))
    # write the modified header file.
    file.write(ifaceStream)
    file.close()
    
    # find the names of all the structures defined in the file, and make
    # a list of objects to store information about the structures.
    # could be done as part of 'accessorize' but left here for clarity.
    am=struct.finditer(ifaceStream)
    for m in am:
        structures+=[PlayerStruct(m.group('name'))]

    # Pattern to match function prototypes that could be changed to 'member functions'
    # of structures, allowing object-oriented style calling from Python.
    function=re.compile("""
        \s*    
        ((?P<retval>                  # Capture the return type:
            \w+\s*?(                  # Type name, is a 
            \** |                     # pointer OR
            &?  |                     # reference OR
            (\[\s*[0-9]*\s*\])        # array.
            )
        )\s*)?                        # Only one (potential) return value.
        (?P<name>                     # Capture function name
            playerc_\w+               # Class name and member function name
        )\s*
        (?P<params>\(.*?\))           # Parameters
        \s*;
    """, re.DOTALL|re.VERBOSE)

    # remove the _t from the end of all the struct references.
    structNames=re.compile(r"\b(?P<name>playerc_\w+)_t\b")
    ifaceStream=structNames.sub(underscore,ifaceStream)

    # using the 'memberize' function, find functions that should be struct 'members' and shift them.
    ifaceStream=function.sub(memberize,ifaceStream)

    # Using the gathered information about functions and structures, rewrite the structure
    # definitions in the interface file to include required extensions (function prototypes), 
    # typedefs and preprocessor directives.  see comments on 'genifacestruct' function above.
    ifaceStream=struct.sub(genifacestruct,ifaceStream)

    # remove all double spacing.
    ifaceStream=blank.sub('\n\n',ifaceStream)
    
    # write the SWIG interface definition file.
    file=open("%s.i" % outfilename,"w")
    # write interfaces for the accessor structs including subscripting functions.
    for type in ["int","double","float"]:
        file.write(accessor_interface.replace("TYPE",type))
    file.write(ifaceStream)
    file.close()
    