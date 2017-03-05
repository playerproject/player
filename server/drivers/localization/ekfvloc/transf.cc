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

#include <gsl/gsl_eigen.h>
#include <gsl/gsl_matrix.h>
#include <stdexcept>
#include <cmath>
#include "transf.hh"

Transf::Transf() :
        MatrixXd(3, 1)
{
}

Transf::Transf(double x, double y, double phi) :
        MatrixXd(3, 1)
{
    operator()(0, 0) = x;
    operator()(1, 0) = y;
    operator()(2, 0) = phi;
}

Transf::Transf(MatrixXd &m) :
        MatrixXd(3, 1)
{
    if ((m.rows() != 3) or(m.cols() != 1))
    {
        throw std::range_error("Impossible conversion from MatrixXd to Transf\n");
    }
    else
    {
        operator()(0, 0) = m(0, 0);
        operator()(1, 0) = m(1, 0);
        operator()(2, 0) = m(2, 0);
    }
}

Transf::~Transf()
{
}

double Transf::tX() const
{
    return operator()(0, 0);
}

double Transf::tY() const
{
    return operator()(1, 0);
}

double Transf::tPhi() const
{
    return operator()(2, 0);
}

double Normalize(double p)
// Normalize the angle p to (-M_PI, PI].
{
    double result = p;
    int    i      = 0;

    while (result > M_PI)
    {
        result -= 2.0 * M_PI;

        if (++i > 50)
        {
            printf("ERROR: normalize, result:%f\n", result);
            break;
        }
    }

    i = 0;

    while (result <= -M_PI)
    {
        result += 2.0 * M_PI;

        if (++i > 50)
        {
            printf("ERROR: normalize, result:%f\n", result);
            break;
        }
    }

    return result;
}

Transf Compose(Transf Tab, Transf Tbc)
{
    Transf Tac;

    Tac.x() = Tbc.tX() * cos(Tab.tPhi()) -
            Tbc.tY() * sin(Tab.tPhi()) +
            Tab.tX();

    Tac.y()   = Tbc.tX() * sin(Tab.tPhi()) +
            Tbc.tY() * cos(Tab.tPhi()) +
            Tab.tY();

    Tac.phi() = Normalize(Tab.tPhi() + Tbc.tPhi());

    return Tac;
}

Transf Inv(Transf Tab)
{
    Transf Tba;
    Tba.x() = -Tab.tY() * sin(Tab.tPhi()) -
            Tab.tX() * cos(Tab.tPhi());

    Tba.y() = Tab.tX() * sin(Tab.tPhi()) -
            Tab.tY() * cos(Tab.tPhi());

    Tba.phi() = -Tab.tPhi();

    return Tba;
}

Transf TRel(Transf Twa, Transf Twb)
{
    return Compose(Inv(Twa), Twb);
}

MatrixXd Jacobian(Transf Tab)
{
    MatrixXd Jab(3, 3);

    Jab(0, 0) =  cos(Tab.tPhi());
    Jab(0, 1) = -sin(Tab.tPhi());
    Jab(0, 2) =  Tab.tY();
    Jab(1, 0) =  sin(Tab.tPhi());
    Jab(1, 1) =  cos(Tab.tPhi());
    Jab(1, 2) = -Tab.tX();
    Jab(2, 0) =  0;
    Jab(2, 1) =  0;
    Jab(2, 2) =  1;

    return Jab;
}

MatrixXd InvJacobian(Transf Tab)
{
    MatrixXd Jba(3, 3);

    Jba(0, 0) =  cos(Tab.tPhi());
    Jba(0, 1) =  sin(Tab.tPhi());
    Jba(0, 2) =  Tab.tX() * sin(Tab.tPhi()) - Tab.tY() * cos(Tab.tPhi());
    Jba(1, 0) = -sin(Tab.tPhi());
    Jba(1, 1) =  cos(Tab.tPhi());
    Jba(1, 2) =  Tab.tX() * cos(Tab.tPhi()) + Tab.tY() * sin(Tab.tPhi());
    Jba(2, 0) =  0;
    Jba(2, 1) =  0;
    Jba(2, 2) =  1;

    return Jba;
}

MatrixXd J1(Transf Ta, Transf Tb)
{
    MatrixXd J1(3, 3);

    J1(0, 0) =  1;
    J1(0, 1) =  0;
    J1(0, 2) = -Tb.tX() * sin(Ta.tPhi()) - Tb.tY() * cos(Ta.tPhi());
    J1(1, 0) =  0;
    J1(1, 1) =  1;
    J1(1, 2) =  Tb.tX() * cos(Ta.tPhi()) - Tb.tY() * sin(Ta.tPhi());
    J1(2, 0) =  0;
    J1(2, 1) =  0;
    J1(2, 2) =  1;

    return J1;
}

MatrixXd InvJ1(Transf Ta, Transf Tb)
{
    MatrixXd InvJ1(3, 3);

    InvJ1(0, 0) =  1;
    InvJ1(0, 1) =  0;
    InvJ1(0, 2) =  Tb.tX() * sin(Ta.tPhi()) + Tb.tY() * cos(Ta.tPhi());
    InvJ1(1, 0) =  0;
    InvJ1(1, 1) =  1;
    InvJ1(1, 2) = -Tb.tX() * cos(Ta.tPhi()) + Tb.tY() * sin(Ta.tPhi());
    InvJ1(2, 0) =  0;
    InvJ1(2, 1) =  0;
    InvJ1(2, 2) =  1;

    return InvJ1;
}

MatrixXd J1zero(Transf Ta)
{
    MatrixXd J1z(3, 3);

    J1z(0, 0) =  1;
    J1z(0, 1) =  0;
    J1z(0, 2) = -Ta.tY();
    J1z(1, 0) =  0;
    J1z(1, 1) =  1;
    J1z(1, 2) =  Ta.tX();
    J1z(2, 0) =  0;
    J1z(2, 1) =  0;
    J1z(2, 2) =  1;

    return J1z;
}

MatrixXd InvJ1zero(Transf Ta)
{
    MatrixXd InvJ1z(3, 3);

    InvJ1z(0, 0) =  1;
    InvJ1z(0, 1) =  0;
    InvJ1z(0, 2) =  Ta.tY();
    InvJ1z(1, 0) =  0;
    InvJ1z(1, 1) =  1;
    InvJ1z(1, 2) = -Ta.tX();
    InvJ1z(2, 0) =  0;
    InvJ1z(2, 1) =  0;
    InvJ1z(2, 2) =  1;

    return InvJ1z;
}

MatrixXd J2(Transf Ta, Transf Tb)
{
    MatrixXd J2(3, 3);

    J2(0, 0) =  cos(Ta.tPhi());
    J2(0, 1) = -sin(Ta.tPhi());
    J2(0, 2) =  0;
    J2(1, 0) =  sin(Ta.tPhi());
    J2(1, 1) =  cos(Ta.tPhi());
    J2(1, 2) =  0;
    J2(2, 0) =  0;
    J2(2, 1) =  0;
    J2(2, 2) =  1;

    return J2;
}

MatrixXd InvJ2(Transf Ta, Transf Tb)
{
    MatrixXd InvJ2(3, 3);

    InvJ2(0, 0) =  cos(Ta.tPhi());
    InvJ2(0, 1) =  sin(Ta.tPhi());
    InvJ2(0, 2) =  0;
    InvJ2(1, 0) = -sin(Ta.tPhi());
    InvJ2(1, 1) =  cos(Ta.tPhi());
    InvJ2(1, 2) =  0;
    InvJ2(2, 0) =  0;
    InvJ2(2, 1) =  0;
    InvJ2(2, 2) =  1;

    return InvJ2;
}

MatrixXd J2zero(Transf Ta)
{
    MatrixXd J2z(3, 3);

    J2z(0, 0) =  cos(Ta.tPhi());
    J2z(0, 1) = -sin(Ta.tPhi());
    J2z(0, 2) =  0;
    J2z(1, 0) =  sin(Ta.tPhi());
    ;
    J2z(1, 1) =  cos(Ta.tPhi());
    ;
    J2z(1, 2) =  0;
    J2z(2, 0) =  0;
    J2z(2, 1) =  0;
    J2z(2, 2) =  1;

    return J2z;
}

MatrixXd InvJ2zero(Transf Ta)
{
    MatrixXd InvJ2z(3, 3);

    InvJ2z(0, 0) =  cos(Ta.tPhi());
    InvJ2z(0, 1) =  sin(Ta.tPhi());
    InvJ2z(0, 2) =  0;
    InvJ2z(1, 0) = -sin(Ta.tPhi());
    ;
    InvJ2z(1, 1) =  cos(Ta.tPhi());
    ;
    InvJ2z(1, 2) =  0;
    InvJ2z(2, 0) =  0;
    InvJ2z(2, 1) =  0;
    InvJ2z(2, 2) =  1;

    return InvJ2z;
}

double spAtan2(double y, double x)
// Computes atan2(y, x) and expresses the result anticlockwise
{
    double phi;

    phi = atan2(y, x);

    if (phi > M_PI_2)
        return (phi - 2 * M_PI);

    return phi;
}


double Transf::Distance(const Transf &b) const
{
    return std::sqrt(std::pow(tX() - b.tX(), 2.0) + std::pow(tY() - b.tY(), 2.0));
}

void Eigenv(MatrixXd M, MatrixXd *vectors, MatrixXd *values)
{
    if (M.cols() != M.rows())
        throw std::range_error("MatrixXd isn't square");

    gsl_matrix *m = gsl_matrix_alloc(M.rows(), M.cols());

    for (uint32_t r = 0; r < M.rows(); r++)
        for (uint32_t c = 0; c < M.cols(); c++)
            gsl_matrix_set(m, r, c, M(r, c));

    gsl_eigen_symmv_workspace *w = gsl_eigen_symmv_alloc(M.rows());
    gsl_vector *d = gsl_vector_alloc(M.rows());
    gsl_matrix *v = gsl_matrix_alloc(M.rows(), M.cols());

    const int err = gsl_eigen_symmv(m, d, v, w);

    if (err == 0)
    {
        *values = MatrixXd(M.rows(), M.cols());
        (*values) *= 0;
        for (uint32_t r = 0; r < M.rows(); r++)
            (*values)(r, r) = gsl_vector_get(d, r);

        *vectors = MatrixXd(M.rows(), M.cols());
        for (uint32_t r = 0; r < M.rows(); r++)
            for (uint32_t c = 0; c < M.cols(); c++)
                (*vectors)(r, c) = gsl_matrix_get(v, r, c);
    }

    gsl_eigen_symmv_free(w);
    gsl_vector_free(d);
    gsl_matrix_free(v);
    gsl_matrix_free(m);
    
    if (err != 0)
        throw std::runtime_error(gsl_strerror(err));
}
