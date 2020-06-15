/*
	totally fakes a fully functional jaymod 2.x client
	(basic qvm referencing is still done in net.c send_pure_checksums())

	server sample

	clientCommand: ^00xdeadbeef : 1 : ws 2
	clientCommand: ^00xdeadbeef : 2 : obj -1
	clientCommand: ^00xdeadbeef : 3 : cp 2855 1848930406 1848930406 @ 1848930406 343948379 -1251795828 33266179
	clientCommand: ^00xdeadbeef : 4 : auth "Jaymod 2.1.7"
	clientCommand: ^00xdeadbeef : 5 : userinfo "\cg_delag\1\cg_jaymiscflags\0\cg_etVersion\Enemy Territory, ET 2.60c\cg_uinfo\13 0 30\cg_jaymod_title\Jaymod 2.1.7\cl_cpu\Intel(R) Pentium(R) M processor\g_password\none\cl_guid\unknown\cl_wwwDownload\0\name\^00xdeadbeef\rate\25000\snaps\20\cl_anonymous\0\cl_punkbuster\0"
*/

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <etdos/plugin.h>

#define P_EXPORT __attribute__ ((visibility ("default")))

char jay_version[16];

P_EXPORT
void p_init(hooqz_t *hooks)
{
	ed = hooks;
	strcpy(jay_version, "2.1.7");
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
	// set some jay-specific userinfo cvars
	ed->Cvar_Get("cg_delag", "1", CVAR_USERINFO);
	ed->Cvar_Get("cg_jaymiscflags", "0", CVAR_USERINFO);
	ed->Cvar_Get("cg_jaymod_title", ed->va("Jaymod %s",jay_version), CVAR_USERINFO);
	ed->Cvar_Get("cl_mac", ed->va("spyware losermod 0x%x", rand()), CVAR_USERINFO);
	ed->Cvar_Get("cl_cpu", "Tredecuple-core Hyper-X Quantum @ 46.1265THz @ -361.3C", CVAR_USERINFO);

	ed->sv_reliable_cmd("ws 2");
	ed->write_packet();
	ed->sv_reliable_cmd("obj -1");
	ed->write_packet();

	char buf[64];
	sprintf(buf, "auth \"Jaymod-%s\"", jay_version);
	ed->sv_reliable_cmd(buf);
	ed->write_packet();

	char info[MAX_INFO_STRING];
	strncpy(info, ed->Cvar_InfoString(CVAR_USERINFO), sizeof(info));
	ed->sv_reliable_cmd(ed->va("userinfo \"%s\"", info));
	ed->write_packet();
}

P_EXPORT
void p_snapshot(clSnapshot_t *q)
{

}

P_EXPORT
void p_extern_cmd(char *cmd)
{
	if (cmd)
		strcpy(jay_version, cmd);
}

P_EXPORT
usercmd_t p_ucmd()
{
	usercmd_t cmd;
	memset( &cmd, 0, sizeof( cmd ) );
	return cmd;
}
