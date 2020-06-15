#include <string.h>
#include <etdos/plugin.h>
#define P_EXPORT __attribute__ ((visibility ("default")))

P_EXPORT
void p_init(hooqz_t *hooks)
{
	ed = hooks;
}

P_EXPORT
void p_preconnect()
{

}

P_EXPORT
void p_connected()
{

}

P_EXPORT
void p_gamestate(gameState_t *q)
{

}

P_EXPORT
void p_snapshot(clSnapshot_t *q)
{

}

P_EXPORT
void p_extern_cmd(char *cmd)
{

}

P_EXPORT
usercmd_t p_ucmd()
{
	usercmd_t cmd;
	memset( &cmd, 0, sizeof( cmd ) );
	return cmd;
}
