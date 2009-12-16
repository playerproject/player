%% ===========================================================
%% %% @ Markus Bader, Vienna University of Technology 
%%  Matlab player sample
%%  Wanderer =============================================================

addpath('../mex/')

%% connect to robot
host = 'localhost';
port = 6665;

client = player_client_connect(host, port);
[sonar, sonar_geometry] = player_sonar_connect(client, 0);
pos2d = player_pos2d_connect(client, 0);
laser = player_laser_connect(client, 0);


%% drive
loop = 1;
while (loop < 100)
	player_client_read(client);
	sonar_values = player_sonar(sonar);
	front = sonar_values(4)
	forwardSpeed = 0.1;
	angeleSpeed = 0;
	if ( front < 1 )
		forwardSpeed = 0;
	   	angeleSpeed = 0.1;
	end
	player_pos2d_speed(pos2d, forwardSpeed, angeleSpeed);
	pause(0.2);
	loop = loop +1
end


player_pos2d_disconnect(pos2d);
player_laser_disconnect(laser);
player_sonar_disconnect(sonar);
player_client_disconnect(client);