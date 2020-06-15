#include <QtOpenGL>
#include <QTimer>
#include <math.h>
#include <GL/glu.h>
#include <iostream>

#include "server.h"
#include "spycam.h"

#define DotProduct(x,y)			((x)[0]*(y)[0]+(x)[1]*(y)[1]+(x)[2]*(y)[2])
#define VectorSubtract(a,b,c)	((c)[0]=(a)[0]-(b)[0],(c)[1]=(a)[1]-(b)[1],(c)[2]=(a)[2]-(b)[2])
#define VectorAdd(a,b,c)		((c)[0]=(a)[0]+(b)[0],(c)[1]=(a)[1]+(b)[1],(c)[2]=(a)[2]+(b)[2])
#define VectorCopy(a,b)			((b)[0]=(a)[0],(b)[1]=(a)[1],(b)[2]=(a)[2])
#define	VectorScale(v, s, o)	((o)[0]=(v)[0]*(s),(o)[1]=(v)[1]*(s),(o)[2]=(v)[2]*(s))
#define	VectorMA(v, s, b, o)	((o)[0]=(v)[0]+(b)[0]*(s),(o)[1]=(v)[1]+(b)[1]*(s),(o)[2]=(v)[2]+(b)[2]*(s))
#define VectorDistance(a, b)    sqrtf(((a)[0]-(b)[0])*((a)[0]-(b)[0]) + ((a)[1]-(b)[1])*((a)[1]-(b)[1]) + ((a)[2]-(b)[2])*((a)[2]-(b)[2]))

static void normalize(float *in)
{
	float s = sqrtf(in[0]*in[0] + in[1]*in[1] + in[2]*in[2]);
	if (s == 0.) s = 1.;

	s = 1./s;
	in[0] *= s;
	in[1] *= s;
	in[2] *= s;
}

SpyCam::SpyCam(QWidget *parent) : QGLWidget(parent)
{
	scale = 20.;
	currentViewOrigin[0] = currentViewOrigin[1] = currentViewOrigin[2] = 0;
	server = 0;
	viewRange = 0;
	toggleShift = false;
}

void SpyCam::initializeGL()
{
	GLfloat light_ambient[] = { 0.0, 0.0, 0.0, 1.0 };
	GLfloat light_diffuse[] = { 1.0, 0.9, 1.0, 1.0 };
	GLfloat light_specular[] = { 0.8, 1.0, 1.0, 1.0 };
	GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };

	glLightfv (GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv (GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv (GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv (GL_LIGHT0, GL_POSITION, light_position);

	glEnable (GL_LIGHTING);
	glEnable (GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
	glEnable (GL_LINE_STIPPLE);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
}

void SpyCam::tracer(float x, float y, float z)
{
	glDisable(GL_LIGHTING);

	glColor4f(0.3, 1., 0., 0.2);
	glBegin(GL_LINES);
		glVertex3f(0., 0., 0.);
		glVertex3f(0., 0., -z);
		glVertex3f(0., 0., -z);
		glVertex3f(-x, -y, -z);
	glEnd();

	glEnable(GL_LIGHTING);
}

void SpyCam::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	gluLookAt(0., -15., 0., 0., 0., 0., 0, 0., 1);
	glTranslatef(0, 0, -3);

	glDepthMask (GL_FALSE);
   	glDisable(GL_LIGHTING);
	glColor4f(0.2, 0.2, 0., 0.3);

	glBegin(GL_QUADS);
		glVertex3f(-15, -15, 0);
		glVertex3f(-15, 15, 0);
		glVertex3f(15, 15, 0);
		glVertex3f(15, -15, 0);
	glEnd();

	glEnable(GL_LIGHTING);

	if (!server)
		return;

	float f = maxRange < 1. ? 1. : 0.5*scale/maxRange;

	for (int i=0; i<64; i++) {
		svClient* cl = server->Client(i);
		if (cl->inSnapshot) {
			glPushMatrix ();
			glTranslatef(f*cl->r[0], f*cl->r[1], f*cl->r[2]);

			solidSphere(0.42, 15, 15);
			tracer(f*cl->r[0], f*cl->r[1], f*cl->r[2]);
			glPopMatrix();
		}
	}

	glFlush();
}

void SpyCam::resizeGL(int width, int height)
{
	int side = qMin(width, height);
	glViewport((width - side) / 2, (height - side) / 2, side, side);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity ();
	gluPerspective(120., 1., 0.1, 500.);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
}

void SpyCam::slotNewActiveServer(ServerProfile* sv) { server = sv; }

static void fghCircleTable(double **sint,double **cost,const int n)
{
    int i;
    const int size = abs(n);
    const double angle = 2*M_PI/(double)( ( n == 0 ) ? 1 : n );

    *sint = (double *) calloc(sizeof(double), size+1);
    *cost = (double *) calloc(sizeof(double), size+1);

    if (!(*sint) || !(*cost))
    {
        free(*sint);
        free(*cost);
		return;
    }

    (*sint)[0] = 0.0;
    (*cost)[0] = 1.0;

    for (i=1; i<size; i++)
    {
        (*sint)[i] = sin(angle*i);
        (*cost)[i] = cos(angle*i);
    }

    (*sint)[size] = (*sint)[0];
    (*cost)[size] = (*cost)[0];
}

void SpyCam::solidSphere(GLdouble radius, GLint slices, GLint stacks)
{
    int i,j;
    double z0,z1;
    double r0,r1;
    double *sint1,*cost1;
    double *sint2,*cost2;

    fghCircleTable(&sint1,&cost1,-slices);
    fghCircleTable(&sint2,&cost2,stacks*2);

    z0 = 1.0;
    z1 = cost2[(stacks>0)?1:0];
    r0 = 0.0;
    r1 = sint2[(stacks>0)?1:0];

    glBegin(GL_TRIANGLE_FAN);

        glNormal3d(0,0,1);
        glVertex3d(0,0,radius);

        for (j=slices; j>=0; j--)
        {
            glNormal3d(cost1[j]*r1,        sint1[j]*r1,        z1       );
            glVertex3d(cost1[j]*r1*radius, sint1[j]*r1*radius, z1*radius);
        }

    glEnd();

    for( i=1; i<stacks-1; i++ )
    {
        z0 = z1; z1 = cost2[i+1];
        r0 = r1; r1 = sint2[i+1];

        glBegin(GL_QUAD_STRIP);

            for(j=0; j<=slices; j++)
            {
                glNormal3d(cost1[j]*r1,        sint1[j]*r1,        z1       );
                glVertex3d(cost1[j]*r1*radius, sint1[j]*r1*radius, z1*radius);
                glNormal3d(cost1[j]*r0,        sint1[j]*r0,        z0       );
                glVertex3d(cost1[j]*r0*radius, sint1[j]*r0*radius, z0*radius);
            }

        glEnd();
    }

	z0 = z1;
    r0 = r1;

    glBegin(GL_TRIANGLE_FAN);

        glNormal3d(0,0,-1);
        glVertex3d(0,0,-radius);

        for (j=0; j<=slices; j++)
        {
            glNormal3d(cost1[j]*r0,        sint1[j]*r0,        z0       );
            glVertex3d(cost1[j]*r0*radius, sint1[j]*r0*radius, z0*radius);
        }

    glEnd();

    free(sint1);
    free(cost1);
    free(sint2);
    free(cost2);
}


/* smooth view origin shift */
void SpyCam::doViewShift(int transformTime)
{
	int t = server->Milliseconds();

	if (t-viewChangeTime > transformTime) {
		VectorCopy(toViewOrigin, currentViewOrigin);
		toggleShift = false;
		viewChangeTime = -1;
		viewRange = toViewRange;
		return;
	}

	float lerp[3];
	float up[3];

	float frac = (float)(t-viewChangeTime)/(float)transformTime;
	VectorMA(currentViewOrigin, frac, toViewOrigin, lerp);

	up[0] = -lerp[2]*lerp[0];
	up[1] = -lerp[2]*lerp[1];
	up[2] = lerp[0]*lerp[0]+lerp[1]*lerp[1];

	normalize(up);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(server->scoutOrigin[0], server->scoutOrigin[1], server->scoutOrigin[2],
			  lerp[0], lerp[1], lerp[2],
			  up[0], up[1], up[2]);

	glMatrixMode (GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90., 1., 10., viewRange+frac*(toViewRange-viewRange));
}

void SpyCam::drawScene()
{
	if (!server)
		return;

	int n = 0;
	float R = 0;

	centerOfGravity[0] = centerOfGravity[1] = centerOfGravity[2] = 0;
	maxDisplacement[0] = maxDisplacement[1] = maxDisplacement[2] = 0;
	avgDistance = 0;

	for (int i=0; i<64; i++) {
		svClient* cl = server->Client(i);
		if (cl->inSnapshot) {
			n++;
			VectorAdd(cl->origin, centerOfGravity, centerOfGravity);
		}
	}

	if (n == 0)
		return;

	VectorScale(centerOfGravity, 1./(float)n, centerOfGravity);
	float d;

	for (int i=0; i<64; i++) {
		svClient* cl = server->Client(i);
		if (cl->inSnapshot) {
			VectorSubtract(cl->origin, centerOfGravity, cl->r);
			d = DotProduct(cl->r, cl->r);
			avgDistance += sqrtf(d);

			if (d > R){
				R = d;
				VectorCopy(cl->r, maxDisplacement);
			}
		}
	}
	avgDistance /= n ? (float)n : 1.;
	maxRange = sqrtf(R);

	updateGL();
}
