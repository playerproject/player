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

#include <stdio.h>
#include "percolate.h"


void swapItem(TAsoc *a, TAsoc *b){
	TAsoc c;

	c=*a;
	*a=*b;
	*b=c;
}

void perc_down(TAsoc a[], int i, int n) {
  int child; TAsoc tmp;
  for (tmp=a[i]; i*2 <= n; i=child) {
    child = i*2;
    if ((child != n) && (a[child+1].dist > a[child].dist))
      child++;
    if (tmp.dist < a[child].dist)
      a[i] = a[child];
    else
      break;
  }
  a[i] = tmp;
}

void heapsort(TAsoc a[], int n) {
  int i, j;
  j = n;
  for (i=n/2; i>0; i--)  /* BuildHeap */
    perc_down(a,i,j);
  i = 1;
  for (j=n; j>=2; j--) {
    swapItem(&a[i],&a[j]);   /* DeleteMax */
    perc_down(a,i,j-1);
  }
}
