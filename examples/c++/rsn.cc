#include <netinet/in.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h> 
#include <playerclient.h>
#include <string.h> /* for strcpy() */
#include <unistd.h>
#include <time.h>

#include <list>

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
int   myindex = -1;
char  index_str[6];


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
	strcpy(index_str,  argv[i]);
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
    printf("give each mote an index\n");
    return -1;
  }

  PlayerClient robot(host,port);
  PositionProxy pp(&robot,0,'a');
  MoteProxy mp(&robot,myindex,'a');
  SonarProxy sp(&robot, 0, 'a');

  int speed = 900, turnrate = 25, cc;
  char buf[10];
  double rssi;

  mp.SetStrength(gen_rssi);
  
  srand(time(NULL));
  
  while(1){ 
    /* will block */
    robot.Read();
    
    /* send our id */
    mp.TransmitRaw(index_str, 1);
    
    do{
      cc = mp.RecieveRaw(buf, 1, &rssi);
      if(cc){
	buf[1] = 0;
	printf("%s heard from %s with rssi %.6f\n", index_str, buf, rssi);
      }      
    }while(cc);
  
    
    if( sp[1] < 1000 ||
	sp[2] < 1000 ||
	sp[3] < 1000 ||
	sp[4] < 1000 ||
	sp[5] < 1000 ||
	sp[6] < 1000){
      printf("object at %d,%d,%d,%d,%d,%d! ah!\n",
	     sp[1],sp[2],sp[3],sp[4],sp[5],sp[6]);
      
      speed = 0;
      turnrate = (rand()%100+100) * (rand()%2)*2-1;
    }
    else{
      speed = rand()%800+200;
      turnrate = rand()%30-30;
    }

    /* write commands to robot */
    pp.SetSpeed(speed,turnrate);

  }
}











