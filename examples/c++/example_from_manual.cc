#include <playerclient.h>
#include <stdlib.h> /* for exit() */

int main(int argc, char *argv[]) {
  CRobot robot;

  // where tanis is the hostname of the robot
  if(robot.Connect("tanis"))
    exit(1); 

  if(robot.Request("srpw")) 
    exit(1); 

  for(int i=0;i<1000;i++)  
  {
    if(robot.Read())
      exit(1);
    robot.Print();

    if((robot.sonar[0] + robot.sonar[1]) < 
       (robot.sonar[6] + robot.sonar[7])) 
      robot.newturnrate = -20; // turn 20 degrees per second
    else
      robot.newturnrate = 20;

    if(robot.sonar[3] < 500) 
      robot.newspeed = 0;
    else 
      robot.newspeed = 100;

    robot.Write();
  }

  /* close * everything * */
  robot.Request("scgc");
}

