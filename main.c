#include <stdlib.h>
#include <GL/glew.h>
#include <GLUT/glut.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "world.h"

// Globals

int previous_frame;

int fps_counter;
int fps_last_update;

// Functions

void init(int argc, char **argv) {
	// Seed random number generator
	srandomdev();

	// Initialize timers
	previous_frame = glutGet(GLUT_ELAPSED_TIME);
	fps_counter = 0;
	fps_last_update = 0;

	// Initialize projection matrix
	glMatrixMode(GL_PROJECTION);
	gluPerspective(60.0f, (1024.0 / 768.0), 0.1f, 1024.0f);

	// Depth test
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);

	// Shading
	glShadeModel(GL_SMOOTH);

	// Initialize world
	world_init(argc, argv);
}

void idle() {
	// Get milliseconds since last frame
	int elapsed = glutGet(GLUT_ELAPSED_TIME);
	int delta = elapsed - previous_frame;
	previous_frame = elapsed;

	// Calculate FPS
	fps_counter++;
	if((elapsed - fps_last_update) > 1000) {
		// Update window title with current FPS
		char *title;
		asprintf(&title, "Stone | %d FPS", fps_counter);
		glutSetWindowTitle(title);
		free(title);

		// Reset counter
		fps_counter = 0;
		fps_last_update = elapsed;
	}

	world_tick(delta);

	glutPostRedisplay();
}

void display() {
	world_display();

	glutSwapBuffers();
}

void keyboard(unsigned char key, int x, int y) {
	switch(key) {
		case 'q':	// Quit
			exit(0);
			break;
		default:
			world_keyboard(key, x, y);
			break;
	}
}

void mouse(int button, int state, int x, int y) {
}

int main(int argc, char **argv) {
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
	glutInitWindowSize(1024, 768);
	glutCreateWindow("Stone");

	glutIdleFunc(idle);
	glutDisplayFunc(display);
	glutKeyboardFunc(keyboard);
	glutMouseFunc(mouse);

	glewInit();
	if(!GLEW_VERSION_2_0) {
		fprintf(stderr, "OpenGL 2.0 not available\n");
		return 1;
	}

	init(argc, argv);

	glutMainLoop();

	return 0;
}
