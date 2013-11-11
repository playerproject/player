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


#ifndef SUB_OPT_H_
#define SUB_OPT_H_

#include "transf.hh"

void EIFnn(MatrixXd H, MatrixXd G, MatrixXd h, MatrixXd S, MatrixXd &F, MatrixXd &N);
void CalculateEstimationEIFnn(MatrixXd Fk, MatrixXd Nk, MatrixXd &x, MatrixXd &P);

#endif /* SUB_OPT_H_ */
