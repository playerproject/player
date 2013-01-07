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



#ifndef ULOC_H_
#define ULOC_H_

#include "transf.hh"

enum GeometricEntityKinds { POINT, EDGE, ROBOT };

class Uloc {

public:
	GeometricEntityKinds entity;

	Uloc(GeometricEntityKinds ge);
	virtual ~Uloc();

	GeometricEntityKinds uGEntity();

	Matrix& Loc()  { return x_; };
	Matrix& Pert() { return p_; };
	Matrix& Bind() { return b_; };
	Matrix& Cov()  { return c_; };

	const Transf& kX()    const { return x_; };
	const Matrix& kPert() const { return p_; };
	const Matrix& kBind() const { return b_; };
	const Matrix& kCov()  const { return c_; };

	void SetLoc(Transf loc);
	void SetPert(Matrix pert);
	void SetBind(Matrix bind);
	void SetCov(Matrix cov);

	void CenterUloc ();
	Transf DifferentialLocation ();
	void ChangeBinding (Matrix newb);
	void FilterFeatureRobotDirect (Uloc Lre, Transf Xmw, Matrix &Fk, Matrix &Nk);
	void IntegrateEdge (Uloc Lre, Transf Xma);

private:
	Transf  x_;
	Matrix  p_;
	Matrix  b_;
	Matrix  c_;
};

inline ostream& operator << (ostream& ostrm, Uloc& u)
{

	if (u.uGEntity() == POINT) 	ostrm << "Uloc: Point" << endl;
	if (u.uGEntity() == EDGE) 	ostrm << "Uloc: Edge" << endl;
	if (u.uGEntity() == ROBOT) 	ostrm << "Uloc: Robot" << endl;

	ostrm << "X:" << endl;
	ostrm << u.kX();
	ostrm << "Pert:" << endl;
	ostrm << u.kPert();
	ostrm << "Bind:" << endl;
	ostrm << u.kBind();
	ostrm << "Cov:" << endl;
	ostrm << u.kCov();

   return ostrm;
}

Uloc inverse_uloc (Uloc Lab);
Uloc compose_uloc_transf (Uloc Lwf, Transf Xfe);
Uloc compose_uloc (Uloc Lwf, Uloc Lfe);
Uloc compose_transf_uloc (Transf Xwf, Uloc Lfe);
Uloc CalculateAnalyticalEdge (Transf xp1, Transf xp2);
void information_filter (Matrix Hk, Matrix Gk, Matrix hk, Matrix Sk, Matrix &Fk, Matrix &Nk);
Uloc integrateEndpointsInEdge(Uloc Lsp1, Uloc Lsp2);
void estimate_relative_location (Uloc Lwe, Uloc Lwm, Transf &Xem, Matrix &Cem);
double mahalanobis_distance (Uloc Lwa, Uloc Lwb, Matrix Bab);
double mahalanobis_distance_edge_point(Uloc Lwe, Uloc Lwp);

#endif /* ULOC_H_ */
