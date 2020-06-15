/*
	etdos plugin framework

	multiple plugins can be loaded!

	each plugin *must* have unique cfg identifier
	or collisions occur

	only compatible plugins can be loaded together
	or behaviour is undefined, for example, plugins
	which both create ucmds are incompatible but
	plugins which just set different userinfo key/values
	are compatible

	You can use plugins to add easy support for new "mods"
*/

#ifndef _PLUGIN_H_
#define _PLUGIN_H_

#include "shared.h"
#include "protocol.h"
#include "cvar.h"

/* plugin side */
typedef struct {
	/* function hooks */
	cvar_t *(*Cvar_Get)(const char *, const char *, int);
	void (*sv_reliable_cmd)(const char *);
	void (*write_packet)();
	char *(*Cvar_InfoString)(int);
	char *(*va)(char *, ... );

	/* struct hooks */
	client_t *clients;
	playerState_t *ps;
	int *time;
	int *clnum;
} hooqz_t;

hooqz_t *ed;

/* daemon side */
typedef struct {
	void *handle;
	void (*p_init)(hooqz_t *);
	void (*p_preconnect)();
	void (*p_connected)();
	void (*p_gamestate)(gameState_t *);
	void (*p_snapshot)(clSnapshot_t *);
	void (*p_extern_cmd)(char *);
	usercmd_t (*p_ucmd)();

	hooqz_t phooks;
} plugin_t;

#endif
