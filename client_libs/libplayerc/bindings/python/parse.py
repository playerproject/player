#!/usr/bin/env python

import re
import string
import sys


prefixes = ['playerc_client',
            'playerc_device_info',
            'playerc_laser']



class Rule:

    def __init__(self):

        self.patterns = []
        self.replacements = []
        self.head = ''
        self.foot = ''
        return
    

class Replace:
    pass
    


def compile_comment():
    """Compile comment rule."""

    rule = Rule()
    rule.type = 'comment'
    rule.patterns += [re.compile('/\*.*?\*/', re.DOTALL)]

    return [rule]



def compile(prefix):
    """Compute the grammar."""

    rules = []

    # Create rule for typedefs
    #rule = Rule()
    #rule.type = 'typedef'
    #rule.patterns += [re.compile('\s*\}\s*%s_t\s*;' % prefix)]
    #rules += [rule]

    # Create rule for constructor
    rule = Rule()
    rule.type = 'constructor'
    rule.patterns += [re.compile('%s_t\s*\*\s*%s_create\s*\(.*?;' % (prefix, prefix), re.DOTALL)]

    rule.head = '\n%%extend %s\n{\n' % prefix
    rule.foot = '\n}\n'

    rep = Replace()
    rep.src = re.compile('%s_t\s*\*\s*%s_create\s*' % (prefix, prefix))
    rep.dst = '%s' % prefix
    rule.replacements += [rep]

    rep = Replace()
    rep.src = re.compile('\(*\s*%s_t\s*\*\w*\s*\)' % prefix)
    rep.dst = '()'
    rule.replacements += [rep]

    rep = Replace()
    rep.src = re.compile('\(*\s*%s_t\s*\*\w*\s*,\s*' % prefix)
    rep.dst = '('
    rule.replacements += [rep]

    rules += [rule]

    # Create rule for destructor
    rule = Rule()
    rule.type = 'destructor'
    rule.patterns += [re.compile('\w*\s*%s_destroy\s*\(.*?;' % (prefix), re.DOTALL)]

    rule.head = '\n%%extend %s\n{\n' % prefix
    rule.foot = '\n}\n'

    rep = Replace()
    rep.src = re.compile('\w*\s*%s_destroy\s*' % (prefix))
    rep.dst = '~%s' % prefix
    rule.replacements += [rep]

    rep = Replace()
    rep.src = re.compile('\(*\s*%s_t\s*\*\w*\s*\)' % prefix)
    rep.dst = '()'
    rule.replacements += [rep]

    rep = Replace()
    rep.src = re.compile('\(*\s*%s_t\s*\*\w*\s*,\s*' % prefix)
    rep.dst = '('
    rule.replacements += [rep]

    rules += [rule]

    # Create rule for regular functions
    rule = Rule()
    rule.type = 'method'
    rule.patterns += [re.compile('\w*\s*%s_\w*\(.*?;' % prefix, re.DOTALL)]
    rule.patterns += [re.compile('\w*\s*\w*\s*%s_\w*\(.*?;' % prefix, re.DOTALL)]
    rule.patterns += [re.compile('\w*\s*\*%s_\w*\(.*?;' % prefix, re.DOTALL)]

    rule.head = '\n%%extend %s\n{\n' % prefix
    rule.foot = '\n}\n'

    rep = Replace()
    rep.src = re.compile('%s_*' % prefix)
    rep.dst = ''
    rule.replacements += [rep]

    rep = Replace()
    rep.src = re.compile('\(*\s*%s_t\s*\*\w*\s*\)' % prefix)
    rep.dst = '()'
    rule.replacements += [rep]

    rep = Replace()
    rep.src = re.compile('\(*\s*%s_t\s*\*\w*\s*,\s*' % prefix)
    rep.dst = '('
    rule.replacements += [rep]

    rules += [rule]

    return rules



def parse_file(filename, rules):

    file = open(filename, 'r')

    stream = ''
    while 1:
        line = file.readline()
        if not line:
            break
        stream += line

    outstream = ''
    current_struct = None
    
    while stream:

        line = stream
        m = None
        
        for rule in rules:

            # See if this line matches the rule
            for pattern in rule.patterns:
                m = pattern.match(line)
                if m:
                    break
            if not m:
                continue

            func = line[m.start():m.end()]
            
            # Parse comment blocks
            if rule.type == 'comment':
                stream = stream[m.end():]
                outstream += func
                break

            # Parse function name and args
            (name, sig) = string.split(func, '(', 1)
            sig = '(' + sig

            #print name, sig

            # Apply replacement rules to name
            for rep in rule.replacements:
                mm = rep.src.search(name)
                if not mm:
                    continue
                name = name[:mm.start()] + rep.dst + name[mm.end():]

            # Apply replacement rules to signature
            for rep in rule.replacements:
                mm = rep.src.match(sig)
                if not mm:
                    continue
                sig = sig[:mm.start()] + rep.dst + sig[mm.end():]
                
            #print name, sig

            outstream += rule.head
            outstream += name
            outstream += sig
            outstream += rule.foot
            
            stream = stream[m.end():]
            
            break

        # If no rule matches
        if not m:
            outstream += stream[:1]
            stream = stream[1:]

    return outstream
    

if __name__ == '__main__':

    infilename = sys.argv[1]
    #outfilename = sys.argv[2]

    rules = []
    rules += compile_comment()

    for prefix in prefixes:
        rules += compile(prefix)

    outstream = parse_file(infilename, rules)

    # Do some final replacements
    for prefix in prefixes:
        outstream = string.replace(outstream, '%s_t' % prefix, '%s' % prefix)

    guff = ''
    for prefix in prefixes:
        guff += '%%header\n %%{\ntypedef %s_t %s;\n' % (prefix, prefix)
        guff += '#define new_%s %s_create\n' % (prefix, prefix)
        guff += '#define del_%s %s_destroy\n%%}\n' % (prefix, prefix)

    outstream = guff + outstream

    print outstream

