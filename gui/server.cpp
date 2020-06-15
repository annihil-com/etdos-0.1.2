#include <QString>
#include <QStringList>
#include <QTextEdit>
#include <QScrollBar>
#include "spycam.h"
#include "server.h"
#include "netstuff.h"

#include <iostream>

ServerProfile::ServerProfile(QString &server, NetStuff* ns)
{
	serverAddress = server;
	autoFill = false;
	netstuff = ns;
	spycam = 0;
	clients.resize(64);

	infoBox = new QTextEdit;
	infoBox->setReadOnly(true);
	infoBox->setStyleSheet(QString("background: 939393"));
	infoBox->insertHtml(QString("<font size\"2\">starting session for <b><u>%1</u></b><br><br>").arg(server));

	int idx = server.indexOf(':');
	if (idx >= 0) {
		serverPort = server.mid(idx+1).toInt();
		serverIp = server.left(idx);
	} else {
		serverIp = serverAddress;
		serverPort = 27960;
	}
}

QTextEdit* ServerProfile::Box() { return infoBox; }

void ServerProfile::ServerInfoChange(QString &cfgString)
{
	serverConfig = cfgString;
}

void ServerProfile::InsertHtml(QString &html)
{
	infoBox->insertHtml(html);
	QScrollBar *sb = infoBox->verticalScrollBar();
	infoBox->verticalScrollBar()->setValue(sb->maximum());
}

void ServerProfile::setClientFromCfg(int clientNum)
{
	//n\Scumdog\t\3\c\0\r\0\m\0000000\s\0000000\dn\\dr\0\w\3\lw\3\sw\0\mu\0\ref\0
	QStringList pairs = clients[clientNum].cfgStr.split('\\');
	int p = pairs.size()/2;

	for (int c = 0; c<p; c+=2) {
		if (pairs.at(c) == QString("n")) {
			clients[clientNum].name = pairs.at(c+1);
			continue;
		} else if (pairs.at(c) == QString("ref")) {
			clients[clientNum].ref = pairs.at(c+1).toInt();
			continue;
		} else if (pairs.at(c) == QString("t")) {
			clients[clientNum].team = pairs.at(c+1).toInt();
			continue;
		}
	}
	clients[clientNum].clNum = clientNum;
}

svClient* ServerProfile::Client(int clientNum)
{
	if (clientNum < 0 || clientNum > 63)
		return NULL;
	return &clients[clientNum];
}

void ServerProfile::ClientChange(QString &cfgString, int clientNum)
{
	if (clientNum < 0 || clientNum > 63)
		return;

	clients[clientNum].inSnapshot = true;
	clients[clientNum].cfgStr = cfgString;
	setClientFromCfg(clientNum);
}

void ServerProfile::RemoveClient(int clientNum)
{
	if (clientNum < 0 || clientNum > 63)
		return;

	clients[clientNum].inSnapshot = false;
}

NetStuff* ServerProfile::GetNet() { return netstuff; }

QString &ServerProfile::ServerAddress()
{
	return serverAddress;
}

QString &ServerProfile::ServerIP()
{
	return serverIp;
}

int ServerProfile::ServerPort()
{
	return serverPort;
}

void ServerProfile::NewSnapshot()
{
	for (int i=0; i<64; i++)
		clients[i].inSnapshot = false;
}

int ServerProfile::Milliseconds() { return netstuff->Milliseconds(); }

void ServerProfile::ClientSnapData(int clnum, int flags, float *org, float *vel, float *ang)
{
	if (clnum < 0 || clnum > 63)
		return;

	clients[clnum].inSnapshot = true;
	clients[clnum].flags = flags;

	memcpy(&clients[clnum].origin[0], org, 12);
	memcpy(&clients[clnum].velocity[0], vel, 12);
	memcpy(&clients[clnum].angles[0], ang, 12);
}

SpyCam *ServerProfile::spyCam() {return spycam; }
void ServerProfile::SetSpyCam(SpyCam *nsc) { spycam = nsc; }

svClient::svClient()
{
	valid = false;
	inSnapshot = false;
}
