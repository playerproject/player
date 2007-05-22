/*
 * The functions that tell XDR how to pack, unpack, deep copy and clean up
 * message structures are defined here. This file was generated automatically
 * by playerxdrgen.py.
 */

#include <libplayerxdr/playerxdr.h>
#include <string.h>

#include "eginterf_xdr.h"

int
xdr_player_eginterf_data(XDR* xdrs, player_eginterf_data* msg)
{
  if(xdr_u_int(xdrs,&msg->stuff_count) != 1)
    return(0);
  if(xdrs->x_op == XDR_DECODE)
  {
    if((msg->stuff = malloc(msg->stuff_count*sizeof(double))) == NULL)
      return(0);
  }
  {
    double* stuff_p = msg->stuff;
    if(xdr_array(xdrs, (char**)&stuff_p, &msg->stuff_count, msg->stuff_count, sizeof(double), (xdrproc_t)xdr_double) != 1)
      return(0);
  }
  return(1);
}

int
player_eginterf_data_pack(void* buf, size_t buflen, player_eginterf_data* msg, int op)
{
  XDR xdrs;
  int len;
  if(!buflen)
    return(0);
  xdrmem_create(&xdrs, buf, buflen, op);
  if(xdr_u_int(&xdrs,&msg->stuff_count) != 1)
    return(-1);
  if(op == PLAYERXDR_DECODE)
  {
    if((msg->stuff = malloc(msg->stuff_count*sizeof(double))) == NULL)
      return(-1);
  }
  {
    double* stuff_p = msg->stuff;
    if(xdr_array(&xdrs, (char**)&stuff_p, &msg->stuff_count, msg->stuff_count, sizeof(double), (xdrproc_t)xdr_double) != 1)
      return(-1);
  }
  if(op == PLAYERXDR_ENCODE)
    len = xdr_getpos(&xdrs);
  else
    len = sizeof(player_eginterf_data);
  xdr_destroy(&xdrs);
  return(len);
}

unsigned int
player_eginterf_data_dpcpy(const player_eginterf_data* src, player_eginterf_data* dest)
{
  if(src == NULL)
    return(0);
  unsigned int size = 0;
  if(src->stuff != NULL && src->stuff_count > 0)
  {
    if((dest->stuff = malloc(src->stuff_count*sizeof(double))) == NULL)
      return(0);
    memcpy(dest->stuff, src->stuff, src->stuff_count*sizeof(double));
    size += src->stuff_count*sizeof(double);
  }
  else
    dest->stuff = NULL;
  return(size);
}

void
player_eginterf_data_cleanup(player_eginterf_data* msg)
{
  if(msg == NULL)
    return;
  if(msg->stuff == NULL)
    return;
  free(msg->stuff);
}

int
xdr_player_eginterf_req(XDR* xdrs, player_eginterf_req* msg)
{
  if(xdr_int(xdrs,&msg->value) != 1)
    return(0);
  return(1);
}

int
player_eginterf_req_pack(void* buf, size_t buflen, player_eginterf_req* msg, int op)
{
  XDR xdrs;
  int len;
  if(!buflen)
    return(0);
  xdrmem_create(&xdrs, buf, buflen, op);
  if(xdr_int(&xdrs,&msg->value) != 1)
    return(-1);
  if(op == PLAYERXDR_ENCODE)
    len = xdr_getpos(&xdrs);
  else
    len = sizeof(player_eginterf_req);
  xdr_destroy(&xdrs);
  return(len);
}

int
xdr_player_eginterf_cmd(XDR* xdrs, player_eginterf_cmd* msg)
{
  if(xdr_char(xdrs,&msg->doStuff) != 1)
    return(0);
  return(1);
}

int
player_eginterf_cmd_pack(void* buf, size_t buflen, player_eginterf_cmd* msg, int op)
{
  XDR xdrs;
  int len;
  if(!buflen)
    return(0);
  xdrmem_create(&xdrs, buf, buflen, op);
  if(xdr_char(&xdrs,&msg->doStuff) != 1)
    return(-1);
  if(op == PLAYERXDR_ENCODE)
    len = xdr_getpos(&xdrs);
  else
    len = sizeof(player_eginterf_cmd);
  xdr_destroy(&xdrs);
  return(len);
}

