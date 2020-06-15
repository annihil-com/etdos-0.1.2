#include "http.h"

#include <QtGui>
#include <QHttp>
#include <zzip/lib.h>

#include <iostream>

extern "C" {
	extern char *encode_mem(void *inb, int mlen);
}

ETDownload::ETDownload(QStringList &Urls, QWidget *parent) : QDialog(parent)
{
	out = 0;
	http = 0;
	urls = Urls;

	QGroupBox *main = new QGroupBox("Downloading Server Files", this);
	QVBoxLayout *vbox = new QVBoxLayout;
	QHBoxLayout *hbox = new QHBoxLayout;

	currentURL = new QLabel(QString("(1 of %1) %2").arg(urls.size()).arg(urls.at(0)), main);
	status = new QLabel("...", main);
	status->setFixedWidth(100);
	bar = new QProgressBar(main);
	bar->setFixedWidth(350);
	cancel = new QPushButton("Cancel", main);
	connect(cancel, SIGNAL(pressed()), this, SLOT(slotAbort()));

	hbox->addWidget(bar);
	hbox->addWidget(status);

	vbox->addWidget(currentURL);
	vbox->addLayout(hbox);
	vbox->addWidget(cancel);
	main->setLayout(vbox);

	vbox = new QVBoxLayout;
	vbox->addWidget(main);
	this->setLayout(vbox);

	show();

	downloading = httpAbort = false;
	http = new QHttp;

	connect(http, SIGNAL(requestFinished(int, bool)), this, SLOT(slotFinishedRequest(int, bool)));
	connect(http, SIGNAL(dataReadProgress(int, int)), this, SLOT(slotDataReadProgress(int, int)));

	out = new QFile;

	timer = 0;
	current = 0;

	startNextDownload(current);

}

void ETDownload::calcChecksum()
{
	bool qvm = false;

	QString path = QString("downloads/%1").arg(out->fileName());
	const char *file = qPrintable(path);
	char *b = 0;
	ZZIP_DIR* dir = zzip_dir_open(file, 0);
	if (dir) {
		int ncrc = 0;
		unsigned int *crcs = NULL;

		ZZIP_DIRENT dirent;
		struct zzip_dir_hdr * hdr = dir->hdr0;
		while (zzip_dir_read(dir,&dirent)) {
			if (hdr->d_crc32) {
				ncrc++;
				crcs = (unsigned int*)realloc(crcs, 4*ncrc);
				crcs[ncrc-1] = hdr->d_crc32;
				if (strstr(dirent.d_name, "cgame.mp.i386.so")) qvm = true;
			}
			hdr = (struct zzip_dir_hdr *) ((char *)hdr + hdr->d_reclen)	;
		}
		zzip_dir_close(dir);

		if (ncrc) {
			b = encode_mem(crcs, ncrc*4);
			free(crcs);
		}
	}

	if (b) {
		QFile pk3(tr("data/pk3list") + ((qvm) ? tr(".qvm") : tr(".ref")));
		if (!pk3.open(QIODevice::Append | QIODevice::Text)) {
			QErrorMessage err;
			err.showMessage(tr("failed to open data/pk3list"));
			finished();
			return;
		}

		QTextStream pk3s(&pk3);
		pk3s << out->fileName() << " " << b << "\n";
		pk3.close();
		free(b);
	}
}

void ETDownload::startNextDownload(int idx)
{
	if (current >= urls.size()) {
		error(tr("donez"));
		return;
	}

	if (!urls.at(idx).contains(tr("http://"), Qt::CaseInsensitive)) {
		QErrorMessage err;
		err.showMessage(tr("invalid URL"));
		finished();
		return;
	}

	QStringList elements = urls.at(idx).split(QChar('/'), QString::KeepEmptyParts);
	if (elements.size() < 4) {
		QErrorMessage err;
		err.showMessage(tr("invalid URL"));
		finished();
		return;
	}

	QString curPath = QDir::currentPath();
	QString fl = curPath + tr("/downloads");
	QDir::setCurrent(fl);

	QString fm = elements.at(elements.size()-1);
	out->setFileName(fm);

	if (!out->open(QIODevice::WriteOnly)) {
		QErrorMessage err;
		err.showMessage(tr("error opening file"));
		QDir::setCurrent(curPath);
		finished();
		return;
	}

	QDir::setCurrent(curPath);

	QStringList hostlist = elements.at(2).split(QChar(':'));
	int port = 80;
	if (hostlist.size() > 1)
		port = hostlist.at(1).toInt();

	QString host = hostlist.at(0);
	http->setHost(host, (quint16)port);
	downloading = false;
	currentURL->setText(QString("(%1 of %2) %3").arg(idx+1).arg(urls.size()).arg(urls.at(idx)));
}

void ETDownload::finished()
{
	if (out && out->isOpen())
		out->close();
	if (http)
		http->close();

	done(0);
	close();
}

void ETDownload::slotAbort()
{
	httpAbort = true;
	http->abort();
	disconnect(timer, SIGNAL(timeout()), this, SLOT(slotUpdate()));
	delete timer;
	QMessageBox::critical(this, QString("Info"), QString("Downloads aborted"));
}

void ETDownload::slotFinishedRequest(int, bool err)
{
	if (httpAbort) {
		finished();
		return;
	}

	// start one
	if (!downloading) {
		if (err) {
			error(tr("err starting dl"));
			return;
		}
		if (timer)
			delete timer;

		timer = new QTimer;
		timer->setInterval(200);
		timer->setSingleShot(false);
		connect(timer, SIGNAL(timeout()), this, SLOT(slotUpdate()));
		lastCnt = 0;

		timer->start();
		http->get(urls.at(current), out);
		downloading = true;
		return;
	}

	if (downloading) {
		if (err) {
			error(tr("err finishing dl"));
			return;
		}
		out->close();
		calcChecksum();
		startNextDownload(++current);
	}
}

void ETDownload::slotUpdate()
{
	float rate = 4.88281e-3*((float)lastBts-(float)lastCnt);
	QString t;
	t.setNum(rate, 'f', 1);
	status->setText(QString("%1 kb/s").arg(t));
	lastCnt = lastBts;
}

void ETDownload::slotDataReadProgress(int bytes, int size)
{
	bar->setMinimum(0);
	bar->setMaximum(size);
	bar->setValue(bytes);
	lastBts = bytes;
}

void ETDownload::error(QString hdr)
{
	QErrorMessage err;
	err.showMessage(hdr + http->errorString());
	finished();
}
