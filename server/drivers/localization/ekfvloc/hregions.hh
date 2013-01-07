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

#ifndef HREGIONS_H_
#define HREGIONS_H_

#include <vector>
#include "scan.hh"
#include "types.hh"

/////////////////////////////////////////////////////////
#define DF      (20)
#define NALPHAS (6)

const double
CHISQUARE[DF][NALPHAS] =
{ { 7.88, 6.63, 5.02, 3.84, 2.71, 1.32 },
    { 10.6, 9.21, 7.38, 5.99, 4.61, 2.77 },
    { 12.8, 11.3, 9.35, 7.81, 6.25, 4.11 },
    { 14.9, 13.3, 11.1, 9.49, 7.78, 5.39 },
    { 16.7, 15.1, 12.8, 11.1, 9.24, 6.63 },
    { 18.5, 16.8, 14.4, 12.6, 10.6, 7.84 },
    { 20.3, 18.5, 16.0, 14.1, 12.0, 9.04 },
    { 22.0, 20.1, 17.5, 15.5, 13.4, 10.2 },
    { 23.6, 21.7, 19.0, 16.9, 14.7, 11.4 },
    { 25.2, 23.2, 20.5, 18.3, 16.0, 12.5 },
    { 26.8, 24.7, 21.9, 19.7, 17.3, 13.7 },
    { 28.3, 26.2, 23.3, 21.0, 18.5, 14.8 },
    { 29.8, 27.7, 24.7, 22.4, 19.8, 16.0 },
    { 31.3, 29.1, 26.1, 23.7, 21.1, 17.1 },
    { 32.8, 30.6, 27.5, 25.0, 22.3, 18.2 },
    { 34.3, 32.0, 28.8, 26.3, 23.5, 19.4 },
    { 35.7, 33.4, 30.2, 27.6, 24.8, 20.5 },
    { 37.2, 34.8, 31.5, 28.9, 26.0, 21.6 },
    { 38.6, 36.2, 32.9, 30.1, 27.2, 22.7 },
    { 40.0, 37.6, 34.2, 31.4, 28.4, 23.8 }
};

#define COLUMN(a)  ((a) <= 5 ? 0 : ((a) <= 10 ? 1 : ((a) <= 20 ? 2 : ((a) <= 50 ? 3 : ((a) <= 100 ? 4 : 5)))))
#define RADIANS(d) ((d)*M_PI/180.0)

// see params.hh
// const int    D_MAX_LOST_PNT  = 4;
// const double D_LASER_ANG_RES = 0.5;

class Endpoint
{
public:
    Endpoint(const Scan &s, int idx) : idx_(idx) {}

    const int idx(void) const    { return idx_; }

    bool operator < (const Endpoint &rhs) const { return idx_ < rhs.idx_; }

private:
    int idx_;
};

typedef std::vector<Endpoint> EndpointsVector;

class HRegion
{
public:
    HRegion(const Scan &s, int idxFrom, int idxTo);
    virtual ~HRegion() {}
    
    const int NumEps(void) const { return endpoints_.size(); }
    const Endpoint &Ep(int i) const { return endpoints_.at(i); }

    void IterativeLineSplit(int fromIdx, int toIdx);
    void IterativeLineSplit(void); // Initiator of recursion

    void PushGuiData(GuiData *gui_data) const;
private:
    GuiRegion GetGuiRegion(void) const;
    GuiSplit GetGuiSplit(int i) const;
    
    const Scan *scan_;
    EndpointsVector endpoints_;
};

typedef std::vector<HRegion> RegionsVector;

/*
#define	RG_LEN(r)		((r).len)
#define RG_EP2(r, i)    ((r).ep[i])
#define RG_NUM_EP(r, i) ((r).ep[i].len)
#define RG_EP(r, i, j)  ((r).ep[i].idx[j])
#define RG_FROM(r, i)   ((r).ep[i].idx[0])
#define RG_TO(r, i)     ((r).ep[i].idx[(r).ep[i].len-1])
*/

/*
#define HRP(p)					((p).par)
#define HRP_MAX_EMPTY_ANGLE(p)	((p).maxEmptyAng)
#define HRP_ALPHA_HREG(p)		((p).alphaReg)
#define HRP_MIN_PNT_REG(p)		((p).minPntReg)
#define HRP_MIN_LEN_REG(p)		((p).minLenReg)

#define HRP_ALPHA_ILF(p)		((p).alphaILF)
#define HRP_CHECK_RESIDUAL(p)	((p).checkResidual)
#define HRP_MIN_DIST_EP(p)		((p).minDistEP)
#define HRP_MAX_EBE_ANG(p)		((p).maxAngEBE)
*/

void FindHomogeneousRegions(const Scan &s, RegionsVector *r);
void IterativeLineFitting(const Scan &s, RegionsVector *r);

#endif /* HREGIONS_H_ */
