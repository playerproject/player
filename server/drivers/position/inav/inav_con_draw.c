
/**************************************************************************
 * Desc: Local controller; GUI stuff
 * Author: Andrew Howard
 * Date: 18 Jan 2003
 * CVS: $Id$
**************************************************************************/

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <rtk.h>

#include "inav_con.h"


// Draw diagnostics
void icon_draw(icon_t *icon, rtk_fig_t *fig)
{
  int i, j;
  char text[32];
  icon_path_t *path;
  inav_vector_t *pose_a, *pose_b;

  // Draw the goal
  pose_a = &icon->goal_pose;
  rtk_fig_ellipse(fig, pose_a->v[0], pose_a->v[1], pose_a->v[2], 0.20, 0.20, 0);

  rtk_fig_color(fig, 0.7, 0.7, 0.7);
        
  // Draw the paths
  for (i = 0; i < icon->path_count; i++)
  {
    path = icon->paths + i;

    for (j = 0; j < path->pose_count - 1; j++)
    {
      pose_a = path->poses + j;
      pose_b = path->poses + j + 1;

      rtk_fig_line(fig, pose_a->v[0], pose_a->v[1], pose_b->v[0], pose_b->v[1]);
    }
  }

  rtk_fig_color(fig, 0, 0, 1);
    
  // Draw the best path
  if (icon->best_path)
  {
    path = icon->best_path;
    for (j = 0; j < path->pose_count - 1; j++)
    {
      pose_a = path->poses + j;
      pose_b = path->poses + j + 1;
    
      rtk_fig_line(fig, pose_a->v[0], pose_a->v[1], pose_b->v[0], pose_b->v[1]);
    }

    // Draw the path index
    pose_b = path->poses + j;  
    snprintf(text, sizeof(text), "path %d %e", icon->best_path->param.id, icon->best_path->cost);
    rtk_fig_text(fig, pose_b->v[0], pose_b->v[1], 0, text);
  }

  return;
}

