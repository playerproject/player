///////////////////////////////////////////////////////////////////////////
//
// File: puck.hh
// Author: brian gerkey, richard vaughan
// Date: 23 June 2001
// Desc: Simulates pucks
//
// CVS info:
//  $Source$
//  $Author$
//  $Revision$
//
// Usage:
//  (empty)
//
// Theory of operation:
//  (empty)
//
// Known bugs:
//  (empty)
//
// Possible enhancements:
//  (empty)
//
///////////////////////////////////////////////////////////////////////////

#ifndef _PUCK_HH
#define _PUCK_HH

#include "entity.hh"

class CPuck : public CEntity
{
  // Default constructor
  public: CPuck( char* name, char* type, char* color, CEntity *parent);

  // a static named constructor - a pointer to this function is given
  // to the Library object and paired with a string.  When the string
  // is seen in the worldfile, this function is called to create an
  // instance of this entity
public: static CPuck* Creator( char* name,  char* type, 
			       char* color, CEntity *parent )
  { return( new CPuck( name, type, color, parent ) ); }
  
  // Update the device
  public: virtual int Update();

  // Move the puck
  private: void PuckMove();
    
  // Return diameter of puck
  public: double GetDiameter() { return(this->size_x); }

  // Timings (for movement)
  private: double m_last_time;

  // coefficient of "friction"
  private: double m_friction;

  // Vision return value when we are being held
  // Vision return value when we are not being held
  private: int vision_return_held;
  private: int vision_return_notheld;
};

#endif
