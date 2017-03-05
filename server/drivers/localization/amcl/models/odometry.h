/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2003
 *     Andrew Howard
 *     Brian Gerkey    
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
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 *
 */


/**************************************************************************
 * Desc: Sensor/action models for odometry.
 * Author: Andrew Howard
 * Date: 15 Dec 2002
 * CVS: $Id$
 *************************************************************************/

#ifndef ODOMETRY_H
#define ODOMETRY_H

#include "../pf/pf.h"
#include "../pf/pf_pdf.h"

#ifdef __cplusplus
extern "C" {
#endif

  
// Model information
typedef struct
{
  // PDF used for initialization
  pf_pdf_gaussian_t *init_pdf;

  // PDF used to generate action samples
  pf_pdf_gaussian_t *action_pdf;

} odometry_t;


// Create an sensor model
odometry_t *odometry_alloc();

// Free an sensor model
void odometry_free(odometry_t *sensor);

// Prepare to initialize the distribution
void odometry_init_init(odometry_t *self, pf_vector_t mean, pf_matrix_t cov);

// Finish initializing the distribution
void odometry_init_term(odometry_t *self);

// Initialize the distribution
pf_vector_t odometry_init_model(odometry_t *self);

// Prepare to update the distribution using the action model.
void odometry_action_init(odometry_t *self, pf_vector_t old_pose, pf_vector_t new_pose);

// Finish updating the distrubiotn using the action model
void odometry_action_term(odometry_t *self);

// The action model function
pf_vector_t odometry_action_model(odometry_t *self, pf_vector_t pose);


#ifdef __cplusplus
}
#endif

#endif

