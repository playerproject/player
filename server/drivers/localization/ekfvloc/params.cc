/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2010
 *     Alejandro R. Mosteo
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <math.h>

#include "params.hh"

double kMaxEmptyAngle           = 2.0 * M_PI / 180.0;
double kMaxEmptyDistance        = 0.1;
double kMinRegionLength         = 0.2;
int    kMinPointsInRegion       = 8;
int    kMinPointsInSegment      = 5;
double kConfidence              = 95.0;
double kAlphaILF(void)          { return 1000.0 - 10 * kConfidence; }
bool   kCheckResidual           = false;
double kMaxAngEBE               = 0.0;
double kMinDistBetweenEndpoints = 0.0;
double kMinOdomDistChange       = 0.0;
double kMinOdomAngChange        = 0.0;
long   kMinMillisBetweenScans   = 50;