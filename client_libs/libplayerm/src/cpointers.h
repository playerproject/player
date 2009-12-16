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

#ifndef GET_C_POINTER
#define GET_C_POINTER
template <class T>
inline T* getCPointer( const mxArray* pSrc) {
		T* pDes =  (T *) *((long*) mxGetPr (pSrc));
    return pDes;
}
#endif


#ifndef SET_C_POINTER
#define SET_C_POINTER
template <class T>
inline mxArray *setCPointer(T* pSrc) {
		mxArray *pDes = mxCreateNumericMatrix(1, 1, mxUINT64_CLASS, mxREAL);
		long *pAddress =  (long *) mxGetPr (pDes);
		*pAddress = (long) pSrc;
		return pDes;
}
#endif