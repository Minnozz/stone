#ifndef _WORLD_H
#define _WORLD_H

#include <stdbool.h>

#define WORLD_SIZE_X 128
#define WORLD_SIZE_Y 64
#define WORLD_SIZE_Z 128

#define WORLD_SIZE_XZ WORLD_SIZE_X * WORLD_SIZE_Z
#define WORLD_SIZE_XYZ WORLD_SIZE_X * WORLD_SIZE_Y * WORLD_SIZE_Z

#define TYPE_AIR 0
#define TYPE_STONE 1

struct vec3 {
	GLfloat x;
	GLfloat y;
	GLfloat z;
};

struct vec4 {
	GLfloat x;
	GLfloat y;
	GLfloat z;
	GLfloat w;
};

struct mat4 {
	struct vec4 x;
	struct vec4 y;
	struct vec4 z;
	struct vec4 w;
};

struct color {
	GLfloat r;
	GLfloat g;
	GLfloat b;
};

struct directions {
	float right;
	float left;
	float up;
	float down;
	float front;
	float back;
};

struct block {
	char type;
	struct color color;
	struct directions occlusion;
};

struct vertex {
	struct vec3 position;
	struct vec3 normal;
	struct color color;
	GLfloat occlusion;
};

struct height_point {
	int x;
	int z;
	int height;
};

struct block *get_block(int x, int y, int z);
bool has_neighbors(int x, int y, int z);

void world_init(int argc, char **argv);
void world_tick(int delta);
void world_display(void);
void world_keyboard(unsigned char key, int x, int y);
void world_mouse(int button, int state, int x, int y);

#endif /* !defined _WORLD_H */
