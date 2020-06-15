#ifndef _MAINWINDOW_H_
#define _MAINWINDOW_H_

#include <QMainWindow>
#include <QDialog>

class NetStuff;
class QTabBar;
class QMenuBar;
class QTimer;
class QMenu;
class QAction;
class OverViewWidget;
class ServerWidget;
class QLineEdit;
class QGroupBox;
class QComboBox;
class QCheckBox;
class QTextEdit;
class QPushButton;
class ServerProfile;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
	MainWindow(QWidget *parent = 0);
	ServerProfile* GetSelectedServer();
	static QString Q3ColorStringtoHtml(const QString &q3c);
	int freePort(int exclude = 0);
	~MainWindow();

public slots:
	void slotNewStatusMessage(QString &msg);

private slots:
	void slotMainLoop();
	void slotTerminate();
	void slotNewServerDlg();
	void slotNewServer(QString);
	void slotAbout();
	void slotNewActiveServer(int);
	void slotNewCommandInput();
	void slotEtSession();

signals:
	void newActiveServer(NetStuff*);

private:
	void cleanup();
	void createActions();
	void createMenus();
    void createToolBars();
    void createStatusBar();
    void createDockWindows();

	QTimer *timer;
	QTimer *timerGui;
	QTabBar *tabbar;
	QMenuBar *menu;
	QList<NetStuff *> ns;

	QMenu *fileMenu;
	QMenu *viewMenu;
	QMenu *serverMenu;
	QMenu *optionsMenu;
	QMenu *aboutMenu;

	QAction *quitAct;
	QAction *newserverAct;
	QAction *newEtAct;
	QAction *aboutAct;

	QLineEdit* input;
	QComboBox* inputType;
	QCheckBox* inputToAll;
	QTextEdit* statusView;

	QComboBox* serverSelector;
	OverViewWidget *overView;
	ServerWidget *serverWidget;
	NetStuff* currentActiveServer;
};

class NewServerDialog : public QDialog
{
	Q_OBJECT

public:
	NewServerDialog(QWidget *parent = 0);

signals:
	void newServerSignal(QString);

private slots:
	void slotNewServer();

private:
	QLineEdit *serverField;
	QPushButton *Ok;
	QPushButton *Cancel;

};

#endif
