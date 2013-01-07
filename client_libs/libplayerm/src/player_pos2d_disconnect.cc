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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA.             *
 ***************************************************************************/


#include "mex.h"
#include <libplayerc++/playerc++.h>
#include "cpointers.h"

void mexFunction ( int nlhs, mxArray *plhs[], int nrhs,  const mxArray *prhs[] ) {
    if (nrhs != 1 || ! mxIsNumeric (prhs[0])) {
        mexPrintf("usage: %s ( pose2d )\n", mexFunctionName());
				mexErrMsgTxt ("Wrong command");
    }
    PlayerCc::Position2dProxy *pPos2D = getCPointer<PlayerCc::Position2dProxy>(prhs[0]);
    delete pPos2D;
}
