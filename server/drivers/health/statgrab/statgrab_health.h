/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2005-2006
 *     Raphael Sznitman, Brad Kratochvil
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdint.h>
#include <libplayercore/playercore.h>

extern "C"
{
#include <statgrab.h>
}
	
////////////////////////////////////////////////////////////////////////////////
// The class for the driver
class StatGrabDriver : public ThreadedDriver
{
  public:

    StatGrabDriver(ConfigFile* cf, int section);
    
  private:
    // Main function for device thread.
    virtual void Main();
    int MainSetup();
    void RefreshData();
    
    // Structure for specific process data 
    
    // Structure holding Swap data 
    sg_swap_stats         *swap_stats;
    
    // Structure holding CPU data  
    sg_cpu_percents       *cpu_percent;
     
    // Structure holding memory stat  
    sg_mem_stats          *mem_data;
    double mem_percent;
   
    // Health Interface
    player_devaddr_t     mHealthId;
    player_health_data_t   mHealth;
   
    // For status checking priviledge
    int                   status;
    int32_t               mSleep;

};
