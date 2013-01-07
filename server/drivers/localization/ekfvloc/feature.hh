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

#ifndef FEATURE_H_
#define FEATURE_H_


#define MAX_OBS_FEATURES 100

#include <vector>
#include "scan.hh"
#include "types.hh"
#include "uloc.hh"

class Feature
{
public:
    /// Defaults to EDGE
    Feature(GeometricEntityKinds entity_kind);
    virtual ~Feature();

    double dimension() const;
    double codimension() const;
    const Transf& Loc() const;

    const Matrix& Cov() const;
    double Cov(int i, int j) const;
    void SetCov(const Matrix& c) { uloc_.SetCov(c); };

    const Uloc& uloc(void) const { return uloc_; };
    void set_uloc(const Uloc& u) { uloc_ = u; };

    void ComputeSegmentLength(Uloc p1, Uloc p2);

    void SetScan(const GuiSplit &split) { split_ = split; };
    // Keep the original raw data for debug
    const GuiSplit &GetScan(void) const { return split_; };
private:
    void GeometricRelationsObservationPointToPoint(Uloc Lsp1, Uloc Lsp2);

    double dimension_;
    double codimension_;
    Uloc   uloc_;

    GuiSplit split_;
};


class ObservedFeatures
{
public:
    ObservedFeatures();
    virtual ~ObservedFeatures();

    void AddObservedFeature(Feature f);
    int Count() const;
    //void SetPaired(int i, bool b);
    const Feature & features(int f) const;
    
    void Clear(void) { features_.clear(); }
    
private:
    vector<Feature> features_;
    vector<bool>    is_paired_;
};

void ScanDataSegmentation(const Scan &laser_raw_data, ObservedFeatures *feat_table);

#endif /* FEATURE_H_ */
