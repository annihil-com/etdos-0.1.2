/*
		Slave Config Dialog
		all recognized slave config options
*/

#ifndef __SLAVE_CFG_DLG_H_
#define __SLAVE_CFG_DLG_H_

#include <QDialog>

class QLineEdit;
class QString;
class QTextStream;

class NewConfigDialog : public QDialog
{
	Q_OBJECT

public:
	NewConfigDialog(QWidget *parent = 0);

private slots:
	void slotNewCfg();

signals:
	void newCfg(QString &);

private:
	QLineEdit* cfgFPS;
	QLineEdit* cfgSnaps;
	QLineEdit* cfgName;
	QLineEdit* cfgRate;
	QLineEdit* cfgMaxUCmds;
	QLineEdit* cfgPlugins;
	QLineEdit* cfgPassword;
	QLineEdit* cfgGuid;
	QLineEdit* cfgCl_anonymous;
	QLineEdit* cfgCg_etversion;
	QLineEdit* cfgCg_uinfo;
	QLineEdit* cfgProto;
	QLineEdit* cfgAbsMaxUCmds;

	bool saveConfig(QString &file);
	void writeEntry(QTextStream &stream, QLineEdit *val, QString field);
};

#endif
