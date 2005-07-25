#include "pack.h"
#include "functiontable.h"

typedef struct
{
  uint16_t interface;
  uint8_t subtype;
  player_pack_fn_t func;
} playerxdr_function_t;


static playerxdr_function_t ftable[] = 
{
  {PLAYER_PLAYER_CODE, PLAYER_PLAYER_DEVLIST, (player_pack_fn_t)player_device_devlist_pack},
  {PLAYER_PLAYER_CODE, PLAYER_PLAYER_DRIVERINFO, (player_pack_fn_t)player_device_driverinfo_pack},
  {PLAYER_PLAYER_CODE, PLAYER_PLAYER_DEV, (player_pack_fn_t)player_device_req_pack},
  {PLAYER_PLAYER_CODE, PLAYER_PLAYER_DATA, (player_pack_fn_t)player_device_data_req_pack},
  {PLAYER_PLAYER_CODE, PLAYER_PLAYER_DATAMODE, (player_pack_fn_t)player_device_datamode_req_pack},
  {PLAYER_PLAYER_CODE, PLAYER_PLAYER_DATAFREQ, (player_pack_fn_t)player_device_datafreq_req_pack},
  {PLAYER_PLAYER_CODE, PLAYER_PLAYER_AUTH, (player_pack_fn_t)player_device_auth_req_pack},
  {PLAYER_PLAYER_CODE, PLAYER_PLAYER_NAMESERVICE, (player_pack_fn_t)player_device_nameservice_req_pack},
  {0,0,NULL}
};


player_pack_fn_t
playerxdr_get_func(uint16_t interface, uint8_t subtype)
{
  playerxdr_function_t* curr;

  for(curr=ftable; curr->func; curr++)
  {
    if((curr->interface == interface) && (curr->subtype == subtype))
      return(curr->func);
  }
  return(NULL);
}

