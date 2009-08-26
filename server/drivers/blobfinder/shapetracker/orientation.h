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

/******************************************************
 *      orientation - header
 *
 * Description
 *	estimate orientation of the ground object
 *
 ******************************************************
 */

#ifndef __ORIENTATION__H_
#define __ORIENTATION__H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdio.h>
#include <math.h>
#include <cv.h>


//Estimate the length between two points (cvpoint)
double length(CvPoint *pnt1, CvPoint *pnt2);

//estimate direction of line
int deltaline(CvPoint *pnt1, CvPoint *pnt2, char ch);

//estimate total difference between two lines (using delta x and delta y of
//the the two points of a line)
int difference(CvPoint *pnt1, CvPoint *pnt2, CvPoint *pnt3, CvPoint *pnt4, char ch);

//put length values of lines into array
void fillarray(CvSeq *result);

//sort array
void sortarray();

//return centre point of two points
CvPoint centralpoint(CvPoint pnt1, CvPoint pnt2);

//Get the central point between the two parallel lines
CvPoint getcentralpoint(IplImage *image, CvSeq *res);

//pnt1 is center of mass of object, pnt2 is found central point
//between parallel lines
float getorientation(CvPoint pnt1, CvPoint pnt2);

#ifdef __cplusplus
}
#endif

#endif /* __ORIENTATION__H_ */
