
# path to the Player source tree
PLAYER_SRC_DIR = ../../..

INCLUDES = -I$(PLAYER_SRC_DIR)/server

all: multidriver.so

%.o: %.cc
	$(CXX) -Wall -fpic -g3 $(INCLUDES) -c $<

%.so: %.o
	$(CXX) -shared -nostartfiles -Wl,-exportdynamic -o $@ $<

clean:
	rm -f *.o *.so
