/*
	Server Settings, data, control & profile

*/

#ifndef __SERVER_C_H_
#define __SERVER_C_H_

#include <QVector>

class QString;
class NetStuff;
class QTextEdit;
class SpyCam;

class svClient
{
public:
	svClient();
	QString name;
	QString cfgStr;
	int		clNum;
	int		team;
	bool	ref;
	bool	valid;
	bool	slave;

	bool	inSnapshot;
	int		flags;
	float	origin[3];
	float	velocity[3];
	float	angles[3];

	float	r[3];	// relative to center of gravity
};

class ServerProfile
{
public:
	ServerProfile(QString &server, NetStuff* ns);
	QTextEdit *Box();
	QString &ServerAddress();
	QString &ServerIP();
	int ServerPort();
	NetStuff* GetNet();

	SpyCam *spyCam();
	void SetSpyCam(SpyCam*);
	void NewSnapshot();
	void ClientSnapData(int clnum, int flags, float *org, float *vel, float *ang);
	void InsertHtml(QString &html);
	void ServerInfoChange(QString &cfgString);
	void ClientChange(QString &cfgString, int clientNum);
	void RemoveClient(int clientNum);
	int Milliseconds();
	svClient* Client(int clientNum);

	/* public options */
	bool autoFill;

	float scoutOrigin[3];

private:
	QTextEdit *infoBox;
	QString serverIp;
	QString serverAddress;
	int serverPort;
	NetStuff *netstuff;

	QString serverConfig;
	QVector<svClient> clients;

	SpyCam *spycam;
	void setClientFromCfg(int clientNum);
};

#endif
