#!/bin/bash

echo $'\n\n'starting $1 "mote(s) at port" $2

for ((index = 0, port = $2, rssi = $3 ; index < ($1) ; index++, port++ )) ; do
   cmd='./rsn -p '$port' -i '$index' -s '$rssi
   echo $cmd
   $cmd&
done
