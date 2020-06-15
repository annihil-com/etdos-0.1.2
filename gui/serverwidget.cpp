#include <QtGui>

#include "serverwidget.h"
#include "spycam.h"
#include "netstuff.h"
#include "server.h"
#include "newconfigdialog.h"
#include "mainwindow.h"

ServerWidget::ServerWidget(MainWindow* mw, QWidget *parent) : QWidget(parent)
{
	QVBoxLayout *centralLayout = new QVBoxLayout;
	Overview = new serverOverview(mw, this);
	Settings = new serverSettings(this);

	Overview->setVisible(true);
	Settings->setVisible(false);

	resize(600, 250);

	tabbar = new QTabBar(this);
	tabbar->addTab("Overview");
	tabbar->addTab("Server Options");
	connect(tabbar, SIGNAL(currentChanged(int)), this, SLOT(slotTabChange(int)));

	centralLayout->addWidget(tabbar);
	centralLayout->addWidget(Overview);
	setLayout(centralLayout);
}

ServerClients* ServerWidget::ClientWindow()
{
	return Overview->ClientWindow();
}

void ServerWidget::slotTabChange(int idx)
{
	switch (idx)
	{
		default:
		case 0:
			Overview->setVisible(true);
			Settings->setVisible(false);
			this->layout()->removeWidget(Settings);
			this->layout()->addWidget(Overview);
			break;
		case 1:
			Settings->NewInstance();
			Overview->setVisible(false);
			Settings->setVisible(true);
			this->layout()->removeWidget(Overview);
			this->layout()->addWidget(Settings);
			break;
	}
}

void ServerWidget::slotNewActiveServer(NetStuff *ns)
{
	Settings->NewActiveServer(ns);
	Overview->slotServerChange(ns);
}

serverOverview::serverOverview(MainWindow* mw, QWidget *parent) : QWidget(parent)
{
	mainWindow = mw;
	QVBoxLayout *vbox = new QVBoxLayout;
	QSplitter *split = new QSplitter;

	QGroupBox *centralBox = new QGroupBox("", this);
	centralBox->setFlat(true);
	centralBox->setContentsMargins(0,0,0,0);

	svInfoBox = new QGroupBox("Server Info", centralBox);
	nullTextBox = new QTextEdit(svInfoBox);
	nullTextBox->setReadOnly(true);
	nullTextBox->setStyleSheet(QString("background: #afafaf"));
	nullTextBox->insertHtml("<font size=\"2\"><b>No server selected</b></font>");

	vbox->addWidget(nullTextBox);
	svInfoBox->setLayout(vbox);

	QGroupBox *spycamBox = new QGroupBox("Spycam", centralBox);
	QHBoxLayout *hbox = new QHBoxLayout;
	spycam3D = new SpyCam;

    QScrollArea* glWidgetArea3D = new QScrollArea(spycamBox);
    glWidgetArea3D->setWidget(spycam3D);
    glWidgetArea3D->setWidgetResizable(true);
    glWidgetArea3D->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    glWidgetArea3D->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    //glWidgetArea->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    glWidgetArea3D->setMinimumSize(200, 150);
	hbox->addWidget(glWidgetArea3D);
	spycamBox->setLayout(hbox);

	QGroupBox *clientsBox = new QGroupBox("Clients", centralBox);
	hbox = new QHBoxLayout;
	serverClients = new ServerClients(clientsBox);
	hbox->addWidget(serverClients);
	clientsBox->setLayout(hbox);

	split->addWidget(svInfoBox);
	split->addWidget(clientsBox);

	hbox = new QHBoxLayout;
	hbox->addWidget(split);
	hbox->addWidget(spycamBox);
	centralBox->setLayout(hbox);
}

void serverOverview::slotServerChange(NetStuff *ns)
{
	nullTextBox->setDocument(mainWindow->GetSelectedServer()->Box()->document());
	//nullTextBox->setVerticalScrollBar(mainWindow->GetSelectedServer()->Box()->verticalScrollBar());
	//nullTextBox->verticalScrollBar()->setVisible(true);
	serverClients->slotReset();

	for (int i=0; i<64; i++){
		svClient* cl = ns->GetServerProfile()->Client(i);
		if (cl->valid)
			serverClients->slotClientChange(cl);
	}

	spycam3D->slotNewActiveServer(ns->GetServerProfile());
	ns->GetServerProfile()->SetSpyCam(spycam3D);
}

ServerClients* serverOverview::ClientWindow()
{
	return serverClients;
}

serverSettings::serverSettings(QWidget *parent) : QWidget(parent)
{
	currentActive = 0;
	nameListCnt = 0;

	QVBoxLayout *vbox = new QVBoxLayout;
	QHBoxLayout *hbox = new QHBoxLayout;

	QGroupBox *entireWidget = new QGroupBox("", this);
	entireWidget->setFlat(true);
	entireWidget->setContentsMargins(0,0,0,0);


	QGroupBox *optionsBox = new QGroupBox("Server Options", entireWidget);
	autoFill = new QCheckBox("Auto Fill", optionsBox);
	vbox->addWidget(autoFill);
	optionsBox->setLayout(vbox);

	QGridLayout *grid = new QGridLayout;
	QGroupBox *slaveBox = new QGroupBox("Slaves", entireWidget);
	slaveName = new QLineEdit(slaveBox);
	slaveCfg = new QLineEdit(slaveBox);
	slavePlugin = new QLineEdit(slaveBox);
	addSlave = new QPushButton("Spawn", slaveBox);
	openSlaveCfg = new QPushButton("...", slaveBox);
	openSlavePlugin = new QPushButton("...", slaveBox);
	newConfig = new QPushButton("New", slaveBox);

	roleDefault = new QRadioButton("Default Role", slaveBox);
	roleCustom = new QRadioButton("Custom Role", slaveBox);

	slaveAddStatus = new QLabel("...", slaveBox);
	QLabel *label = new QLabel("Name", slaveBox);
	grid->addWidget(label, 0, 0);
	label = new QLabel("Config", slaveBox);
	grid->addWidget(label, 1, 0);
	label = new QLabel("Plugin", slaveBox);
	grid->addWidget(label, 2, 0);

	grid->addWidget(slaveName, 0, 1);
	grid->addWidget(slaveCfg,  1, 1);
	grid->addWidget(openSlaveCfg, 1, 2);
	grid->addWidget(slavePlugin, 2, 1);
	grid->addWidget(openSlavePlugin, 2, 2);
	grid->addWidget(roleDefault, 3, 1);
	grid->addWidget(newConfig, 3, 2);
	grid->addWidget(roleCustom, 4, 1);
	grid->addWidget(addSlave, 5, 1);
	grid->addWidget(slaveAddStatus, 5, 2);
	slaveBox->setLayout(grid);

	hbox->addWidget(optionsBox);
	hbox->addWidget(slaveBox);
	//hbox->insertSpacing(-1, 1000);
	entireWidget->setLayout(hbox);

	vbox = new QVBoxLayout;
	vbox->addWidget(entireWidget);
	//vbox->insertSpacing(-1, 1000);
	this->setLayout(vbox);

	connect(addSlave, SIGNAL(pressed()), this, SLOT(slotAddSlave()));
	connect(openSlaveCfg, SIGNAL(pressed()), this, SLOT(slotShowCfgDlg()));
	connect(openSlavePlugin, SIGNAL(pressed()), this, SLOT(slotShowPluginDlg()));
	connect(roleDefault, SIGNAL(pressed()), this, SLOT(slotRoleChange()));
	connect(roleCustom, SIGNAL(pressed()), this, SLOT(slotRoleChange()));
	connect(newConfig, SIGNAL(pressed()), this, SLOT(slotShowNewConfig()));
}

void serverSettings::NewInstance()
{

	slaveName->setText("<random>");
	slaveCfg->setText("data/defaults.cfg");
	slavePlugin->setText("plugins/nullplugin.so");

	// disable if no active server
	if (!currentActive) {
		slavePlugin->setEnabled(false);
		roleDefault->setChecked(false);
		openSlaveCfg->setEnabled(false);
		addSlave->setEnabled(false);
		openSlaveCfg->setEnabled(false);
		openSlavePlugin->setEnabled(false);
		roleCustom->setEnabled(false);
		roleDefault->setEnabled(false);
		newConfig->setEnabled(false);
		slaveAddStatus->setText("No server.");
	} else {
		slavePlugin->setEnabled(false);
		roleDefault->setChecked(true);
		openSlaveCfg->setEnabled(true);
		addSlave->setEnabled(true);
		openSlaveCfg->setEnabled(true);
		openSlavePlugin->setEnabled(false);
		roleDefault->setEnabled(true);
		roleCustom->setEnabled(true);
		newConfig->setEnabled(true);
		slaveAddStatus->setText(currentActive->GetServerProfile()->ServerAddress());
	}
}

void serverSettings::slotRoleChange()
{
	if (roleDefault->isChecked()) {
		slavePlugin->setEnabled(true);
		openSlaveCfg->setEnabled(true);
	} else {
		slavePlugin->setText("plugins/nullplugin.so");
		slavePlugin->setEnabled(false);
		openSlaveCfg->setEnabled(false);
	}
}

void serverSettings::NewActiveServer(NetStuff *ns)
{
	currentActive = ns;
}

void serverSettings::slotAddSlave()
{
	QString botName;

	if (slaveName->text() == QString("<random>")) {
		QFile namelist("data/namelist.txt");

		if (!namelist.open(QIODevice::ReadOnly | QIODevice::Text)) {
			QErrorMessage errr;
			errr.showMessage("Error: data/namelist.txt not found!");
			errr.exec();
			return;
		}

		QTextStream stream(&namelist);
		QString line = stream.readLine();
		int cnt = 0;

		if (nameListCnt == 0) {
			while (!line.isNull()) {
				line = stream.readLine();
				nameListCnt++;
			}
		}

		int rnd = (rand() << 16) & rand();
		rnd = rnd % nameListCnt;
		stream.seek(0);
		line = stream.readLine();
		while (!line.isNull()) {
			if (rnd == cnt) {
				botName = line;
				break;
			}
			line = stream.readLine();
			cnt++;
		}
	} else
		botName = slaveName->text();

	if (slaveCfg->text().isEmpty()) {
		slaveAddStatus->setText("error: empty cfg");
		return;
	}

	currentActive->AddClient(botName, slaveCfg->text());
	slaveAddStatus->setText("added!");
}

void serverSettings::slotNewCfg(QString &fileName)
{
	slaveCfg->setText(fileName);
}

void serverSettings::slotShowNewConfig()
{
	NewConfigDialog* newConfig = new NewConfigDialog;
	connect(newConfig, SIGNAL(newCfg(QString&)), this, SLOT(slotNewCfg(QString &)));
	newConfig->exec();
	delete newConfig;
}

void serverSettings::slotShowCfgDlg()
{
	QString fileName = QFileDialog::getOpenFileName(this,
			"Open Bot Config", "default.cfg", "Config Files (*.cfg)");
}

void serverSettings::slotShowPluginDlg()
{
	QString fileName = QFileDialog::getOpenFileName(this,
			"Select plugin", "nullplugin.so", "Plugin Files (*.so)");
}

// reimplement treeview to customize the way our items are drawn
// with Q3 color codes actually colored ;)
ServerClients::ServerClients(QWidget *parent) : QTreeView(parent)
{
	clientList = new QStandardItemModel(0, 3, this);
	clientList->setHeaderData(0, Qt::Horizontal, QObject::tr("cl"));
	clientList->setHeaderData(1, Qt::Horizontal, QObject::tr("Team"));
	clientList->setHeaderData(2, Qt::Horizontal, QObject::tr("Name"));

	setModel(clientList);
	setRootIsDecorated(false);
	setAlternatingRowColors(false);
	setUniformRowHeights(true);
	setSelectionMode(QAbstractItemView::NoSelection);
	setEditTriggers(QAbstractItemView::NoEditTriggers);
	setSortingEnabled(true);
	sortByColumn(0, Qt::AscendingOrder);

	header()->resizeSection(0, 25);
	header()->resizeSection(1, 45);

	teamList["spec"] = 3;
	teamList["axis"] = 1;
	teamList["ally"] = 2;
}

void ServerClients::slotClientChange(svClient *cl)
{
	int cnt = clientList->rowCount();

	for(int i=0; i<cnt; i++) {
		if (clientList->index(i, 0).data().toInt() == cl->clNum) {
			clientList->setData(clientList->index(i, 0), cl->clNum);
			clientList->setData(clientList->index(i, 2), cl->name);
			clientList->setData(clientList->index(i, 1), cl->team > 0 && cl->team <= 3 ? teamList.key(cl->team) : tr("???"));
			return;
		}
	}
	// not found, assume new client
	clientList->insertRow(cnt);
	clientList->setData(clientList->index(cnt, 0), cl->clNum);
	clientList->setData(clientList->index(cnt, 2), cl->name);
	clientList->setData(clientList->index(cnt, 1), cl->team > 0 && cl->team <= 3 ? teamList.key(cl->team) : tr("???"));
}

void ServerClients::slotRemoveClient(int clientNum)
{
	if (clientNum < 0 && clientNum > 63)
		return;

	int c = clientList->rowCount();
	for (int i = 0; i < c; i++) {
		QStandardItem *entry = clientList->item(i, 0);
		if (entry && entry->text().toInt() == clientNum)
			clientList->removeRow(i);
	}
}

void ServerClients::slotReset()
{
	int c = clientList->rowCount();
	for (int i = 0; i < c; i++)
		clientList->removeRow(i);
}

void ServerClients::drawRow(QPainter *painter, const QStyleOptionViewItem &, const QModelIndex &index) const
{
	QFont f(painter->font());
	f.setPointSize(7);
	painter->setFont(f);

	for (int c=0; c<3; c++) {
		QModelIndex s = index.sibling(index.row(), c);
		if (s.isValid()) {
			QRect rect = visualRect(s);
			QBrush bg(Qt::SolidPattern);

			switch (c)
			{
				default:
				{
					bg.setColor(QColor(223,240,255));
					painter->fillRect(rect, bg);
					painter->translate(4., 0.);
					painter->drawText(QRectF(rect), s.data().toString());
					painter->translate(-4., 0.);
					break;
				}
				case 1:
				{
					QString t = s.data().toString();
					if (teamList.value(t) == 3) {
						bg.setColor(QColor(225,255,223));
					} else if (teamList.value(t) == 2) {
						bg.setColor(QColor(255,223,223));
					} else if (teamList.value(t) == 1) {
						bg.setColor(QColor(223,226,255));
					} else
						bg.setColor(QColor(223,240,255));

					painter->fillRect(rect, bg);
					painter->translate(4., 0.);
					painter->drawText(QRectF(rect), t);
					painter->translate(-4., 0.);
					break;
				}
				case 2:
				{
					bg.setColor(QColor(160,160,160));
					painter->fillRect(rect, bg);
					QTextDocument richText;
					QString htmlCode = QString("%1%2%3")
							.arg("<font size=\"2\">")
							.arg(MainWindow::Q3ColorStringtoHtml(s.data().toString()))
							.arg("</font>");

					richText.setHtml(htmlCode);

					painter->translate(QPointF(rect.x(),rect.y()-2.));
					const QRectF vp(0, 0, rect.width(), rect.height());
					richText.drawContents(painter, vp);
					painter->translate(-QPointF(rect.x(),rect.y()-2.));
					break;
				}
			}
		}
	}
}
