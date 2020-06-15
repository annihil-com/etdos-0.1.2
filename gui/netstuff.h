/*
	NetStuff
	Handle network communications & slaves

	One NetStuff instance for each server session
	Each server session has dedicated port to communicate
	with associated slaves :)
*/

#ifndef _NETSTUFF_H_
#define _NETSTUFF_H_

#include <QMap>
#include <QObject>
#include <QAbstractSocket>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

class QUdpSocket;
class MainWindow;
class SlaveClient;
class ServerProfile;
class QHostAddress;
class svClient;
class UnixSocket;
class QTimer;

class NetStuff : public QObject
{
	Q_OBJECT

public:
	NetStuff(MainWindow *parent, QString &server);
	~NetStuff();

	void Status();
	int Port();
	int Milliseconds();
	void InitUdp(int port);
	void send(QByteArray& datagram, int port);
	void send(void *buf, int len, int port);
	void AddClient(QString &name, QString cfgFile);
	void RemoveClient(int pid);
	void DisconnectClient(int pid);
	void HouseKeeping();
	void ETSession();

	QString createCfgString(QString &cfgFile, QString *name=0);
	ServerProfile* GetServerProfile();

	QList<SlaveClient*>::const_iterator SlaveBegin();
	QList<SlaveClient*>::const_iterator SlaveEnd();

signals:
	void statusMessage(QString &msg);
	void slaveChanged(SlaveClient*);
	void slaveSpawn(SlaveClient*);
	void slaveExit(SlaveClient*);
	void svClientChange(svClient*);
	void svRemoveClient(int);
	void svCfgChange();

public slots:
	void slotKillSlaves();
	void slotETUcmd(char *data, int s);
	void slotUnixSockError();

private slots:
	void processPendingDatagrams();

private:
	QHostAddress* serverAddress;
	int serverPort;
	int m_port;

	pthread_t ml_thr;
	MainWindow* mainW;
	QUdpSocket *udpSocket;
	UnixSocket *etSocket;
	ServerProfile *serverProfile;

	QList<SlaveClient*> slaves;
	QMap<QString, int> teamList;

	unsigned int sys_timeBase;

	void processSlaveMessage(QByteArray &msg, SlaveClient *cl);
	void processNotify(char *s, SlaveClient *cl);
	void processClientData(char *data, int nclients);
	void addNewChat(QString &msg);
};

class UnixSocket : public QObject
{
	Q_OBJECT

public:
	UnixSocket(const char *sockPath);
	~UnixSocket();
	void close();
	bool status();

signals:
	void newData(char*, int);
	void fatalError();

private slots:
	void slotPeek();

private:
	struct sockaddr_un remote;
	int sock;
	bool connect_status;
	char recvbuf[8192];
	QTimer *peek;
};

#endif
