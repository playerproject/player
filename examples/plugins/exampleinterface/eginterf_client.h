/*
 * This file declares a C client library proxy for the interface. The functions
 * are defined in eginterf_client.c.
 */

#ifdef __cplusplus
extern "C" {
#endif

// If you want your plugin interface to be usable on Windows, this block is
// important. Make sure to replace EXAMPLE with the exact name of your
// interface as specified in the interfName argument of
// PLAYER_ADD_PLUGIN_INTERFACE() in your CMakeLists.txt file, but in captital
// letters, all throughout this file. Also ensure that the "example" in
// "example_EXPORTS" is replaced with the exact name of your interface.
#if defined (WIN32)
  #if defined (PLAYER_STATIC)
    #define EXAMPLE_EXPORT
  #elif defined (example_EXPORTS)
    #define EXAMPLE_EXPORT    __declspec (dllexport)
  #else
    #define EXAMPLE_EXPORT    __declspec (dllimport)
  #endif
#else
  #define EXAMPLE_EXPORT
#endif

typedef struct
{
	/* Device info; must be at the start of all device structures. */
	playerc_device_t info;

	/* Other stuff the proxy should store during its lifetime. */
	uint32_t stuff_count;
	double *stuff;
	int value;
} eginterf_t;

EXAMPLE_EXPORT eginterf_t *eginterf_create (playerc_client_t *client, int index);

EXAMPLE_EXPORT void eginterf_destroy (eginterf_t *device);

EXAMPLE_EXPORT int eginterf_subscribe (eginterf_t *device, int access);

EXAMPLE_EXPORT int eginterf_unsubscribe (eginterf_t *device);

EXAMPLE_EXPORT int eginterf_cmd (eginterf_t *device, char value);

EXAMPLE_EXPORT int eginterf_req (eginterf_t *device, int blah);

#ifdef __cplusplus
}
#endif
