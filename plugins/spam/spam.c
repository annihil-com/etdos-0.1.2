/*
	Spam Plugin - automated spammer
*/

#include <string.h>
#include <stdio.h>
#include <etdos/plugin.h>
#define P_EXPORT __attribute__ ((visibility ("default")))

int do_spam;
int last_teamchange;
int last_chat;
int last_vchat;

char *vchats[] =
{
	"PathCleared",
	"EnemyWeak",
	"AllClear",
	"Incoming",
	"FireInTheHole",
	"OnDefense",
	"OnOffense",
	"TakingFire",
	"MinesCleared",
	"EnemyDisguised",
	"Medic",
	"NeedAmmo",
	"NeedBackup",
	"NeedEngineer",
	"CoverMe",
	"HoldFire",
	"WhereTo",
	"NeedOps",
	"FollowMe",
	"LetsGo",
	"Move",
	"ClearPath",
	"DefendObjective",
	"DisarmDynamite",
	"ClearMines",
	"ReinforceDefense",
	"ReinforceOffense",
	"Affirmative",
	"Negative",
	"Thanks",
	"Welcome",
	"Sorry",
	"Oops",
	"Hi",
	"Bye",
	"GreatShot",
	"Cheer",
	"GoodGame",
	"IamSoldier",
	"IamMedic",
	"IamEngineer",
	"IamFieldOps",
	"IamCovertOps",
	"DestroyPrimary",
	"DestroySecondary",
	"DestroyConstruction",
	"RepairVehicle",
	"DestroyVehicle",
	"EscortVehicle",
	"CommandAcknowledged",
	"CommandDeclined",
	"CommandCompleted",
	"ConstructionCommencing",
	"FTAttack",
	"FTFallBack",
	"FTCoverMe",
	"FTCoveringFire",
	"FTMortar",
	"FTHealSquad",
	"FTHealMe",
	"FTReviveTeamMate",
	"FTReviveMe",
	"FTDestroyObjective",
	"FTRepairObjective",
	"FTConstructObjective",
	"FTDisarmDynamite",
	"FTDeployLandmines",
	"FTDisarmLandmines",
	"FTCallAirStrike",
	"FTCallArtillery",
	"FTResupplySquad",
	"FTResupplyMe",
	"FTExploreArea",
	"FTCheckLandmines",
	"FTSatchelObjective",
	"FTInfiltrate",
	"FTGoUndercover",
	"FTProvideSniperCove",
};

void generate_spam()
{
	int i;

	if (*ed->time - last_teamchange > 3000) {
		int naxis = 0;
		int nally = 0;

		for (i=0; i<64; i++) {
			if (!ed->clients[i].valid) continue;
			if (ed->clients[i].team == 1)
				naxis++;
			else if (ed->clients[i].team == 2)
				nally++;
		}

		if (ed->clients[*ed->clnum].team == 3) {
			if (naxis > nally) {
				ed->sv_reliable_cmd("team b");
			} else {
				ed->sv_reliable_cmd("team r");
			}
		} else {
			ed->sv_reliable_cmd("team s");
		}

		last_teamchange = *ed->time;
		ed->write_packet();
		return;
	}

	if (*ed->time - last_chat > 1500) {
		// dont split into multiple lines (58 chars) for aesthetical reasons
		char buf[512];
		memset(buf, 0, 512);
		sprintf(buf, "say ");
		char *s = &buf[4];

		for (i=0; i<29; i++) {
			*s = '^';	s++;
			*s = (char)('0' + rand() % 45);	s++;
			*s = (rand() & 1 ? 'k' : 'K');	s++;
			*s = '^';	s++;
			*s = (char)('0' + rand() % 45);	s++;
			*s = (rand() & 1 ? 'e' : 'E');	s++;
		}
		printf("%s\n", buf);
		ed->sv_reliable_cmd(buf);
		ed->write_packet();
		last_chat = *ed->time;
		return;
	}

	if (*ed->time - last_vchat > 2500) {
		char buf[128];
		int n = 0;
		while (vchats[i++]) n++;

		sprintf(buf, "vsay %s", vchats[rand() % n]);
		printf("%s\n", buf);
		ed->sv_reliable_cmd(buf);
		last_vchat = *ed->time;
		ed->write_packet();
		return;
	}
}

P_EXPORT
void p_init(hooqz_t *hooks)
{
	last_teamchange = rand() % 2000;
	last_chat = rand() % 2000;
	last_vchat = rand() % 2000;
	do_spam = 1;
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
	if (do_spam) generate_spam();
}

P_EXPORT
void p_extern_cmd(char *cmd)
{
	if (!cmd)
		return;

	if (!strncmp(cmd, "s_start", 7)) {
		do_spam = 1;
	} else if (!strncmp(cmd, "s_stop", 6)) {
		do_spam = 0;
	}

	last_teamchange = rand() % 2000;
	last_chat = rand() % 500;
	last_vchat = rand() % 2000;
}

P_EXPORT
usercmd_t p_ucmd()
{
	usercmd_t cmd;
	memset(&cmd, 0, sizeof(cmd));

	return cmd;
}
