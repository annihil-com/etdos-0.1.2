#include <QtGui>
#include <QTextStream>

#include <etdos/protocol.h>
#include "mainwindow.h"
#include "netstuff.h"
#include "serverwidget.h"
#include "overviewidget.h"
#include "slave.h"
#include "server.h"
#include "q3colors.h"

#include <iostream>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
	setWindowTitle("EtDoS Master -.-");

	QGroupBox *centralBox = new QGroupBox("", this);
	QGroupBox *serverBox = new QGroupBox("Server", this);
	QVBoxLayout *vbox = new QVBoxLayout;

	serverSelector = new QComboBox(serverBox);
	serverSelector->addItem("<...>", QVariant(0));
	vbox->addWidget(serverSelector);
	serverBox->setLayout(vbox);
	connect(serverSelector, SIGNAL(activated(int)), this, SLOT(slotNewActiveServer(int)));

	overView = new OverViewWidget(centralBox);
    vbox = new QVBoxLayout;
	vbox->addWidget(serverBox);
    vbox->addWidget(overView);
	centralBox->setLayout(vbox);
	setCentralWidget(centralBox);

	createActions();
	createMenus();
	createToolBars();
	createStatusBar();
	createDockWindows();

	timer = new QTimer(this);
	connect(timer, SIGNAL(timeout()), this, SLOT(slotMainLoop()));
	timer->start(200);

	resize(800,750);

	currentActiveServer = 0;
}

void MainWindow::createActions()
{
	quitAct = new QAction("quit", this);
	connect (quitAct, SIGNAL(triggered()), this, SLOT(slotTerminate()));

	newserverAct = new QAction("New server session", this);
	connect (newserverAct, SIGNAL(triggered()), this, SLOT(slotNewServerDlg()));
	newEtAct = new QAction("Connect to ET session", this);
	connect (newEtAct, SIGNAL(triggered()), this, SLOT(slotEtSession()));

	aboutAct = new QAction("About", this);
	connect (aboutAct, SIGNAL(triggered()), this, SLOT(slotAbout()));
}

void MainWindow::createMenus()
{
	fileMenu = menuBar()->addMenu("File");
	fileMenu->addAction(quitAct);

	viewMenu = menuBar()->addMenu("View");

	serverMenu = menuBar()->addMenu("Server");
	serverMenu->addAction(newserverAct);
	serverMenu->addAction(newEtAct);

	optionsMenu = menuBar()->addMenu("Options");

	aboutMenu = menuBar()->addMenu("About");
	aboutMenu->addAction(aboutAct);
}

void MainWindow::createToolBars()
{

}

void MainWindow::createStatusBar()
{
	statusBar()->showMessage(tr("Ready"));
}

void MainWindow::slotEtSession()
{
	if (!currentActiveServer)
		return;

	currentActiveServer->ETSession();
}

void MainWindow::slotNewCommandInput()
{
	char buf[256];
	memset(buf, 0, sizeof(buf));

	int len = input->text().size();
	if (inputType->currentIndex() == 0 || inputType->currentIndex() == 2) {
		snprintf(buf+4, sizeof(buf)-4, "%s", input->text().toAscii().data());
	} else if (inputType->currentIndex() == 1) {
		snprintf(buf+4, sizeof(buf)-4, "say %s", input->text().toAscii().data());
		len += 4;
	}

	*(int*)buf = inputType->currentIndex() == 2 ? M_PLUGIN_CMD : M_SV_CMD;

	input->clear();

	QList<NetStuff*>::const_iterator ni;
	QList<SlaveClient*>::const_iterator sc;

	if (inputToAll->checkState() == Qt::Checked) {
		for (ni = ns.constBegin(); ni != ns.constEnd(); ++ni) {
			for (sc = (*ni)->SlaveBegin(); sc != (*ni)->SlaveEnd(); ++sc)
				(*sc)->Send(buf, 4+len);
		}
	} else {
		QList<int> *slaves = overView->SlavesSelected();
		int sz = slaves->count();
		for (int i=0; i<sz; i++) {
			for (ni = ns.constBegin(); ni != ns.constEnd(); ++ni) {
				for (sc = (*ni)->SlaveBegin(); sc != (*ni)->SlaveEnd(); ++sc)
					if ((*sc)->Id() == slaves->at(i))
						(*sc)->Send(buf, 4+len);
			}
		}
		delete slaves;
	}
}

void MainWindow::createDockWindows()
{
	/* input box */
    QDockWidget *dock = new QDockWidget("Command Input", this);
    dock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
	QGroupBox* inputBox = new QGroupBox("", dock);
	inputType = new QComboBox(inputBox);
    inputType->addItem("Server");
	inputType->addItem("Say");
	inputType->addItem("Plugin");
	input = new QLineEdit(inputBox);
	inputToAll = new QCheckBox(tr("to all"), inputBox);
	inputToAll->setCheckState(Qt::Unchecked);
	connect(input, SIGNAL(returnPressed()), this, SLOT(slotNewCommandInput()));
    QHBoxLayout *hbox = new QHBoxLayout;
	hbox->addWidget(inputToAll);
	hbox->addWidget(inputType);
    hbox->addWidget(input);
    inputBox->setLayout(hbox);
    dock->setWidget(inputBox);
    addDockWidget(Qt::BottomDockWidgetArea, dock);
	viewMenu->addAction(dock->toggleViewAction());

	/* status box */
    dock = new QDockWidget("Status", this);
    dock->setAllowedAreas(Qt::AllDockWidgetAreas);
	statusView = new QTextEdit(dock);
	statusView->setFontPointSize(7);
	statusView->setReadOnly(true);
	statusView->insertPlainText("System Ready\n");
	dock->setWidget(statusView);
    addDockWidget(Qt::RightDockWidgetArea, dock);
	viewMenu->addAction(dock->toggleViewAction());

	/* server widget */
    dock = new QDockWidget("Server Infos", this);
    dock->setAllowedAreas(Qt::AllDockWidgetAreas);
	serverWidget = new ServerWidget(this, dock);
	dock->setWidget(serverWidget);
	dock->setMinimumSize(400, 300);
    addDockWidget(Qt::TopDockWidgetArea, dock);
	viewMenu->addAction(dock->toggleViewAction());
	connect(this, SIGNAL(newActiveServer(NetStuff*)), serverWidget, SLOT(slotNewActiveServer(NetStuff*)));
}

QString MainWindow::Q3ColorStringtoHtml(const QString &q3c)
{
	static char m_buf[1024];

	QString htmlstring;
	QTextStream html(&htmlstring, QIODevice::Append);
	strncpy(m_buf, q3c.toAscii().constData(), sizeof(m_buf));

	const char*	s;
	int		c;
	bool inFontTag = false;

	s = m_buf;
	while ((c = *s) != 0 ) {
		if (Q_IsColorString(s)) {
			s++;
			if (inFontTag)
				html << "</font>";
			html << "<font color=\"" << q3colors[ColorIndex(*s)] << "\">";
			inFontTag = true;
		} else if (c >= 0x20 && c <= 0x7e)
			html << (char)c;
		s++;
	}
	if (inFontTag)
		html << "</font>";

	return htmlstring;
}

void MainWindow::slotAbout()
{
    QMessageBox::about(this, "About EtDoS",
		"Dedicated to all those who, like me, are committed to "
		"the free and open sharing of all Information, Ideas and Knowledge."
		"<br><br>Greetings to my haxfriends...<br><br>"
		"<b>Bl0f</b>, <b>ELITE_X</b>, <b>tesla</b>, <b>deso</b>, <b>woeh</b>, "
		"<b>ascii</b>, <b>LeAquR</b>, <b>cheat0ra</b>, <b>Noobie</b>, "
		"<b>buffalo</b>, <b>solcrusher</b>"
	);
}

ServerProfile* MainWindow::GetSelectedServer()
{
	if (currentActiveServer)
		return currentActiveServer->GetServerProfile();

	return NULL;
}

void MainWindow::slotTerminate()
{
	delete timer;

	cleanup();
	close();
}

void MainWindow::slotNewServer(QString serv)
{
	NetStuff *newSess = new NetStuff(this, serv);
	connect(newSess, SIGNAL(statusMessage(QString&)), this, SLOT(slotNewStatusMessage(QString&)));
	newSess->Status();
	ns.push_back(newSess);
	serverSelector->addItem(serv, QVariant(serv));
	serverSelector->setCurrentIndex(serverSelector->count()-1);
	slotNewActiveServer(serverSelector->count()-1);
	connect(newSess, SIGNAL(slaveChanged(SlaveClient*)), overView, SLOT(slotSlaveChange(SlaveClient*)));
	connect(newSess, SIGNAL(slaveSpawn(SlaveClient*)), overView, SLOT(slotSlaveSpawn(SlaveClient*)));
	connect(newSess, SIGNAL(slaveExit(SlaveClient*)), overView, SLOT(slotSlaveExit(SlaveClient*)));
	connect(this, SIGNAL(destroyed()), newSess, SLOT(slotKillSlaves()));
}

void MainWindow::slotNewActiveServer(int id)
{
	if (id == 0)
		return;

	QString seek = serverSelector->itemData(id).toString();

	QList<NetStuff*>::const_iterator ni;
	for (ni = ns.constBegin(); ni != ns.constEnd(); ++ni) {
		if ((*ni)->GetServerProfile()->ServerAddress() == seek) {
			if (currentActiveServer) {
				disconnect(currentActiveServer, SIGNAL(svClientChange(svClient*)), serverWidget->ClientWindow(), SLOT(slotClientChange(svClient*)));
				disconnect(currentActiveServer, SIGNAL(svRemoveClient(int)), serverWidget->ClientWindow(), SLOT(slotRemoveClient(int)));
			}
			currentActiveServer = *ni;
			connect(currentActiveServer, SIGNAL(svClientChange(svClient*)), serverWidget->ClientWindow(), SLOT(slotClientChange(svClient*)));
			connect(currentActiveServer, SIGNAL(svRemoveClient(int)), serverWidget->ClientWindow(), SLOT(slotRemoveClient(int)));
			emit(newActiveServer(currentActiveServer));
			return;
		}
	}
}

void MainWindow::slotNewServerDlg()
{
	NewServerDialog* newServer = new NewServerDialog;
	connect(newServer, SIGNAL(newServerSignal(QString)), this, SLOT(slotNewServer(QString)));
	newServer->exec();
}

void MainWindow::cleanup()
{
	// kill all slaves
	QList<NetStuff*>::iterator nq;
	nq = ns.begin();
	while (nq != ns.end()) {
		(*nq)->slotKillSlaves();
		delete (*nq);
		nq = ns.erase(nq);
	}
}

MainWindow::~MainWindow()
{
	cleanup();
}

void MainWindow::slotMainLoop()
{
	QList<NetStuff*>::const_iterator ni;
	for (ni = ns.constBegin(); ni != ns.constEnd(); ++ni)
		(*ni)->HouseKeeping();
}

void MainWindow::slotNewStatusMessage(QString &msg)
{
	if (!msg.contains(QChar::LineSeparator))
		msg += QChar::LineSeparator;
	statusView->insertPlainText(msg);
	QScrollBar *sb = statusView->verticalScrollBar();
	statusView->verticalScrollBar()->setValue(sb->maximum());
}

// iterate over all slaves & masters and grab a free port
int MainWindow::freePort(int exclude)
{
	QList<NetStuff*>::const_iterator ni;

	while (1) {
		int tryport = rand() & 0xffff;

		//reserved ports
		if (tryport < 1024 || tryport > 0xfff0 || tryport == exclude ||
				  (tryport > 27900 && tryport < 28000))
			continue;

		bool found = false;
		for (ni = ns.constBegin(); ni != ns.constEnd(); ++ni) {
			if ((*ni)->Port() == tryport) {
				found = true;
				break;
			}

			QList<SlaveClient*>::const_iterator sc;
			for (sc = (*ni)->SlaveBegin(); sc != (*ni)->SlaveEnd(); ++sc) {
				if ((*sc)->Port() == tryport) {
					found = true;
					break;
				}
			}
		}

		if (!found)
			return tryport;
	}
}

NewServerDialog::NewServerDialog(QWidget *parent) : QDialog(parent)
{
	QGroupBox *central = new QGroupBox("New Server", this);
	QVBoxLayout *vbox = new QVBoxLayout;
	QHBoxLayout *hbox = new QHBoxLayout;

	QGroupBox *buttons = new QGroupBox("", this);
	Ok = new QPushButton("Ok", buttons);
	Cancel = new QPushButton("Cancel", buttons);
	vbox->addWidget(Ok);
	vbox->addWidget(Cancel);
	buttons->setLayout(vbox);

	serverField = new QLineEdit(central);
	serverField->setFocus();
	vbox = new QVBoxLayout;
	vbox->addWidget(serverField);
	central->setLayout(vbox);
	central->setMinimumSize(250, 10);

	hbox->addWidget(central);
	hbox->addWidget(buttons);
	this->setLayout(hbox);

	connect(Cancel, SIGNAL(pressed()), this, SLOT(close()));
	connect(Ok, SIGNAL(pressed()), this, SLOT(slotNewServer()));
}

void NewServerDialog::slotNewServer()
{
	emit(newServerSignal(serverField->text()));
	close();
}
