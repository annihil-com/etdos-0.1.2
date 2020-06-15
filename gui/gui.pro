##############################
# custom qmake project file :)
##############################

QMAKE_CXXFLAGS_RELEASE = -pipe -O2
TEMPLATE = app
LIBS += ../zzip/zip.a
TARGET = ../etmaster
DEPENDPATH += .
INCLUDEPATH += ../
RESOURCES = gui.qrc
OBJECTS_DIR = build
MOC_DIR = moc
QT += network opengl

# Input
HEADERS += 	mainwindow.h \
		netstuff.h \
		slave.h \
		overviewidget.h \
		serverwidget.h \
		spycam.h \
		server.h \
		newconfigdialog.h \
		http.h

SOURCES +=	b64.c \
		main.cpp \
		mainwindow.cpp \
		netstuff.cpp \
		slave.cpp \
		overviewidget.cpp \
		serverwidget.cpp \
		spycam.cpp \
		server.cpp \
		newconfigdialog.cpp \
		http.cpp
