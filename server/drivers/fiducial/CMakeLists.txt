PLAYERDRIVER_OPTION (laserbar build_laserbar ON)
PLAYERDRIVER_ADD_DRIVER (laserbar build_laserbar SOURCES laserbar.cc)

PLAYERDRIVER_OPTION (laserbarcode build_laserbarcode ON)
PLAYERDRIVER_ADD_DRIVER (laserbarcode build_laserbarcode SOURCES laserbarcode.cc)

PLAYERDRIVER_OPTION (laservisualbarcode build_laservisualbarcode OFF "Has not been updated to use dynamic message structures; will segfault.")
PLAYERDRIVER_ADD_DRIVER (laservisualbarcode build_laservisualbarcode SOURCES laservisualbarcode.cc)

PLAYERDRIVER_OPTION (laservisualbw build_laservisualbw OFF "Has not been updated to use dynamic message structures; will segfault.")
PLAYERDRIVER_ADD_DRIVER (laservisualbw build_laservisualbw SOURCES laservisualbw.cc)
