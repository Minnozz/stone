#define WORLD_SIZE_X 256
#define WORLD_SIZE_Y 64
#define WORLD_SIZE_Z 256

#define WORLD_SIZE_XZ WORLD_SIZE_X * WORLD_SIZE_Z
#define WORLD_SIZE_XYZ WORLD_SIZE_X * WORLD_SIZE_Y * WORLD_SIZE_Z

#define TYPE_AIR 0
#define TYPE_STONE 1

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

struct block {
	char type;
	struct vec3i color;
};

struct vertex {
	struct vec3 position;
	struct vec3 normal;
	struct color color;
};

struct height_point {
	int x;
	int z;
	int height;
};

void world_init(void);
void world_tick(int delta);
void world_display(void);
