#include <stdio.h>

#define MAX_READINGS 1128

typedef struct urg_laser_readings
{
  unsigned short Readings[MAX_READINGS];
} urg_laser_readings_t;

typedef struct urg_laser_config
{
  /** Start and end angles for the laser scan [rad].*/
  float min_angle;
  /** Start and end angles for the laser scan [rad].*/
  float max_angle;
  /** Scan resolution [rad].  */
  float resolution;
  /** Maximum range [m] */
  float max_range;
  /** Range Resolution [m] */
  float range_res;
  /** Enable reflection intensity data. */
  unsigned char  intensity;
  /** Scanning frequency [Hz] */
  float scanning_frequency;
} urg_laser_config_t;

class urg_laser
{
  public:
  	urg_laser();
	~urg_laser();
		
	int Open(const char * PortName, int use_serial, int baud);
        int Close();
        int ChangeBaud(int curr_baud, int new_baud, int timeout);
	int ReadUntil(int fd, unsigned char *buf, int len, int timeout);
		
	int ReadUntil_nthOccurence(int file, int n, char c);
		
	bool PortOpen();

	int GetReadings     (urg_laser_readings_t * readings, int min_i, int max_i);
	int GetIDInfo       ();
	float GetMaxRange   ();
	int GetSensorConfig (urg_laser_config_t *cfg);
        int GetSCIPVersion() { return(this->SCIP_Version); }
        int GetNumRanges() { return(this->num_ranges); }

  private:
	int QuerySCIPVersion  ();
  	int SCIP_Version;
        int num_ranges;
	FILE * laser_port;
};
