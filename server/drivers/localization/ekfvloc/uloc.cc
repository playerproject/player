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

#include "uloc.hh"

Uloc::Uloc(GeometricEntityKinds ge) {
    // TODO Auto-generated constructor stub
    x_ = Transf();
    entity = ge;
    if (ge == POINT) {
        b_ = Matrix(2, 3);
        b_(0, 0) = 1;
        b_(1, 1) = 1;
        p_ = Matrix(2, 1);
        c_ = Matrix(2, 2);
    }
    else if (ge == EDGE) {
        b_ = Matrix(2, 3);
        b_(0, 1) = 1;
        b_(1, 2) = 1;
        p_ = Matrix(2, 1);
        c_ = Matrix(2, 2);
    }
    else if (ge == ROBOT) {
        b_ = Matrix(3, 3);
        b_.Unit();
        p_ = Matrix(3, 1);
        c_ = Matrix(3, 3);
    }
    else {
        cerr << "Uloc: Undefined entity name\n";
        exit(1);
    }
}

Uloc::~Uloc() {
    // TODO Auto-generated destructor stub
}

GeometricEntityKinds Uloc::uGEntity() {
    return entity;
}

void Uloc::SetLoc(Transf loc) {
    x_ = loc;
}

void Uloc::SetPert(Matrix pert) {
    p_ = pert;
}

void Uloc::SetBind(Matrix bind) {
    b_ = bind;
}

void Uloc::SetCov(Matrix cov) {
    c_ = cov;
}

Transf Uloc::DifferentialLocation() {
    // Returns the differential location vector of the uncertain location.
    Matrix df = ~b_ * p_;
    return Transf(df);
}

Uloc inverse_uloc(Uloc Lab) {
    Uloc Lba(Lab.uGEntity());

    Lba.SetLoc (Inv(Lab.kX()));
    Lba.SetBind(Lab.kBind());

    Matrix Jab = Jacobian(Lab.kX());
    Matrix J   = Lba.kBind() * Jab * ~Lba.kBind();

    Lba.SetPert(J * Lab.kPert());
    Lba.SetCov (J * Lab.kCov() * ~J);

    return Lba;
}

void Uloc::CenterUloc() {
    // Centers the uncertain location u, so that the perturbation equals zero.

    Transf de = DifferentialLocation();

    x_ = Compose(x_, de);

    Matrix D = b_ * InvJ2zero(de) * ~b_;
    c_ = D * c_ * ~D;

    if (entity == POINT)
        p_ = Matrix(2, 1);
    if (entity == EDGE)
        p_ = Matrix(2, 1);
    if (entity == ROBOT)
        p_ = Matrix(3, 1);
}

Uloc compose_uloc_transf(Uloc Lwf, Transf Xfe) {
    // Chages the associated reference of Lwf, as given by Xfe, giving Lwe.

    Uloc Lwe(Lwf.uGEntity());

    Lwe.SetLoc(Compose(Lwf.kX(), Xfe));
    Matrix J   = Lwf.kBind() * InvJacobian(Xfe) * ~Lwf.kBind();
    Lwe.Pert() = J * Lwf.kPert();
    Lwe.Cov()  = J * Lwf.kCov () * ~J;

    return Lwe;
}

Uloc compose_uloc(Uloc Lwf, Uloc Lfe) {
    // Composes two (independent!) uncertain locations Lwf and Lfe, giving Lwe.
    Uloc Lwe(Lwf.uGEntity());
    Lwe.Loc() = Compose(Lwf.kX(), Lfe.kX());

    Matrix BeJefBft = Lfe.kBind() * InvJacobian(Lfe.kX())   * ~Lwf.kBind();
    Lwe.Pert()      = BeJefBft    * Lwf.kPert()             + Lfe.kPert();
    Lwe.Cov()       = BeJefBft    * Lwf.kCov()  * ~BeJefBft + Lfe.kCov();

    return Lwe;
}

Uloc compose_transf_uloc(Transf Xwf, Uloc Lfe) {
    //Changes the base reference of Lfe as indicated by Xwf, giving Lwe.

    Uloc Lwe(Lfe.uGEntity());

    Lwe.Loc()  = Compose(Xwf, Lfe.kX());
    Lwe.Pert() = Lfe.kPert();
    Lwe.Cov()  = Lfe.kCov();

    return Lwe;
}

double mahalanobis_distance_edge_point(Uloc Lwe, Uloc Lwp) {

    Transf xep = TRel(Lwe.kX(), Lwp.kX());

    return pow(xep.tY(), 2)
            / (
                    Lwe.kCov()(0, 0)
                    + xep.x() * (2 * Lwe.kCov()(0, 1) + xep.x() * Lwe.kCov()(1, 1))
                    + Lwp.kCov()(0, 0) * pow(sin(xep.phi()), 2)
                    + Lwp.kCov()(1, 1) * pow(cos(xep.phi()), 2)
              );
}

Uloc CalculateAnalyticalEdge(Transf xp1, Transf xp2) {
    // Estimates the reference attached to an edge from two of its points with the x-axis pointing from p1 to p2

    Uloc Lse = Uloc(EDGE);
    ;
    Lse.Loc() = Transf((xp1.x() + xp2.x()) / 2,
                       (xp1.y() + xp2.y()) / 2,
                       atan2(xp2.y() - xp1.y(),
                             xp2.x() - xp1.x()));

    return Lse;
}

void information_filter(Matrix Hk,
                        Matrix Gk,
                        Matrix hk,
                        Matrix Sk,
                        Matrix &Fk,
                        Matrix &Nk) {
    // Calculates de information matrix Fk, and related contribution Nk for the data.

    Matrix Ak =  Gk *  Sk * ~Gk;
    Matrix Ck = ~Hk * !Ak;

    Fk = Ck * Hk;
    Nk = Ck * hk;
}

void integrate_laserpoint_on_laseredge(Uloc Lre,
                                       Uloc Lrp,
                                       Matrix &Fk,
                                       Matrix &Nk) {
    // Filter Subfeature (LaserPoint) Feature (LaserEdge) Direct

    Transf xep = TRel(Lre.kX(), Lrp.kX());
    Matrix hk  = Matrix(1, 1);
    hk(0, 0)   = xep.y();
    Matrix Hk  = Matrix(1, 2);
    Hk(0, 0)   = -1;
    Hk(0, 1)   = -xep.x();
    Matrix Gk  = Matrix(1, 2);
    Gk(0, 0)   = sin(xep.phi());
    Gk(0, 1)   = cos(xep.phi());

    information_filter(Hk, Gk, hk, Lrp.kCov(), Fk, Nk);
}

void calculate_estimation(Matrix Q, Matrix N, Matrix &P, Matrix &X) {
    P = !Q;
    X = P * -N;
}

Uloc integrateEndpointsInEdge(Uloc Lsp1, Uloc Lsp2) {
    //Computes an uncertain edge from two uncertain points

    Uloc Lse = CalculateAnalyticalEdge(Lsp1.kX(), Lsp2.kX());
    Matrix Fk, Nk, Q, N;

    integrate_laserpoint_on_laseredge(Lse, Lsp1, Q, N);
    integrate_laserpoint_on_laseredge(Lse, Lsp2, Fk, Nk);

    Q += Fk;//Q = Fk + Q;
    N += Nk;//N = Nk + N;

    calculate_estimation(Q, N, Lse.Cov(), Lse.Pert());
    Lse.CenterUloc();

    return Lse;
}

void estimate_relative_location(Uloc Lwe, Uloc Lwm, Transf &Xem, Matrix &Cem) {

    Xem = Compose(Inv(Lwe.kX()), Lwm.kX());
    Matrix Ja = J1zero(Xem);
    Matrix Jb = J2zero(Xem);
    Matrix Ca = Ja * ~Lwe.kBind() * Lwe.kCov() * Lwe.kBind() * ~Ja;
    Matrix Cb = Jb * ~Lwm.kBind() * Lwm.kCov() * Lwm.kBind() * ~Jb;
    Cem = Ca + Cb;
}

double mahalanobis_distance(Uloc Lwa, Uloc Lwb, Matrix Bab) {
    Transf Xab;
    Matrix Cab;

    estimate_relative_location(Lwa, Lwb, Xab, Cab);
    Matrix Ca = Bab * Cab * ~Bab;

    Matrix v = Bab * Xab;
    Matrix d = ~v * !Ca * v;

    return d(0, 0);
}

void Uloc::ChangeBinding(Matrix newb) {

    c_ = newb * ~b_ * c_ * b_ * ~newb;
    p_ = newb * DifferentialLocation();
    b_ = newb;
}

void Uloc::FilterFeatureRobotDirect(Uloc Lre,
                                       Transf Xmw,
                                       Matrix &Fk,
                                       Matrix &Nk) {
    //El (This) es Lwr

    Transf Xmr = Compose(Xmw, x_);
    Transf Xme = Compose(Xmr, Lre.kX());
    Matrix Be = Lre.kBind();

    Matrix hk = Be * Xme;
    Matrix Hk = (Be * J1zero(Xme) * ~Be) * (Be * Jacobian(Xmr));
    Matrix Gk = Be * J2zero(Xme) * ~Be;

    information_filter(Hk, Gk, hk, Lre.kCov(), Fk, Nk);
}

void integrate_innovation(Matrix Fk,
                          Matrix Nk,
                          Matrix P,
                          Matrix X,
                          Matrix &Pk,
                          Matrix &Xk) {

    Matrix Q = !P;
    Matrix Qk = Q + Fk;

    Pk = !Qk;
    Xk = Pk * (Q * X - Nk);
}

void Uloc::IntegrateEdge(Uloc Lre, Transf Xma) {

    Matrix Fk, Nk;

    FilterFeatureRobotDirect(Lre, Xma, Fk, Nk);
    integrate_innovation(Fk, Nk, c_, p_, c_, p_);
    CenterUloc();
}
