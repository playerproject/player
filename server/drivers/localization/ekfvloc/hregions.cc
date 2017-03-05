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

#include <algorithm>
#include <libplayercore/playercore.h>
#include "hregions.hh"
#include "params.hh"

/*
HRParameters::HRParameters()
{
    maxEmptyAng   = D_MAX_LOST_PNT * RADIANS(D_LASER_ANG_RES);
    alphaReg      = 50; //(1-HRP_ALPHA_HREG/1000)*100 es el nivel de confianza
    minPntReg     = 4;
    minLenReg     = 0.2;

    alphaILF      = 50;//5; (1-HRP_ALPHA_ILF/1000)*100 es el nivel de confianza
    checkResidual = 0;
    minDistEP     = 0;
    maxAngEBE     = 0;
}
*/

/////////////////////////////////////////////////////////////////////////////////
double randomNormal(double mean, double stdDev)
{
    // Returns a random value from a Normal Distribution with the given mean and standard deviation

    double sum            = 0.0;
    const int n           = 20;
    const double half_sum = n * 0.5;
    const double dt_sum   = sqrt(static_cast<double>(n) / 12.0); // sqrt(int) fails to compile on SunCC, cast it to double

    for (int i = 0; i < n; i++)
        sum += rand() / RAND_MAX;

    return (((sum - half_sum) / dt_sum * stdDev) + mean);
}

double computeLengthHRegion(Scan s, int from, int to)
// Computes the distance between the extremes of the HRegion
{
    const Transf x12 = TRel(s.uloc(from).kX(), s.uloc(to).kX());

    return sqrt(pow(x12.tX(), 2) + pow(x12.tY(), 2));
}

HRegion::HRegion(const Scan &s, int idxFrom, int idxTo) : scan_(&s)
{
    endpoints_.push_back(Endpoint(s, idxFrom));
    endpoints_.push_back(Endpoint(s, idxTo));
}

void FindHomogeneousRegions(const Scan &s, RegionsVector *r)
{

    int    from, to;

    r->clear(); // RG_LEN   (*r)             = 0;
    // RG_NUM_EP(*r, RG_LEN(*r)) = 0;

    for (int k = 0; k < s.ScanCount(); /* below */)
    {
        from = k;
        while (k < (s.ScanCount() - 1))
        {
            if (fabs(s.phi(k) - s.phi(k + 1)) > kMaxEmptyAngle)            // HRP_MAX_EMPTY_ANGLE(HRP(*r)))
                break;
            else if (fabs(s.rho(k + 1) - s.rho(k)) > kMaxEmptyDistance)            // in meters!
                break;
            else
                k++;
        }
        to = k;
        const double len = computeLengthHRegion(s, from, to);
        if ((len > kMinRegionLength) && (to - from > kMinPointsInRegion))
        {
            r->push_back(HRegion(s, from, to));
            // RG_NUM_EP(*r, RG_LEN(*r)) = 2;
            // RG_FROM  (*r, RG_LEN(*r)) = from;
            // RG_TO    (*r, RG_LEN(*r)) = to;
            // RG_LEN   (*r)++;
        }
        k++;
    }
}

int farthestPointToEdge(Scan s,
                        int from,
                        int to,
                        double *maxd2,
                        double *residual)
/* Calculates the point with the greater disparity with respect to an edge   */
{
    if ((from < 0) || (from >= s.ScanCount()) || (to < 0) || (to >= s.ScanCount()))
    {
        PLAYER_ERROR3("Wrong uloc access in farthestPointToEdge: (%d)--(%d) (max:%d)", from, to, s.ScanCount());
    }

    const Uloc Lse(
        integrateEndpointsInEdge(
            s.uloc(from),
            s.uloc(to)));

    int        bp = -1;
    double     d2;

    *maxd2 = 0.0;
    *residual = 0.0;

    for (int k = from + 1; k < to; k++)
    {
        d2 = mahalanobis_distance_edge_point(Lse, s.uloc(k));
        *residual += d2;
        if (d2 > *maxd2)
        {
            *maxd2 = d2;
            bp     = k;
        }
    }

    return bp;
}

double calculateResidual(Scan s, int from, int to)
/* Calculates the residual*/
{
    Uloc Lse = integrateEndpointsInEdge(s.uloc(from), s.uloc(to));
    double r = 0;

    for (int k = from + 1; k < to; k++)
        r += mahalanobis_distance_edge_point(Lse, s.uloc(k));

    return r;
}

bool verifyResidualConditions(Scan s, int from, int to, int bp, double r)
{
    /* Verifies whether the new edge improves the representation    */

    double r1, r2;

    r1 = calculateResidual(s, from, bp);
    r2 = calculateResidual(s, bp,   to);

    return (r >= (r1 + r2));
}

bool verifyEndPointsAlignment(Scan s, int from, int to, int bp, double maxAngle)
/* Verifies whether the detected endpoint is aligned with the endpoints */
{
    double x1, y1, xb, yb, x2, y2, phi;

    x1 = s.uloc(from).kX().tX();
    y1 = s.uloc(from).kX().tY();

    xb = s.uloc(bp).kX().tX();
    yb = s.uloc(bp).kX().tY();

    x2 = s.uloc(to).kX().tX();
    y2 = s.uloc(to).kX().tY();

    phi = spAtan2(y2 - yb, x2 - xb) - spAtan2(yb - y1, xb - x1);

    return (fabs(phi) >= maxAngle);
}

double computeDistanceEndPoints(Scan s, int from, int to)
// Computes the distance between two endpoints
{
    Transf x12 = TRel(s.uloc(from).kX(), s.uloc(to).kX());

    return sqrt(pow(x12.tX(), 2) + pow(x12.tY(), 2));
}

void HRegion::IterativeLineSplit(int fromIdx, int toIdx)
/* Performs one iteration of the iterative line fitting algorithm */
{
    double m;
    double residual;

    const int b = farthestPointToEdge(*scan_, fromIdx, toIdx, &m, &residual);

    PLAYER_MSG1(9, "      Farthest idx: %d", b);
    
    if ((b >= fromIdx) &&
        (m > CHISQUARE[0][COLUMN(kAlphaILF())]) &&
        (kCheckResidual ? verifyResidualConditions(*scan_, fromIdx, toIdx, b, residual) : true) &&
        verifyEndPointsAlignment(*scan_, fromIdx, toIdx, b, kMaxAngEBE))
    {
        const double dfb = computeDistanceEndPoints(*scan_, fromIdx, b);
        const double dbt = computeDistanceEndPoints(*scan_, b,       toIdx);
        
        if ((dfb > kMinDistBetweenEndpoints) && (dbt > kMinDistBetweenEndpoints))
        {
            PLAYER_MSG3(9, "      Splitting: %3d -- %3d -- %3d", fromIdx, b, toIdx);
            PLAYER_MSG2(9, "        because: [m > CHI] [%5.3f > %5.3f]", m, CHISQUARE[0][COLUMN(kAlphaILF())]);
            endpoints_.push_back(Endpoint(*scan_, b));
            // Endpoints are not sorted here, so they will have to be at the end
            
            IterativeLineSplit(fromIdx, b);
            IterativeLineSplit(b,       toIdx);
        }
        else
            PLAYER_MSG0(9, "Split rejected because dist between endpoints");
    }
    else
        PLAYER_MSG0(9, "Split rejected outright");
}

void HRegion::IterativeLineSplit(void)
{
    if (endpoints_.size() != 2)
        PLAYER_ERROR1("Initial endpoints %d != 2", endpoints_.size());

    PLAYER_MSG2(8, "   SPLITTING INDEXES %3d -- %3d",
                endpoints_[0].idx(), endpoints_[1].idx());
                
    IterativeLineSplit(endpoints_[0].idx(), endpoints_[1].idx());
    sort(endpoints_.begin(), endpoints_.end());
}

void IterativeLineFitting(const Scan &s, RegionsVector *r)
/*****************************************************************************/
/* Performs the iterative line fitting algorithm in the set of detected      */
/* Homogeneous Regions                                                       */
/*****************************************************************************/
{
    for (int i = 0; i < (int) r->size(); i++)
    {
        PLAYER_MSG1(8, "SPLITTING REGION %d", i);
        r->at(i).IterativeLineSplit();
    }

    PLAYER_MSG2(5, "SCAN %d -- %d", 0, s.ScanCount());

    for (int i = 0; i < (int) r->size(); i++)
    {
        r->at(i).PushGuiData(&GUI_DATA);
        
        PLAYER_MSG1(6, "REGION %02d", i);
        for (int j = 0; j < (int) r->at(i).NumEps(); j++)
        {
            PLAYER_MSG2(7, "   EP %02d: %d", j, r->at(i).Ep(j).idx());
        }
    }
}

void HRegion::PushGuiData(GuiData *gui_data) const
{
    gui_data->regions.push_back(GetGuiRegion());
    
    for (size_t i = 0; i < endpoints_.size() - 1; i++)
        gui_data->splits.push_back(GetGuiSplit(i));
}

GuiRegion HRegion::GetGuiRegion(void) const
{
    const int ini = endpoints_.front().idx();
    const int fin = endpoints_.back().idx();

    return GuiRegion(scan_->rho(ini), scan_->phi(ini),
                     scan_->rho(fin), scan_->phi(fin));
}

GuiSplit HRegion::GetGuiSplit(int i) const
{
    const int ini = endpoints_.at(i).idx();
    const int fin = endpoints_.at(i + 1).idx();
    
    return GuiSplit(scan_->rho(ini), scan_->phi(ini),
                    scan_->rho(fin), scan_->phi(fin));
}