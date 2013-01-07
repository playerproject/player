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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */


#ifndef PARAMS_HH_
#define PARAMS_HH_

// fine tuning

extern double kMaxEmptyAngle; // = 2.0 * M_PI / 180.0;
// Max angle without scans for breaking into hregions
// This originally was defined as number_of_points * angular_resolution

extern double kMaxEmptyDistance; // = 0.1;
// As above but with euclidean distance
// This originally was hardcoded into the actual check

extern double kMinRegionLength; // = 0.2;
// Minimum size for a region

extern int kMinPointsInRegion; // = 8;
// Minimum number of hits in a region

extern int kMinPointsInSegment; // = 5; // Less than this will cause crashes
// Minimum number of hits in a single segment

extern double kConfidence; // = 95.0;
// Confidence for split test of segments

double kAlphaILF(void); // = 1000.0 - 10 * kConfidence;
// const double kAlphaILF = 50.0;
// Original: alphaILF = 50; //5; (1-HRP_ALPHA_ILF/1000)*100 es el nivel de confianza

extern bool kCheckResidual; // = false;
// Additional check, dunno what it does (amosteo) TODO: ask mayte

extern double kMaxAngEBE; // = 0.0;
// TODO: ask mayte what this is

extern double kMinDistBetweenEndpoints; // = 0.0;
// TODO: verify this setting is good with mayte

extern double kMinOdomDistChange; // = 0.05;
extern double kMinOdomAngChange; //  = 0.08;

extern long kMinMillisBetweenScans; // = 50;
// Process scans no faster than this

const double kTruthWarnDistance = 1.0;
// Deviation from ground truth that will trigger a warning message

#endif
