/***************************************************************************
 *   Copyright (C) 2009 by Markus Bader                                    *
 *   bader@acin.tuwien.ac.at                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/



#include "mex.h"
#include <libplayerc++/playerc++.h>
#include "cpointers.h"

void mexFunction ( int nlhs, mxArray *plhs[], int nrhs,  const mxArray *prhs[] ) {
    if (nrhs != 1  || ! mxIsNumeric (prhs[0])) {
        mexPrintf("usage: measurements = %s ( laser )\n", mexFunctionName());
				mexErrMsgTxt ("Wrong command");
    }
    PlayerCc::LaserProxy *pLaser = getCPointer<PlayerCc::LaserProxy>(prhs[0]);

    int count = pLaser->GetCount();
    mxArray *pPoints = mxCreateDoubleMatrix (2, count, mxREAL);
    double  *p = (double *)mxGetPr(pPoints);

    for (int i = 0; i < count; i++) {
			player_point_2d point = pLaser->GetPoint(i);
        *p++ = point.px;
        *p++ = point.py;
				//mexPrintf("%f, %f\n", point.px, point.py);
    }
    plhs[0] = pPoints;
}
