/*
 *  Player - One Hell of a Robot Server
 *  Copyright (C) 2000
 *     Brian Gerkey, Kasper Stoy, Richard Vaughan, & Andrew Howard
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

/***************************************************************************
 * Desc: Localization helpers
 * Author: Thimo Langbehn <thimo@g4t.de>
 * Date: 05 July 2010
 **************************************************************************/

#include <libplayerutil/localization.h>

void
derive_uncertainty_ellipsis2d(player_pose2d_t* ellipse_pose,
			      double* radius_x, double* radius_y,
			      player_localize_hypoth_t* hypothesis,
			      double probability_coverage)
{
  double xx, yy, rxy;
  double kk, t;

  /** The covariance matrix pose estimate (lower half) 
      (cov(xx) in m$^2$, cov(yy) in $^2$, cov(aa) in rad$^2$, 
      cov(xy), cov(ya), cov(xa) ). */
  xx = hypothesis->cov[0];
  yy = hypothesis->cov[1];
  rxy = hypothesis->cov[3];

  ellipse_pose->px = hypothesis->mean.px;
  ellipse_pose->py = hypothesis->mean.py;
  ellipse_pose->pa = 0.5*atan2(2.0*rxy, xx-yy);

  kk = -1 * log(1-probability_coverage);
  t = sqrt( (xx-yy)*(xx-yy) + 4*rxy*rxy );
  *radius_x = sqrt(kk*(xx+yy+t));
  *radius_y = sqrt(kk*(xx+yy-t));
}
