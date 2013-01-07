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
 * Desc: Generic XBow WSN node communication structure
 * Author: Adrian Jimenez Gonzalez
 * Date: 15 Aug 2011
 **************************************************************************/

#include <libplayercore/playercore.h>
#include "generic_xbow.h"

using namespace mote;

namespace mote{
 
 template<> char *defineStruct<player_coopobject_header_t>(const player_coopobject_header_t &obj){
    const char base[] = "bss";
    char *def = new char[strlen(base)+1];
    strcpy(def, base);
    return def;
 };
 
 template<> char *defineStruct<player_coopobject_rssi_t>(const player_coopobject_rssi_t &obj){
    const char base[] = "bssssswwfff";
    char *def = new char[strlen(base)+1];
    strcpy(def, base);
    return def;
 };
 
 template<> char *defineStruct<player_coopobject_position_t>(const player_coopobject_position_t &obj){
    const char base[] = "bssfffb";
    char *def = new char[strlen(base)+1];
    strcpy(def, base);
    return def;
 };
 
 template<> char *defineStruct<player_coopobject_data_sensor_t>(const player_coopobject_data_sensor_t &obj){
    uint32_t n_elements = 5;
    uint32_t size = 2*obj.data_count;
    char *result = new char[n_elements+size+1+1];
    result[0] = 'b';
    result[1] = 's';
    result[2] = 's';
    result[3] = 'c';
    result[4] = 'p';
    uint32_t i = n_elements;
    for (i = n_elements; i < size+n_elements; i+=2) {
      result[i] = 'b';
      result[i+1] = 's';
    }
    result[n_elements+size] = 'r';
    result[n_elements+size+1] = '\0';
    return result; 
  };
  
  template<> char *defineStruct<player_coopobject_data_userdefined_t>(const player_coopobject_data_userdefined_t &obj){
    uint32_t n_elements = 6;
    char *result = new char[n_elements+obj.data_count+1+1];
    result[0] = 'b';
    result[1] = 's';
    result[2] = 's';
    result[3] = 'b';
    result[4] = 'c';
    result[5] = 'p';
    uint32_t i = n_elements;
    for (i = n_elements; i < obj.data_count+n_elements; i++)
      result[i] = 'b';
    result[n_elements+obj.data_count] = 'r';
    result[n_elements+obj.data_count+1] = '\0';
    return result; 
  };
  
  template<> char *defineStruct<player_coopobject_req_t>(const player_coopobject_req_t &obj){
    uint32_t n_elements = 6;
    char *result = new char[n_elements+obj.parameters_count+1+1];
    result[0] = 'b';
    result[1] = 's';
    result[2] = 's';
    result[3] = 'b';
    result[4] = 'c';
    result[5] = 'p';
    uint32_t i = n_elements;
    for (i = n_elements; i < obj.parameters_count+n_elements; i++)
      result[i] = 'b';
    result[n_elements+obj.parameters_count] = 'r';
    result[n_elements+obj.parameters_count+1] = '\0';
    return result; 
  };
  
  template<> char *defineStruct<player_coopobject_cmd_t>(const player_coopobject_cmd_t &obj){
    uint32_t n_elements = 6;
    char *result = new char[n_elements+obj.parameters_count+1+1];
    result[0] = 'b';
    result[1] = 's';
    result[2] = 's';
    result[3] = 'b';
    result[4] = 'c';
    result[5] = 'p';
    uint32_t i = n_elements;
    for (i = n_elements; i < obj.parameters_count+n_elements; i++)
      result[i] = 'b';
    result[n_elements+obj.parameters_count] = 'r';
    result[n_elements+obj.parameters_count+1] = '\0';
    return result; 
  };
        
  template<> char *defineStruct<XMeshHeader>(const XMeshHeader &obj){ 
    const char base[] = "sssb";
    char *def = new char[strlen(base)+1];
    strcpy(def, base);
    return def;
 };

  template<> char *defineStruct<HealthMsg>(const HealthMsg &obj){ 
    const char base[] = "sssbss";
    char *def = new char[strlen(base)+1];
    strcpy(def, base);
    return def;
 };

  template<> char *defineStruct<RSSIBeaconMsg>(const RSSIBeaconMsg &obj){
    const char base[] = "bbbsswwfff";
    char *def = new char[strlen(base)+1];
    strcpy(def, base);
    return def;
 };

  template<> char *defineStruct<PositionMsg>(const PositionMsg &obj){ 
    const char base[] = "bssfffb";
    char *def = new char[strlen(base)+1];
    strcpy(def, base);
    return def;
 };
  template<> char *defineStruct<SensorMsg>(const SensorMsg &obj){
    uint32_t n_elements = 5;
    char *result = new char[n_elements+2*obj.sensor_count+1+1];
    result[0] = 'b';
    result[1] = 's';
    result[2] = 's';
    result[3] = 'b';
    result[4] = 'p';
    uint32_t i = n_elements;
    while (i < 2*obj.sensor_count+n_elements) {
      result[i++] = 'b';
      result[i++] = 's';
    }
    result[n_elements+2*obj.sensor_count] = 'r';
    result[n_elements+2*obj.sensor_count+1] = '\0';
    return result; 
  };
  
  template<> char *defineStruct<UserDataMsg>(const UserDataMsg &obj){
    uint32_t n_elements = 5;
    char *result = new char[n_elements+obj.data_size+1+1];
    result[0] = 'b';
    result[1] = 's';
    result[2] = 's';
    result[3] = 'b';
    result[4] = 'p';
    uint32_t i = n_elements;
    while (i < obj.data_size+n_elements) {
      result[i++] = 'b';
    }
    result[n_elements+obj.data_size] = 'r';
    result[n_elements+obj.data_size+1] = '\0';
    return result; 
  };
 
  template<> char *defineStruct<RequestMsg>(const RequestMsg &obj){
    uint32_t n_elements = 6;
    char *result = new char[n_elements+obj.parameters_size+1+1];
    result[0] = 'b';
    result[1] = 's';
    result[2] = 's';
    result[3] = 'b';
    result[4] = 'b';
    result[5] = 'p';
    uint32_t i = n_elements;
    while (i < obj.parameters_size+n_elements) {
      result[i++] = 'b';
    }
    result[n_elements+obj.parameters_size] = 'r';
    result[n_elements+obj.parameters_size+1] = '\0';
    return result; 
  };
  template<> char *defineStruct<CommandMsg>(const CommandMsg &obj){
    uint32_t n_elements = 6;
    char *result = new char[n_elements+obj.parameters_size+1+1];
    result[0] = 'b';
    result[1] = 's';
    result[2] = 's';
    result[3] = 'b';
    result[4] = 'b';
    result[5] = 'p';
    uint32_t i = n_elements;
    while (i < obj.parameters_size+n_elements) {
      result[i++] = 'b';
    }
    result[n_elements+obj.parameters_size] = 'r';
    result[n_elements+obj.parameters_size+1] = '\0';
    return result; 
  };

}
