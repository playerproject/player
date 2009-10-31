
#Octave
 #CPP   = mkoctfile --mex
#Matlab
 CPP    = mex

#-------------------------------------------#
INCDIR = -I. -I/usr/local/include/player-3.0/
DBG    = -g
OPT    = -O3
CFLAGS = $(INCDIR)
LINKCC   = -L/usr/local/lib/ -lm -lplayerc++ -lm -lboost_thread-mt -lboost_signals-mt

all:  client sonar laser pos2d localize
	cd cpp/ && make all

client: player_client_connect player_client_disconnect player_client_read
sonar: player_sonar_connect player_sonar_disconnect player_sonar 
laser: player_laser_connect player_laser_disconnect player_laser player_laser_range
pos2d: player_pos2d_connect player_pos2d_disconnect player_pos2d player_pos2d_set player_pos2d_speed player_pos2d_enable player_pos2d_goto
localize: player_localize_connect player_localize_disconnect player_localize_particles player_localize_hypothesis


%.o : %.cc
	$(CPP) $(CFLAGS) -c $<
 
player_client_connect: player_client_connect.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_client_disconnect: player_client_disconnect.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_client_read: player_client_read.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_sonar: player_sonar.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_sonar_connect: player_sonar_connect.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_sonar_disconnect: player_sonar_disconnect.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_laser: player_laser.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/
	
player_laser_range: player_laser_range.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_laser_connect: player_laser_connect.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_laser_disconnect: player_laser_disconnect.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_pos2d_connect: player_pos2d_connect.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_pos2d_disconnect: player_pos2d_disconnect.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_pos2d: player_pos2d.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_pos2d_set: player_pos2d_set.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_pos2d_speed: player_pos2d_speed.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_pos2d_goto: player_pos2d_goto.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_pos2d_enable: player_pos2d_enable.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_localize_connect: player_localize_connect.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_localize_disconnect: player_localize_disconnect.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_localize_particles: player_localize_particles.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

player_localize_hypothesis: player_localize_hypothesis.o
	$(CPP) $(LINKCC) $<
	mv $@.mex* ../mex/

clean:
	rm -f  *.o *~ 

clean-all:
	rm -f  *.o *~ 



