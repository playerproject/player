#include <playerclient.h>
#include <stdlib.h> /* for exit() */

int main(int argc, char *argv[]) {
  //PlayerClient* robot = new PlayerClient("localhost");
  PlayerClient robot("localhost");
  //SonarProxy* sp = new SonarProxy(robot,0,'r');
  SonarProxy sp(&robot,0,'r');
  //PositionProxy* pp = new PositionProxy(robot,0,'w');
  P2PositionProxy p2pp(&robot,0,'w');
  PositionProxy &pp = p2pp;

  int newturnrate,newspeed;

  for(int i=0;i<1000;i++)  
  {
    if(robot.Read())
      exit(1);

    // print out sonars for fun
    sp.Print();

    // do simple collision avoidance
    if((sp.ranges[0] + sp.ranges[1]) < 
       (sp.ranges[6] + sp.ranges[7])) 
      newturnrate = -20; // turn 20 degrees per second
    else
      newturnrate = 20;

    if(sp.ranges[3] < 500) 
      newspeed = 0;
    else 
      newspeed = 100;

    // command the motors
    pp.SetSpeed(newspeed,newturnrate);
  }
}

