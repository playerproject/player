#!/bin/bash

let robot=0
let baseport=$1

while [ $robot -lt $2 ] ; do

./swarmbot -p $((baseport+robot)) -n 1 &

let robot=$robot+1

done