#include <playerclient.h>
#include <unistd.h>

int
main(void)
{
  //player_debug_level(0);
  PlayerClient* pc = new PlayerClient("localhost");
  LaserProxy* lp = new LaserProxy(pc,0,'r');
  //LaserProxy* lp = new LaserProxy(pc,0);
  SonarProxy* sp = new SonarProxy(pc,0,'r');
  //MiscProxy* mp = new MiscProxy(pc,0,'r');
  //PositionProxy* pp = new PositionProxy(pc,0,'a');
  //PtzProxy* zp = new PtzProxy(pc,0,'a');
  //GripperProxy* gp = new GripperProxy(pc,0,'a');
  //SpeechProxy* fp = new SpeechProxy(pc,0,'w');
  //LaserbeaconProxy* bp = new LaserbeaconProxy(pc,0,'r');
  //VisionProxy* vp = new VisionProxy(pc,0,'r');

  
  //lp->Configure(-4500,4500,25,1);

  //bp->Configure(3,45,3,2);

  //pp->SetSpeed(100,40);
  //pp->SetMotorState(1);
  //pp->SelectVelocityControl(1);
  //pp->ResetOdometry();
 
  //sp->SetSonarState(0);

  //gp->SetGrip(0x01,0);
  
  //fp->Say("foo bar");

  //int pandir = 1;
  //int tiltdir = 1;
  //int zoomdir = 1;

  //sleep(2);
  //if(lp->Close())
    //exit(1);
  //sleep(2);
  //if(lp->ChangeAccess('r'))
    //exit(1);
  //sleep(2);
  //if(lp->ChangeAccess('a'))
    //exit(1);

  //int freq = 10;
  //pc->SetFrequency(freq);

  pc->SetDataMode(1);

  for(int i=0;i<5;i++)
  {
    sleep(3);
    pc->RequestData();
    if(pc->Read())
      exit(1);
    sp->Print();
  }

  pc->SetDataMode(0);

  for(int i=0;i<1000;i++)
  {
    if(pc->Read())
      exit(1);
    
    //if(!(i % 10))
    //{
      //freq /= 2;
      //pc->SetFrequency(freq);
    //}
    
    //lp->Print();
    sp->Print();
    //mp->Print();
    //pp->Print();
    //zp->Print(); 
    //gp->Print(); 
    //fp->Print();
    //bp->Print();
    //vp->Print();


    //if(abs(zp->pan) > 80)
      //pandir = -pandir;
    //if(abs(zp->tilt) > 20)
      //tiltdir = -tiltdir;
    //if(zp->zoom > 1000 || zp->zoom < 50)
      //zoomdir = -zoomdir;
    //zp->SetCam(zp->pan+(pandir*5),zp->tilt+(tiltdir*5),zp->zoom+(zoomdir*50));
  }

  /*
     pc->Disconnect();

  pc->Connect("localhost");
  for(int i=0;i<10;i++)
  {
    if(pc->Read())
      exit(1);
    puts("read");
    sp->ChangeAccess('r');
    sp->Print();
  }
  */



  return(0);
}
