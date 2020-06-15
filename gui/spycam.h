/*
	Server Entity Spycam ;)

	Warning! OpenGL ahead!

*/

#ifndef __SO_HAX_
#define __SO_HAX_

#include <QGLWidget>

class ServerProfile;

class SpyCam : public QGLWidget
{
	Q_OBJECT

public:
	SpyCam(QWidget *parent = 0);

public slots:
	void drawScene();
	void slotNewActiveServer(ServerProfile* );

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

private:
	float inclination;
	float scale;
	float maxRange;
	float centerOfGravity[3];
	float maxDisplacement[3];
	float currentViewOrigin[3];
	float avgDistance;
	float viewRange;

	float toViewOrigin[3];
	float toViewRange;

	int viewChangeTime;

	bool toggleShift;

	ServerProfile* server;

	void doViewShift(int);
	void solidSphere(GLdouble radius, GLint slices, GLint stacks);
	void tracer(float x, float y, float z);
};

#endif
