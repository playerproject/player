define chatterbox model
(
  scount 8
  spose[0] [  0.050  0.0   0 ]
  spose[1] [  0.035  0.035  45 ]
  spose[2] [  0.0    0.050  90 ]
  spose[3] [ -0.035  0.035 135 ]
  spose[4] [ -0.050  0.0 180 ]
  spose[5] [ -0.035 -0.035 225 ]
  spose[6] [  0.0   -0.050 270 ]
  spose[7] [  0.035 -0.035 315 ]

  sbounds[0] [ 0 0.5 ]
  sbounds[1] [ 0 0.5 ]
  sbounds[2] [ 0 0.5 ]
  sbounds[3] [ 0 0.5 ]
  sbounds[4] [ 0 0.5 ]
  sbounds[5] [ 0 0.5 ]
  sbounds[6] [ 0 0.5 ]
  sbounds[7] [ 0 0.5 ]

  # mass in KG
  mass 0.5 

  size [0.10 0.10]
  nose 1

  energy_capacity 500.0
  energy_probe_range 0.6

  # fiducialfinder configuration
  # format [ minrange maxrange maxrange_ids fov ]
  fiducial [ 0.0 2.0 2.0 360.0 ]  

  # blobfinder configuration
  # format [ channels imagewidth imageheight maxrange pan lilt zoom(fov) ]
  blobfinder [ 6 80 60 7.0 0 0 60.0 ]  
  blobchannels [ "red" "green" "blue" "cyan" "yellow" "magenta" ]	
) 