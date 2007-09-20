#include <rpc/types.h>
#include <rpc/xdr.h>

#include "eginterf_xdr.h"
#include <string.h>
#include <stdlib.h>


int xdr_player_eginterf_data (XDR* xdrs, player_eginterf_data * msg)
{   if(xdr_u_int(xdrs,&msg->stuff_count) != 1)
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
player_eginterf_data_pack(void* buf, size_t buflen, player_eginterf_data * msg, int op)
{
  XDR xdrs;
  int len;
  if(!buflen)
    return 0;
  xdrmem_create(&xdrs, buf, buflen, op);
  if(xdr_player_eginterf_data(&xdrs,msg) != 1)
    return(-1);
  if(op == PLAYERXDR_ENCODE)
    len = xdr_getpos(&xdrs);
  else
    len = sizeof(player_eginterf_data);
  xdr_destroy(&xdrs);
  return(len);
} 
unsigned int player_eginterf_data_copy(player_eginterf_data *dest, const player_eginterf_data *src)
{      
  
  unsigned int size = 0;
  if(src == NULL)
    return(0);
  size += sizeof(uint32_t)*1;
  memcpy(&dest->stuff_count,&src->stuff_count,sizeof(uint32_t)*1); 
  if(src->stuff != NULL && src->stuff_count > 0)
  {
    if((dest->stuff = calloc(src->stuff_count,sizeof(double))) == NULL)
      return(0);
  }
  else
    dest->stuff = NULL;
  size += sizeof(double)*src->stuff_count;
  memcpy(dest->stuff,src->stuff,sizeof(double)*src->stuff_count); 
  return(size);
}
void player_eginterf_data_cleanup(const player_eginterf_data *msg)
{      
  
  if(msg == NULL)
    return;
  free(msg->stuff); 
}
player_eginterf_data * player_eginterf_data_clone(const player_eginterf_data *msg)
{      
  player_eginterf_data * clone = malloc(sizeof(player_eginterf_data));
  if (clone)
    player_eginterf_data_copy(clone,msg);
  return clone;
}
void player_eginterf_data_free(player_eginterf_data *msg)
{      
  player_eginterf_data_cleanup(msg);
  free(msg);
}

int xdr_player_eginterf_req (XDR* xdrs, player_eginterf_req * msg)
{   if(xdr_int(xdrs,&msg->value) != 1)
    return(0);
  return(1);
}
int 
player_eginterf_req_pack(void* buf, size_t buflen, player_eginterf_req * msg, int op)
{
  XDR xdrs;
  int len;
  if(!buflen)
    return 0;
  xdrmem_create(&xdrs, buf, buflen, op);
  if(xdr_player_eginterf_req(&xdrs,msg) != 1)
    return(-1);
  if(op == PLAYERXDR_ENCODE)
    len = xdr_getpos(&xdrs);
  else
    len = sizeof(player_eginterf_req);
  xdr_destroy(&xdrs);
  return(len);
} 
unsigned int player_eginterf_req_copy(player_eginterf_req *dest, const player_eginterf_req *src)
{
  if (dest == NULL || src == NULL)
    return 0;
  memcpy(dest,src,sizeof(player_eginterf_req));
  return sizeof(player_eginterf_req);
} 
void player_eginterf_req_cleanup(const player_eginterf_req *msg)
{
} 
player_eginterf_req * player_eginterf_req_clone(const player_eginterf_req *msg)
{      
  player_eginterf_req * clone = malloc(sizeof(player_eginterf_req));
  if (clone)
    player_eginterf_req_copy(clone,msg);
  return clone;
}
void player_eginterf_req_free(player_eginterf_req *msg)
{      
  player_eginterf_req_cleanup(msg);
  free(msg);
}

int xdr_player_eginterf_cmd (XDR* xdrs, player_eginterf_cmd * msg)
{   if(xdr_char(xdrs,&msg->doStuff) != 1)
    return(0);
  return(1);
}
int 
player_eginterf_cmd_pack(void* buf, size_t buflen, player_eginterf_cmd * msg, int op)
{
  XDR xdrs;
  int len;
  if(!buflen)
    return 0;
  xdrmem_create(&xdrs, buf, buflen, op);
  if(xdr_player_eginterf_cmd(&xdrs,msg) != 1)
    return(-1);
  if(op == PLAYERXDR_ENCODE)
    len = xdr_getpos(&xdrs);
  else
    len = sizeof(player_eginterf_cmd);
  xdr_destroy(&xdrs);
  return(len);
} 
unsigned int player_eginterf_cmd_copy(player_eginterf_cmd *dest, const player_eginterf_cmd *src)
{
  if (dest == NULL || src == NULL)
    return 0;
  memcpy(dest,src,sizeof(player_eginterf_cmd));
  return sizeof(player_eginterf_cmd);
} 
void player_eginterf_cmd_cleanup(const player_eginterf_cmd *msg)
{
} 
player_eginterf_cmd * player_eginterf_cmd_clone(const player_eginterf_cmd *msg)
{      
  player_eginterf_cmd * clone = malloc(sizeof(player_eginterf_cmd));
  if (clone)
    player_eginterf_cmd_copy(clone,msg);
  return clone;
}
void player_eginterf_cmd_free(player_eginterf_cmd *msg)
{      
  player_eginterf_cmd_cleanup(msg);
  free(msg);
}
