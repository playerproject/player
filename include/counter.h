/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000  
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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

/*
 * $Id$
 *  
 *  a mutex-protected counter.
 */

#ifndef _COUNTER_H
#define _COUNTER_H

#include <pthread.h>

class CCounter 
{
  pthread_mutex_t access;
  int counter;

 public:
  CCounter() {
    counter = 0;
    pthread_mutex_init( &access, NULL );
  }

  ~CCounter() {
    pthread_mutex_destroy( & access );
  }

  void operator += ( int i ) {
    pthread_mutex_lock( &access );
    this->counter += i;
    pthread_mutex_unlock( &access );
  }

  void operator -= ( int i ) {
    pthread_mutex_lock( &access );
    this->counter -= i;
    pthread_mutex_unlock( &access );
  }

  int Value() {
    return(counter);
  }

};


#endif
