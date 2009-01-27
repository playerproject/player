
/* @(#)xdr.c	2.1 88/07/29 4.0 RPCSRC */
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
#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = "@(#)xdr.c 1.35 87/08/12";
#endif

/*
 * xdr.c, Generic XDR routines implementation.
 *
 * Copyright (C) 1986, Sun Microsystems, Inc.
 *
 * These are the "generic" xdr routines used to serialize and de-serialize
 * most common data items.  See xdr.h for more info on the interface to
 * xdr.
 */

#include <stdio.h>
#include <limits.h>
#include <string.h>
//#include <libintl.h>

#include "rpc/types.h"
#include "rpc/xdr.h"

//#ifdef USE_IN_LIBIO
//# include <wchar.h>
//#endif

/*
 * constants specific to the xdr "protocol"
 */
#define XDR_FALSE	((long) 0)
#define XDR_TRUE	((long) 1)
#define LASTUNSIGNED	((u_int) 0-1)

/*
 * for unit alignment
 */
static const char xdr_zero[BYTES_PER_XDR_UNIT] = {0, 0, 0, 0};

/*
 * XDR nothing
 */
bool_t
xdr_void (void)
{
  return TRUE;
}

/*
 * XDR integers
 */
bool_t
xdr_int (XDR *xdrs, int *ip)
{

#if INT_MAX < LONG_MAX
  long l;

  switch (xdrs->x_op)
    {
    case XDR_ENCODE:
      l = (long) *ip;
      return XDR_PUTLONG (xdrs, &l);

    case XDR_DECODE:
      if (!XDR_GETLONG (xdrs, &l))
	{
	  return FALSE;
	}
      *ip = (int) l;
    case XDR_FREE:
      return TRUE;
    }
  return FALSE;
#elif INT_MAX == LONG_MAX
  return xdr_long (xdrs, (long *) ip);
#elif INT_MAX == SHRT_MAX
  return xdr_short (xdrs, (short *) ip);
#else
#error unexpected integer sizes in_xdr_int()
#endif
}

/*
 * XDR unsigned integers
 */
bool_t
xdr_u_int (XDR *xdrs, u_int *up)
{
#if UINT_MAX < ULONG_MAX
  long l;

  switch (xdrs->x_op)
    {
    case XDR_ENCODE:
      l = (u_long) * up;
      return XDR_PUTLONG (xdrs, &l);

    case XDR_DECODE:
      if (!XDR_GETLONG (xdrs, &l))
	{
	  return FALSE;
	}
      *up = (u_int) (u_long) l;
    case XDR_FREE:
      return TRUE;
    }
  return FALSE;
#elif UINT_MAX == ULONG_MAX
  return xdr_u_long (xdrs, (u_long *) up);
#elif UINT_MAX == USHRT_MAX
  return xdr_short (xdrs, (short *) up);
#else
#error unexpected integer sizes in_xdr_u_int()
#endif
}

/*
 * XDR long integers
 * The definition of xdr_long() is kept for backward
 * compatibility. Instead xdr_int() should be used.
 */
bool_t
xdr_long (XDR *xdrs, long *lp)
{

  if (xdrs->x_op == XDR_ENCODE
      && (sizeof (int32_t) == sizeof (long)
	  || (int32_t) *lp == *lp))
    return XDR_PUTLONG (xdrs, lp);

  if (xdrs->x_op == XDR_DECODE)
    return XDR_GETLONG (xdrs, lp);

  if (xdrs->x_op == XDR_FREE)
    return TRUE;

  return FALSE;
}

/*
 * XDR unsigned long integers
 * The definition of xdr_u_long() is kept for backward
 * compatibility. Instead xdr_u_int() should be used.
 */
bool_t
xdr_u_long (XDR *xdrs, u_long *ulp)
{
  switch (xdrs->x_op)
    {
    case XDR_DECODE:
      {
	long int tmp;

	if (XDR_GETLONG (xdrs, &tmp) == FALSE)
	  return FALSE;

	*ulp = (uint32_t) tmp;
	return TRUE;
      }

    case XDR_ENCODE:
      if (sizeof (uint32_t) != sizeof (u_long)
	  && (uint32_t) *ulp != *ulp)
	return FALSE;

      return XDR_PUTLONG (xdrs, (long *) ulp);

    case XDR_FREE:
      return TRUE;
    }
  return FALSE;
}

/*
 * XDR hyper integers
 * same as xdr_u_hyper - open coded to save a proc call!
 */
bool_t
xdr_hyper (XDR *xdrs, quad_t *llp)
{
  long int t1, t2;

  if (xdrs->x_op == XDR_ENCODE)
    {
      t1 = (long) ((*llp) >> 32);
      t2 = (long) (*llp);
      return (XDR_PUTLONG(xdrs, &t1) && XDR_PUTLONG(xdrs, &t2));
    }

  if (xdrs->x_op == XDR_DECODE)
    {
      if (!XDR_GETLONG(xdrs, &t1) || !XDR_GETLONG(xdrs, &t2))
	return FALSE;
      *llp = ((quad_t) t1) << 32;
      *llp |= (uint32_t) t2;
      return TRUE;
    }

  if (xdrs->x_op == XDR_FREE)
    return TRUE;

  return FALSE;
}

/*
 * XDR hyper integers
 * same as xdr_hyper - open coded to save a proc call!
 */
bool_t
xdr_u_hyper (XDR *xdrs, u_quad_t *ullp)
{
  long int t1, t2;

  if (xdrs->x_op == XDR_ENCODE)
    {
      t1 = (unsigned long) ((*ullp) >> 32);
      t2 = (unsigned long) (*ullp);
      return (XDR_PUTLONG(xdrs, &t1) && XDR_PUTLONG(xdrs, &t2));
    }

  if (xdrs->x_op == XDR_DECODE)
    {
      if (!XDR_GETLONG(xdrs, &t1) || !XDR_GETLONG(xdrs, &t2))
	return FALSE;
      *ullp = ((u_quad_t) t1) << 32;
      *ullp |= (uint32_t) t2;
      return TRUE;
    }

  if (xdrs->x_op == XDR_FREE)
    return TRUE;

  return FALSE;
}

bool_t
xdr_longlong_t (XDR *xdrs, quad_t *llp)
{
  return xdr_hyper (xdrs, llp);
}

bool_t
xdr_u_longlong_t (XDR *xdrs, u_quad_t *ullp)
{
  return xdr_u_hyper (xdrs, ullp);
}

/*
 * XDR short integers
 */
bool_t
xdr_short (XDR *xdrs, short *sp)
{
  long l;

  switch (xdrs->x_op)
    {
    case XDR_ENCODE:
      l = (long) *sp;
      return XDR_PUTLONG (xdrs, &l);

    case XDR_DECODE:
      if (!XDR_GETLONG (xdrs, &l))
	{
	  return FALSE;
	}
      *sp = (short) l;
      return TRUE;

    case XDR_FREE:
      return TRUE;
    }
  return FALSE;
}

/*
 * XDR unsigned short integers
 */
bool_t
xdr_u_short (XDR *xdrs, u_short *usp)
{
  long l;

  switch (xdrs->x_op)
    {
    case XDR_ENCODE:
      l = (u_long) * usp;
      return XDR_PUTLONG (xdrs, &l);

    case XDR_DECODE:
      if (!XDR_GETLONG (xdrs, &l))
	{
	  return FALSE;
	}
      *usp = (u_short) (u_long) l;
      return TRUE;

    case XDR_FREE:
      return TRUE;
    }
  return FALSE;
}


/*
 * XDR a char
 */
bool_t
xdr_char (XDR *xdrs, char *cp)
{
  int i;

  i = (*cp);
  if (!xdr_int (xdrs, &i))
    {
      return FALSE;
    }
  *cp = i;
  return TRUE;
}

/*
 * XDR an unsigned char
 */
bool_t
xdr_u_char (XDR *xdrs, u_char *cp)
{
  u_int u;

  u = (*cp);
  if (!xdr_u_int (xdrs, &u))
    {
      return FALSE;
    }
  *cp = u;
  return TRUE;
}

/*
 * XDR booleans
 */
bool_t
xdr_bool (XDR *xdrs, bool_t *bp)
{
  long lb;

  switch (xdrs->x_op)
    {
    case XDR_ENCODE:
      lb = *bp ? XDR_TRUE : XDR_FALSE;
      return XDR_PUTLONG (xdrs, &lb);

    case XDR_DECODE:
      if (!XDR_GETLONG (xdrs, &lb))
	{
	  return FALSE;
	}
      *bp = (lb == XDR_FALSE) ? FALSE : TRUE;
      return TRUE;

    case XDR_FREE:
      return TRUE;
    }
  return FALSE;
}

/*
 * XDR enumerations
 */
bool_t
xdr_enum (XDR *xdrs, enum_t *ep)
{
  enum sizecheck
    {
      SIZEVAL
    };				/* used to find the size of an enum */

  /*
   * enums are treated as ints
   */
  if (sizeof (enum sizecheck) == 4)
    {
#if INT_MAX < LONG_MAX
      long l;

      switch (xdrs->x_op)
	{
	case XDR_ENCODE:
	  l = *ep;
	  return XDR_PUTLONG (xdrs, &l);

	case XDR_DECODE:
	  if (!XDR_GETLONG (xdrs, &l))
	    {
	      return FALSE;
	    }
	  *ep = l;
	case XDR_FREE:
	  return TRUE;

	}
      return FALSE;
#else
      return xdr_long (xdrs, (long *) ep);
#endif
    }
  else if (sizeof (enum sizecheck) == sizeof (short))
    {
      return xdr_short (xdrs, (short *) ep);
    }
  else
    {
      return FALSE;
    }
}

/*
 * XDR opaque data
 * Allows the specification of a fixed size sequence of opaque bytes.
 * cp points to the opaque object and cnt gives the byte length.
 */
bool_t
xdr_opaque (XDR *xdrs, caddr_t cp, u_int cnt)
{
  u_int rndup;
  static char crud[BYTES_PER_XDR_UNIT];

  /*
   * if no data we are done
   */
  if (cnt == 0)
    return TRUE;

  /*
   * round byte count to full xdr units
   */
  rndup = cnt % BYTES_PER_XDR_UNIT;
  if (rndup > 0)
    rndup = BYTES_PER_XDR_UNIT - rndup;

  switch (xdrs->x_op)
    {
    case XDR_DECODE:
      if (!XDR_GETBYTES (xdrs, cp, cnt))
	{
	  return FALSE;
	}
      if (rndup == 0)
	return TRUE;
      return XDR_GETBYTES (xdrs, (caddr_t)crud, rndup);

    case XDR_ENCODE:
      if (!XDR_PUTBYTES (xdrs, cp, cnt))
	{
	  return FALSE;
	}
      if (rndup == 0)
	return TRUE;
      return XDR_PUTBYTES (xdrs, xdr_zero, rndup);

    case XDR_FREE:
      return TRUE;
    }
  return FALSE;
}

/*
 * XDR counted bytes
 * *cpp is a pointer to the bytes, *sizep is the count.
 * If *cpp is NULL maxsize bytes are allocated
 */
bool_t
xdr_bytes (xdrs, cpp, sizep, maxsize)
     XDR *xdrs;
     char **cpp;
     u_int *sizep;
     u_int maxsize;
{
  char *sp = *cpp;	/* sp is the actual string pointer */
  u_int nodesize;

  /*
   * first deal with the length since xdr bytes are counted
   */
  if (!xdr_u_int (xdrs, sizep))
    {
      return FALSE;
    }
  nodesize = *sizep;
  if ((nodesize > maxsize) && (xdrs->x_op != XDR_FREE))
    {
      return FALSE;
    }

  /*
   * now deal with the actual bytes
   */
  switch (xdrs->x_op)
    {
    case XDR_DECODE:
      if (nodesize == 0)
	{
	  return TRUE;
	}
      if (sp == NULL)
	{
	  *cpp = sp = (char *) mem_alloc (nodesize);
	}
      if (sp == NULL)
	{
	  fprintf (stderr, "%s", "xdr_bytes: out of memory\n");
	  return FALSE;
	}
      /* fall into ... */

    case XDR_ENCODE:
      return xdr_opaque (xdrs, sp, nodesize);

    case XDR_FREE:
      if (sp != NULL)
	{
	  mem_free (sp, nodesize);
	  *cpp = NULL;
	}
      return TRUE;
    }
  return FALSE;
}

