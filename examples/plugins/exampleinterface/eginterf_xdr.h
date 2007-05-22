/*
 * The functions that tell XDR how to pack, unpack, deep copy and clean up
 * message structures are declared here. This file was generated automatically
 * by playerxdrgen.py, then cleaned up.
 */

#include "eginterf.h"

int xdr_player_eginterf_data(XDR* xdrs, player_eginterf_data* msg);
int player_eginterf_data_pack(void* buf, size_t buflen, player_eginterf_data* msg, int op);
unsigned int player_eginterf_data_dpcpy(const player_eginterf_data* src, player_eginterf_data* dest);
void player_eginterf_data_cleanup(player_eginterf_data* msg);
int xdr_player_eginterf_req(XDR* xdrs, player_eginterf_req* msg);
int player_eginterf_req_pack(void* buf, size_t buflen, player_eginterf_req* msg, int op);
int xdr_player_eginterf_cmd(XDR* xdrs, player_eginterf_cmd* msg);
int player_eginterf_cmd_pack(void* buf, size_t buflen, player_eginterf_cmd* msg, int op);
