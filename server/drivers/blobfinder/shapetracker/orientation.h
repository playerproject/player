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
#include <opencv/cv.h>


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
