/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 */
/*
 * xdr_sizeof.c
 *
 * Copyright 1990 Sun Microsystems, Inc.
 *
 * General purpose routine to see how much space something will use
 * when serialized using XDR.
 */

#include "rpc/types.h"
#include "rpc/xdr.h"
#include <sys/types.h>
#include <stdlib.h>

/* ARGSUSED */
static bool_t
x_putlong (XDR *xdrs, const long *longp)
{
  xdrs->x_handy += BYTES_PER_XDR_UNIT;
  return TRUE;
}

/* ARGSUSED */
static bool_t
x_putbytes (XDR *xdrs, const char *bp, u_int len)
{
  xdrs->x_handy += len;
  return TRUE;
}

static u_int
x_getpostn (const XDR *xdrs)
{
  return xdrs->x_handy;
}

/* ARGSUSED */
static bool_t
x_setpostn (XDR *xdrs, u_int len)
{
  /* This is not allowed */
  return FALSE;
}

static int32_t *
x_inline (XDR *xdrs, u_int len)
{
  if (len == 0)
    return NULL;
  if (xdrs->x_op != XDR_ENCODE)
    return NULL;
  if (len < (u_int) (long int) xdrs->x_base)
    {
      /* x_private was already allocated */
      xdrs->x_handy += len;
      return (int32_t *) xdrs->x_private;
    }
  else
    {
      /* Free the earlier space and allocate new area */
      if (xdrs->x_private)
	free (xdrs->x_private);
      if ((xdrs->x_private = (caddr_t) malloc (len)) == NULL)
	{
	  xdrs->x_base = 0;
	  return NULL;
	}
      xdrs->x_base = (void *) (long) len;
      xdrs->x_handy += len;
      return (int32_t *) xdrs->x_private;
    }
}

static int
harmless (void)
{
  /* Always return FALSE/NULL, as the case may be */
  return 0;
}

static void
x_destroy (XDR *xdrs)
{
  xdrs->x_handy = 0;
  xdrs->x_base = 0;
  if (xdrs->x_private)
    {
      free (xdrs->x_private);
      xdrs->x_private = NULL;
    }
  return;
}

static bool_t
x_putint32 (XDR *xdrs, const int32_t *int32p)
{
  xdrs->x_handy += BYTES_PER_XDR_UNIT;
  return TRUE;
}

unsigned long
xdr_sizeof (xdrproc_t func, void *data)
{
  XDR x;
  struct xdr_ops ops;
  bool_t stat;
  /* to stop ANSI-C compiler from complaining */
  typedef bool_t (*dummyfunc1) (XDR *, long *);
  typedef bool_t (*dummyfunc2) (XDR *, caddr_t, u_int);
  typedef bool_t (*dummyfunc3) (XDR *, int32_t *);

  ops.x_putlong = x_putlong;
  ops.x_putbytes = x_putbytes;
  ops.x_inline = x_inline;
  ops.x_getpostn = x_getpostn;
  ops.x_setpostn = x_setpostn;
  ops.x_destroy = x_destroy;
  ops.x_putint32 = x_putint32;

  /* the other harmless ones */
  ops.x_getlong = (dummyfunc1) harmless;
  ops.x_getbytes = (dummyfunc2) harmless;
  ops.x_getint32 = (dummyfunc3) harmless;

  x.x_op = XDR_ENCODE;
  x.x_ops = &ops;
  x.x_handy = 0;
  x.x_private = (caddr_t) NULL;
  x.x_base = (caddr_t) 0;

  stat = func (&x, data);
  if (x.x_private)
    free (x.x_private);
  return stat == TRUE ? x.x_handy : 0;
}
