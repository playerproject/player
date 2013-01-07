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

#ifndef TRANSF_H_
#define TRANSF_H_

#include "string.h" // Somehow this is needed by matrix.h
#include "matrix.h"
#include "replace/replace.h"

typedef math::matrix<double> Matrix;

// #define X(t)   (t(0,0))
// #define Y(t)   (t(1,0))
// #define Phi(t) (t(2,0))

class Transf: public Matrix {

public:
	//Constructor and destructor
	Transf();
	Transf(double x, double y, double phi);
	Transf(Matrix &m);
	virtual ~Transf();

	//////////////
	double tX() const;
	double tY() const;
	double tPhi() const;
	/////////////
    double & x() { return (*this)(0, 0); }
    double & y() { return (*this)(1, 0); }
    double & phi() { return (*this)(2, 0); }
    /////////////
    const double x() const { return (*this)(0, 0); }
    const double y() const { return (*this)(1, 0); }
    const double phi() const { return (*this)(2, 0); }

    double Distance(const Transf &b) const;
};

Transf Compose(Transf Tab, Transf Tbc);
Transf Inv(Transf Tab);
Transf TRel(Transf Twa, Transf Twb);
Matrix Jacobian (Transf Tab);
Matrix InvJacobian (Transf Tab);
Matrix J1 (Transf Ta, Transf Tb);
Matrix InvJ1 (Transf Ta, Transf Tb);
Matrix J1zero (Transf Ta);
Matrix InvJ1zero (Transf Ta);
Matrix J2 (Transf Ta, Transf Tb);
Matrix InvJ2 (Transf Ta, Transf Tb);
Matrix J2zero (Transf Ta);
Matrix InvJ2zero (Transf Ta);
double spAtan2 (double y, double x);
double Normalize (double p);

// Added by Alex
void Eigenv(Matrix M, Matrix *vectors, Matrix *values);
   // Compute eigenvalues/vectors.
   // Results are reset, so the original content/dimensions do not matter.
   // Results are of same dimensions (NxN) as This, with the caveat that
   //   "values" is a diagonal matrix with the eigenvalues in the diagonal.
   // This is done to match matlab/octave behavior.
   // See gsl_eigen_symmv for the guarantees on the results.

#endif /* TRANSF_H_ */
