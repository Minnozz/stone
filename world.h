#ifndef _WORLD_H
#define _WORLD_H

#define WORLD_SIZE_X 32
#define WORLD_SIZE_Y 32
#define WORLD_SIZE_Z 32

#define WORLD_SIZE_XZ WORLD_SIZE_X * WORLD_SIZE_Z
#define WORLD_SIZE_XYZ WORLD_SIZE_X * WORLD_SIZE_Y * WORLD_SIZE_Z

#define TYPE_AIR 0
#define TYPE_STONE 1

#define SELECT_BLOCK(x, y, z) world[(x) + (y) * WORLD_SIZE_X + (z) * WORLD_SIZE_X * WORLD_SIZE_Y]
#define SELECT_BLOCK_P(x, y, z) &SELECT_BLOCK(x, y, z)
#define SELECT_BLOCK_CHECKED_P(x, y, z) (((x) < 0 || (x) >= WORLD_SIZE_X || (y) < 0 || (y) >= WORLD_SIZE_Y || (z) < 0 || (z) >= WORLD_SIZE_Z) ? NULL : SELECT_BLOCK_P(x, y, z))

enum FACE {
	UP,
	DOWN,
	LEFT,
	RIGHT,
	FRONT,
	BACK
};

struct vec3i {
	int r;
	int g;
	int b;
};

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
	float up;
	float down;
	float left;
	float right;
	float front;
	float back;
};

struct block {
	char type;
	struct vec3i color;
	struct directions occlusion;
};

struct vertex {
	struct vec3 position;
	struct vec3 normal;
	struct color color;
	GLfloat light;
};

struct height_point {
	int x;
	int z;
	int height;
};

void world_init(const char *vertex_shader_filename, const char *fragment_shader_filename);
void world_tick(int delta);
void world_display(void);

#endif /* !defined _WORLD_H */
