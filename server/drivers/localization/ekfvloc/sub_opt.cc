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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include "sub_opt.hh"


void CalculateEstimationEIFnn(Matrix Fk, Matrix Nk, Matrix &x, Matrix &P){

	P = !Fk;
	x = P * Nk;

	for (unsigned int i = 0; i < x.RowNo(); i++)
		x(i,0) *= -1;
}

void EIFnn(Matrix H, Matrix G, Matrix h, Matrix S, Matrix &F, Matrix &N){

	Matrix R = ~H * !(G * S * ~G);

	F = R * H;
	N = R * h;
}
