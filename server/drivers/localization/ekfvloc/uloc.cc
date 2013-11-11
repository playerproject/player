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
#include <iostream>

Uloc::Uloc(GeometricEntityKinds ge) {
    // TODO Auto-generated constructor stub
    x_ = Transf();
    entity = ge;
    if (ge == POINT) {
        b_ = MatrixXd(2, 3);
        b_(0, 0) = 1;
        b_(1, 1) = 1;
        p_ = MatrixXd(2, 1);
        c_ = MatrixXd(2, 2);
    }
    else if (ge == EDGE) {
        b_ = MatrixXd(2, 3);
        b_(0, 1) = 1;
        b_(1, 2) = 1;
        p_ = MatrixXd(2, 1);
        c_ = MatrixXd(2, 2);
    }
    else if (ge == ROBOT) {
        b_ = MatrixXd(3, 3);
        b_.setIdentity();
        p_ = MatrixXd(3, 1);
        c_ = MatrixXd(3, 3);
    }
    else {
        std::cerr << "Uloc: Undefined entity name\n";
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

void Uloc::SetPert(MatrixXd pert) {
    p_ = pert;
}

void Uloc::SetBind(MatrixXd bind) {
    b_ = bind;
}

void Uloc::SetCov(MatrixXd cov) {
    c_ = cov;
}

Transf Uloc::DifferentialLocation() {
    // Returns the differential location vector of the uncertain location.
    MatrixXd df = b_.transpose() * p_;
    return Transf(df);
}

Uloc inverse_uloc(Uloc Lab) {
    Uloc Lba(Lab.uGEntity());

    Lba.SetLoc (Inv(Lab.kX()));
    Lba.SetBind(Lab.kBind());

    MatrixXd Jab = Jacobian(Lab.kX());
    MatrixXd J   = Lba.kBind() * Jab * Lba.kBind().transpose();

    Lba.SetPert(J * Lab.kPert());
    Lba.SetCov (J * Lab.kCov() * J.transpose());

    return Lba;
}

void Uloc::CenterUloc() {
    // Centers the uncertain location u, so that the perturbation equals zero.

    Transf de = DifferentialLocation();

    x_ = Compose(x_, de);

    MatrixXd D = b_ * InvJ2zero(de) * b_.transpose();
    c_ = D * c_ * D.transpose();

    if (entity == POINT)
        p_ = MatrixXd(2, 1);
    if (entity == EDGE)
        p_ = MatrixXd(2, 1);
    if (entity == ROBOT)
        p_ = MatrixXd(3, 1);
}

Uloc compose_uloc_transf(Uloc Lwf, Transf Xfe) {
    // Chages the associated reference of Lwf, as given by Xfe, giving Lwe.

    Uloc Lwe(Lwf.uGEntity());

    Lwe.SetLoc(Compose(Lwf.kX(), Xfe));
    MatrixXd J   = Lwf.kBind() * InvJacobian(Xfe) * Lwf.kBind().transpose();
    Lwe.Pert() = J * Lwf.kPert();
    Lwe.Cov()  = J * Lwf.kCov () * J.transpose();

    return Lwe;
}

Uloc compose_uloc(Uloc Lwf, Uloc Lfe) {
    // Composes two (independent!) uncertain locations Lwf and Lfe, giving Lwe.
    Uloc Lwe(Lwf.uGEntity());
    Lwe.Loc() = Compose(Lwf.kX(), Lfe.kX());

    MatrixXd BeJefBft = Lfe.kBind() * InvJacobian(Lfe.kX())   * Lwf.kBind().transpose();
    Lwe.Pert()      = BeJefBft    * Lwf.kPert()             + Lfe.kPert();
    Lwe.Cov()       = BeJefBft    * Lwf.kCov()  * BeJefBft.transpose() + Lfe.kCov();

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

    return std::pow(xep.tY(), 2)
            / (
                    Lwe.kCov()(0, 0)
                    + xep.x() * (2 * Lwe.kCov()(0, 1) + xep.x() * Lwe.kCov()(1, 1))
                    + Lwp.kCov()(0, 0) * std::pow(sin(xep.phi()), 2)
                    + Lwp.kCov()(1, 1) * std::pow(cos(xep.phi()), 2)
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

void information_filter(MatrixXd Hk,
                        MatrixXd Gk,
                        MatrixXd hk,
                        MatrixXd Sk,
                        MatrixXd &Fk,
                        MatrixXd &Nk) {
    // Calculates de information matrix Fk, and related contribution Nk for the data.

    MatrixXd Ak =  Gk *  Sk * Gk.transpose();
    MatrixXd Ck = Hk.transpose() * Ak.inverse();

    Fk = Ck * Hk;
    Nk = Ck * hk;
}

void integrate_laserpoint_on_laseredge(Uloc Lre,
                                       Uloc Lrp,
                                       MatrixXd &Fk,
                                       MatrixXd &Nk) {
    // Filter Subfeature (LaserPoint) Feature (LaserEdge) Direct

    Transf xep = TRel(Lre.kX(), Lrp.kX());
    MatrixXd hk  = MatrixXd(1, 1);
    hk(0, 0)   = xep.y();
    MatrixXd Hk  = MatrixXd(1, 2);
    Hk(0, 0)   = -1;
    Hk(0, 1)   = -xep.x();
    MatrixXd Gk  = MatrixXd(1, 2);
    Gk(0, 0)   = sin(xep.phi());
    Gk(0, 1)   = cos(xep.phi());

    information_filter(Hk, Gk, hk, Lrp.kCov(), Fk, Nk);
}

void calculate_estimation(MatrixXd Q, MatrixXd N, MatrixXd &P, MatrixXd &X) {
    P = Q.inverse();
    X = P * N.transpose();
}

Uloc integrateEndpointsInEdge(Uloc Lsp1, Uloc Lsp2) {
    //Computes an uncertain edge from two uncertain points

    Uloc Lse = CalculateAnalyticalEdge(Lsp1.kX(), Lsp2.kX());
    MatrixXd Fk, Nk, Q, N;

    integrate_laserpoint_on_laseredge(Lse, Lsp1, Q, N);
    integrate_laserpoint_on_laseredge(Lse, Lsp2, Fk, Nk);

    Q += Fk;//Q = Fk + Q;
    N += Nk;//N = Nk + N;

    calculate_estimation(Q, N, Lse.Cov(), Lse.Pert());
    Lse.CenterUloc();

    return Lse;
}

void estimate_relative_location(Uloc Lwe, Uloc Lwm, Transf &Xem, MatrixXd &Cem) {

    Xem = Compose(Inv(Lwe.kX()), Lwm.kX());
    MatrixXd Ja = J1zero(Xem);
    MatrixXd Jb = J2zero(Xem);
    MatrixXd Ca = Ja * Lwe.kBind().transpose() * Lwe.kCov() * Lwe.kBind() * Ja.transpose();
    MatrixXd Cb = Jb * Lwm.kBind().transpose() * Lwm.kCov() * Lwm.kBind() * Jb.transpose();
    Cem = Ca + Cb;
}

double mahalanobis_distance(Uloc Lwa, Uloc Lwb, MatrixXd Bab) {
    Transf Xab;
    MatrixXd Cab;

    estimate_relative_location(Lwa, Lwb, Xab, Cab);
    MatrixXd Ca = Bab * Cab * Bab.transpose();

    MatrixXd v = Bab * Xab;
    MatrixXd d = v.transpose() * Ca.inverse() * v;

    return d(0, 0);
}

void Uloc::ChangeBinding(MatrixXd newb) {

    c_ = newb * b_.transpose() * c_ * b_ * newb.transpose();
    p_ = newb * DifferentialLocation();
    b_ = newb;
}

void Uloc::FilterFeatureRobotDirect(Uloc Lre,
                                       Transf Xmw,
                                       MatrixXd &Fk,
                                       MatrixXd &Nk) {
    //El (This) es Lwr

    Transf Xmr = Compose(Xmw, x_);
    Transf Xme = Compose(Xmr, Lre.kX());
    MatrixXd Be = Lre.kBind();

    MatrixXd hk = Be * Xme;
    MatrixXd Hk = (Be * J1zero(Xme) * Be.transpose()) * (Be * Jacobian(Xmr));
    MatrixXd Gk = Be * J2zero(Xme) * Be.transpose();

    information_filter(Hk, Gk, hk, Lre.kCov(), Fk, Nk);
}

void integrate_innovation(MatrixXd Fk,
                          MatrixXd Nk,
                          MatrixXd P,
                          MatrixXd X,
                          MatrixXd &Pk,
                          MatrixXd &Xk) {

    MatrixXd Q = P.inverse();
    MatrixXd Qk = Q + Fk;

    Pk = Qk.inverse();
    Xk = Pk * (Q * X - Nk);
}

void Uloc::IntegrateEdge(Uloc Lre, Transf Xma) {

    MatrixXd Fk, Nk;

    FilterFeatureRobotDirect(Lre, Xma, Fk, Nk);
    integrate_innovation(Fk, Nk, c_, p_, c_, p_);
    CenterUloc();
}
