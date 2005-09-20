#include <stdio.h>

#define MAX_READINGS 769

typedef struct urg_laser_readings
{
	unsigned short Readings[MAX_READINGS];
} urg_laser_readings_t;

class urg_laser
{
	public:
		urg_laser();
		~urg_laser();
		
		void Open(const char * PortName);
		
		
		bool PortOpen();

		int GetReadings(urg_laser_readings_t * readings);
		
				
	private:
		FILE * laser_port;
		
};
