/*
	 "follows" a real ET session by cloning ucmds
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
#define VectorAdd(a,b,c)		((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2])
#define VectorCopy(a,b)			((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define	VectorScale(v, s, o)	((o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s),(o)[2]=(v)[2]*(s))
#define	VectorMA(v, s, b, o)	((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))


int running = 0;

/*
	0 - dont touch angles/movement
	1 - copy from ucmd
	2 - keep vision locked to us
*/
int lock_angles = 1;
int follow_mode = 1;
int cangles[3];
int target = 0;

usercmd_t oldcmd;

P_EXPORT
void p_init(hooqz_t *hooks)
{
	cangles[0] = cangles[1] = cangles[2] = 0;
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
	vectoangles(r, angles);


	float dp, dy;
	dp = SHORT2ANGLE(ed->ps->delta_angles[PITCH]);
	dy = SHORT2ANGLE(ed->ps->delta_angles[YAW]);

	angles[PITCH] += dp;
	angles[YAW] += dy;

	oldcmd.angles[PITCH] = ANGLE2SHORT(angles[PITCH]);
	oldcmd.angles[YAW] = ANGLE2SHORT(angles[YAW]);

}

P_EXPORT
void p_extern_cmd(char *cmd)
{
	if (!cmd)
		return;

	if (!strncmp(cmd, "f_start", 7)) {
		running = 1;
	} else if (!strncmp(cmd, "f_stop", 6)) {
		running = 0;
	} else if (!strncmp(cmd, "f_targ", 6)) {
		target = atoi(cmd+7);
	} else if (!strncmp(cmd, "f_angles", 8)) {
		lock_angles = atoi(cmd+9);
		memcpy(cangles, oldcmd.angles, 12);
	} else if (!strncmp(cmd, "f_follow", 8)) {
		follow_mode = atoi(cmd+9);
	} else if (!strncmp(cmd, "ucmd", 4)) {
		memcpy(&oldcmd, cmd+4, sizeof(oldcmd));
		switch (lock_angles) {
			default:
				break;
			case 0:
				memcpy(oldcmd.angles, cangles, 12);
				break;
			case 2:
				if (target >= 0 && target < 64 && ed->clients[target].insnap)
					adjust_angles();
				break;
		}

		if (!follow_mode)
			oldcmd.forwardmove = oldcmd.rightmove = 0;

		// tapout
		if (ed->clients[*ed->clnum].flags & 1)
			oldcmd.upmove = 64;

	} else
		return;
}

P_EXPORT
usercmd_t p_ucmd()
{
	usercmd_t cmd;
	if (running)
		memcpy(&cmd, &oldcmd, sizeof(cmd));
	else
		memset(&cmd, 0, sizeof(cmd));
	return cmd;
}
