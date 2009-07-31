/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  Brian Gerkey et al
 *                      gerkey@usc.edu
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


#include "orientation.h"
#include <stdio.h>

#define ORIENT_ARRAY_SIZE 12

int points[ORIENT_ARRAY_SIZE][2];
double lines[ORIENT_ARRAY_SIZE];

CvSeq *result;

//Estimate the length between two points (cvpoint)
double length(CvPoint *pnt1, CvPoint *pnt2)
{
	int a,b;
	a = (pnt1->x - pnt2->x);
	b = (pnt1->y - pnt2->y);
	return sqrt(a*a + b*b);
}


//estimate direction of line
int deltaline(CvPoint *pnt1, CvPoint *pnt2, char ch)
{
    if (ch == 'x') 
        return abs(pnt2->x - pnt1->x);
    if (ch == 'y')
        return abs(pnt2->y - pnt1->y);
}


//estimate total difference between two lines (using delta x and delta y of
//the the two points of a line)
int difference(CvPoint *pnt1, CvPoint *pnt2, CvPoint *pnt3, CvPoint *pnt4, char ch)
{   
    if (ch == 'x')
      return abs(deltaline(pnt1, pnt2, 'x') - deltaline(pnt3, pnt4, 'x'));
     if (ch == 'y')
        return abs(deltaline(pnt1, pnt2, 'y') - deltaline(pnt3, pnt4, 'y'));
}

    

//put length values of lines into array
void fillarray(CvSeq *result)
{
    int i;
    for (i = 0; i < ORIENT_ARRAY_SIZE; i++) {
        lines[i] = length(((CvPoint*)cvGetSeqElem(result,i,0)) , ((CvPoint*)cvGetSeqElem(result,i+1,0)));       

        //fill points[] with which points are connected for the lines
        points[i][0] = i;
        points[i][1] = i + 1;
    }
    lines[ORIENT_ARRAY_SIZE - 1] = length(((CvPoint*)cvGetSeqElem(result,ORIENT_ARRAY_SIZE-1,0)) , ((CvPoint*)cvGetSeqElem(result,0,0)));
    points[ORIENT_ARRAY_SIZE -1][0] = ORIENT_ARRAY_SIZE -1;
    points[ORIENT_ARRAY_SIZE -1][1] = 0; 
}


//sort array
void sortarray()
{
    double hold;
    int hold2,hold3;
    int pass, i;
    for (pass = 0; pass < ORIENT_ARRAY_SIZE - 1; pass++) //passes
        for (i = 0; i < ORIENT_ARRAY_SIZE - 1; i++) {
            if ( lines[i] > lines[i + 1] ) {
                //switch lines[]
                hold = lines[i];
                lines[i] = lines[i + 1];
                lines[i + 1] = hold;
                
                //switch points[]
                hold2 = points[i][0];
                hold3 = points[i][1];
                points[i][0] = points[i + 1][0];
                points[i][1] = points[i + 1][1];
                points[i + 1][0] = hold2;
                points[i + 1][1] = hold3;
            }
        }
}



//return centre point of two points
CvPoint centralpoint(CvPoint pnt1, CvPoint pnt2)
{
    CvPoint temp;
    temp.x = (int) (fabs(pnt1.x - pnt2.x) / 2) + min(pnt1.x, pnt2.x);
    temp.y = (int) (fabs(pnt1.y - pnt2.y) / 2) + min(pnt1.y, pnt2.y);
    return temp;
}


//Get the central point between the two parallel lines
CvPoint getcentralpoint(IplImage *image, CvSeq *res) 
{   
    CvPoint *pnt1, *pnt2, *pnt3, *pnt4, *pnt5, *pnt6;
    double dif1, dif2, dif3;
    int dif;
    CvPoint tmpPnt1, tmpPnt2, tmpPnt3;
    fillarray(res);
    sortarray();
    if(ORIENT_ARRAY_SIZE == 8){
    
        //get the points
        pnt1 = (CvPoint*)cvGetSeqElem(res,points[ORIENT_ARRAY_SIZE-3][0],0);
        pnt2 = (CvPoint*)cvGetSeqElem(res,points[ORIENT_ARRAY_SIZE-3][1],0);
        pnt3 = (CvPoint*)cvGetSeqElem(res,points[ORIENT_ARRAY_SIZE-2][0],0);
        pnt4 = (CvPoint*)cvGetSeqElem(res,points[ORIENT_ARRAY_SIZE-2][1],0);
        pnt5 = (CvPoint*)cvGetSeqElem(res,points[ORIENT_ARRAY_SIZE-1][0],0);
        pnt6 = (CvPoint*)cvGetSeqElem(res,points[ORIENT_ARRAY_SIZE-1][1],0);
        //get directions of the lines
        dif1 = difference(pnt1, pnt2, pnt3, pnt4, 'x') + difference(pnt1, pnt2, pnt3, pnt4, 'y'); 
        dif2 = difference(pnt1, pnt2, pnt5, pnt6, 'x') + difference(pnt1, pnt2, pnt5, pnt6, 'y'); 
        dif3 = difference(pnt3, pnt4, pnt5, pnt6, 'x') + difference(pnt3, pnt4, pnt5, pnt6, 'y'); 
    
        //get lines who run parallel to each other
        //dir3 = line 1 and 2
        //dir4 = line 1 and 3
        //dir5 = line 2 and 3
        if ((dif1 <= dif2) && (dif1 <= dif3)) 
            dif = 3;
        if ((dif2 <= dif1) && (dif2 <= dif3))
            dif = 4;
        if ((dif3 <= dif1) && (dif3 <= dif2))
            dif = 5;
            
        //estimate middle point between two parallel lines
        if (dif == 3) {
            tmpPnt1 = centralpoint(*pnt3, *pnt4);
            tmpPnt2 = centralpoint(*pnt1, *pnt2);
            tmpPnt3 = centralpoint(tmpPnt1, tmpPnt2);
        }
        if (dif == 4) {
            tmpPnt1 = centralpoint(*pnt5, *pnt6);
            tmpPnt2 = centralpoint(*pnt1, *pnt2);
            tmpPnt3 = centralpoint(tmpPnt1, tmpPnt2);
        }
        if (dif == 5) {
            tmpPnt1 = centralpoint(*pnt3, *pnt4);
            tmpPnt2 = centralpoint(*pnt5, *pnt6);
            tmpPnt3 = centralpoint(tmpPnt1, tmpPnt2);
        }
    }
    if(ORIENT_ARRAY_SIZE == 12){
        pnt3 = (CvPoint*)cvGetSeqElem(res,points[ORIENT_ARRAY_SIZE-2][0],0);
        pnt4 = (CvPoint*)cvGetSeqElem(res,points[ORIENT_ARRAY_SIZE-2][1],0);
        tmpPnt3 =  centralpoint(*pnt3, *pnt4);
    }
    //print center point in image
    //cvCircle(image, tmpPnt1, 10, CV_RGB(100,100,100), 2);

    //return the central point
    return tmpPnt3;
}


//pnt1 is center of mass of object, pnt2 is found central point
//between parallel lines
float getorientation(CvPoint pnt1, CvPoint pnt2)
{
    //here pnt1.y - pnt2.y because y is flipped in image (the LOWER in the
    //image the BIGGER the y
    double heading = atan2((pnt1.y - pnt2.y), (pnt2.x - pnt1.x)) * 180.0 / (M_PI);
    if(ORIENT_ARRAY_SIZE == 12){
        //convert heading from +/- 180 to 0 to 180 (symmetric H)
        if(heading < 0.0)
            heading = 180.0 + heading;
        //Now convert to +/- 90 for the helicopter
        if(heading > 90.0)
            heading = heading - 180;
    }
    return heading;
}


