The best way to use this stuff is to perform an out-of-source build. In-source
builds make a mess of the source tree (just like with autotools). Create a
directory somewhere (I make a subdir of the root dir called build/), change to
it and execute "cmake <path to source root>" (for me, "cmake ../"). This will
run the configuration and create the make files for the system in use. Then run
make to build it, and make install to install it

$ cd ~/src/player_cmake/
$ mkdir build
$ cd build/
$ cmake ../ [or ccmake ../ if there are options you want to change]
$ make install

If you want to change build options, you could do it the old-fashioned way
using cmake, or you could replace cmake with ccmake in your command to change
them using an ncurses-based GUI. You might want to change CMAKE_INSTALL_PREFIX
so it doesn't install over your current copy of player.

To get under the hood and see the actual compile / link lines (very helpful
in debugging build problems):

$ make VERBOSE=1

