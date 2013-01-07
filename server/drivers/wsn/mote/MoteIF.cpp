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


#include "MoteIF.h"

namespace mote {

MoteIF::~MoteIF() { 
//	delete ifs;
	close(); 
}

/** \brief Configures and opens the serial port connection.
  The required configuration is 115200 bps, parity none, flow control none,
  8bits per byte and 1 stop bit.
  \param tty Device to open. If none is specified, /dev/ttyUSB0 is used by default.
*/
void MoteIF::open( const char *tty, SerialStreamBuf::BaudRateEnum baud_rate ) {

	serial.Open( tty );
	// baud_rate bps, sin paridad, sin control de flujo y 1 bit de parada.
	if (baud_rate) {
	  serial.SetBaudRate( baud_rate );
//	  std::cout << "MoteIF::open: setting baud rate " << baud_rate << std::endl;
	}
	else std::cerr << "MoteIF::open: baud rate not set" << std::endl;
	
	serial.SetParity( SerialStreamBuf::PARITY_NONE );
	serial.SetCharSize( SerialStreamBuf::CHAR_SIZE_8  );
	serial.SetFlowControl( SerialStreamBuf::FLOW_CONTROL_NONE );
	serial.SetNumOfStopBits( 1 );
	serial.unsetf( std::ios_base::skipws ) ;
	
	protocol.bind( serial, serial );
	



}

bool MoteIF::setTiming( short vmin, short vtime ) {
	assert(serial.IsOpen());
	serial.SetVMin( vmin );
	serial.SetVTime( vtime );
	
	return serial.bad();
}

void MoteIF::close() {
	if (serial.IsOpen() ){
		serial.Close();
	}
}


void MoteIF::setOS(int v){
	os = v;
	protocol.setOS(v);
}

int MoteIF::getOS(){
	return protocol.getOS();
}

}

