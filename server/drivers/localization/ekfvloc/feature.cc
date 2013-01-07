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

#include "feature.hh"
#include "hregions.hh"
#include "params.hh"
#include "sub_opt.hh"

Feature::Feature(GeometricEntityKinds entity_kind) :
    uloc_(entity_kind),
    split_(0, 0, 0, 0)
{
    dimension_   = 0;
    codimension_ = 0;
}

Feature::~Feature()
{
}

double Feature::dimension() const
{
    return dimension_;
}

double Feature::codimension() const
{
    return codimension_;
}

const Transf& Feature::Loc() const
{
    return uloc_.kX();
}

const Matrix& Feature::Cov() const
{
    return uloc_.kCov();
}

double Feature::Cov(int i, int j) const
{
    return uloc_.kCov()(i, j);
}

ObservedFeatures::ObservedFeatures()
{
}

ObservedFeatures::~ObservedFeatures()
{
}

void ObservedFeatures::AddObservedFeature(Feature f)
{
    features_.push_back(f);
    is_paired_.push_back(false);
}

int ObservedFeatures::Count() const
{
    return features_.size();
}

/*
void ObservedFeatures::SetPaired(int i, bool b)
{
    is_paired_[i] = b;
}
*/

const Feature & ObservedFeatures::features(int f) const
{
    return features_[f];
}

void IntegrateScanPoint(Uloc segment, Uloc point, Matrix &Fk, Matrix &Nk)
{
    Matrix hk = Matrix(0, 0);
    Matrix Hk = Matrix(0, 0);
    Matrix Gk = Matrix(0, 0);
    Matrix Sk = Matrix(0, 0);

    const Transf xep = Compose(Inv(segment.kX()), point.kX());

    Matrix bep = Matrix(1, 3);
    bep(0, 1) = 1;

    const Matrix hp = bep * xep;
    hk.Include(hp, 0, 0);

    const Matrix Hp = -bep * J1zero(xep) * ~segment.kBind();
    Hk.Include(Hp, 0, 0);

    const Matrix Gp = bep * J2zero(xep) * ~point.kBind();
    Gk.Include(Gp, 0, 0);

    Sk.Include(point.kCov(), 0, 0);

    EIFnn(Hk, Gk, hk, Sk, Fk, Nk);
}

void IntegrateScanPoints(Uloc *seg, Scan sTbl, int pFrom, int pEnd, int step)
{
    Matrix FkTotal = Matrix(2, 2);
    Matrix NkTotal = Matrix(2, 1);

    Matrix Nk, Fk;

    for (int pk = pFrom; pk < pEnd; pk += step)
    {
        IntegrateScanPoint(*seg, sTbl.uloc(pk), Fk, Nk);
        FkTotal = FkTotal + Fk;
        NkTotal = NkTotal + Nk;
    }

    IntegrateScanPoint(*seg, sTbl.uloc(pEnd), Fk, Nk);
    FkTotal = FkTotal + Fk;
    NkTotal = NkTotal + Nk;

    Matrix Mean(FkTotal.RowNo(), 1);
    CalculateEstimationEIFnn(FkTotal, NkTotal, Mean, seg->Cov());

    seg->Pert()(0, 0) = Mean(0, 0);
    seg->Pert()(1, 0) = Mean(1, 0);

    seg->CenterUloc();
}

void Feature::GeometricRelationsObservationPointToPoint(Uloc Lsp1, Uloc Lsp2)
{

    Transf xp1p2 = TRel(Lsp1.kX(), Lsp2.kX());

    double phi1 = atan2(xp1p2.tY(), xp1p2.tX());
    double phi2 = phi1 - xp1p2.tPhi();

    dimension_ = sqrt(pow(Lsp2.kX().tX() - Lsp1.kX().tX(), 2) +
                      pow(Lsp2.kX().tY() - Lsp1.kX().tY(), 2));

    codimension_ =
            Lsp1.kCov()(0, 0) * pow(cos(phi1), 2) +
            Lsp1.kCov()(1, 1) * pow(sin(phi1), 2) +
            Lsp2.kCov()(0, 0) * pow(cos(phi2), 2) +
            Lsp2.kCov()(1, 1) * pow(sin(phi2), 2);
}

void Feature::ComputeSegmentLength(Uloc pnt1, Uloc pnt2)
{
    GeometricRelationsObservationPointToPoint(pnt1, pnt2);
}

void ComputeSegments(Scan sTbl, const RegionsVector &rTbl, ObservedFeatures *mTbl)
{
    Feature seg = Feature(EDGE);
    int pFrom, pTo;

    for (int hr = 0; hr < (int)rTbl.size(); hr++)
    {
        for (int ep = 0; ep < rTbl.at(hr).NumEps() - 1; ep++)
        {
            pFrom = rTbl[hr].Ep(ep).idx();     // RG_EP(rTbl, hr, ep);
            pTo   = rTbl[hr].Ep(ep + 1).idx(); // RG_EP(rTbl, hr, ep+1);
            // ALEX: the +1,-1 wasn't originally there. This way we take a
            //   conservative shorter observed segment, but the former way,
            //   induced errors with the misaligned splitting extra scan point.
            //   Since I'm not sure which of the two endpoints will be mis-
            //   aligned, better safer than sorry.

            if (pTo - pFrom < kMinPointsInSegment) // ALEX ADDED
                continue;

            // Calculate segment pFrom -- pTo
            seg.set_uloc(CalculateAnalyticalEdge(sTbl.uloc(pFrom).kX(),
                                                 sTbl.uloc(pTo).kX()));

            {
                Uloc u = seg.uloc();
                IntegrateScanPoints(&u, sTbl, pFrom, pTo, 1);
                seg.set_uloc(u);
            }

            // This is critical: it adjusts segment co-variance realistically.
            // Without this, Kalman filter won't work properly.
            {
                Uloc u = seg.uloc();
                IntegrateScanPoints(&u, sTbl, pFrom, pTo, pTo - pFrom);
                seg.SetCov(u.kCov());
            }

            seg.ComputeSegmentLength(sTbl.uloc(pFrom), sTbl.uloc(pTo));

            seg.SetScan(GuiSplit(sTbl.rho(pFrom), sTbl.phi(pFrom),
                                 sTbl.rho(pTo),   sTbl.phi(pTo)));

            //Add feature to Obs_Features
            mTbl->AddObservedFeature(seg);
        }
    }
}

void ScanDataSegmentation(const Scan &laser_raw_data, ObservedFeatures *feat_table)
{
    RegionsVector HomRegionsTable;

    FindHomogeneousRegions(laser_raw_data, &HomRegionsTable);
    IterativeLineFitting(laser_raw_data, &HomRegionsTable);
    
    feat_table->Clear();
    ComputeSegments(laser_raw_data, HomRegionsTable, feat_table);
}

