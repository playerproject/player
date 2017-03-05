/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2010
 *     Mayte Lázaro, Alejandro R. Mosteo
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


#ifndef SEG_MAP_H_
#define SEG_MAP_H_

#include <vector>
#include "transf.hh"

class SegmentMap
{
public:
    SegmentMap();
    SegmentMap(string filename);
    virtual ~SegmentMap();

    void AddSegment(double x1, double y1, double x2, double y2);

    bool IsEmpty(void) const;

    int NumSegments(void) const;

    Transf segments(int i) const;
    double lengths(int i) const;

private:
    vector<Transf> segments_;
    vector<double> lengths_;
};

#endif /* SEG_MAP_H_ */
