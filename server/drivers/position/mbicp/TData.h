/***************************************************/
/* Last Revised: 
$Id$
*/
/***************************************************/
/*
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
#ifndef TData
#define TData

#ifdef __cplusplus
extern "C" {
#endif

/* 
   Este fichero contiene los tipos de datos utilizados por todos 
*/

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define MAXLASERPOINTS 361

#define RADIO 0.4F  /* Radio del robot */

typedef struct {
  float x;
  float y;
}Tpf;


typedef struct {
  float r;
  float t;
}Tpfp;

typedef struct {
  int x;
  int y;
}Tpi;

typedef struct {
  float x;
  float y;
  float tita;
}Tsc;

typedef struct {
  int numPuntos;
  Tpf laserC[MAXLASERPOINTS];  // Cartesian coordinates
  Tpfp laserP[MAXLASERPOINTS]; // Polar coordinates
}Tscan;




// Associations information
typedef struct{
  float rx,ry,nx,ny,dist;				// Point (nx,ny), static corr (rx,ry), dist 
  int numDyn;							// Number of dynamic associations
  float unknown;						// Unknown weight
  int index;							// Index within the original scan
  int L,R;
}TAsoc;

#ifdef __cplusplus
}
#endif

#endif
