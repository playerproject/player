#!/bin/bash

echo ""
echo "simulation(  worldfile \"stage1p4_swarmbot.world\" ) "
echo ""

let robot=0
let baseport=$1

while [ $robot -lt $2 ] ; do

echo "robot:$(($baseport+$robot))
(
  position:0( driver \"stg_position\"  model \"sb$robot\"   )
  fiducial:0( driver \"stg_fiducial\" model \"sb$robot\"  )
  blinkenlight:0( driver \"stg_blinkenlight\" model \"sb$robot:light\"  )
  blinkenlight:1( driver \"stg_blinkenlight\" model \"sb$robot:light1\"  )
  blinkenlight:2( driver \"stg_blinkenlight\" model \"sb$robot:light2\"  )
  blinkenlight:3( driver \"stg_blinkenlight\" model \"sb$robot:light3\"  )
  sonar:0( driver \"stg_sonar\" model \"sb$robot\" )
)
"

let robot=$robot+1

done