#include <netinet/in.h>
#include <list.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h> 
#include <playerclient.h>
#include <string.h> /* for strcpy() */
#include <unistd.h>

#define USAGE \
"\nUSAGE: rsn_wave [-h <host>] [-p <port>] [-m]\n"\
"       -h <host>          : connect to Player on this host\n"\
"       -p <port>          : connect to Player on this TCP port\n"\
"       -s <radio strength>: set the motes radio strength\n"\
"       -i <index>         : the id of this robot\n"\
"       -m                 : turn on motors (be CAREFUL!)"
bool turnOnMotors = false;
char host[256] = "localhost";
int port = PLAYER_PORTNUM;

int   gen_rssi = 10;
int   who = -1;
int   myindex = 0;
char* index_str;


/* easy little command line argument parser */
void parse_args(int argc, char** argv)
{
  int i;

  i=1;
  while(i<argc){
    
    if(!strcmp(argv[i],"-h")){
      if(++i<argc)
	strcpy(host,argv[i]);
      else{
        puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-s")){
      if(++i<argc){
        gen_rssi = atoi(argv[i]);
      }
      else{
        puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-i")){
      if(++i<argc){
        myindex = atoi(argv[i]);
	index_str = argv[i];
      }
      else{
        puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-p")){
      if(++i<argc)
        port = atoi(argv[i]);
      else{
        puts(USAGE);
        exit(1);
      }
    }
    else if(!strcmp(argv[i],"-m")){
      turnOnMotors = true;
    }
    else{
      puts(USAGE);
      exit(1);
    }
    i++;
  }
}

int main(int argc, char **argv)
{
  parse_args(argc,argv);

  if(myindex == -1){
    printf("you should give each mote an id\n");
    return -1;
  }

  PlayerClient  robot(host,port);
  PositionProxy pp(&robot,0,'a');
  MoteProxy     mp(&robot,myindex,'a');
  SonarProxy    sp(&robot,0,'a');
  
  int cc, speed = 900, turnrate = 25;
  char buf[10];
  double rssi;

  mp.SetStrength(gen_rssi);
  
  while(1){ 
    /* will block */
    robot.Read();

#define MIN_IR_DIST  1000
	
    /* simple object avoidance */   
    if((sp[2] < MIN_IR_DIST) ||
       (sp[3] < MIN_IR_DIST) ||
       (sp[4] < MIN_IR_DIST) ||
       (sp[5] < MIN_IR_DIST) ||
       (sp[6] < MIN_IR_DIST)) {
      speed = 0;
      turnrate = 150;
    }	
    else{
      speed = 900;
      turnrate = 25; 
    }

    /* send our id */
    mp.TransmitRaw(index_str, 1);
    
    cc=mp.RecieveRaw(buf, 1, &rssi);
    if(cc){
      printf("%d heard from %s(node, rssi)\n", myindex, buf);
    }

    for(buf[1]=0;cc;cc=mp.RecieveRaw(buf, 1, &rssi)){
      printf("              (%s, %.6f)\n", buf, rssi);
      buf[0] = '-';
    }
    

    /* write commands to robot */
    pp.SetSpeed(speed,turnrate);

  }
}











