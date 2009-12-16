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
        mexPrintf("usage: [pose Hypothesis] = %s ( localize )\n", mexFunctionName());
        mexErrMsgTxt ("Wrong command");
    }
    PlayerCc::LocalizeProxy *pLocalize = getCPointer<PlayerCc::LocalizeProxy>(prhs[0]);

    int count =  pLocalize->GetNumHypoths();
    mxArray *pPoses = mxCreateDoubleMatrix (count, 7, mxREAL);
    double  *p = (double *)mxGetPr(pPoses);

    for (int i = 0; i < count; i++) {
        player_localize_hypoth hypoth = pLocalize->GetHypoth(i);
        *p++ = hypoth.mean.px;
        *p++ = hypoth.mean.py;
        *p++ = hypoth.mean.pa;
        *p++ = hypoth.cov[0];
        *p++ = hypoth.cov[1];
        *p++ = hypoth.cov[2];
        *p++ = hypoth.alpha;
    }
    plhs[0] = pPoses;
}
