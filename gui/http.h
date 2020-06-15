/*
	HttP downloader
*/

#ifndef _HTTP_H_
#define _HTTP_H_

#include <QDialog>

class QStringList;
class QProgressBar;
class QPushButton;
class QLabel;
class QHttp;
class QTimer;
class QFile;

class ETDownload : public QDialog
{
	Q_OBJECT

public:
	ETDownload(QStringList &Urls, QWidget *parent = 0);

private slots:
	void slotUpdate();
	void slotAbort();
	void slotFinishedRequest(int, bool);
	void slotDataReadProgress(int, int);

private:
	QFile *out;
	QHttp *http;
	QStringList urls;
	QProgressBar *bar;
	QPushButton *cancel;
	QLabel *status;
	QLabel *currentURL;
	QTimer *timer;
	int current;
	int lastBts;
	int lastCnt;

	void startNextDownload(int idx);
	void finished();
	bool downloading;
	bool httpAbort;
	void error(QString hdr);
	void calcChecksum();
};

#endif
