/*
	Server Widget
	Displays realtime information about a server
	obtained from slave feedback

*/

#ifndef __SERVERWIDGET_WID_H_
#define __SERVERWIDGET_WID_H_

#include <QWidget>
#include <QTreeView>
#include <QMap>

class QTextEdit;
class QTextCursor;
class SpyCam;
class QTabBar;
class QVBoxLayout;
class QCheckBox;
class QPushButton;
class QLineEdit;
class QLabel;
class QRadioButton;
class NetStuff;
class ServerClients;
class QAbstractItemModel;
class QStandardItemModel;
class svClient;
class QGroupBox;
class MainWindow;

class serverOverview : public QWidget
{
	Q_OBJECT

public:
	serverOverview(MainWindow* mw, QWidget *parent = 0);
	ServerClients* ClientWindow();

public slots:
	void slotServerChange(NetStuff *ns);

private:
	MainWindow *mainWindow;
	QGroupBox *svInfoBox;
	QTextEdit *statusTextBox;
	QTextEdit *nullTextBox;
	QTextCursor *statusCursor;
	ServerClients *serverClients;
	SpyCam *spycam3D;
};

class serverSettings : public QWidget
{
	Q_OBJECT

public:
	serverSettings(QWidget *parent = 0);
	void NewInstance();
	void NewActiveServer(NetStuff *ns);

private slots:
	void slotAddSlave();
	void slotShowCfgDlg();
	void slotShowPluginDlg();
	void slotRoleChange();
	void slotShowNewConfig();
	void slotNewCfg(QString &fileName);

private:
	int nameListCnt;
	NetStuff *currentActive;
	QCheckBox *autoFill;
	QPushButton *newConfig;
	QPushButton *addSlave;
	QPushButton *openSlaveCfg;
	QPushButton *openSlavePlugin;
	QLineEdit *slaveName;
	QLineEdit *slavePlugin;
	QLineEdit *slaveCfg;
	QLabel *slaveAddStatus;
	QRadioButton *roleDefault;
	QRadioButton *roleCustom;
};

class ServerWidget : public QWidget
{
	Q_OBJECT

public:
	ServerWidget(MainWindow* mw, QWidget *parent = 0);
	ServerClients* ClientWindow();

private slots:
	void slotTabChange(int);
	void slotNewActiveServer(NetStuff *ns);

private:
	serverOverview *Overview;
	serverSettings *Settings;
	QVBoxLayout *centralLayout;
	QTabBar *tabbar;
};

class ServerClients : public QTreeView
{
	Q_OBJECT

public:
	ServerClients(QWidget * parent = 0);

public slots:
	void slotClientChange(svClient* cl);
	void slotRemoveClient(int clientNum);
	void slotReset();

protected:
	void drawRow(QPainter *painter, const QStyleOptionViewItem & option, const QModelIndex & index ) const;

private:
	QMap<QString, int> teamList;
	QStandardItemModel *clientList;
};

#endif
