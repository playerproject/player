#!/bin/bash
grep -c "[Cc]opyright" `for ext in .h .hh .hpp .c .cc .cxx .cpp .py .rb .i .java; do find -iname "*$ext"; done | grep -v ^./build` | grep 0$ | sort | sed -e "s/:0//"
