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
    if (nrhs != 2  || ! mxIsNumeric (prhs[0]) || ! mxIsNumeric (prhs[1])) {
        mexPrintf("usage: sonar = %s ( client, index )\n", mexFunctionName());
				mexErrMsgTxt ("Wrong command");
    }
    double index = *mxGetPr(prhs[1]);
    PlayerCc::PlayerClient *pRobot =  getCPointer<PlayerCc::PlayerClient>(prhs[0]);
    PlayerCc::SonarProxy *pSonar = new PlayerCc::SonarProxy(pRobot, (int) index);
    pRobot->Read();

    if (nlhs == 2 ) {
        mexPrintf("Read Geometry\n");
        pSonar->RequestGeom ();
        int count = pSonar->GetCount();
        mxArray *pPoses = mxCreateDoubleMatrix (count, 6, mxREAL);
        double  *p = (double *)mxGetPr(pPoses);

        for (int i = 0; i < count; i++) {
            player_pose3d_t pose = pSonar->GetPose(i);
            *p++ = pose.px;
            *p++ = pose.py;
            *p++ = pose.pz;
            *p++ = pose.proll;
            *p++ = pose.ppitch;
            *p++ = pose.pyaw;
        }
        plhs[1] = pPoses;
    }
    plhs[0] = setCPointer(pSonar);
}
