// objects definitions
#if !defined(WIN32)
	#include <unistd.h>
	#include <netinet/in.h>
#endif
#include <string.h>
#include <math.h>
#include <libplayercore/playercore.h>
#include <iostream>
#include <map>  //stl
/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003  
 *     Brian Gerkey, Andrew Howard
 *                      
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/*
 * Desc: A Mapping driver which maps from sonar data
 * from the multidriver example by Andrew Howard
 * Author: Marco Paladini, (mail: breakthru@inwind.it)
 * Date: 29 April 2006
 */
/// objects definitions for mapping driver
#define LOCAL2GLOBAL_X(x,y,px,py,pa) (cos(pa)*(x) - sin(pa)*(y) + px)
#define LOCAL2GLOBAL_Y(x,y,px,py,pa) (sin(pa)*(x) + cos(pa)*(y) + py)
#define MAP_INDEX(map, i, j) (int)((i) + (j) * map.width)
#define MAP_VALID(map, i, j) ((i >= 0) && (i <= (int)map.width) && (j >= 0) && (j <= (int)map.height))
#define ROTATE_X(x,y,th) (cos(th)*(x) - sin(th)*(y))
#define ROTATE_Y(x,y,th) (sin(th)*(x) + cos(th)*(y))

using namespace std;


class Sonar
{
public:
double px,py,th;
double sonar_treshold; //default to 4.5
double sonar_aperture; // 30 degrees
double sensor_model(double x,double y,double r);
	Sonar() 
	{
	sonar_treshold=4.5;
	sonar_aperture=0.5235987;
	}
};

class MAP_POINT
{
public:
int x;
int y; // coordinates on map

MAP_POINT(int x1,int y1)
{
  x=x1;
  y=y1;
}

 bool operator<(const MAP_POINT &b) const {
   if (x < b.x) return(1);
   else if (x == b.x) return(y < b.y);
   else return(0);
 }

};

class MAP_POSE
{
public:
double px; 
double py;
double pa; // where the robot was when this point was added
double P;  // occupancy probability

 MAP_POSE()
   {pa=px=py=P=0;}

MAP_POSE(double px1,
         double py1,
         double pa1,
	 double P1)   {
 pa=pa1;
 px=px1;
 py=py1;
 P=P1;
 }

};


class Map : public map<MAP_POINT,MAP_POSE>
{
///
/// the map is defined as x,y -> pose (px,py,pa,P)
///
public:
int width;
int height;
int startx;
int starty;
float scale; //default to 0.028
float sonar_treshold; //default to 4.5
Map();
Map(int width,
    int height,
    int startx,
    int starty,
    int scale,
    int sonar_treshold);
~Map();
player_map_data_t ToPlayer();
};

Map::~Map() {
}

Map::Map()
{
//some default values (not always good)
width=800;
height=800;
startx=0;
starty=0;
scale=0.028f;
sonar_treshold=4.5;
}

Map::Map(int width,
    int height,
    int startx,
    int starty,
    int scale,
    int sonar_treshold)
{
std::cout<< "not implemented yet" << std::endl;
}



double Sonar::sensor_model(double x,double y,double r)
{
return(
exp((-pow(x,2)/r)-(pow(y,2)/sonar_aperture))/((double)1.7)
);

}
