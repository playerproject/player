ADD_LIBRARY (opaquedriver SHARED opaquedriver.cc)

INCLUDE_DIRECTORIES (${PROJECT_SOURCE_DIR}/client_libs)
ADD_EXECUTABLE (opaque_client opaque.c)
TARGET_LINK_LIBRARIES (opaque_client playerc playerxdr playererror ${PLAYERC_EXTRA_LINK_LIBRARIES})