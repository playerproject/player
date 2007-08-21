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

