PLAYERDRIVER_OPTION (wavefront build_wavefront ON)
PLAYERDRIVER_REQUIRE_PKG (wavefront build_wavefront gdk-pixbuf-2.0
    wavefront_includeDir wavefront_libDir wavefront_linkFlags wavefront_cFlags)
PLAYERDRIVER_ADD_DRIVER (wavefront build_wavefront
    "${wavefront_includeDir}" "${wavefront_libDir}" "${wavefront_linkFlags}" "${wavefront_cFlags}"
    plan.c plan_plan.c plan_waypoint.c wavefront.cc heap.c)