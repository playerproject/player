%% ===========================================================
%%  @ Markus Bader, Vienna University of Technology 
%%  Matlab player sample
%%  GoTo using vhf 
%% =============================================================


addpath('../mex/')

%% connect to robot
host = 'localhost';
port = 6665;

client = player_client_connect(host, port);
pos2d = player_pos2d_connect(client, 1);
laser = player_laser_connect(client, 0);


%% drive
loop = 1;
player_pos2d_goto(pos2d,10,0,0);
alpha = [-pi:pi/50:pi];
while (loop < 100)
	pause(.2);
	loop = loop +1
end


player_pos2d_disconnect(pos2d);
player_client_disconnect(client);