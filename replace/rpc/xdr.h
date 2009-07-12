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
 * xdr.h, External Data Representation Serialization Routines.
 *
 * Copyright (C) 1984, Sun Microsystems, Inc.
 */

#ifndef _RPC_XDR_H
#define _RPC_XDR_H 1

#if defined (WIN32)
  #if defined (PLAYER_STATIC)
    #define PLAYERREPLACE_EXPORT
  #elif defined (playerreplace_EXPORTS)
    #define PLAYERREPLACE_EXPORT    __declspec (dllexport)
  #else
    #define PLAYERREPLACE_EXPORT    __declspec (dllimport)
  #endif
#else
  #define PLAYERREPLACE_EXPORT
#endif

#include <sys/types.h>
#include "rpc/types.h"

#ifndef __const
#define __const const
#endif

/*
 * XDR provides a conventional way for converting between C data
 * types and an external bit-string representation.  Library supplied
 * routines provide for the conversion on built-in C data types.  These
 * routines and utility routines defined here are used to help implement
 * a type encode/decode routine for each user-defined type.
 *
 * Each data type provides a single procedure which takes two arguments:
 *
 *      bool_t
 *      xdrproc(xdrs, argresp)
 *              XDR *xdrs;
 *              <type> *argresp;
 *
 * xdrs is an instance of a XDR handle, to which or from which the data
 * type is to be converted.  argresp is a pointer to the structure to be
 * converted.  The XDR handle contains an operation field which indicates
 * which of the operations (ENCODE, DECODE * or FREE) is to be performed.
 *
 * XDR_DECODE may allocate space if the pointer argresp is null.  This
 * data can be freed with the XDR_FREE operation.
 *
 * We write only one procedure per data type to make it easy
 * to keep the encode and decode procedures for a data type consistent.
 * In many cases the same code performs all operations on a user defined type,
 * because all the hard work is done in the component type routines.
 * decode as a series of calls on the nested data types.
 */

/*
 * Xdr operations.  XDR_ENCODE causes the type to be encoded into the
 * stream.  XDR_DECODE causes the type to be extracted from the stream.
 * XDR_FREE can be used to release the space allocated by an XDR_DECODE
 * request.
 */
enum xdr_op {
  XDR_ENCODE = 0,
  XDR_DECODE = 1,
  XDR_FREE = 2
};

/*
 * This is the number of bytes per unit of external data.
 */
#define BYTES_PER_XDR_UNIT	(4)
/*
 * This only works if the above is a power of 2.  But it's defined to be
 * 4 by the appropriate RFCs.  So it will work.  And it's normally quicker
 * than the old routine.
 */
#if 1
#define RNDUP(x)  (((x) + BYTES_PER_XDR_UNIT - 1) & ~(BYTES_PER_XDR_UNIT - 1))
#else /* this is the old routine */
#define RNDUP(x)  ((((x) + BYTES_PER_XDR_UNIT - 1) / BYTES_PER_XDR_UNIT) \
		    * BYTES_PER_XDR_UNIT)
#endif

/*
 *  * The XDR handle.
 *   * Contains operation which is being applied to the stream,
 *    * an operations vector for the particular implementation (e.g. see xdr_mem.c),
 *     * and two private fields for the use of the particular implementation.
 *      */
typedef struct XDR XDR;
struct XDR
{
	enum xdr_op x_op;  /* operation; fast additional param */
	struct xdr_ops
	{
		bool_t (*x_getlong) (XDR *__xdrs, long *__lp);
		/* get a long from underlying stream */
		bool_t (*x_putlong) (XDR *__xdrs, __const long *__lp);
		/* put a long to " */
		bool_t (*x_getbytes) (XDR *__xdrs, caddr_t __addr, u_int __len);
		/* get some bytes from " */
		bool_t (*x_putbytes) (XDR *__xdrs, __const char *__addr, u_int __len);
		/* put some bytes to " */
		u_int (*x_getpostn) (__const XDR *__xdrs);
		/* returns bytes off from beginning */
		bool_t (*x_setpostn) (XDR *__xdrs, u_int __pos);
		/* lets you reposition the stream */
		int32_t *(*x_inline) (XDR *__xdrs, u_int __len);
		/* buf quick ptr to buffered data */
		void (*x_destroy) (XDR *__xdrs);
		/* free privates of this xdr_stream */
		bool_t (*x_getint32) (XDR *__xdrs, int32_t *__ip);
		/* get a int from underlying stream */
		bool_t (*x_putint32) (XDR *__xdrs, __const int32_t *__ip);
		/* put a int to " */
	}
	*x_ops;
	caddr_t x_public;   /* users' data */
	caddr_t x_private;  /* pointer to private data */
	caddr_t x_base;     /* private used for position info */
	u_int x_handy;      /* extra private word */
};

/*
 * A xdrproc_t exists for each data type which is to be encoded or decoded.
 *
 * The second argument to the xdrproc_t is a pointer to an opaque pointer.
 * The opaque pointer generally points to a structure of the data type
 * to be decoded.  If this pointer is 0, then the type routines should
 * allocate dynamic storage of the appropriate size and return it.
 * bool_t       (*xdrproc_t)(XDR *, caddr_t *);
 */
typedef bool_t (*xdrproc_t) (XDR *, void *,...);

/* XDR using memory buffers */
PLAYERREPLACE_EXPORT extern void xdrmem_create (XDR *__xdrs, __const caddr_t __addr,
					                            u_int __size, enum xdr_op __xop);

/*
 * Operations defined on a XDR handle
 *
 * XDR          *xdrs;
 * int32_t      *int32p;
 * long         *longp;
 * caddr_t       addr;
 * u_int         len;
 * u_int         pos;
 */
#define XDR_GETINT32(xdrs, int32p)                      \
        (*(xdrs)->x_ops->x_getint32)(xdrs, int32p)
#define xdr_getint32(xdrs, int32p)                      \
        (*(xdrs)->x_ops->x_getint32)(xdrs, int32p)

#define XDR_PUTINT32(xdrs, int32p)                      \
        (*(xdrs)->x_ops->x_putint32)(xdrs, int32p)
#define xdr_putint32(xdrs, int32p)                      \
        (*(xdrs)->x_ops->x_putint32)(xdrs, int32p)

#define XDR_GETLONG(xdrs, longp)			\
	(*(xdrs)->x_ops->x_getlong)(xdrs, longp)
#define xdr_getlong(xdrs, longp)			\
	(*(xdrs)->x_ops->x_getlong)(xdrs, longp)

#define XDR_PUTLONG(xdrs, longp)			\
	(*(xdrs)->x_ops->x_putlong)(xdrs, longp)
#define xdr_putlong(xdrs, longp)			\
	(*(xdrs)->x_ops->x_putlong)(xdrs, longp)

#define XDR_GETBYTES(xdrs, addr, len)			\
	(*(xdrs)->x_ops->x_getbytes)(xdrs, addr, len)
#define xdr_getbytes(xdrs, addr, len)			\
	(*(xdrs)->x_ops->x_getbytes)(xdrs, addr, len)

#define XDR_PUTBYTES(xdrs, addr, len)			\
	(*(xdrs)->x_ops->x_putbytes)(xdrs, addr, len)
#define xdr_putbytes(xdrs, addr, len)			\
	(*(xdrs)->x_ops->x_putbytes)(xdrs, addr, len)

#define XDR_GETPOS(xdrs)				\
	(*(xdrs)->x_ops->x_getpostn)(xdrs)
#define xdr_getpos(xdrs)				\
	(*(xdrs)->x_ops->x_getpostn)(xdrs)

#define XDR_SETPOS(xdrs, pos)				\
	(*(xdrs)->x_ops->x_setpostn)(xdrs, pos)
#define xdr_setpos(xdrs, pos)				\
	(*(xdrs)->x_ops->x_setpostn)(xdrs, pos)

#define	XDR_INLINE(xdrs, len)				\
	(*(xdrs)->x_ops->x_inline)(xdrs, len)
#define	xdr_inline(xdrs, len)				\
	(*(xdrs)->x_ops->x_inline)(xdrs, len)

#define	XDR_DESTROY(xdrs)					\
	do {							\
		if ((xdrs)->x_ops->x_destroy)			\
			(*(xdrs)->x_ops->x_destroy)(xdrs);	\
	} while (0)
#define	xdr_destroy(xdrs)					\
	do {							\
		if ((xdrs)->x_ops->x_destroy)			\
			(*(xdrs)->x_ops->x_destroy)(xdrs);	\
	} while (0)

/*
 * These are the "generic" xdr routines.
 * None of these can have const applied because it's not possible to
 * know whether the call is a read or a write to the passed parameter
 * also, the XDR structure is always updated by some of these calls.
 */
PLAYERREPLACE_EXPORT extern bool_t xdr_void (void);
PLAYERREPLACE_EXPORT extern bool_t xdr_short (XDR *__xdrs, short *__sp);
PLAYERREPLACE_EXPORT extern bool_t xdr_u_short (XDR *__xdrs, u_short *__usp);
PLAYERREPLACE_EXPORT extern bool_t xdr_int (XDR *__xdrs, int *__ip);
PLAYERREPLACE_EXPORT extern bool_t xdr_u_int (XDR *__xdrs, u_int *__up);
PLAYERREPLACE_EXPORT extern bool_t xdr_long (XDR *__xdrs, long *__lp);
PLAYERREPLACE_EXPORT extern bool_t xdr_u_long (XDR *__xdrs, u_long *__ulp);
PLAYERREPLACE_EXPORT extern bool_t xdr_hyper (XDR *__xdrs, quad_t *__llp);
PLAYERREPLACE_EXPORT extern bool_t xdr_u_hyper (XDR *__xdrs, u_quad_t *__ullp);
PLAYERREPLACE_EXPORT extern bool_t xdr_longlong_t (XDR *__xdrs, quad_t *__llp);
PLAYERREPLACE_EXPORT extern bool_t xdr_u_longlong_t (XDR *__xdrs, u_quad_t *__ullp);
PLAYERREPLACE_EXPORT extern bool_t xdr_int8_t (XDR *__xdrs, int8_t *__ip);
PLAYERREPLACE_EXPORT extern bool_t xdr_uint8_t (XDR *__xdrs, uint8_t *__up);
PLAYERREPLACE_EXPORT extern bool_t xdr_int16_t (XDR *__xdrs, int16_t *__ip);
PLAYERREPLACE_EXPORT extern bool_t xdr_uint16_t (XDR *__xdrs, uint16_t *__up);
PLAYERREPLACE_EXPORT extern bool_t xdr_int32_t (XDR *__xdrs, int32_t *__ip);
PLAYERREPLACE_EXPORT extern bool_t xdr_uint32_t (XDR *__xdrs, uint32_t *__up);
PLAYERREPLACE_EXPORT extern bool_t xdr_int64_t (XDR *__xdrs, int64_t *__ip);
PLAYERREPLACE_EXPORT extern bool_t xdr_uint64_t (XDR *__xdrs, uint64_t *__up);
PLAYERREPLACE_EXPORT extern bool_t xdr_bool (XDR *__xdrs, bool_t *__bp);
PLAYERREPLACE_EXPORT extern bool_t xdr_enum (XDR *__xdrs, enum_t *__ep);
PLAYERREPLACE_EXPORT extern bool_t xdr_array (XDR * _xdrs, caddr_t *__addrp, u_int *__sizep,
			 u_int __maxsize, u_int __elsize, xdrproc_t __elproc);
PLAYERREPLACE_EXPORT extern bool_t xdr_bytes (XDR *__xdrs, char **__cpp, u_int *__sizep,
			 u_int __maxsize);
PLAYERREPLACE_EXPORT extern bool_t xdr_opaque (XDR *__xdrs, caddr_t __cp, u_int __cnt);
PLAYERREPLACE_EXPORT extern bool_t xdr_char (XDR *__xdrs, char *__cp);
PLAYERREPLACE_EXPORT extern bool_t xdr_u_char (XDR *__xdrs, u_char *__cp);
PLAYERREPLACE_EXPORT extern bool_t xdr_vector (XDR *__xdrs, char *__basep, u_int __nelem,
			  u_int __elemsize, xdrproc_t __xdr_elem);
PLAYERREPLACE_EXPORT extern bool_t xdr_float (XDR *__xdrs, float *__fp);
PLAYERREPLACE_EXPORT extern bool_t xdr_double (XDR *__xdrs, double *__dp);
PLAYERREPLACE_EXPORT extern u_long xdr_sizeof (xdrproc_t, void *);

#endif /* rpc/xdr.h */

