/*
 *      gap_and_valley.cpp
 *
 *      Copyright 2007 Joey Durham <joey@engineering.ucsb.edu>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */



#ifndef GAP_AND_VALLEY_H
#define GAP_AND_VALLEY_H

#include <vector>

extern int getIndex( int circularIdx, int max );
extern int sign( double num );
extern int getSectorsBetween( int iS1, int iS2, int iSMax );
extern int getSectorsBetweenDirected( int iS1, int iS2, int iSMax, int iDirection );
 
class Gap
{
	public :
		int m_iSector;
		double m_dist;
		int m_iDir;
		bool m_bExplored;
		bool m_bContaminated;
		
		Gap();
		Gap( int iSector, double dist, int iDir );
		Gap( Gap* copyFromGap );
		~Gap();
		
		void Update( int iNewSector, double newDist );
		void Update( int iNewSector, double newDist, int iNewDir );
	
 };

// ---------------------------------------------------------------------

class Valley
{
	public :
		Gap* m_pRisingDisc;
		Gap* m_pOtherDisc;
		int m_iRisingToOther;
		
		Valley();
		Valley( Gap* risingGap, Gap* otherGap, int risingToOther );
		virtual ~Valley() {}
		
		void overwrite( Gap* risingGap, Gap* otherGap, int risingToOther );
		
		int getValleyWidth( std::vector<double> fullLP );
		
		bool isSectorInValley( int iSector, int iSMax );
};

// -------------------------------------------------------------------

#endif
