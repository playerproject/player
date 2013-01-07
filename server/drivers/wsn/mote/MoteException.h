/********************************************************************
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
 *
 ********************************************************************/
/*********************************************************************
 * TinyOS data structures.
 * Portions borrowed from the TinyOS project (http://www.tinyos.net), 
 * distributed according to the Intel Open Source License.
 *********************************************************************/
/***************************************************************************
 * Desc: Library for generic Crossbow WSN nodes communication
 * Author: Jose Manuel Sanchez Matamoros, Adrian Jimenez Gonzalez
 * Date: 15 Aug 2011
 **************************************************************************/


#ifndef MOTEEXCEPION_H
#define MOTEEXCEPION_H

#include <string>
using std::string;

namespace mote {

class MoteException {
public:
	const char *what();
	void append( const char *text );
protected:
	std::string str;
};


class CRCException : public MoteException {};
class TimeoutException : public MoteException {};
class IOException : public MoteException {};

}

#endif

