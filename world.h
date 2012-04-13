#define WORLD_SIZE_X 5
#define WORLD_SIZE_Y 1
#define WORLD_SIZE_Z 5

#define WORLD_SIZE_XYZ WORLD_SIZE_X * WORLD_SIZE_Y * WORLD_SIZE_Z

#define TYPE_AIR 0
#define TYPE_STONE 1

struct vec3i {
	int r;
	int g;
	int b;
	char __padding[4];
};

struct vec3 {
	GLfloat x;
	GLfloat y;
	GLfloat z;
};

struct color {
	GLfloat r;
	GLfloat g;
	GLfloat b;
};

struct block {
	char type;
	char __padding[3];
	struct vec3i color;
};

struct vertex {
	struct vec3 position;
	struct vec3 normal;
	struct color color;
};

void world_init(void);
void world_tick(int delta);
void world_display(void);
