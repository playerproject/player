
# path to the Player source tree
PLAYER_SRC_DIR = ../../..

INCLUDES = -I$(PLAYER_SRC_DIR)/server

all: exampledriver.so

%.o: %.cc
	$(CXX) -Wall -g3 $(INCLUDES) -c $<

%.so: %.o
	$(CXX) -shared -nostartfiles -o $@ $<

clean:
	rm -f *.o *.so
