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
        mexPrintf("usage: pose = %s ( pose2d )\n", mexFunctionName());
				mexErrMsgTxt ("Wrong command");
				return;
    }
    PlayerCc::Position2dProxy *pPos2D  =  getCPointer<PlayerCc::Position2dProxy>(prhs[0]);

    mxArray *pInfo = mxCreateDoubleMatrix (1, 6, mxREAL);
    double  *p = (double *)mxGetPr(pInfo);

    *p++ = pPos2D->GetXPos ();
    *p++ = pPos2D->GetYPos ();
    *p++ = pPos2D->GetYaw ();
    *p++ = pPos2D->GetXSpeed ();
    *p++ = pPos2D->GetYSpeed ();
    *p++ = pPos2D->GetYawSpeed ();

    plhs[0] = pInfo;
}
