#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glew.h>
#include <GLUT/glut.h>

#include "shader.h"
#include "world.h"

// Globals

struct block *world;

unsigned int vertex_amount = 0;
unsigned int vertex_capacity = 0;

struct vec3 camera_position;
struct vec3 camera_target;

// GL resources

static struct {
	// Vertex buffer
	GLuint vertex_buffer_handle;
	struct vertex *vertex_buffer_data;

	// Shaders
	GLuint vertex_shader;
	GLuint fragment_shader;
	GLuint program;

	// Uniforms
	struct {
		GLint modelview;
		GLint mvp;
	} uniforms;

	struct mat4 modelview;
	struct mat4 mvp;

	// Attributes
	struct {
		GLint position;
		GLint normal;
		GLint color;
	} attributes;
} resources;

// Util functions

#define BUFFER_OFFSET(n) ((void *) (n))

#define SELECT_HEIGHT(x, z) height_map[x + z * WORLD_SIZE_X]

#define SELECT_BLOCK(x, y, z) world[(x) + (y) * WORLD_SIZE_X + (z) * WORLD_SIZE_X * WORLD_SIZE_Y]
#define SELECT_BLOCK_P(x, y, z) &SELECT_BLOCK(x, y, z)
#define SELECT_BLOCK_CHECKED_P(x, y, z) (((x) < 0 || (x) >= WORLD_SIZE_X || (y) < 0 || (y) >= WORLD_SIZE_Y || (z) < 0 || (z) >= WORLD_SIZE_Z) ? NULL : SELECT_BLOCK_P(x, y, z))

static void dump_vertex(struct vertex *vert) {
	printf("Dumping vertex:\n");
	printf("\tposition = (%f, %f, %f)\n", vert->position.x, vert->position.y, vert->position.z);
	printf("\tnormal = (%f, %f, %f)\n", vert->normal.x, vert->normal.y, vert->normal.z);
	printf("\tcolor = (%f, %f, %f)\n", vert->color.r, vert->color.g, vert->color.b);
}

inline static int clamp(int n, int min, int max) {
	if(n < min) {
		return min;
	}
	if(n > max) {
		return max;
	}
	return n;
}

// VBO

static void create_vertex(struct vec3 position, struct vec3 normal, struct color color) {
	// Grow the vertex buffer if necessary
	unsigned int need = vertex_amount + 1;
	if(need > vertex_capacity) {
		vertex_capacity += 100;
		struct vertex *new_vertex_buffer_data = (struct vertex *) realloc(resources.vertex_buffer_data, sizeof(struct vertex) * vertex_capacity);
		if(new_vertex_buffer_data == NULL) {
			fprintf(stderr, "Could not allocate enough memory for %u vertices", need);
			exit(1);
		}
		resources.vertex_buffer_data = new_vertex_buffer_data;
	}

	struct vertex *new_vertex = resources.vertex_buffer_data + vertex_amount;

	new_vertex->position = position;
	new_vertex->normal = normal;
	new_vertex->color = color;

	vertex_amount++;
}

static struct color random_color() {
	struct color color;
	color.r = (random() % 256) / 256.0f;
	color.g = (random() % 256) / 256.0f;
	color.b = (random() % 256) / 256.0f;
	return color;
}

static void create_vertex_long(int position_x, int position_y, int position_z, int normal_x, int normal_y, int normal_z, int color_r, int color_g, int color_b) {
	struct vec3 position = { (GLfloat) position_x, (GLfloat) position_y, (GLfloat) position_z };
	struct vec3 normal = { (GLfloat) normal_x, (GLfloat) normal_y, (GLfloat) normal_z };
	struct color color = { (GLfloat) (color_r / 256.0f), (GLfloat) (color_g / 256.0f), (GLfloat) (color_b / 256.0f) };

	create_vertex(position, normal, color);
}

static void fill_vertex_buffer() {
	// Bind buffer
	glBindBuffer(GL_ARRAY_BUFFER, resources.vertex_buffer_handle);

	// Create quads
	for(int x = -1; x <= WORLD_SIZE_X; x++) {
		for(int y = -1; y <= WORLD_SIZE_Y; y++) {
			for(int z = -1; z <= WORLD_SIZE_Z; z++) {
				// Get block
				struct block *current = SELECT_BLOCK_CHECKED_P(x, y, z);
				if(current != NULL && current->type != TYPE_AIR) {
					// Only create vertices from air/empty blocks (looking outwards)
					continue;
				}

				// Check the blocks directly adjacent to the six faces of the current block
				struct block *other;

				// Positive x
				if((other = SELECT_BLOCK_CHECKED_P(x+1, y, z)) != NULL && other->type != TYPE_AIR) {
					create_vertex_long(x+1, y, z, -1, 0, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y, z+1, -1, 0, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y+1, z+1, -1, 0, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y+1, z, -1, 0, 0, other->color.r, other->color.g, other->color.b);
				}
				// Negative x
				if((other = SELECT_BLOCK_CHECKED_P(x-1, y, z)) != NULL && other->type != TYPE_AIR) {
					create_vertex_long(x, y, z+1, 1, 0, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y, z, 1, 0, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y+1, z, 1, 0, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y+1, z+1, 1, 0, 0, other->color.r, other->color.g, other->color.b);
				}
				// Positive y
				if((other = SELECT_BLOCK_CHECKED_P(x, y+1, z)) != NULL && other->type != TYPE_AIR) {
					create_vertex_long(x+1, y+1, z+1, 0, -1, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y+1, z, 0, -1, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y+1, z, 0, -1, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y+1, z+1, 0, -1, 0, other->color.r, other->color.g, other->color.b);
				}
				// Negative y
				if((other = SELECT_BLOCK_CHECKED_P(x, y-1, z)) != NULL && other->type != TYPE_AIR) {
					create_vertex_long(x, y, z, 0, 1, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y, z, 0, 1, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y, z+1, 0, 1, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y, z+1, 0, 1, 0, other->color.r, other->color.g, other->color.b);
				}
				// Positive z
				if((other = SELECT_BLOCK_CHECKED_P(x, y, z+1)) != NULL && other->type != TYPE_AIR) {
					create_vertex_long(x+1, y, z+1, 0, 0, -1, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y, z+1, 0, 0, -1, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y+1, z+1, 0, 0, -1, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y+1, z+1, 0, 0, -1, other->color.r, other->color.g, other->color.b);
				}
				// Negative z
				if((other = SELECT_BLOCK_CHECKED_P(x, y, z-1)) != NULL && other->type != TYPE_AIR) {
					create_vertex_long(x, y, z, 0, 0, 1, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y+1, z, 0, 0, 1, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y+1, z, 0, 0, 1, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y, z, 0, 0, 1, other->color.r, other->color.g, other->color.b);
				}
			}
		}
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(struct vertex) * vertex_amount, resources.vertex_buffer_data, GL_STATIC_DRAW);
}

// Main functions

void world_init(const char *vertex_shader_filename, const char *fragment_shader_filename) {
	printf("World size: %dx%dx%d\n", WORLD_SIZE_X, WORLD_SIZE_Y, WORLD_SIZE_Z);

	// Seed random number generator
	srandomdev();

	// Create height points
	unsigned int height_points_amount = WORLD_SIZE_XZ / 2000;
	printf("Generating %u height points\n", height_points_amount);
	struct height_point *height_points = (struct height_point *) malloc(sizeof(struct height_point) * height_points_amount);
	for(unsigned int i = 0; i < height_points_amount; i++) {
		struct height_point *point = &height_points[i];

		point->x = random() % WORLD_SIZE_X;
		point->z = random() % WORLD_SIZE_Z;
		point->height = random() % WORLD_SIZE_Y;
	}

	// Create height map (height for every (x,z) coordinate)
	printf("Generating height map\n");
	int *height_map = (int *) malloc(sizeof(int) * WORLD_SIZE_XZ);
	for(int x = 0; x < WORLD_SIZE_X; x++) {
		for(int z = 0; z < WORLD_SIZE_Z; z++) {
			float total_height = 0;
			float total_weight = 0;

			// Height is the weighted average of all height points (weight = distance) with some noise
			for(unsigned int i = 0; i < height_points_amount; i++) {
				struct height_point *point = &height_points[i];

				if(x == point->x && z == point->z) {
					total_height += point->height;
					total_weight += 1;
				} else {
					float weight = (float) (1 / (pow(x - point->x, 2) + pow(z - point->z, 2)));
					total_height += point->height * weight;
					total_weight += weight;
				}
			}

			SELECT_HEIGHT(x, z) = (int) (total_height / total_weight) + (random() % 2);
		}
	}

	// Populate world with blocks
	printf("Generating blocks\n");
	world = (struct block *) malloc(sizeof(struct block) * WORLD_SIZE_XYZ);
	for(int x = 0; x < WORLD_SIZE_X; x++) {
		for(int y = 0; y < WORLD_SIZE_Y; y++) {
			for(int z = 0; z < WORLD_SIZE_Z; z++) {
				struct block *current = SELECT_BLOCK_P(x, y, z);

				if(y <= SELECT_HEIGHT(x, z)) {
					current->type = TYPE_STONE;
					int r = random() % 8;
					current->color.r = 38 + r;
					current->color.g = 32 + r;
					current->color.b = 32 + r;
				} else {
					current->type = TYPE_AIR;
				}
			}
		}
	}

	// Create VBO
	glGenBuffers(1, &resources.vertex_buffer_handle);
	fill_vertex_buffer();
	fprintf(stderr, "Filled vertex buffer with %u vertices (%f MB)\n", vertex_amount, (sizeof(struct vertex) * vertex_amount) / (float)(1024 * 1024));

	// Create shaders
	resources.vertex_shader = make_shader(GL_VERTEX_SHADER, vertex_shader_filename);
	if(!resources.vertex_shader) {
		exit(1);
	}
	resources.fragment_shader = make_shader(GL_FRAGMENT_SHADER, fragment_shader_filename);
	if(!resources.fragment_shader) {
		exit(1);
	}
	resources.program = make_program(resources.vertex_shader, resources.fragment_shader);
	if(!resources.program) {
		exit(1);
	}

	// Bind program variables
	resources.uniforms.modelview = glGetUniformLocation(resources.program, "modelview");
	resources.uniforms.mvp = glGetUniformLocation(resources.program, "mvp");
	resources.attributes.position = glGetAttribLocation(resources.program, "position");
	resources.attributes.normal = glGetAttribLocation(resources.program, "normal");
	resources.attributes.color = glGetAttribLocation(resources.program, "color");

	// Set camera position and target
	camera_position.x = WORLD_SIZE_X * 0.0f;
	camera_position.y = WORLD_SIZE_Y * 0.5f;
	camera_position.z = WORLD_SIZE_Z * 0.05f;
	camera_target.x = WORLD_SIZE_X * 1.0f;
	camera_target.y = WORLD_SIZE_Y * 0.6f;
	camera_target.z = WORLD_SIZE_Z * 1.0f;
}

void world_tick(int delta) {
	// Move camera position
	camera_position.x += delta * WORLD_SIZE_X * 0.00005f;
	camera_position.y += delta * WORLD_SIZE_Y * 0.00001f;
	camera_position.z += delta * WORLD_SIZE_Z * 0.0f;

	// Move camera target
	camera_target.x += delta * WORLD_SIZE_X * -0.00005f;
	camera_target.y += delta * WORLD_SIZE_Y * 0.0f;
	camera_target.z += delta * WORLD_SIZE_Z * 0.0f;
}

void world_display() {
	glClearColor(0.8f, 0.8f, 0.8f, 1.0f);
	glClearDepth(1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(resources.program);

	// Set uniforms
	glUniformMatrix4fv(resources.uniforms.modelview, 1, GL_FALSE, (const GLfloat *) &resources.modelview);
	glUniformMatrix4fv(resources.uniforms.mvp, 1, GL_FALSE, (const GLfloat *) &resources.mvp);

	// Vertex buffer
	glBindBuffer(GL_ARRAY_BUFFER, resources.vertex_buffer_handle);
	glVertexAttribPointer((GLuint) resources.attributes.position, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), BUFFER_OFFSET(0));
	glEnableVertexAttribArray((GLuint) resources.attributes.position);
	glVertexAttribPointer((GLuint) resources.attributes.normal, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), BUFFER_OFFSET(sizeof(struct vec3)));
	glEnableVertexAttribArray((GLuint) resources.attributes.normal);
	glVertexAttribPointer((GLuint) resources.attributes.color, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), BUFFER_OFFSET(sizeof(struct vec3) * 2));
	glEnableVertexAttribArray((GLuint) resources.attributes.color);

	// Draw quads, starting at offset 0, and specify the amount
	glPushMatrix();
	gluLookAt(camera_position.x, camera_position.y, camera_position.z, camera_target.x, camera_target.y, camera_target.z, 0.0f, 1.0f, 0.0f);
	glDrawArrays(GL_QUADS, 0, (GLsizei) vertex_amount);
	glPopMatrix();

	glDisableVertexAttribArray((GLuint) resources.attributes.position);
	glDisableVertexAttribArray((GLuint) resources.attributes.normal);
	glDisableVertexAttribArray((GLuint) resources.attributes.color);
}
