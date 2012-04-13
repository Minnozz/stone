#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GLUT/glut.h>

#include "world.h"

// Globals

int *height_map;
struct block *world;

unsigned int vertex_amount = 0;
unsigned int vertex_capacity = 0;
GLuint vertex_buffer_handle;
struct vertex *vertex_buffer;

struct vec3 camera_position;
struct vec3 camera_target;

// Util functions

#define BUFFER_OFFSET(n) ((void *) (n))

#define SELECT_HEIGHT(x, z) height_map[x + z * WORLD_SIZE_X]

#define SELECT_BLOCK(x, y, z) world[x + y * WORLD_SIZE_X + z * WORLD_SIZE_X * WORLD_SIZE_Y]
#define SELECT_BLOCK_P(x, y, z) &SELECT_BLOCK(x, y, z)
#define SELECT_BLOCK_CHECKED_P(x, y, z) ((x < 0 || x >= WORLD_SIZE_X || y < 0 || y >= WORLD_SIZE_Y || z < 0 || z >= WORLD_SIZE_Z) ? NULL : SELECT_BLOCK_P(x, y, z))

static void print_vertex(struct vertex *vert) {
	printf("Dumping vertex:\n");
	printf("position = (%f, %f, %f)\n", vert->position.x, vert->position.y, vert->position.z);
	printf("normal = (%f, %f, %f)\n", vert->normal.x, vert->normal.y, vert->normal.z);
	printf("color = (%f, %f, %f)\n", vert->color.r, vert->color.g, vert->color.b);
}

// VBO

static void create_vertex(struct vec3 position, struct vec3 normal, struct color color) {
	// Grow the vertex buffer if necessary
	unsigned int need = vertex_amount + 1;
	if(need > vertex_capacity) {
		vertex_capacity += 100;
		struct vertex *new_vertex_buffer = (struct vertex *) realloc(vertex_buffer, sizeof(struct vertex) * vertex_capacity);
		if(new_vertex_buffer == NULL) {
			fprintf(stderr, "Could not allocate enough memory for %u vertices", need);
			exit(1);
		}
		vertex_buffer = new_vertex_buffer;
	}

	struct vertex *new_vertex = vertex_buffer + vertex_amount;

	new_vertex->position = position;
	new_vertex->normal = normal;
	new_vertex->color = color;

	vertex_amount++;
}

static void create_vertex_long(int position_x, int position_y, int position_z, int normal_x, int normal_y, int normal_z, int color_r, int color_g, int color_b) {
	struct vec3 position = { (GLfloat) position_x, (GLfloat) position_y, (GLfloat) position_z };
	struct vec3 normal = { (GLfloat) normal_x, (GLfloat) normal_y, (GLfloat) normal_z };
	struct color color = { (GLfloat) (color_r / 256.0f), (GLfloat) (color_g / 256.0f), (GLfloat) (color_b / 256.0f) };

	create_vertex(position, normal, color);
}

static void fill_vertex_buffer() {
	// Bind buffer
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_handle);

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
				if((other = SELECT_BLOCK_CHECKED_P(x + 1, y, z)) != NULL && other->type != TYPE_AIR) {
					create_vertex_long(x+1, y, z, -1, 0, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y, z+1, -1, 0, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y+1, z+1, -1, 0, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y+1, z, -1, 0, 0, other->color.r, other->color.g, other->color.b);
				}
				// Negative x
				if((other = SELECT_BLOCK_CHECKED_P(x - 1, y, z)) != NULL && other->type != TYPE_AIR) {
					create_vertex_long(x, y, z+1, 1, 0, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y, z, 1, 0, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y+1, z, 1, 0, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y+1, z+1, 1, 0, 0, other->color.r, other->color.g, other->color.b);
				}
				// Positive y
				if((other = SELECT_BLOCK_CHECKED_P(x, y + 1, z)) != NULL && other->type != TYPE_AIR) {
					create_vertex_long(x+1, y+1, z+1, 0, -1, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y+1, z, 0, -1, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y+1, z, 0, -1, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y+1, z+1, 0, -1, 0, other->color.r, other->color.g, other->color.b);
				}
				// Negative y
				if((other = SELECT_BLOCK_CHECKED_P(x, y - 1, z)) != NULL && other->type != TYPE_AIR) {
					create_vertex_long(x, y, z, 0, 1, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y, z, 0, 1, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y, z+1, 0, 1, 0, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y, z+1, 0, 1, 0, other->color.r, other->color.g, other->color.b);
				}
				// Positive z
				if((other = SELECT_BLOCK_CHECKED_P(x, y, z + 1)) != NULL && other->type != TYPE_AIR) {
					create_vertex_long(x+1, y, z+1, 0, 0, -1, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y, z+1, 0, 0, -1, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y+1, z+1, 0, 0, -1, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y+1, z+1, 0, 0, -1, other->color.r, other->color.g, other->color.b);
				}
				// Negative z
				if((other = SELECT_BLOCK_CHECKED_P(x, y, z - 1)) != NULL && other->type != TYPE_AIR) {
					create_vertex_long(x, y, z, 0, 0, 1, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x, y+1, z, 0, 0, 1, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y+1, z, 0, 0, 1, other->color.r, other->color.g, other->color.b);
					create_vertex_long(x+1, y, z, 0, 0, 1, other->color.r, other->color.g, other->color.b);
				}
			}
		}
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(struct vertex) * vertex_amount, vertex_buffer, GL_STATIC_DRAW);
}

// Main functions

void world_init() {
	// Generate random world
	height_map = (int *) malloc(sizeof(int) * WORLD_SIZE_XYZ);

	for(int x = 0; x < WORLD_SIZE_X; x++) {
		for(int z = 0; z < WORLD_SIZE_Z; z++) {
			SELECT_HEIGHT(x, z) = WORLD_SIZE_Y;
#if 0
			int offset = 0;

			switch(random() % 8) {
				case 0:	offset = -2; break;
				case 1:	offset = -1; break;
				case 2:	offset = 0; break;
				case 3:	offset = 0; break;
				case 4:	offset = 0; break;
				case 5:	offset = 0; break;
				case 6:	offset = 1; break;
				case 7:	offset = 2; break;
			}

			if(x == 0 && z == 0) {
				SELECT_HEIGHT(x, z) = WORLD_SIZE_Y / 2;
			} else if(x == 0) {
				SELECT_HEIGHT(x, z) = offset + SELECT_HEIGHT(x, z - 1);
			} else if(z == 0) {
				SELECT_HEIGHT(x, z) = offset + SELECT_HEIGHT(x - 1, z);
			} else {
				SELECT_HEIGHT(x, z) = offset + (int) round((SELECT_HEIGHT(x - 1, z) + SELECT_HEIGHT(x, z - 1) + SELECT_HEIGHT(x - 1, z - 1)) / 3.0f);
			}
#endif
		}
	}

	// Populate world with blocks
	world = (struct block *) malloc(sizeof(struct block) * WORLD_SIZE_XYZ);

	for(int x = 0; x < WORLD_SIZE_X; x++) {
		for(int y = 0; y < WORLD_SIZE_Y; y++) {
			for(int z = 0; z < WORLD_SIZE_Z; z++) {
				struct block *current = SELECT_BLOCK_P(x, y, z);

				if(y <= SELECT_HEIGHT(x, z)) {
					current->type = TYPE_STONE;
					current->color.r = (random() % 128) + 64;
					current->color.g = (random() % 128) + 64;
					current->color.b = (random() % 128) + 64;
				} else {
					current->type = TYPE_AIR;
				}
			}
		}
	}

	// Create VBO
	glGenBuffers(1, &vertex_buffer_handle);
	fill_vertex_buffer();
	fprintf(stderr, "Filled vertex buffer with %u vertices\n", vertex_amount);

	// Set camera position and target
	camera_position.x = WORLD_SIZE_X * 2.0f;
	camera_position.y = WORLD_SIZE_Y * 4.0f;
	camera_position.z = WORLD_SIZE_Z * 2.0f;
	camera_target.x = WORLD_SIZE_X / 2.0f;
	camera_target.y = WORLD_SIZE_Y / 2.0f;
	camera_target.z = WORLD_SIZE_Z / 2.0f;

	print_vertex(&vertex_buffer[0]);
}

void world_tick(int delta __unused) {
#if 0
	// Set camera position and target
	camera_position.x += delta * -0.005f;
	camera_position.y += delta * 0.002f;
	camera_position.z += delta * -0.005f;
	//camera_target.x += delta * 0.0005f;
	camera_target.y += delta * -0.005f;
	camera_target.z += delta * -0.01f;
#endif
}

void world_display() {
	glPushMatrix();

	// Set camera position and target
	gluLookAt(camera_position.x, camera_position.y, camera_position.z, camera_target.x, camera_target.y, camera_target.z, 0.0f, 1.0f, 0.0f);

	// Bind buffer
	glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer_handle);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof(struct vertex), BUFFER_OFFSET(0));	// 3 coordinates per vertex, float coordinates, 0 bytes spacing, 0 byte offset from start
	glNormalPointer(GL_FLOAT, sizeof(struct vertex), BUFFER_OFFSET(sizeof(struct vec3)));
	glColorPointer(3, GL_FLOAT, sizeof(struct vertex), BUFFER_OFFSET(sizeof(struct vec3) * 2));

	// Draw quads, starting at offset 0, and specify the amount
	glDrawArrays(GL_QUADS, 0, (GLsizei) vertex_amount);

	glPopMatrix();
}
