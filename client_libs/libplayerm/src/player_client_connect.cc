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
    if ( nrhs != 2 || ! mxIsChar ( prhs[0] ) || ! mxIsNumeric ( prhs[1] ) ) {
        mexPrintf("usage: client = %s ( hostname, port )\n", mexFunctionName());
				mexErrMsgTxt ("Wrong command");
    }
    char *pHost = mxArrayToString ( prhs[0] );
    double port = *mxGetPr ( prhs[1] );
    PlayerCc::PlayerClient *pRobot = new PlayerCc::PlayerClient ( pHost, ( int ) port );
    try {
        pRobot->SetDataMode ( PLAYERC_DATAMODE_PULL );
    } catch ( PlayerCc::PlayerError e ) {
        std::cerr << e << std::endl;
        return;
    }
    try {
        pRobot->SetReplaceRule ( true, PLAYER_MSGTYPE_DATA, -1, -1 );
    } catch ( PlayerCc::PlayerError e ) {
        std::cerr << e << std::endl;
        return;
    }

    plhs[0] = setCPointer ( pRobot );
}
