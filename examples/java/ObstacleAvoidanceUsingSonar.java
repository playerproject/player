public class ObstacleAvoidanceUsingSonar {
    static public void main(String args[]) {
	
	// Setup communications with robot
	PlayerClient ant=new PlayerClient("localhost",6665);
	
	// Setup requests for 'ant'
	ant.request("srpw"); // (Sonar, Read), (Position, Write)
	
	// Do obstackle avoidance for 400 time steps (40 sec.)
	int minR, minL;
	for (int i=0; i<400; i++) { 
	    ant.getData(); // Blocking read of data
	    minL=Integer.MAX_VALUE; minR=Integer.MAX_VALUE;
	    for (int j=1; j<4; j++) {
		if (minL>ant.sonar[j]) minL=ant.sonar[j];
	    }
	    for (int j=4; j<7; j++) {
		if (minR>ant.sonar[j]) minR=ant.sonar[j];
	    }
	    int l=(100*minR)/300-100;
	    int r=(100*minL)/300-100;
	    if (l>150) l=150; if (r>150) r=150;
	    ant.setSpeed(r+l,r-l);
	}
    }
}

