public class ObstacleAvoidanceUsingLaser {
    static public void main(String args[]) {
	
	// Setup communications with robot 
	PlayerClient ant=new PlayerClient("localhost",6665);
	
	// Setup requests for 'ant'
	ant.request("lrpa"); // (Laser, Read), (Position, Write)
	
	// Do obstackle avoidance for 400 time steps
	int minR, minL;
	for (int i=0; i<400; i++) { 
	    ant.getData(); // Blocking read of data
	    minL=Integer.MAX_VALUE; minR=Integer.MAX_VALUE;
	    for (int j=0; j<180; j++) {
		if (minR>ant.laser[j]) minR=ant.laser[j];
	    }
	    for (int j=181; j<361; j++) {
		if (minL>ant.laser[j]) minL=ant.laser[j];
	    }
	    int l=(100*minR)/500-100;
	    int r=(100*minL)/500-100;
	    if (l>150) l=150; if (r>150) r=150;
	    ant.setSpeed(r+l,r-l);
	}
    }
}

