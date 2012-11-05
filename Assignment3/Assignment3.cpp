#include <stdlib.h>
#include <stdio.h>
#include <gl/glut.h>
#include "glm/glm.h"
#include "common.h"
#include <Cg/cg.h>
#include <Cg/cgGl.h>
using namespace Raytracer;

#define APP_NAME "Assignment 3"

int winW = 256;
int winH = 256;

int showShadows = 0;
int showReflectionColor = 0;
int showRefractedColor = 0;
int lightOverhead = 1;

GLfloat light_ambient[] = {1.0, 1.0, 1.0};
GLfloat light_specular[] = {1.0, 1.0, 1.0};
GLfloat light_diffuse[] = {1.0, 1.0, 1.0};
GLfloat light_position[] = {0.0, 5.0, 0.0, 1};

GLMmodel *model;

float angle_v = 0;
float angle_h = 0;

static CGcontext   myCgContext;
static CGprofile   myCgVertexProfile,
				   myCgFragmentProfile;
static CGprogram   myCgVertexProgram,
				   myCgFragmentProgram;
static CGparameter myCgFragmentParam_eyePosition;

static const char *myVertexProgramFileName = "vertex.cg",
				  *myVertexProgramName = "main",
				  *myFragmentProgramFileName = "fragment.cg",
				  *myFragmentProgramName = "main";

void glutDisplay();
void glutKeyboard(unsigned char key, int x, int y);
void glutSpecial(int key, int xx, int yy);
void glutResize(int width, int height);
void getObjModel();

static void checkForCgError(const char *situation)
{
  CGerror error;
  const char *string = cgGetLastErrorString(&error);

  if (error != CG_NO_ERROR) {
    printf("%s: %s: %s\n",
      APP_NAME, situation, string);
    if (error == CG_COMPILER_ERROR) {
	  const char *compError = cgGetLastListing(myCgContext);
      printf("%s\n", compError);
    }
    exit(1);
  }
}

void main() {
	getObjModel();

	glutInitWindowSize(winW, winH);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	
	glutCreateWindow(APP_NAME);
	glutDisplayFunc(glutDisplay);
	glutReshapeFunc(glutResize);
	glutKeyboardFunc(glutKeyboard);
	glutSpecialFunc(glutSpecial);

	glClearColor(0.0, 0.0, 0.0, 0.0);

	myCgContext = cgCreateContext();
	checkForCgError("creating context");
	cgGLSetDebugMode(CG_TRUE);
	cgSetParameterSettingMode(myCgContext, CG_DEFERRED_PARAMETER_SETTING);

	myCgVertexProfile = cgGLGetLatestProfile(CG_GL_VERTEX);
	cgGLSetOptimalOptions(myCgVertexProfile);
	checkForCgError("selecting vertex profile");

	myCgVertexProgram =
    cgCreateProgramFromFile(
      myCgContext,              // Cg runtime context
      CG_SOURCE,                // Program in human-readable form
      myVertexProgramFileName,  // Name of file containing program
      myCgVertexProfile,        // Profile: OpenGL ARB vertex program
      myVertexProgramName,      // Entry function name
      NULL);                    // No extra compiler options
	checkForCgError("creating vertex program from file");
	cgGLLoadProgram(myCgVertexProgram);
	checkForCgError("loading vertex program");

	myCgFragmentProfile = cgGLGetLatestProfile(CG_GL_FRAGMENT);
	cgGLSetOptimalOptions(myCgFragmentProfile);
	checkForCgError("selecting fragment profile");

	myCgFragmentProgram =
    cgCreateProgramFromFile(
      myCgContext,                // Cg runtime context
      CG_SOURCE,                  // Program in human-readable form
      myFragmentProgramFileName,  // Name of file containing program
      myCgFragmentProfile,        // Profile: OpenGL ARB vertex program
      myFragmentProgramName,      // Entry function name
      NULL);                      // No extra compiler options
	checkForCgError("creating fragment program from file");
	cgGLLoadProgram(myCgFragmentProgram);
	checkForCgError("loading fragment program");

#define GET_PARAM(name) \
	myCgFragmentParam_##name = \
		cgGetNamedParameter(myCgFragmentProgram, #name); \
	checkForCgError("could not get " #name " parameter");

	GET_PARAM(eyePosition);

	cgGLSetParameter3f(myCgFragmentParam_eyePosition, 0,0,-2);
	cgUpdateProgramParameters(myCgFragmentProgram);

	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	glLightModelf(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);

	//glEnable(GL_LIGHTING);
	//glEnable(GL_LIGHT0);
	//glEnable(GL_NORMALIZE);
	//glEnable(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	glutMainLoop();
}

void glutResize(int width, int height) {
	winW = width;
	winH = height;

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, winW, winH);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glFrustum(-1.1, 1.1, -1.1, 1.1, .9, 3.1);
	glMatrixMode(GL_MODELVIEW);

	gluLookAt(0, 0, -2, // eye position
			  0, 0, 1,  // look at vector
			  0, 1, 0); // up vector
}

void glutKeyboard(unsigned char key, int x, int y) {
	if (key == 's' || key == 'S') {
		showShadows = !showShadows;
		printf("Shadows on? %d\n", showShadows);
	} else if (key == 'r' || key == 'R') {
		showReflectionColor = !showReflectionColor;
		printf("Reflection on? %d\n", showReflectionColor);
	} else if (key == 'f' || key == 'F') {
		showRefractedColor = !showRefractedColor;
		printf("Refraction on? %d\n", showRefractedColor);
	} else if (key == 'l' || key == 'L') {
		lightOverhead = !lightOverhead;
		if (lightOverhead)
			light_position[2] = 0.0f;
		else
			light_position[2] = -5.0f;
		glLightfv(GL_LIGHT0, GL_POSITION, light_position);
		printf("Light overhead? %d\n", lightOverhead);
	} else if (key == 'u' || key == 'U') {
		printf("Unitizing models\n");
		glmUnitize(model);
	}

	glutPostRedisplay();
}

void glutSpecial(int key, int xx, int yy) {
	switch (key) {
	case GLUT_KEY_LEFT:
		angle_h -= 5;
		break;
	case GLUT_KEY_RIGHT:
		angle_h += 5;
		break;
	case GLUT_KEY_UP:
		angle_v += 5;
		break;
	case GLUT_KEY_DOWN:
		angle_v -= 5;
		break;
	}

	glutPostRedisplay();
}

void glutDisplay() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	cgGLBindProgram(myCgVertexProgram);
	checkForCgError("binding vertex program");

	cgGLEnableProfile(myCgVertexProfile);
	checkForCgError("enabling vertex profile");

	cgGLBindProgram(myCgFragmentProgram);
	checkForCgError("binding fragment program");

	cgGLEnableProfile(myCgFragmentProfile);
	checkForCgError("enabling fragment profile");

	int i, j;
	GLMgroup *group = model->groups;
	while (group) {
		GLMmaterial material = model->materials[group->material];
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, material.ambient);
		glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, material.diffuse);
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, material.specular);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, material.shininess);
		glBegin(GL_TRIANGLES);
		for (i=0; i<group->numtriangles; i++) {
			int triangleIndex = group->triangles[i];
			GLMtriangle triangle = model->triangles[triangleIndex];
			//glNormal3fv(&model->facetnorms[3*triangle.findex]);
			for (j=0; j<3; j++) {
				int index = triangle.vindices[j];
				vector3 vertex(model->vertices[3*index],		// x
								model->vertices[3*index+1],		// y
								model->vertices[3*index+2]);	// z
				glNormal3fv(&model->normals[3*triangle.nindices[j]]);
				glVertex3f(vertex.x, vertex.y, vertex.z);
			}
		}
		glEnd();
		group = group->next;
	}

	cgGLDisableProfile(myCgVertexProfile);
	checkForCgError("disabling vertex profile");

	cgGLDisableProfile(myCgFragmentProfile);
	checkForCgError("disabling fragment profile");

	glutSwapBuffers();
}

void getObjModel() {
	model = glmReadOBJ("input.obj");
	glmFacetNormals(model);
	glmVertexNormals(model, 89); // 89 because 90 creates weird shadows on flat surfaces
	//glmUnitize(model);
}