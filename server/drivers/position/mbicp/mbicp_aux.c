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
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#include "MbICP.h"
#include "MbICP2.h"

int C_Fixed_Num_Readings (void)
{
  return MAXLASERPOINTS;
}

int C_Num_Associations (float max_dist)
{
  int result = 0;
  int i;

  for (i = 0; i < cntAssociationsTemp; i++) {
    if (cp_associationsTemp [i].dist <= max_dist)
      result++;
  }

  return result;
}

float C_Mean_Error (void)
{
  float error = 0.0;
  int i;

  if (cntAssociationsTemp == 0)
    return 1000000.0f;

  for (i = 0; i < cntAssociationsTemp; i++) {
    error += cp_associationsTemp [i].dist;
  }

  return error / (float)cntAssociationsTemp;
}

