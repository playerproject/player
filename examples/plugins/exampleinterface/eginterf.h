/*
 * The new interface is declared in this file. It consists of a few parts:
 *
 * #define'd interface code and name string
 *
 * These are used throughout the interface. Be careful not to conflict with
 * existing Player interfaces when setting them.
 *
 * Message subtype codes
 *
 * These are used by both the client and the server to differentiate between
 * different messages within the same type group (e.g. different types of data
 * message).
 *
 * Message structures
 *
 * Each message structure defines the layout of data in the body of a message.
 * One message structure can be used by multiple types and subtypes.
 *
 * Exported plugin interface function
 */

#ifndef __EGINTERF_H
#define __EGINTERF_H

#include <libplayerxdr/playerxdr.h>
#include <libplayerxdr/functiontable.h>

#define PLAYER_EGINTERF_CODE 100
#define PLAYER_EGINTERF_STRING "eginterf"

#define EGINTERF_DATA		1
#define EGINTERF_REQ		1
#define EGINTERF_CMD		1

typedef struct player_eginterf_data
{
	uint32_t stuff_count;
	double *stuff;
} player_eginterf_data;

typedef struct player_eginterf_req
{
	int value;
} player_eginterf_req;

typedef struct player_eginterf_cmd
{
	char doStuff;
} player_eginterf_cmd;

playerxdr_function_t* player_plugininterf_gettable (void);

#endif // __EGINTERF_H
