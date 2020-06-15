/*
	SlaveClient
	interface to the slave clients

	one class instance for each spawned etdos process
*/


#ifndef __SLAVE_H_
#define __SLAVE_H_

#include <QMap>
#include <spawn.h>

class QString;
class NetStuff;

class SlaveClient
{

public:
	SlaveClient(QString &name, NetStuff *net);
	~SlaveClient();

	QString Name();
	QString Server();
	QString Status();

	void setPid(int spid);
	void setState(int stt);
	void setRole(int newRole);
	void setClientNum(int cl);
	int State();
	int Role();
	int Id();
	int Port();
	int ClientNum();
	bool Spawn(int port);
	void Kill();
	void KeepAlive();
	void DisconnectKill();
	void Connect();
	void NewCfgState(QString &cfg);
	void Send(void *buf, int len);
	void SetLastCmd(int time);
	void SetCfgString(QString cfg);
	int LastCmd();

private:
	NetStuff *ns;
	int lastCmdTime;
	int sl_port;
	pid_t pid;
	char **argv;
	int clientNum;
	int team;

	int sl_state;
	int sl_role;

	QString botname;
	QString server;
	QString cfgInit;

	QMap<QString, int> teamList;
};

#endif
