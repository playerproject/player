# If Player isn't installed to /usr, you'll have to edit this path to point
# to the location of UserPlayerC++.cmake and UsePlayerPlugin.cmake
SET (CMAKE_MODULE_PATH "/usr/local/share/cmake/Modules")

# These files include macros that make building against Player easier.  They provide
# the PLAYER_ADD* macros
INCLUDE (UsePlayerC++)
INCLUDE (UsePlayerPlugin)

# This command adds an example PlayerC++ client called 'exampleclient', using
# the 'exampleclient.cc' source file.
PLAYER_ADD_PLAYERCPP_CLIENT (exampleclient SOURCES exampleclient.cc)

# You can install the example client system-wide, using the INSTALL command.  The
# path is relative to CMAKE_INSTALL_PREFIX which defaults to /usr/local.  This will
# install the exampleclient to /usr/local/bin
INSTALL(exampleclient bin)

# If you also want to include a Player plugin driver, use these directions:
# This command adds an example plugin driver which will be called 'libexampledriver.so'
# which uses the source file 'exampleplugin.cc'
# PLAYER_ADD_PLUGIN_DRIVER (exampledriver SOURCES exampleplugin.cc)

