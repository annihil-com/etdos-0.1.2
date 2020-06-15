#include "netstuff.h"
#include <iostream>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <QtNetwork>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QDataStream>
#include <QStringList>
#include <etdos/protocol.h>

#include "mainwindow.h"
#include "slave.h"
#include "spycam.h"
#include "server.h"
#include "http.h"

NetStuff::NetStuff(MainWindow *parent, QString &server)
{
	mainW = parent;
	udpSocket = NULL;
	sys_timeBase = 0;
	etSocket = 0;

	int idx = server.indexOf(':');
	serverAddress = new QHostAddress( idx < 0 ? server : server.left(idx) );

	serverPort = idx < 0 ? 27960 : server.mid(idx).toInt();

	m_port = mainW->freePort(serverPort);
	InitUdp(m_port);
	serverProfile = new ServerProfile(server, this);

	teamList[":/resx/chat_spec.png"] = 3;
	teamList[":/resx/chat_allie.png"] = 2;
	teamList[":/resx/chat_axis.png"] = 1;
}

void NetStuff::Status()
{
	QString msg = QString("New Master for %1 listening on %2")
			.arg(serverProfile->ServerAddress())
			.arg(m_port);
	emit(statusMessage(msg));
}

int NetStuff::Port()
{
	return m_port;
}

NetStuff::~NetStuff()
{
	QList<SlaveClient*>::const_iterator sl;
	for (sl = slaves.constBegin(); sl != slaves.constEnd(); ++sl)
		(*sl)->DisconnectKill();
	qDeleteAll(slaves.constBegin(), slaves.constEnd());
}

QList<SlaveClient*>::const_iterator NetStuff::SlaveBegin()
{
	return slaves.constBegin();
}

QList<SlaveClient*>::const_iterator NetStuff::SlaveEnd()
{
	return slaves.constEnd();
}

QString NetStuff::createCfgString(QString &cfgFile, QString *name)
{
#warning handle cfg file errors
	QFile cfg(cfgFile);
	if (!cfg.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QString emptyStr = QString("");
		return emptyStr;
	}

	QTextStream stream(&cfg);

	QString uinfo;
	QString line = stream.readLine();
	while (!stream.atEnd()) {
		QStringList cfgItem = line.split(" = ");
		uinfo += '\\';
		uinfo += cfgItem.at(0);
		uinfo += '\\';

		// supplied overrides cfg
		if (name && cfgItem.at(0) == QString("name"))
			uinfo += *name;
		else
			uinfo += cfgItem.at(1);

		line = stream.readLine();
	}
	return uinfo;
}

void NetStuff::AddClient(QString &name, QString cfgFile)
{
	SlaveClient *newslave = new SlaveClient(name, this);
	newslave->Spawn(mainW->freePort(serverPort));
	newslave->SetCfgString(createCfgString(cfgFile, name.isEmpty() ? NULL : &name));
	slaves.push_back(newslave);

	QString msg = QString("Slave \"%1\" spawned on %2, port %3")
			.arg(newslave->Name())
			.arg(newslave->Server())
			.arg(newslave->Port());

	emit(slaveSpawn(newslave));
	emit(statusMessage(msg));
}

ServerProfile* NetStuff::GetServerProfile() { return serverProfile; }

void NetStuff::DisconnectClient(int pid)
{
	QList<SlaveClient*>::const_iterator sl;
	for (sl = slaves.constBegin(); sl != slaves.constEnd(); ++sl) {
		if ((*sl)->Id() == pid) {
			emit(slaveExit(*sl));
			SlaveClient *s = (*sl);
			s->DisconnectKill();
			int idx = slaves.indexOf(s);
			if (idx >= 0)
				slaves.removeAt(idx);
			delete s;
			return;
		}
	}
}

void NetStuff::RemoveClient(int pid)
{
	bool isScout = false;

	QList<SlaveClient*>::const_iterator sl;
	for (sl = slaves.constBegin(); sl != slaves.constEnd(); ++sl) {
		if ((*sl)->Id() == pid) {
			emit(slaveExit(*sl));
			int idx = slaves.indexOf(*sl);
			if ((*sl)->Role() == ROLE_SCOUT)
				isScout = true;
			if (idx >= 0)
				slaves.removeAt(idx);
			delete (*sl);
			break;
		}
	}

	if (isScout) {
		for (sl = slaves.constBegin(); sl != slaves.constEnd(); ++sl) {
			(*sl)->setRole(ROLE_SCOUT);
			return;
		}
	}
}

void NetStuff::InitUdp(int port)
{
	udpSocket = new QUdpSocket(this);
	if (!udpSocket->bind(QHostAddress::LocalHost, port)) {
		QString msg = QString("failed to bind to %1").arg(m_port);
		emit(statusMessage(msg));
		return;
	}
	connect(udpSocket, SIGNAL(readyRead()), this, SLOT(processPendingDatagrams()));
}

/*
chat "console: consolez"
chat "glock^7: ^2spec chat !@#4|\ " 1 0 3
*/
void NetStuff::addNewChat(QString &msg)
{
	if (msg.size() == 0)
		return;

	QString textChat;
	int k = msg.indexOf(tr("\""));
	int l = msg.lastIndexOf(tr("\""));
	if (l == -1 || l == msg.size() || k == l) {
		textChat += QString("<font size=\"2\">");
		textChat += MainWindow::Q3ColorStringtoHtml(msg);
		textChat += QString("</font><br>");
		serverProfile->InsertHtml(textChat);
		return;
	}

	QStringList params = msg.mid(l+1).split(tr(" "));
	QString p;

	int team = -1;
	if (params.size() >= 2) {
		int cl = params.at(1).toInt();
		if (cl >= 0 && cl < 64)
			team = serverProfile->Client(cl)->team;

	}
	if (team > 0 && team < 4)
		textChat += QString("<img src=\"%1\" width=\"16\" height=\"16\" />").arg(teamList.key(team));

	textChat += QString("<font size=\"2\">");
	textChat += MainWindow::Q3ColorStringtoHtml(msg.mid(k+1, l-k-1));
	textChat += QString("</font><br>");

	serverProfile->InsertHtml(textChat);
}

void NetStuff::processNotify(char *s, SlaveClient *cl)
{
	if (!strncmp(s, "[c][", 4)) {
		char *q = strchr(s+4, ']');
		if (!q)
			return;

		int cln = atoi(s+4);
		QString noti((char*)(q+1));
		serverProfile->ClientChange(noti, cln);
		emit(svClientChange(serverProfile->Client(cln)));
	} else if (!strncmp(s, "[ci]", 4)) {
		QString noti((char*)(s+4));
		cl->NewCfgState(noti);
		serverProfile->ClientChange(noti, cl->ClientNum());
		emit(slaveChanged(cl));
		emit(svClientChange(serverProfile->Client(cl->ClientNum())));
	} else if (!strncmp(s, "[status]", 8)) {
		QString noti((char*)(s+8));
		QString msg;
		msg += QString("%1").arg(cl->Id());
		msg += ": ";
		msg += noti;
		emit(statusMessage(msg));
	} else if (!strncmp(s, "[error]", 7)) {
		QString noti((char*)(s+7));
		QString msg;
		msg += QString("%1").arg(cl->Id());
		msg += ": ";
		msg += noti;
		emit(statusMessage(msg));
		RemoveClient(cl->Id());
	} else if (!strncmp(s, "[cn]", 4)) {
		cl->setClientNum(atoi(s+4));
	} else if (!strncmp(s, "[chat]", 6)) {
		QString noti((char*)(s+6));
		addNewChat(noti);
	} else if (!strncmp(s, "[svinfo]", 8)) {
		QString noti((char*)(s+8));
		serverProfile->ServerInfoChange(noti);
		emit(svCfgChange());
	}  else if (!strncmp(s, "[www]\xff", 6)) {
		QString msg = QString("%1 disc' wwwdl").arg(cl->Id());
		RemoveClient(cl->Id());
		emit(statusMessage(msg));

		QString noti((char*)(s+6));
		QStringList urlList = noti.split(QChar('\xff'),QString::SkipEmptyParts);
		ETDownload *dl = new ETDownload(urlList);
		dl->exec();
	} else if (!strncmp(s, "[disconnect]", 10)) {
		QString noti((char*)(s+10));
		QString msg;
		msg += QString("%1: disconnected: %2").arg(cl->Id()).arg(noti);
		RemoveClient(cl->Id());
		emit(statusMessage(msg));
	}
}

void NetStuff::processClientData(char *data, int nclients)
{
	serverProfile->NewSnapshot();

	client_t cl;
	for (int i=0; i<nclients; i++) {
		memcpy(&cl, data+i*sizeof(client_t), sizeof(client_t));
		if (cl.insnap)
			serverProfile->ClientSnapData(
										(int)cl.clnum,
										cl.flags,
										&cl.origin[0],
										&cl.velocity[0],
										&cl.angles[0]
										);

		// insnap = 0 but included means its scout announcing his own pos
		if (!cl.insnap)
			memcpy(&serverProfile->scoutOrigin[0], &cl.origin[0], 12);
	}
}

void NetStuff::processSlaveMessage(QByteArray &msg, SlaveClient *cl)
{
	if (msg.size() < 4)
		return;

	quint16 cmd, state;
	cl->SetLastCmd(Milliseconds());
	QDataStream stream(&msg, QIODevice::ReadOnly);
	stream.setByteOrder(QDataStream::LittleEndian);

	stream >> cmd >> state;

	// process handshaking
	if (cl->State() < ST_ASC) {
		const char *data = msg.constData();
		if (!strcmp(data, SLAVE_ACK)) {
			if (cl->State() == ST_LIMBO)
				cl->setState(ST_ACK);
			else {
				cl->setState(ST_ASC);

				// if this is first bot to server, make it a scout
				if (slaves.count() == 1)
					cl->setRole(ROLE_SCOUT);

				cl->Connect();
			}
			emit(slaveChanged(cl));
			return;
		}
	}

	if ((cl->State() + (cl->Role() >> 8)) != state) {
		cl->setState((int)state);
		emit(slaveChanged(cl));
	}

	switch (cmd)
	{
		default:
			return;
		case SL_CRASH:
		case SL_SV:
			RemoveClient(cl->Id());
			break;
		case SL_CLIENTS:
		{
			char *s = msg.data();
			s += 4;
			processClientData(s, (msg.size()-4)/sizeof(client_t));
			serverProfile->spyCam()->drawScene();
			break;
		}
		case SL_NOTIFY:
		{
			char *s = msg.data();
			s+=4;
			processNotify(s, cl);
			break;
		}
	}
}

// last resort kill slaves to destroy process - only called for
// irreversable proc exit
void NetStuff::slotKillSlaves()
{
	QList<SlaveClient*>::iterator sl;
	sl = slaves.begin();
	while (sl != slaves.end()) {
		(*sl)->DisconnectKill();
		delete (*sl);
		sl = slaves.erase(sl);
	}

	usleep(25);
}

void NetStuff::processPendingDatagrams()
{
	QHostAddress from;
	quint16 port;

	while (udpSocket->hasPendingDatagrams()) {
		QByteArray datagram;
		datagram.resize(udpSocket->pendingDatagramSize());
		udpSocket->readDatagram(datagram.data(), datagram.size(), &from, &port);

		if (from != QHostAddress::LocalHost)
			continue;

		QList<SlaveClient*>::const_iterator sl;
		for (sl = slaves.constBegin(); sl != slaves.constEnd(); ++sl) {
			if ((quint16)(*sl)->Port() == port) {
				processSlaveMessage(datagram, *sl);
			}
		}
	}
}

void NetStuff::send(QByteArray& datagram, int port)
{
    udpSocket->writeDatagram(datagram.data(), datagram.size(), QHostAddress::LocalHost, port);
}

void NetStuff::send(void *buf, int len, int port)
{
	QByteArray datagram((char*)buf, len);
    udpSocket->writeDatagram(datagram.data(), datagram.size(), QHostAddress::LocalHost, port);
}

int NetStuff::Milliseconds()
{
	struct timeval tp;

	gettimeofday(&tp, NULL);

	if (!sys_timeBase) {
		sys_timeBase = tp.tv_sec;
		return tp.tv_usec/1000;
	}

	int curtime = (tp.tv_sec - sys_timeBase)*1000 + tp.tv_usec/1000;

	return curtime;
}

void NetStuff::HouseKeeping()
{
	QList<SlaveClient*>::const_iterator sl;
	for (sl = slaves.constBegin(); sl != slaves.constEnd(); ++sl) {
		(*sl)->KeepAlive();
	}
}

void NetStuff::ETSession()
{
	if (etSocket) {
		etSocket->close();
		delete etSocket;
	}

	etSocket = new UnixSocket("/tmp/etslave_socket");
	if (!etSocket->status()) {
		QMessageBox dlg(QMessageBox::Warning, tr("Warning"),
					tr("Couldn't connect to ET session. Make sure you have an open"
						"ET running with the ethook included & started waiting for a connection"
						"with the command /hooq_listen"
					  ),
	  				QMessageBox::Ok);
		dlg.exec();
		delete etSocket;
		etSocket = 0;
		return;
	}
	connect(etSocket, SIGNAL(newData(char*, int)), this, SLOT(slotETUcmd(char*, int)));
}

void NetStuff::slotETUcmd(char *data, int size)
{
	#define UCMD_SIZE 28
	char buf[64];
	*(int*)buf = M_PLUGIN_CMD;
	strcpy(buf+4, "ucmd");
	memcpy(buf+8, data, UCMD_SIZE);

	QList<SlaveClient*>::const_iterator sc;
	for (sc = SlaveBegin(); sc != SlaveEnd(); ++sc)
		(*sc)->Send(buf, 8+UCMD_SIZE);
}

void NetStuff::slotUnixSockError()
{
	QMessageBox dlg(QMessageBox::Warning, tr("Warning"),
				tr("A unix socket error occured"),
				QMessageBox::Ok);
	dlg.exec();
	delete etSocket;
	etSocket = 0;
}

UnixSocket::UnixSocket(const char* sockPath) : QObject(0)
{
	if ((sock = ::socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		connect_status = false;
		return;
	}

	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, sockPath);
	int len = strlen(remote.sun_path) + sizeof(remote.sun_family);
	if (::connect(sock, (struct sockaddr *)&remote, len) == -1) {
		perror("connect");
		connect_status = false;
		return;
	}

	connect_status = true;
	peek = new QTimer;
	peek->setSingleShot(false);
	peek->start(2);
	connect(peek, SIGNAL(timeout()), this, SLOT(slotPeek()));
}

bool UnixSocket::status() { return connect_status; }
void UnixSocket::close() { if (sock) ::close(sock); }
UnixSocket::~UnixSocket() { close(); }

void UnixSocket::slotPeek()
{
	int t;
	if ((t=::recv(sock, recvbuf, 8192, MSG_DONTWAIT)) > 0) {
		emit(newData(recvbuf, t));
	} else {
		if (t == -1) return;
		else {
			perror("recv");
			peek->stop();
			connect_status = false;
			emit(fatalError());
		}
	}
}
