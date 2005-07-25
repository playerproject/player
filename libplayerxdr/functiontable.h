

/** Generic Prototype for a player XDR packing function */
typedef int (*player_pack_fn_t) (void* buf, size_t buflen, void* msg, int op);

player_pack_fn_t playerxdr_get_func(uint16_t interface, uint8_t subtype);
