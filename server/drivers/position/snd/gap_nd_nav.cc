/*
 *      gap_nd_nav.cpp
 *
 *      Copyright 2007 Joey Durham <joey@engineering.ucsb.edu>
 *      modified by:   Luca Invernizzi <invernizzi.l@gmail.com>
 *
 *      lp program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      lp program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with lp program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <libplayercore/playercore.h>
#include <iostream>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <vector>
#if defined (WIN32)
	#if defined (min)
		#undef min
	#endif
	#if defined (max)
		#undef max
	#endif
#else
	#include <sys/times.h>
#endif


#include "dllist.h"
#include "gap_nd_nav.h"
#include "gap_and_valley.h"
#include "snd.h"

//using namespace PlayerCc;

// Use gDebug to set verbosity of output, -1 for silent, 0 for hardly any output, 5 for normal debug
int gDebug=5;




/// Convert radians to degrees
inline double rtod(double r)
{
  return r * 180.0 / M_PI;
}

/// Convert degrees to radians
inline double dtor(double r)
{
  return r * M_PI / 180.0;
}

/// Normalize angle to domain -pi, pi
inline double normalize(double z)
{
  return atan2(sin(z), cos(z));
}




/// Limit a value to the range of min, max
template<typename T>
inline T limit(T a, T min, T max)
{
  if (a < min)
    return min;
  else if (a > max)
    return max;
  else
    return a;
}
















double timeval_subtract( timeval *end, timeval *start )
{
	if( end->tv_usec < start->tv_usec )
	{
		int nsec = (start->tv_usec - end->tv_usec)/1000000 + 1;
		start->tv_usec -= 1000000*nsec;
		start->tv_sec  += nsec;
	}
	if( end->tv_usec - start->tv_usec > 1000000 )
	{
		int nsec = (end->tv_usec - start->tv_usec)/1000000;
		start->tv_usec += 1000000*nsec;
		start->tv_sec  -= nsec;
	}
	
	return (end->tv_sec - start->tv_sec) + (end->tv_usec - start->tv_usec)/1000000.0;
}

bool isRisingGapSafe( Gap* pRisingGap, int iValleyDir, std::vector<double> fullLP, double fScanRes, double fMaxRange, double R )
{
	// TODO: only checks if point creating gap is to close to obstacle on other side ...
	// does not guarantee safe passage through gap
	
	//double fScanRes = lp.GetScanRes();
	//double fMaxRange = lp.GetMaxRange();
	
	int iNumSectors = static_cast<int> (fullLP.size());
	int iRisingGap = pRisingGap->m_iSector;
	double gapDistance = pRisingGap->m_dist;
	
	if (gDebug >1)
	{
		std::cout<< "  Distance to gap at " << iRisingGap << ": " << gapDistance;
		std::cout<< ", " << fullLP[iRisingGap] << std::endl;
	}
	
	double xGap = gapDistance*cos(fScanRes*(iRisingGap - iNumSectors/2));
	double yGap = gapDistance*sin(fScanRes*(iRisingGap - iNumSectors/2));
	
	for( int i = 1; i < iNumSectors/4; i++ )
	{
		int iTestSector = getIndex(iRisingGap + iValleyDir*i, iNumSectors);
		
		if( fullLP[iTestSector] < fMaxRange - 0.01 )
		{
		
			double xI = fullLP[iTestSector]*cos(fScanRes*(iTestSector - iNumSectors/2));
			double yI = fullLP[iTestSector]*sin(fScanRes*(iTestSector - iNumSectors/2));
			
			double dist = sqrt( pow(xGap - xI, 2) + pow(yGap - yI, 2));
			
			if( dist < 2.2*R )
			{
				if (gDebug > 1) std::cout<< "Gap at " << iRisingGap << " ruled out by proximity to obstacle at sector " << iTestSector << std::endl;
				return false;
			}
		}
	}
	
	return true;
}


bool isFilterClear( int iCenterSector, double width, double forwardLength, bool bDoRearCheck, std::vector<double> fullLP, double angRes, bool bPrint )
{
	int iCount = static_cast<int> (fullLP.size());
	//double angRes = lp.GetScanRes();
	for( int i = 0; i < iCount; i++ )
	{
		int iDeltaSec = abs(getSectorsBetween(i,iCenterSector,iCount));
		
		if( iDeltaSec > iCount/4 )
		{
			// Semi-circle behind sensor
			if( bDoRearCheck && (fullLP[i] < width/2.0) )
			{
				if( bPrint && gDebug >= 0 ) std::cout<< "  Filter:  obstacle at sector " << i << " in rear semi-circle" << std::endl;
				return false;
			}
		}
		else
		{
			// Rectangle in front of robot
			double deltaAngle = iDeltaSec*angRes;
			double d1 = (width/2.0)/(sin(deltaAngle));
			double d2 = (forwardLength)/(cos(deltaAngle));
			
			if( fullLP[i] < std::min(d1,d2) )
			{
				if( bPrint && gDebug >= 0 ) std::cout<< "  Filter: obstacle at sector " << i << " in front rectangle" << std::endl;
				return false;
			}
		}
	}
	
	
	return true;
}


void* main_algorithm(void* proxy)
{
	try
	{
	    snd_Proxy& lp	=*(snd_Proxy*) proxy;
	    snd_Proxy& pp	=*(snd_Proxy*) proxy;
	    snd_Proxy& robot	=*(snd_Proxy*) proxy;
		
		double R = robot.robot_radius;
		double minGapWidth = robot.min_gap_width;
		double safetyDistMax = robot.obstacle_avoid_dist;
		double maxSpeed = robot.max_speed;
		double maxTurnRate = robot.max_turn_rate;
		
		double fMaxRange = lp.GetMaxRange();
		double fScanRes = lp.GetScanRes();
		int iNumLPs = lp.GetCount();
		
		if( gDebug >= 0 ) std::cout << "Starting SND driver" << std::endl;
		if( gDebug >= 0 ) std::cout << "Robot radius: " << R << "; obstacle_avoid_dist " << safetyDistMax << std::endl;
		if( gDebug >= 0 ) std::cout << "Pos tol: " << robot.goal_position_tol << "; angle tol " << robot.goal_angle_tol << std::endl;
		
		while( iNumLPs <= 0 || iNumLPs > 100000 || fScanRes <= 0.0 || fScanRes > 1.0 )
		{
			if (gDebug > 0) std::cout << "Waiting for real data" << std::endl;
			robot.Read();
			iNumLPs = lp.GetCount();
			fMaxRange = lp.GetMaxRange();
			fScanRes = lp.GetScanRes();
		}
		
        pthread_testcancel();
		pp.SetMotorEnable(true);
		pp.SetOdometry(0.0,0.0,0.0);
		
		int iNumSectors = (int)round( 2*M_PI/fScanRes +0.5 );
		
		if( gDebug > 0 ) std::cout << "iNumLPs: " << iNumLPs << ", iNumSectors: " << iNumSectors << std::endl;
		
		if( iNumSectors <= 0 || iNumSectors > 100000 || iNumSectors < iNumLPs )
		{
			std::cerr<< "ERROR: Invalid number of sectors " << std::endl;
			return NULL;
		}
		
		if( gDebug > 0 )
		{
			std::cerr<< std::endl;	
			std::cerr<< "Robot at " << pp.GetXPos() << ", " << pp.GetYPos() << std::endl;
			std::cerr<< std::endl;
		}
		
		// Variables that get overwritten in loop
		//player_localize_hypoth_t hypo = localp.GetHypoth(0);
		//player_pose_t pose = hypo.mean;
		pp.RequestGeom();
		
		timeval startTimeval, loopTimeval, endTimeval;
		double diffTime, totalTime;
		
		int loopCount = 0;
		
		double distToGoal;
		double radToGoal;
		double SGoal;
		int iSGoal;
		
		double safetyDist;
		double minObsDist;
		int iSMinObs;
		double di;
		std::vector<double> fullLP(iNumSectors, 0.0);
		std::vector<double> PND(iNumSectors, 0.0);
		
		DLList<Gap*> gapList;
		
		DLLNode<Gap*>* pLoopNode = NULL;
		DLLNode<Gap*>* pNextNode = NULL;
		int iLoop, iNext;
		
		int iDeltaLoop, iDeltaNext;
		DLList<Valley*> valleyList;
		Valley* pValley = NULL;
		Valley* pBestValley = NULL;
		
		int iSTheta;
		double theta, newTurnRate, newSpeed;
		double fullTheta;
		double thetaDes = -99;
		double thetaAvoid = -99;
		
		gettimeofday( &endTimeval, NULL );
		gettimeofday( &startTimeval, NULL );
		
		for(;;)
		{
            if( gDebug > 0 ) PLAYER_MSG0(1,"LOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOP");
			gapList.clear();
			valleyList.clear();
			
			pLoopNode = NULL;
			pNextNode = NULL;
			
			// lp blocks until new data comes; 10Hz by default
            robot.Read();
			
			gettimeofday( &loopTimeval, NULL);
			diffTime = timeval_subtract( &loopTimeval, &endTimeval );
						
			if( gDebug > 2 ) printf("Waited %.4f for data\n",diffTime);
			
			pp.RequestGeom();
			
			// Compute which sector goal is in
            pthread_mutex_lock(&(pp.goal_mutex));
            double goalX = pp.goalX;
            double goalY = pp.goalY;
            double goalA = normalize(pp.goalA);
            pthread_mutex_unlock(&(pp.goal_mutex));
            if( gDebug > 4 ) std::cout<< "Goal at: " << goalX << "," << goalY << std::endl;
            
			distToGoal = std::max(0.01,sqrt( pow(goalX - pp.GetXPos(),2) + pow(goalY-pp.GetYPos(),2)));
			radToGoal = normalize(atan2( (goalY-pp.GetYPos())/distToGoal, (goalX-pp.GetXPos())/distToGoal ) - pp.GetYaw());
			
			if( distToGoal < 1000 )
			{
				if( gDebug > 6 ) std::cout<< "Goal " << distToGoal << "m away at " << rtod(radToGoal) << std::endl;
				SGoal = iNumSectors/2.0 + radToGoal/fScanRes;
				iSGoal = (int)round(SGoal);
			}
			else
			{
				if( gDebug > 4 ) std::cout<< "Using direction mode" << std::endl;
				SGoal = iNumSectors/2.0 + goalA/fScanRes;
				iSGoal = (int)round(SGoal);
			}
			
			if( gDebug > 4 ) std::cout<< "Goal sector = " << iSGoal << "  angle " << rtod(M_PI - 2*M_PI*iSGoal/(1.0*iNumSectors)) << std::endl;
			
			
			// Goal position fulfilled, no need to continue
			if( distToGoal < robot.goal_position_tol )
			{
				if( fabs(normalize(pp.GetYaw() - goalA)) < robot.goal_angle_tol )
				{
					pp.SetSpeed(0.0, 0.0);
					if( gDebug > 4 ) std::cout<< "Reached goal location" << std::endl;
                	pp.WaitForNextGoal();
				
					continue;
				}
				else
				{
					newTurnRate = limit((goalA - pp.GetYaw())/3, -maxTurnRate, maxTurnRate);
					if( gDebug > 4 ) 
					{
						std::cout<< "Spinning to goal angle " << goalA << " from " << pp.GetYaw(); 
						std::cout<< ", tolerance " << robot.goal_angle_tol << ", turn rate " << newTurnRate << std::endl;
					}
					pp.SetSpeed(0.0, newTurnRate);
					
					continue;
				}
			}
			
			// fill out fullLP, like lp but covers 2*M_PI with short distances where there is no data
			for( int i = 0; i < iNumSectors; i++ )
			{
				int lpIdx = i - iNumSectors/2 + iNumLPs/2;
				if( lpIdx >= 0 && lpIdx < iNumLPs )
				{
					fullLP[i] = lp.range(lpIdx);
				}
				else 
				{
					fullLP[i] = fMaxRange;//R + safetyDistMax; 
				}
			}
		
			// Compute PND
			minObsDist = fMaxRange;
			iSMinObs = iNumSectors/2;
			for (int i = 0; i < iNumSectors; i++)
			{
				if( fullLP[getIndex(i,iNumSectors)] >= fMaxRange ) 
				{
					PND[i] = 0;
				}
				else 
				{
					PND[i] = fMaxRange + 2*R - fullLP[getIndex(i,iNumSectors)];
					if( fullLP[getIndex(i,iNumSectors)] < minObsDist )
					{
						minObsDist = fullLP[getIndex(i,iNumSectors)];
						iSMinObs = getIndex(i,iNumSectors);
					}
				}
			}
			
			// Actual safety distance shrinks with proximity of closest obstacle point
			safetyDist = limit( 5*(minObsDist-R), 0.0, safetyDistMax );
			
			if( iNumLPs < iNumSectors )
			{
				// Force right edge of laser to be a left gap
				int idx = iNumSectors/2 - iNumLPs/2;
				if (gDebug > 5) std::cout<< "Forcing left gap at right edge of laser scan: " << idx << std::endl;
				gapList.insertAtEnd( new Gap(getIndex(idx-1,iNumSectors), fullLP[getIndex(idx,iNumSectors)], 1) );
			}
			
			// Find discontinuties, should always be 'located' at the smaller PND, so that valley def is straightforward
			for( int i = 1; i < iNumLPs; i++ )
			{
				int PNDIdx = i + iNumSectors/2 - iNumLPs/2;
				
				di = PND[getIndex(PNDIdx,iNumSectors)] - PND[getIndex(PNDIdx-1,iNumSectors)];
				if( di > minGapWidth )
				{
					if (gDebug > 5)
					{
						std::cout<< "Left gap before " << PNDIdx << ", di " << di;
						if (gDebug > 5) std::cout<< ", pairs " << fullLP[getIndex(PNDIdx-1,iNumSectors)] << ", " << PND[getIndex(PNDIdx-1,iNumSectors)] << "; ";
						if (gDebug > 5) std::cout<< fullLP[getIndex(PNDIdx,iNumSectors)] << ", " << PND[getIndex(PNDIdx,iNumSectors)];
						std::cout<< std::endl;
					}
					
					gapList.insertAtEnd( new Gap(getIndex(PNDIdx-1,iNumSectors), fullLP[PNDIdx], 1) );
				}
				else if( di < -minGapWidth )
				{
					if (gDebug > 5)
					{
						std::cout<< "Right gap at " << PNDIdx << ", di " << di;
						if (gDebug > 5) std::cout<< ", pairs " << fullLP[getIndex(PNDIdx-1,iNumSectors)] << ", " << PND[getIndex(PNDIdx-1,iNumSectors)] << "; ";
						if (gDebug > 5) std::cout<< fullLP[getIndex(PNDIdx,iNumSectors)] << ", " << PND[getIndex(PNDIdx,iNumSectors)];
						std::cout<< std::endl;
					}
					
					gapList.insertAtEnd( new Gap(getIndex(PNDIdx,iNumSectors), fullLP[PNDIdx-1], -1) );
				}
			}
			
			if( iNumLPs < iNumSectors )
			{
				// Force left edge of laser to be a right gap
				int idx = iNumSectors/2 - iNumLPs/2 + iNumLPs;
				if (gDebug > 5) std::cout<< "Forcing right gap at left edge of laser scan: " << idx << std::endl;				
				gapList.insertAtEnd( new Gap(getIndex(idx,iNumSectors), fullLP[getIndex(idx-1,iNumSectors)], -1) );
			}
			
			pLoopNode = gapList.head();
			pNextNode = NULL;
			while ( pLoopNode != NULL )
			{
				pNextNode = gapList.next(pLoopNode);
				if( pNextNode == NULL )
					pNextNode = gapList.head();
				
				iLoop = pLoopNode->m_data->m_iSector;
				iNext = pNextNode->m_data->m_iSector;
				
				if( iLoop == getIndex(iNext - 1,iNumSectors) )
				{
					// Combine any gaps that fall next to each other, going in same direction
					if( pLoopNode->m_data->m_iDir == pNextNode->m_data->m_iDir )
					{
						if( pLoopNode->m_data->m_iDir > 0 )
						{
							// TODO:  Left gap removal should be done left to right to allow > 2 gaps to be combined
							// Keep the right most of the two left gaps
							if( gDebug > 1 ) std::cout<< "Removed duplicate left gap at " << iLoop << std::endl;
							gapList.deleteNode(pNextNode);
						}
						else if( pLoopNode->m_data->m_iDir < 0 )
						{
							// Keep the left most of the two right gaps
							if( gDebug > 1 ) std::cout<< "Removed duplicate right gap at " << iNext << std::endl;
							pLoopNode->m_data->m_iSector = iNext;
							gapList.deleteNode(pNextNode);
						}
					}
				}
				
				pLoopNode = gapList.next(pLoopNode);
			}
			
		
			pLoopNode = NULL;
			pNextNode = NULL;
			
			if (gDebug > 0) std::cout<< "Searching for valleys" << std::endl;

			// Find valleys, gaps must be in angle order with lowest (rightmost) first
			
			pLoopNode = gapList.head();
			while ( pLoopNode != NULL )
			{				
				pNextNode = gapList.next(pLoopNode);
				if( pNextNode == NULL )
				{
					pNextNode = gapList.head();
				}
					
				iLoop = pLoopNode->m_data->m_iSector;
				iNext = pNextNode->m_data->m_iSector;

				if( gDebug > 0 ) 
				{
					std::cout<< "Considering valley between " << iLoop << ", " << iNext;
					std::cout << std::endl;
				}
				
				pValley = NULL;
				if( pLoopNode->m_data->m_iDir < 0 )
				{
					if ( pNextNode->m_data->m_iDir > 0 )
					{
						if( gDebug > 4 ) std::cout<< "Both disc. are rising" << std::endl;
						// Both rising, pick one closest to direction of goal
						iDeltaLoop = abs(getSectorsBetween(iLoop, iSGoal, iNumSectors));
						iDeltaNext = abs(getSectorsBetween(iNext, iSGoal, iNumSectors));
						
						if( iDeltaLoop <= iDeltaNext )
						{
							// Rising gap on the right
							if( isRisingGapSafe( pLoopNode->m_data, 1, fullLP, fScanRes, fMaxRange, R ) )
							{
								pValley = new Valley( pLoopNode->m_data, pNextNode->m_data, 1 );
							}
						}
						else
						{
							// Rising gap on the left
							if( isRisingGapSafe( pNextNode->m_data, -1, fullLP, fScanRes, fMaxRange, R ) )
							{
								pValley = new Valley( pNextNode->m_data, pLoopNode->m_data, -1 );
							}
						}
							
					}
					else 
					{
						if( gDebug > 4 ) std::cout<< "Right is rising" << std::endl;
						if( isRisingGapSafe( pLoopNode->m_data, 1, fullLP, fScanRes, fMaxRange, R ) )
						{
							pValley = new Valley( pLoopNode->m_data, pNextNode->m_data, 1 );
						};
					}
				}
				else 
				{
					if ( pNextNode->m_data->m_iDir > 0 )
					{
						if( gDebug > 4 ) std::cout<< "Left is rising" << std::endl;
						if( isRisingGapSafe( pNextNode->m_data, -1, fullLP, fScanRes, fMaxRange, R ) )
						{
							pValley = new Valley( pNextNode->m_data, pLoopNode->m_data, -1 );
						}
					}
				}
				
				if( pValley != NULL )
				{
					if( gDebug > 0 ) 
					{
						std::cout<< "Found valley between " << iLoop << ", " << iNext;
						std::cout<< " with rising gap at " << pValley->m_pRisingDisc->m_iSector;
						std::cout<< " dir " << pValley->m_iRisingToOther << std::endl;
					}
					
					
					if( false )
					{
						// See if lp valley should be combined with a neighbor
						DLLNode<Valley*>* pValleyLoopNode = valleyList.head();
						Gap* newRight = NULL;
						Gap* newLeft  = NULL;
						
						while ( pValleyLoopNode != NULL )
						{
							Gap* pValleyLeft = pValleyLoopNode->m_data->m_pRisingDisc;
							Gap* pValleyRight = pValleyLoopNode->m_data->m_pOtherDisc;
							if (pValleyLoopNode->m_data->m_iRisingToOther > 0)
							{
								pValleyLeft = pValleyLoopNode->m_data->m_pOtherDisc;
								pValleyRight = pValleyLoopNode->m_data->m_pRisingDisc;
							}
							
							if (pValleyRight->m_iSector == iNext)
							{
								newRight = new Gap(pLoopNode->m_data);
								newLeft  = new Gap(pValleyLeft);
								
								if( gDebug > 0 ) std::cout<< "Left matches right of prior valley!" << std::endl;

								break;
							}
							else if( pValleyLeft->m_iSector == iLoop )
							{
								newRight = new Gap(pValleyRight);
								newLeft  = new Gap(pNextNode->m_data);
								
								if( gDebug > 0 ) std::cout<< "Right matches left of prior valley!" << std::endl;

								break;
							}
							
							pValleyLoopNode = valleyList.next(pValleyLoopNode);
						}
						
						if( newRight != NULL && newLeft != NULL )
						{
							if( newRight->m_iDir < 0 )		
							{
								if ( newLeft->m_iDir > 0 )
								{
									if( gDebug > 8 ) std::cout<< "Both disc. are rising" << std::endl;
									// Both rising, pick one closest to direction of goal
									iDeltaLoop = abs(getSectorsBetween(newRight->m_iSector, iSGoal, iNumSectors));
									iDeltaNext = abs(getSectorsBetween(newLeft->m_iSector, iSGoal, iNumSectors));
									
									if( iDeltaLoop <= iDeltaNext )
									{
										pValleyLoopNode->m_data->overwrite( new Gap(newRight), new Gap(newLeft), 1 );
									}
									else
									{
										pValleyLoopNode->m_data->overwrite( new Gap(newLeft), new Gap(newRight), -1 );
									}
								}
								else 
								{
									if( gDebug > 5 ) std::cout<< "Right is rising" << std::endl;
									pValleyLoopNode->m_data->overwrite( new Gap(newRight), new Gap(newLeft), 1 );
								}
							}
							else 
							{
								if ( newLeft->m_iDir > 0 )
								{
									if( gDebug > 5 ) std::cout<< "Left is rising" << std::endl;
									pValleyLoopNode->m_data->overwrite( new Gap(newLeft), new Gap(newRight), -1 );
								}
							}
							
							if( gDebug > 0 ) 
							{
								std::cout<< "Merging valleys, valley now spans " << newRight->m_iSector << ", " << newLeft->m_iSector;
								std::cout<< " with rising gap at " << pValleyLoopNode->m_data->m_pRisingDisc->m_iSector;
								std::cout<< " dir " << pValleyLoopNode->m_data->m_iRisingToOther << std::endl;
							}

							delete newRight;
							delete newLeft;
							delete pValley;
						}
						else
						{
							valleyList.insertAtEnd( pValley );
						}
					}
					else
					{
						valleyList.insertAtEnd( pValley );	
					}
				}
				
				pLoopNode = gapList.next(pLoopNode);
			}
			
			
			
			// Pick best valley
			DLLNode<Valley*>* pValleyLoopNode = valleyList.head();
			int iPass = 1;
			pBestValley = NULL;
			int iBestStoGoal = iNumSectors;
			while ( pValleyLoopNode != NULL )
			{

				if( iPass == 1 )
				{
					if( iNumLPs >= iNumSectors || !(pValleyLoopNode->m_data->isSectorInValley( 0, iNumSectors )) )
					{
						// Ignore the non-visible valley behind robot on first pass
						int iStoGoal = abs(getSectorsBetween(pValleyLoopNode->m_data->m_pRisingDisc->m_iSector, iSGoal, iNumSectors));
						
						if( iStoGoal < iBestStoGoal )
						{
							iBestStoGoal = iStoGoal;
							pBestValley = pValleyLoopNode->m_data;
							
							if( gDebug > 5 ) 
							{
								std::cout<< "  Pass " << iPass << ": considering valley ";
								std::cout<< pBestValley->m_pRisingDisc->m_iSector << ", " << pBestValley->m_pOtherDisc->m_iSector;
								std::cout<< std::endl;
							}
						}
					}
				}
				else if( iPass == 2 )
				{
					if( pBestValley == NULL || iBestStoGoal > (1 + iNumSectors/4) )
					{
						/*if( pValleyLoopNode->m_data->isSectorInValley( iSGoal, iNumSectors ) )
						{
							// Goal in valley, we're done
							pBestValley = pValleyLoopNode->m_data;
							
							if( gDebug > 5 ) 
							{
								cout<< "  Pass " << iPass << ": valley ";
								cout<< pBestValley->m_pRisingDisc->m_iSector << ", " << pBestValley->m_pOtherDisc->m_iSector;
								cout<< " contains goal" << endl;
							}
							break;
						}*/
						
						// Pick new best if prior best has dot-product with iSGoal < 0
						int iStoGoal = abs(getSectorsBetween(pValleyLoopNode->m_data->m_pRisingDisc->m_iSector, iSGoal, iNumSectors));
						
						if( iStoGoal < iBestStoGoal )
						{
							iBestStoGoal = iStoGoal;
							pBestValley = pValleyLoopNode->m_data;
							
							if( gDebug > 5 ) 
							{
								std::cout<< "  Pass " << iPass << ": considering valley ";
								std::cout<< pBestValley->m_pRisingDisc->m_iSector << ", " << pBestValley->m_pOtherDisc->m_iSector;
								std::cout<< std::endl;
							}
						}
					}
				}
				
				pValleyLoopNode = valleyList.next(pValleyLoopNode);
				if( pValleyLoopNode == NULL && iNumLPs < iNumSectors && iPass == 1 )
				{
					pValleyLoopNode = valleyList.head();
					iPass = 2;
				}
			}
			
			pthread_mutex_lock(&(pp.goal_mutex));
			distToGoal = sqrt( pow(pp.goalX - pp.GetXPos(),2) + pow(pp.goalY-pp.GetYPos(),2));
			pthread_mutex_unlock(&(pp.goal_mutex));
			
			if( minObsDist < R )
			{
				if (gDebug > 0) std::cout<< "!!! Obstacle inside robot radius !!!";
				if (gDebug > 0) std::cout<< "   Stopping.";
				iSTheta = static_cast<int> (fullLP.size()/2);
			}
			else if( pBestValley == NULL )
			{
				// No gaps
				if (gDebug > 0) std::cout<< "No gaps to follow ... ";
				
				// Check if goal is clear (covers the larger empty room case)
				if( isFilterClear( iSGoal, 2*R, std::min(fMaxRange - R, distToGoal - R), false, fullLP, fScanRes, false ) )
				{
					if (gDebug>0)	std::cout<< "clear path to goal" << std::endl;
					
					iSTheta = iSGoal;
				}
				else
				{
					// Nowhere to go ... stay here and spin until a valley comes up
					// write commands to robot
					if (gDebug > 0) std::cout<< "spinning in place" << std::endl;
					
					iSTheta = 0;
				}
			}
			else 
			{
				// Determine scenario robot is in
				
				int iRisingDisc = pBestValley->m_pRisingDisc->m_iSector;
				int iValleyDir = pBestValley->m_iRisingToOther;
				
				// Valley width is smaller of sector distance to next gap or sector distance to
				// first point in valley that is closer to robot than rising gap
				int iValleyWidth = pBestValley->getValleyWidth(fullLP);
				
				if( gDebug > 0 ) std::cout<< "Best valley: " << iRisingDisc << " to " << pBestValley->m_pOtherDisc->m_iSector << ", dir " << iValleyDir << std::endl;
				
				// Find iSsrd, safe direction where robot will head towards rising disc but on the safe side of the corner that created it
				double cornerDist = pBestValley->m_pRisingDisc->m_dist;
				if( gDebug > 0 ) std::cout<< "Adjusted width of valley is " << iValleyWidth << ", with corner dist " << cornerDist << std::endl;
				
				int iSAngle;
				if( cornerDist < (safetyDistMax + R) )
				{
					iSAngle = iNumSectors/4;
				}
				else
				{
					iSAngle = (int)round(asin( limit((safetyDistMax + R)/cornerDist, -1.0, 1.0) )/fScanRes);
				}
				
				// Issue occurs when goal is behind robot, close obstacle at edge of field of view
				// can cause iSsrd to point 90 degrees away from right/left edge of visibility
				// Quick fix: limit iSAngle to less than 1/2 of laser fov
				// TODO: this is kind of a hack
				iSAngle = std::min(iSAngle, iNumLPs/3);
					
				// iSSrd, safe rising discontinuity
				int iSsrd = getIndex(iRisingDisc + iSAngle*iValleyDir, iNumSectors);
				
				// iSMid, middle of valley
				int iSMid = getIndex( iRisingDisc + iValleyDir*((iValleyWidth/2) - 1) , iNumSectors );
				
				int iSt = -1;
				
				// Goal position behavior
				if( iSt < 0 && abs(getSectorsBetween(iSGoal,iNumSectors/2,iNumSectors)) < std::min(iNumSectors/4, iNumLPs/2) )
				{
					// Goal is in front of robot
					if( isFilterClear( iSGoal, 2*R, std::min(fMaxRange - R, distToGoal - R), false, fullLP, fScanRes, false ) )
					{
						if (gDebug>1)	std::cout<< "Clear path to goal" << std::endl;
						iSt = iSGoal;
					}
				}
				
				if( iSt < 0 && abs(getSectorsBetween(iRisingDisc,iSMid,iNumSectors)) < abs(getSectorsBetween(iRisingDisc,iSsrd,iNumSectors)) )
				{
					iSt = iSMid;
				}
				
				if( iSt < 0 ) 
				{
					iSt = iSsrd;
				}
				
				assert( iSt >= 0 && iSt < iNumSectors );
				
				if( gDebug > 0 ) 
				{
					std::cout<< "Best valley has rising disc. at " << iRisingDisc;
					std::cout<< " with iSSrd " << iSsrd << ", iSMid " << iSMid;
					std::cout<< ", iSt " << iSt << std::endl;
				}
					
				double fracExp = 1.0;
				double Sao = 0;
				
				double modS;
				double modAreaSum = 0.0;
				int iDeltaS;
				int iSaoS;
				
				for (int i = 0; i < iNumSectors; i++)
				{
					modS = pow(limit((safetyDist + R - fullLP[i])/safetyDist,0.0,1.0),fracExp);
					iSaoS = getIndex( i + iNumSectors/2, iNumSectors );
					iDeltaS = getSectorsBetween( iSt, iSaoS, iNumSectors );
					
					modAreaSum += modS*modS;
					
					if( false ) 
					{
						// Weighted by "perimeter" method for all-in-one nav
						Sao += modS*modS*iDeltaS;
					}
					else
					{
						// Weighted by "area" method for all-in-one nav
						Sao += modS*modS*modS*iDeltaS;
					}
				}
				
				if( modAreaSum > 0 )
				{
					Sao /= modAreaSum;
				}
				else
				{
					Sao = 0;
				}
				
				if ( gDebug > 0 )
				{
					std::cout<< "Sao " << (int)Sao << ", mod area sum " << modAreaSum << std::endl;
				}
				
				thetaAvoid = fScanRes*(Sao);
				thetaDes = fScanRes*(iSt - iNumSectors/2.0);
				
				iSTheta = (int)round(iSt + Sao);
				
				//iSTheta = getIndex( iSTheta, iNumSectors );
				// Don't let obstacle avoidance change turn direction, we can turn in place
				iSTheta = std::max(0,iSTheta);
				iSTheta = std::min(iNumSectors-1,iSTheta);

			}
			
			theta = fScanRes*(iSTheta - iNumSectors/2.0);
			fullTheta = theta;
			theta = limit( theta, -M_PI/2.0, M_PI/2.0 );
			
			newTurnRate = maxTurnRate*(2.0*theta/M_PI);
			
			theta = limit( theta, -M_PI/4.0, M_PI/4.0 );
			
			newSpeed = maxSpeed;
			newSpeed *= limit(2*distToGoal,0.0,1.0);
			newSpeed *= limit((minObsDist-R)/safetyDistMax,0.0,1.0);
			newSpeed *= limit((M_PI/6.0 - fabs(theta))/(M_PI/6.0),0.0,1.0);

			if( gDebug > 0 ) 
			{
				std::cout<< "Theta: " << theta << " (" << iSTheta << ")";
				std::cout<< ",  Vel:  " << newSpeed;
				std::cout<< ",  Turn: " << newTurnRate;
				std::cout<< std::endl;
			}	
			
			// write commands to robot
			pp.SetSpeed(newSpeed, newTurnRate);
			
			loopCount++;
			
			gettimeofday(&endTimeval, NULL);
			diffTime = timeval_subtract( &endTimeval, &loopTimeval );
			totalTime = timeval_subtract( &endTimeval, &startTimeval );			
			if( gDebug > 2 ) printf("Execution time: %.5f\n",diffTime);
			
			//if( gDebug > 0 ) printf("Data: %f, %f, %f, %f, %f, %f, %f, %f, %f, %f, %f",pp.GetXPos(),pp.GetYPos(),pp.GetYaw(),newSpeed,newTurnRate,fullTheta,thetaDes,thetaAvoid,minObsDist,safetyDist,totalTime);
			
			if( gDebug > 0 ) std::cout<< std::endl;
			
			//usleep( 50000 );
		}
		
	}
	catch (...)
    {
      std::cerr << "Error!  " << std::endl;
      return NULL;		
	}
	
	return NULL;
}
