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

#include <assert.h>
#include <stdlib.h>
#include <iostream>

#include "gap_and_valley.h"

int getIndex( int circularIdx, int max )
{
	while( circularIdx < 0 )
		circularIdx += max;
	return circularIdx%max;
}

int sign( double num )
{
	if( num > 0 )
		return 1;
	if( num < 0 )
		return -1;
	return 0;
}

int getSectorsBetween( int iS1, int iS2, int iSMax )
{
	int iS = getIndex(iS2,iSMax) - getIndex(iS1,iSMax);
	if( abs(iS) < iSMax/2 )
		return iS;
	else
		return sign((double)(-iS))*(iSMax - abs(iS));
}

int getSectorsBetweenDirected( int iS1, int iS2, int iSMax, int iDirection )
{
	assert( iDirection == -1 || iDirection == 1 );
	int iS = iDirection*(getIndex(iS2,iSMax) - getIndex(iS1,iSMax));
	
	while( iS < 0 )
	{
		iS += iSMax;
	}
	
	return iS;
}

// ---------------------------------------------------------------------

Gap::Gap( )
{
	m_iSector = 0;
	m_dist = -1;
	m_iDir = 0;
	m_bExplored = false;
	m_bContaminated = true;	
}
Gap::Gap( int iSector, double dist, int iDir )
{
	m_iSector = iSector;
	m_dist = dist;
	m_iDir = iDir;
	m_bExplored = false;
	m_bContaminated = true;	
}
Gap::Gap( Gap* copyFromGap )
{
	m_iSector = copyFromGap->m_iSector;
	m_dist = copyFromGap->m_dist;
	m_iDir = copyFromGap->m_iDir;
	m_bExplored = copyFromGap->m_bExplored;
	m_bContaminated = copyFromGap->m_bContaminated;	
}
Gap::~Gap( )
{
	
}
void Gap::Update( int iNewSector, double newDist )
{
	Update( iNewSector, newDist, m_iDir );
}
void Gap::Update( int iNewSector, double newDist, int iNewDir )
{
	m_iSector = iNewSector;
	m_dist = newDist;
	m_iDir = iNewDir;
}

// -------------------------------------------------------------------
		
Valley::Valley()
{
	m_pRisingDisc = NULL;
	m_pOtherDisc = NULL;
	m_iRisingToOther = 0;
}
Valley::Valley( Gap* risingGap, Gap* otherGap, int risingToOther )
{
	assert( abs(risingToOther) == 1);
	m_pRisingDisc = new Gap(risingGap);
	m_pOtherDisc = new Gap(otherGap);
	m_iRisingToOther = risingToOther;
}

void Valley::overwrite( Gap* risingGap, Gap* otherGap, int risingToOther )
{
	assert( abs(risingToOther) == 1);
	delete m_pRisingDisc;
	m_pRisingDisc = risingGap;
	delete m_pOtherDisc;
	m_pOtherDisc = otherGap;
	m_iRisingToOther = risingToOther;
}

int Valley::getValleyWidth( std::vector<double> fullLP )
{
	int iSector = getIndex(m_pRisingDisc->m_iSector + m_iRisingToOther, fullLP.size());
	
	while( isSectorInValley(iSector, fullLP.size())  )
	{
		if( fullLP[iSector] < m_pRisingDisc->m_dist )
		{
			std::cout<< "Dist " << fullLP[iSector] << " to sector " << iSector << " less than " << m_pRisingDisc->m_dist << std::endl;
			break;
		}
		
		iSector = getIndex(iSector + m_iRisingToOther, fullLP.size());
	}
	
	return getSectorsBetweenDirected( m_pRisingDisc->m_iSector, iSector, fullLP.size(), m_iRisingToOther );
}

bool Valley::isSectorInValley( int iSector, int iSMax )
{
	return (getSectorsBetweenDirected(m_pRisingDisc->m_iSector, iSector, iSMax, m_iRisingToOther) < getSectorsBetweenDirected( m_pRisingDisc->m_iSector, m_pOtherDisc->m_iSector, iSMax, m_iRisingToOther ));
}

// -------------------------------------------------------------------

