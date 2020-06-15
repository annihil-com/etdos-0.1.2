#include <QApplication>
#include <time.h>

#include <QtGui>
#include "mainwindow.h"

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

	srand(time(0));

	// setup organization info so settings go in right place
	QApplication::setOrganizationName("niXcoders");
	QApplication::setOrganizationDomain("nixcoders.org");
	QApplication::setApplicationName("EtDoS Master");

	MainWindow mainWin;
	mainWin.show();

	return app.exec();
}
