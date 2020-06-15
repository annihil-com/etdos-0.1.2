#include <QString>
#include <QStringList>
#include <QMap>
#include <etdos/protocol.h>
#include <spawn.h>
#include "slave.h"
#include "netstuff.h"
#include "server.h"

#include <iostream>

SlaveClient::SlaveClient(QString &name, NetStuff *net)
{
	ns = net;
	pid = 0;
	clientNum = -1;
	botname = name;
	sl_role = 0;

	teamList["spec"] = 3;
	teamList["axis"] = 1;
	teamList["ally"] = 2;
}

SlaveClient::~SlaveClient()
{

}

QString SlaveClient::Name() { return botname; }
QString SlaveClient::Server() { return server; }
QString SlaveClient::Status()
{
	switch (sl_state) {
		default:
			return QString("??");
		case ST_LIMBO:
			return QString("limbo");
		case ST_ACK:
			return QString("handshaking");
		case ST_ASC:
			return QString("awaiting orders");
		case ST_CHALLENGING:
			return QString("awaiting challenge");
		case ST_CONNECTING:
			return QString("connecting");
		case ST_JOINED:
			return teamList.key(team);
	}
}

int SlaveClient::Id() { return (int)pid; }
int SlaveClient::Port() { return (int)sl_port; }
int SlaveClient::ClientNum() { return clientNum; }
void SlaveClient::SetLastCmd(int time) { lastCmdTime = time; }
int SlaveClient::LastCmd() { return lastCmdTime; }
void SlaveClient::setPid(int spid){ pid = (pid_t)spid; }
int SlaveClient::State() { return sl_state; }
int SlaveClient::Role() { return sl_role; }
void SlaveClient::SetCfgString(QString cfg)
{
	cfgInit = cfg;
}

void SlaveClient::setRole(int newRole)
{
	sl_role = newRole;

	char buf[6];
	*(quint32*)buf = M_ROLE;
	*(quint16*)(buf+4) = (quint16)newRole;
	Send((char*)&buf, 6);
}

void SlaveClient::setState(int stt)
{
	int st = stt & 0xff;
	int rl = stt >> 8;

	if (st >= 0 && st <= ST_JOINED)
		sl_state = st;

	if (rl >= 0 && rl <= ROLE_SCOUT)
		sl_role = rl;
}

void SlaveClient::setClientNum(int cl)
{
	if (cl >= 0 && cl < 64)
		clientNum = cl;
}

bool SlaveClient::Spawn(int port)
{
	if (pid)
		return true;

	argv = NULL;
	char arg0[] = "master";
	char arg1[7];

	argv = (char**)realloc(argv, sizeof(char*)*3);
	sprintf(arg1, "%i", port & 0xffff);
	argv[0] = arg0;
	argv[1] = arg1;
	argv[2] = NULL;

	if (posix_spawn(&this->pid, "./etdosd", NULL, NULL, (char**)this->argv, NULL)) {
		pid = 0;
		return false;
	}

	setPid(pid);
	sl_state = ST_LIMBO;
	sl_port = port;

	if (pid > 0)
		return true;
	return false;
}

void SlaveClient::Send(void *buf, int len)
{
	if (!ns)
		return;
	ns->send(buf, len, sl_port);
}

void SlaveClient::Kill()
{
	master_cmd_t cmd = M_DIE;
	Send((void*)&cmd, 4);
}

void SlaveClient::Connect()
{
	server = ns->GetServerProfile()->ServerIP();
	int sPort = ns->GetServerProfile()->ServerPort();

	char buf[2048];
	sprintf(&buf[4], "%s:%i:%s", qPrintable(server), sPort, qPrintable(cfgInit));
	int cmd = M_CONNECT;
	memcpy((void*)buf, &cmd, 4);
	Send((void*)&buf[0], 4+strlen((char*)&buf[4]));
}

//"n\^0-AnThraX-\t\2\c\0\r\0\m\0000000\s\0000000\dn\\dr\0\w\0\lw\0\sw\0\mu\0\ref\0"
void SlaveClient::NewCfgState(QString &cfg)
{
	QStringList infoString = cfg.split("\\");
	int n =infoString.size();

	for (int i=0; i<n; i++) {
		if (infoString.at(i) == QString("n")) {
			if (i+1 < n) botname = infoString.at(++i);
		} else if (infoString.at(i) == QString("t")) {
			if (i+1 < n) team = infoString.at(++i).toInt();
		}
	}
}

void SlaveClient::KeepAlive()
{
	switch (sl_state) {
		default:
			break;
		case ST_LIMBO:
			Send((void*)SLAVE_WAKEUP, strlen(SLAVE_WAKEUP));
			return;
		case ST_ACK:
			Send((void*)MASTER_ACK, strlen(MASTER_ACK));
			return;
	}

	if (sl_state >= ST_ASC){
		int cmd = M_NOP;
		Send((void*)&cmd, 4);
		return;
	}
}

void SlaveClient::DisconnectKill()
{
	master_cmd_t cmd = M_SV_DSC;
	Send((char*)&cmd, 4);
}
