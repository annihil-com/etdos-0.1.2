/*
	follows you and gives health and revives you
*/

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <etdos/plugin.h>
#define P_EXPORT __attribute__ ((visibility ("default")))

#define	PITCH 0
#define	YAW 1
#define	ROLL 2
#define EF_DEAD 1
#define	BUTTON_WALKING 16
#define	ANGLE2SHORT(x)	((int)((x)*65536/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0/65536))
#define DotProduct(x,y)			((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define VectorSubtract(a,b,c)	((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2])
#define VectorAdd(a,b,c)		((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2])
#define VectorCopy(a,b)			((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define	VectorScale(v, s, o)	((o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s),(o)[2]=(v)[2]*(s))
#define	VectorMA(v, s, b, o)	((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))

int following = 0;
int jointeam = 0;
int running = 0;
int target = 0;
int warning_time = 0;
float maxrange = 500.0;

usercmd_t oldcmd;

void vectoangles(const vec3_t value1, vec3_t angles)
{
	float	forward;
	float	yaw, pitch;

	if ( value1[1] == 0 && value1[0] == 0 ) {
		yaw = 0;
		if ( value1[2] > 0 ) {
			pitch = 90;
		}
		else {
			pitch = 270;
		}
	} else {
		if ( value1[0] ) {
			yaw = ( atan2 ( value1[1], value1[0] ) * 180 / M_PI );
		}
		else if ( value1[1] > 0 ) {
			yaw = 90;
		}
		else {
			yaw = 270;
		}
		if ( yaw < 0 ) {
			yaw += 360;
		}

		forward = sqrt ( value1[0]*value1[0] + value1[1]*value1[1] );
		pitch = ( atan2(value1[2], forward) * 180 / M_PI );
		if ( pitch < 0 ) {
			pitch += 360;
		}
	}

	angles[PITCH] = -pitch;
	angles[YAW] = yaw;
	angles[ROLL] = 0;
}

void adjust_angles()
{
	vec3_t angles;
	vec3_t r;

	VectorSubtract(ed->clients[target].origin, ed->clients[*ed->clnum].origin, r);
	if (sqrtf(DotProduct(r, r)) > maxrange)
		return;

	vectoangles(r, angles);

	float dp, dy;
	dp = SHORT2ANGLE(ed->ps->delta_angles[PITCH]);
	dy = SHORT2ANGLE(ed->ps->delta_angles[YAW]);

	angles[PITCH] += dp;
	angles[YAW] += dy;

	oldcmd.angles[PITCH] = ANGLE2SHORT(angles[PITCH]);
	oldcmd.angles[YAW] = ANGLE2SHORT(angles[YAW]);
}

/* try join target team :) */
void join_team()
{
	if (!jointeam || !ed->clients[target].valid)
		return;

	int i;
	int naxis = 0;
	int nally = 0;
	following = 0;

	if (ed->clients[target].team != 1 && ed->clients[target].team != 2)
		return;

	for (i=0; i<64; i++) {
		if (!ed->clients[i].valid) continue;
		if (ed->clients[i].team == 1)
			naxis++;
		else if (ed->clients[i].team == 2)
			nally++;
	}

	printf("targ: %i, team %i {%i, %i}\n", target, ed->clients[target].team, naxis, nally);

	switch (ed->clients[target].team)
	{
		case 1:
			if (naxis <= nally) {
				ed->sv_reliable_cmd("team r 1 3 2");
				ed->write_packet();
				ed->sv_reliable_cmd(ed->va("m %i ^2MedicBot: ^3joined your team, Master, please go to spawn point", target));
				ed->write_packet();
				jointeam = 0;
			}
			break;
		case 2:
			if (nally <= naxis) {
				ed->sv_reliable_cmd("team b 1 3 2");
				ed->write_packet();
				ed->sv_reliable_cmd(ed->va("m %i ^2MedicBot: ^3joined your team, Master, please go to spawn point", target));
				ed->write_packet();
				jointeam = 0;
			}
			break;
	}
}

void warn_range()
{
	if (*ed->time - warning_time > 20000) {
		ed->sv_reliable_cmd(ed->va("m %i ^2MedicBot: ^3Master, you are out of range", target));
		ed->write_packet();
		warning_time = *ed->time;
	}
}

void run_medic_bot(usercmd_t *u)
{
	float dist;
	vec3_t angles;
	vec3_t r;

	join_team();

	if (!running || (running && jointeam))
		return;

	if (!ed->clients[target].insnap) {
		warn_range();
		if (following) {
			following = 0;
			ed->sv_reliable_cmd("kill");
			ed->write_packet();
		}
		memset(u, 0, sizeof(usercmd_t));
		return;
	}

	VectorSubtract(ed->clients[target].origin, ed->clients[*ed->clnum].origin, r);
	dist = sqrtf(DotProduct(r, r));
	if (dist > maxrange) {
		warn_range();
		memset(u, 0, sizeof(usercmd_t));
		u->weapon = 5;
		return;
	}

	vectoangles(r, angles);
	u->angles[PITCH] = ANGLE2SHORT(angles[PITCH]);
	u->angles[YAW] = ANGLE2SHORT(angles[YAW]);
	following = 1;

	if (u->forwardmove < 0)
		u->forwardmove = 0;

	if (ed->clients[target].flags & EF_DEAD) {
		u->buttons &= ~BUTTON_WALKING;
		u->forwardmove = 127;
		u->weapon = 11;		// 11 = syringe, 19 = medpax
		if (dist < 90.) {
			u->forwardmove = 64;
			u->upmove = -64;
			u->buttons |= 1;
		}
	}
}

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
	if (!cmd)
		return;

	if (!strncmp(cmd, "m_start", 7)) {
		running = 1;
		jointeam = 1;
	} else if (!strncmp(cmd, "m_stop", 6)) {
		ed->sv_reliable_cmd("team s");
		running = 0;
		jointeam = 0;
	} else if (!strncmp(cmd, "m_targ", 6)) {
		target = atoi(cmd+7);
		if (target < 0 || target > 63) {
			target = 0;
			running = 0;
		}
	} else if (!strncmp(cmd, "ucmd", 4)) {
		memcpy(&oldcmd, cmd+4, sizeof(oldcmd));
	} else
		return;
}

P_EXPORT
usercmd_t p_ucmd()
{
	usercmd_t cmd;
	if (running) {
		memcpy(&cmd, &oldcmd, sizeof(cmd));
		run_medic_bot(&cmd);
	} else
		memset(&cmd, 0, sizeof(cmd));
	return cmd;
}
