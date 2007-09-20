#ifndef _EGINTERF_

#include <libplayerxdr/playerxdr.h>

#include "eginterf.h"

#ifdef __cplusplus
extern "C" {
#endif

int xdr_player_eginterf_data (XDR* xdrs, player_eginterf_data * msg);
int player_eginterf_data_pack(void* buf, size_t buflen, player_eginterf_data * msg, int op);
unsigned int player_eginterf_data_copy(player_eginterf_data *dest, const player_eginterf_data *src);
void player_eginterf_data_cleanup(const player_eginterf_data *msg);
player_eginterf_data * player_eginterf_data_clone(const player_eginterf_data *msg);
void player_eginterf_data_free(player_eginterf_data *msg);
int xdr_player_eginterf_req (XDR* xdrs, player_eginterf_req * msg);
int player_eginterf_req_pack(void* buf, size_t buflen, player_eginterf_req * msg, int op);
unsigned int player_eginterf_req_copy(player_eginterf_req *dest, const player_eginterf_req *src);
void player_eginterf_req_cleanup(const player_eginterf_req *msg);
player_eginterf_req * player_eginterf_req_clone(const player_eginterf_req *msg);
void player_eginterf_req_free(player_eginterf_req *msg);
int xdr_player_eginterf_cmd (XDR* xdrs, player_eginterf_cmd * msg);
int player_eginterf_cmd_pack(void* buf, size_t buflen, player_eginterf_cmd * msg, int op);
unsigned int player_eginterf_cmd_copy(player_eginterf_cmd *dest, const player_eginterf_cmd *src);
void player_eginterf_cmd_cleanup(const player_eginterf_cmd *msg);
player_eginterf_cmd * player_eginterf_cmd_clone(const player_eginterf_cmd *msg);
void player_eginterf_cmd_free(player_eginterf_cmd *msg);

#ifdef __cplusplus
}
#endif

#endif
