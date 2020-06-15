/*
	Miniature Aimbot plugin
	Shoots everything that moves ;)
*/

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <etdos/plugin.h>
#define P_EXPORT __attribute__ ((visibility ("default")))

#define	PITCH 0
#define	YAW 1
#define	ROLL 2
#define	ANGLE2SHORT(x)	((int)((x)*65536/360) & 65535)
#define	SHORT2ANGLE(x)	((x)*(360.0/65536))
#define DotProduct(x,y)			((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define VectorSubtract(a,b,c)	((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2])

int cangles[3];
float range = 800.0;
int mode;	// 0 = off, 1 = enemy, 2 = all

P_EXPORT
void p_init(hooqz_t *hooks)
{
	ed = hooks;
	cangles[0] = cangles[1] = cangles[2] = 0;
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

	if (!strncmp(cmd, "a_mode", 6)) {
		mode = atoi(cmd+7);
	} else if (!strncmp(cmd, "a_range", 7)) {
		range = (float)atof(cmd+8);
	}
}

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

#define EF_DEAD 1

// selects target closest & within range
void do_aimbot(usercmd_t *u)
{
	int i, c = -1;
	vec3_t r;
	vec3_t angles;
	float closest = range*range;
	float dist;

	// find targ
	for (i=0; i<64; i++) {
		if (!ed->clients[i].insnap || ed->clients[i].flags & EF_DEAD) continue;
		if (mode == 1 && ed->clients[i].team == ed->clients[*ed->clnum].team) continue;

		VectorSubtract(ed->clients[i].origin, ed->clients[*ed->clnum].origin, r);
		dist = DotProduct(r, r);

		if (dist < closest) {
			c = i;
			closest = dist;
		}
	}

	// do aim + fire
	if (c >= 0) {
		VectorSubtract(ed->clients[c].origin, ed->clients[*ed->clnum].origin, r);
		vectoangles(r, angles);

		float dp, dy;
		dp = SHORT2ANGLE(ed->ps->delta_angles[PITCH]);
		dy = SHORT2ANGLE(ed->ps->delta_angles[YAW]);

		angles[PITCH] += dp;
		angles[YAW] += dy;

		u->angles[PITCH] = ANGLE2SHORT(angles[PITCH]);
		u->angles[YAW] = ANGLE2SHORT(angles[YAW]);
		u->buttons |= 1;
		memcpy(cangles, u->angles, 12);
	}
}

P_EXPORT
usercmd_t p_ucmd()
{
	usercmd_t cmd;
	memset(&cmd, 0, sizeof(cmd));

	// no warpz
	memcpy(cmd.angles, cangles, 12);

	if (mode)
		do_aimbot(&cmd);

	// tapout
	if (ed->clients[*ed->clnum].flags & 1)
		cmd.upmove = 64;

	return cmd;
}
