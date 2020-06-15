#include <QLineEdit>
#include <QLabel>
#include <QCheckBox>
#include <QPushButton>
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QToolTip>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include "newconfigdialog.h"

#define DEFAULT(x, y) x->setText(y)

NewConfigDialog::NewConfigDialog(QWidget *parent) : QDialog(parent)
{
	QGroupBox *central = new QGroupBox("Bot Settings", this);
	central->setMinimumSize(320, 100);
	QGridLayout *grid = new QGridLayout;

	cfgSnaps = new QLineEdit(central); 			DEFAULT(cfgSnaps, "5");
	cfgFPS = new QLineEdit(central); 			DEFAULT(cfgFPS, "150");
	cfgName  = new QLineEdit(central); 			DEFAULT(cfgName, "<random>");
	cfgRate = new QLineEdit(central); 			DEFAULT(cfgRate, "3000");
	cfgMaxUCmds = new QLineEdit(central); 		DEFAULT(cfgMaxUCmds, "4");
	cfgAbsMaxUCmds = new QLineEdit(central);	DEFAULT(cfgAbsMaxUCmds, "50");
	cfgPlugins = new QLineEdit(central); 		DEFAULT(cfgPlugins, "plugins/nullplugin.so;");
	cfgPassword = new QLineEdit(central); 		DEFAULT(cfgPassword, "");
	cfgGuid = new QLineEdit(central); 			DEFAULT(cfgGuid, "<random>");
	cfgCl_anonymous = new QLineEdit(central); 	DEFAULT(cfgCl_anonymous, "1");
	cfgCg_etversion = new QLineEdit(central); 	DEFAULT(cfgCg_etversion, "Enemy Territory, ET 2.60");
	cfgCg_uinfo = new QLineEdit(central); 		DEFAULT(cfgCg_uinfo, "13 0 30");
	cfgProto = new QLineEdit(central); 			DEFAULT(cfgProto, "0");

	cfgMaxUCmds->setToolTip(tr("idle rate at which ucmds are fired off to server"));
	cfgAbsMaxUCmds->setToolTip(tr("absolute max rate at which ucmds are send\n"
		"the ping hack sends an ucmd for every rec'vd server message\n"
		"this is an absolute limit usefull for limiting total upload rate\n"
		"tx rate ~ (ucmds*40+256) bytes/s"));
	cfgProto->setToolTip(tr("0 = auto select, 82 = ET 2.55/2.56, 84 = ET 2.60(b)"));
	cfgName->setToolTip(tr("Custom name in bot screen takes priority over"
		"the name in the cfg file"));
	cfgFPS->setToolTip(tr("frames per second of slave client\n"
		"too low, and weird things happen, too high, and much CPU usage"));
	cfgSnaps->setToolTip(tr("snapshots/sec server sends us\n"
		"lower means less bandwith and higher ping"));
	cfgPlugins->setToolTip(tr("plugins to load\n"
		"Warning: every plugin file must be terminated with a ';'"));

	QLabel *label = new QLabel("FPS", central);
	grid->addWidget(label, 0, 0);
	grid->addWidget(cfgFPS, 0, 1);

	label = new QLabel("Name", central);
	grid->addWidget(label, 1, 0);
	grid->addWidget(cfgName, 1, 1);

	label = new QLabel("Guid", central);
	grid->addWidget(label, 1, 2);
	grid->addWidget(cfgGuid, 1, 3);

	label = new QLabel("Rate", central);
	grid->addWidget(label, 2, 0);
	grid->addWidget(cfgRate, 2, 1);

	label = new QLabel("ucmds/s", central);
	grid->addWidget(label, 3, 0);
	grid->addWidget(cfgMaxUCmds, 3, 1);

	label = new QLabel("abs max ucmds/s", central);
	grid->addWidget(label, 3, 2);
	grid->addWidget(cfgAbsMaxUCmds, 3, 3);

	label = new QLabel("Plugins", central);
	grid->addWidget(label, 4, 0);
	grid->addWidget(cfgPlugins, 4, 1);

	label = new QLabel("Password", central);
	grid->addWidget(label, 5, 0);
	grid->addWidget(cfgPassword, 5, 1);

	label = new QLabel("cl_anonymous", central);
	grid->addWidget(label, 6, 0);
	grid->addWidget(cfgCl_anonymous, 6, 1);

	label = new QLabel("cg_etversion", central);
	grid->addWidget(label, 7, 0);
	grid->addWidget(cfgCg_etversion, 7, 1);

	label = new QLabel("cg_uinfo", central);
	grid->addWidget(label, 8, 0);
	grid->addWidget(cfgCg_uinfo, 8, 1);

	label = new QLabel("Protocol", central);
	grid->addWidget(label, 9, 0);
	grid->addWidget(cfgProto, 9, 1);

	label = new QLabel("Snaps", central);
	grid->addWidget(label, 10, 0);
	grid->addWidget(cfgSnaps, 10, 1);

	QPushButton *Ok = new QPushButton("Save", central);
	QPushButton *Cancel = new QPushButton("Cancel", central);
	grid->addWidget(Ok, 11, 0);
	grid->addWidget(Cancel, 11, 1);

	central->setLayout(grid);
	QVBoxLayout *vbox = new QVBoxLayout;
	vbox->addWidget(central);
	this->setLayout(vbox);

	connect(Cancel, SIGNAL(pressed()), this, SLOT(close()));
	connect(Ok, SIGNAL(pressed()), this, SLOT(slotNewCfg()));
}

void NewConfigDialog::slotNewCfg()
{
	QString fileName = QFileDialog::getSaveFileName(this,
			"Save config as", "default.cfg", "Config Files (*.cfg)");

	if (saveConfig(fileName))
		emit(newCfg(fileName));
	close();
}

void NewConfigDialog::writeEntry(QTextStream &stream, QLineEdit *val, QString field)
{
	stream << field << " = " << val->text() << '\n';
}

bool NewConfigDialog::saveConfig(QString &fileName)
{
	QFile file(fileName);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
		return false;

	QTextStream out(&file);

	/* NOTE should match the values in daemon.c */
	writeEntry(out, cfgName, QString("name"));
	writeEntry(out, cfgFPS, QString("fps"));
	writeEntry(out, cfgRate, QString("rate"));
	writeEntry(out, cfgMaxUCmds, QString("maxucmds"));
	writeEntry(out, cfgAbsMaxUCmds, QString("absucmds"));
	writeEntry(out, cfgPlugins, QString("plugins"));
	writeEntry(out, cfgPassword, QString("g_password"));
	writeEntry(out, cfgGuid, QString("cl_guid"));
	writeEntry(out, cfgCl_anonymous, QString("cl_anonymous"));
	writeEntry(out, cfgCg_etversion, QString("cg_etversion"));
	writeEntry(out, cfgProto, QString("protocol"));
	writeEntry(out, cfgCg_uinfo, QString("cg_uinfo"));
	writeEntry(out, cfgSnaps, QString("snaps"));
	out << '\n';

	return true;
}

