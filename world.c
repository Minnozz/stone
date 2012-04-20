#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <assert.h>
#include <string.h>
#include <GL/glew.h>
#include <GLUT/glut.h>

#include "shader.h"
#include "occlusion.h"
#include "util.h"
#include "world.h"

// Globals

int ticks = 0;

int *height_map;
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
		GLint occlusion;
	} attributes;
} resources;

// Util functions

#define BUFFER_OFFSET(n) ((void *) (n))

inline struct block *get_block(int x, int y, int z) {
	if(x < 0 || x >= WORLD_SIZE_X) {
		return NULL;
	}
	if(y < 0 || y >= WORLD_SIZE_Y) {
		return NULL;
	}
	if(z < 0 || z >= WORLD_SIZE_Z) {
		return NULL;
	}
	return world + x + y * WORLD_SIZE_X + z * WORLD_SIZE_X * WORLD_SIZE_Y;
}

static inline int get_height(int x, int z) {
	return height_map[x + z * WORLD_SIZE_X];
}

static inline void set_height(int x, int z, int height) {
	height_map[x + z * WORLD_SIZE_X] = height;
}

static void dump_vertex(struct vertex *vert) {
	fprintf(stderr, "Dumping vertex:\n");
	fprintf(stderr, "\tposition = (%f, %f, %f)\n", vert->position.x, vert->position.y, vert->position.z);
	fprintf(stderr, "\tnormal = (%f, %f, %f)\n", vert->normal.x, vert->normal.y, vert->normal.z);
	fprintf(stderr, "\tcolor = (%f, %f, %f)\n", vert->color.r, vert->color.g, vert->color.b);
	fprintf(stderr, "\tocclusion = %f\n", vert->occlusion);
}

static void dump_block(struct block *block) {
	fprintf(stderr, "Dumping block:\n");
	fprintf(stderr, "\ttype = %d\n", block->type);
	fprintf(stderr, "\tcolor = (%f, %f, %f)\n", block->color.r, block->color.g, block->color.b);
	fprintf(stderr, "\tocclusion = (%f, %f, %f, %f, %f, %f)\n", block->occlusion.up, block->occlusion.down, block->occlusion.left, block->occlusion.right, block->occlusion.front, block->occlusion.back);
}

// Height map

static void load_height_map(char *filename) {
	fprintf(stderr, "Loading height map %s\n", filename);
	int length;
	char *data = file_contents(filename, &length);
	if(data == NULL) {
		fprintf(stderr, "Could not load height map %s\n", filename);
		exit(1);
	}

	height_map = (int *) malloc(sizeof(int) * WORLD_SIZE_XZ);
	for(int z = 0; z < WORLD_SIZE_Z; z++) {
		for(int x = 0; x < WORLD_SIZE_X; x++) {
			assert(*data++ == '[');
			char *end = strchr(data, ']');
			assert(end != NULL);
			*end = '\0';
			int height = atoi(data);
			assert(height >= 0);
			data = end + 1;

			set_height(x, z, height);
			fprintf(stderr, "Set height to %d for coordinate (%d,%d)\n", height, x, z);
		}
		assert(*data++ == '\n');
	}
	assert(*data == '\0');
}

static void create_random_height_map() {
	// Determine amount of points to use
	unsigned int height_points_amount = WORLD_SIZE_XZ / 2000;
	if(height_points_amount < 3) {
		height_points_amount = 3;
	}
	fprintf(stderr, "Generating %u height points\n", height_points_amount);
	struct height_point *height_points = (struct height_point *) malloc(sizeof(struct height_point) * height_points_amount);
	for(unsigned int i = 0; i < height_points_amount; i++) {
		struct height_point *point = &height_points[i];

		point->x = random() % WORLD_SIZE_X;
		point->z = random() % WORLD_SIZE_Z;
		point->height = random() % (WORLD_SIZE_Y + 1);

		fprintf(stderr, "\ty = %d at (%d,%d)\n", point->height, point->x, point->z);
	}

	// Create height map (height for every (x,z) coordinate)
	fprintf(stderr, "Generating height map\n");
	height_map = (int *) malloc(sizeof(int) * WORLD_SIZE_XZ);
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

			set_height(x, z, (int) (total_height / total_weight) + (random() % 2));
		}
	}
}

// VBO

static void create_vertex(int px, int py, int pz, int nx, int ny, int nz, struct color color, GLfloat occlusion) {
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

	new_vertex->position.x = (GLfloat) px;
	new_vertex->position.y = (GLfloat) py;
	new_vertex->position.z = (GLfloat) pz;
	new_vertex->normal.x = (GLfloat) nx;
	new_vertex->normal.y = (GLfloat) ny;
	new_vertex->normal.z = (GLfloat) nz;
	new_vertex->color = color;
	new_vertex->occlusion = occlusion;

	vertex_amount++;
}

static void fill_vertex_buffer() {
	// Bind buffer
	glBindBuffer(GL_ARRAY_BUFFER, resources.vertex_buffer_handle);

	// Loop through all blocks (and a 1 block layer outside the map to 'look at' the outer faces)
	for(int x = -1; x <= WORLD_SIZE_X; x++) {
		for(int y = -1; y <= WORLD_SIZE_Y; y++) {
			for(int z = -1; z <= WORLD_SIZE_Z; z++) {
				struct block *current = get_block(x, y, z);
				if(current != NULL && current->type != TYPE_AIR) {
					// Only create vertices from the empty layer around the map or an AIR block
					continue;
				}

				// Check the blocks directly adjacent to the six faces of the current empty block
				struct block *other;

				// Positive x
				if((other = get_block(x+1, y, z)) != NULL && other->type != TYPE_AIR) {
					float occlusion = (current == NULL) ? 0.0f : current->occlusion.right;
					create_vertex(x+1, y, z, -1, 0, 0, other->color, occlusion);
					create_vertex(x+1, y, z+1, -1, 0, 0, other->color, occlusion);
					create_vertex(x+1, y+1, z+1, -1, 0, 0, other->color, occlusion);
					create_vertex(x+1, y+1, z, -1, 0, 0, other->color, occlusion);
				}
				// Negative x
				if((other = get_block(x-1, y, z)) != NULL && other->type != TYPE_AIR) {
					float occlusion = (current == NULL) ? 0.0f : current->occlusion.left;
					create_vertex(x, y, z+1, 1, 0, 0, other->color, occlusion);
					create_vertex(x, y, z, 1, 0, 0, other->color, occlusion);
					create_vertex(x, y+1, z, 1, 0, 0, other->color, occlusion);
					create_vertex(x, y+1, z+1, 1, 0, 0, other->color, occlusion);
				}
				// Positive y
				if((other = get_block(x, y+1, z)) != NULL && other->type != TYPE_AIR) {
					float occlusion = (current == NULL) ? 0.0f : current->occlusion.up;
					create_vertex(x+1, y+1, z+1, 0, -1, 0, other->color, occlusion);
					create_vertex(x+1, y+1, z, 0, -1, 0, other->color, occlusion);
					create_vertex(x, y+1, z, 0, -1, 0, other->color, occlusion);
					create_vertex(x, y+1, z+1, 0, -1, 0, other->color, occlusion);
				}
				// Negative y
				if((other = get_block(x, y-1, z)) != NULL && other->type != TYPE_AIR) {
					float occlusion = (current == NULL) ? 0.0f : current->occlusion.down;
					create_vertex(x, y, z, 0, 1, 0, other->color, occlusion);
					create_vertex(x+1, y, z, 0, 1, 0, other->color, occlusion);
					create_vertex(x+1, y, z+1, 0, 1, 0, other->color, occlusion);
					create_vertex(x, y, z+1, 0, 1, 0, other->color, occlusion);
				}
				// Positive z
				if((other = get_block(x, y, z+1)) != NULL && other->type != TYPE_AIR) {
					float occlusion = (current == NULL) ? 0.0f : current->occlusion.front;
					create_vertex(x+1, y, z+1, 0, 0, -1, other->color, occlusion);
					create_vertex(x, y, z+1, 0, 0, -1, other->color, occlusion);
					create_vertex(x, y+1, z+1, 0, 0, -1, other->color, occlusion);
					create_vertex(x+1, y+1, z+1, 0, 0, -1, other->color, occlusion);
				}
				// Negative z
				if((other = get_block(x, y, z-1)) != NULL && other->type != TYPE_AIR) {
					float occlusion = (current == NULL) ? 0.0f : current->occlusion.back;
					create_vertex(x, y, z, 0, 0, 1, other->color, occlusion);
					create_vertex(x, y+1, z, 0, 0, 1, other->color, occlusion);
					create_vertex(x+1, y+1, z, 0, 0, 1, other->color, occlusion);
					create_vertex(x+1, y, z, 0, 0, 1, other->color, occlusion);
				}
			}
		}
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(struct vertex) * vertex_amount, resources.vertex_buffer_data, GL_STATIC_DRAW);
}

// Main functions

void world_init(int argc, char **argv) {
	// Parse options
	char *vertex_shader_file = NULL, *fragment_shader_file = NULL, *height_map_file = NULL;
	int c;
	while((c = getopt(argc, argv, "v:f:m:")) != -1) {
		switch(c) {
			case 'v':
				vertex_shader_file = optarg;
				break;
			case 'f':
				fragment_shader_file = optarg;
				break;
			case 'm':
				height_map_file = optarg;
				break;
			case '?':
			default:
				fprintf(stderr, "Invalid arguments\n");
				exit(1);
		}
	}
	argc -= optind;
	argv += optind;

	// Default options
	if(vertex_shader_file == NULL) {
		vertex_shader_file = "res/shaders/vertex.glsl";
	}
	if(fragment_shader_file == NULL) {
		fragment_shader_file = "res/shaders/fragment.glsl";
	}

	fprintf(stderr, "World size: %dx%dx%d\n", WORLD_SIZE_X, WORLD_SIZE_Y, WORLD_SIZE_Z);

	// Create height map
	if(height_map_file != NULL) {
		load_height_map(height_map_file);
	} else {
		create_random_height_map();
	}

	// Populate world with blocks
	fprintf(stderr, "Generating blocks\n");
	world = (struct block *) malloc(sizeof(struct block) * WORLD_SIZE_XYZ);
	for(int x = 0; x < WORLD_SIZE_X; x++) {
		for(int z = 0; z < WORLD_SIZE_Z; z++) {
			int height = get_height(x, z);

			for(int y = 0; y < WORLD_SIZE_Y; y++) {
				struct block *current = get_block(x, y, z);

				if(y < height) {
					current->type = TYPE_STONE;
					current->color.r = (64 + random() % 16) / 256.0f;
					current->color.g = (64 + random() % 16) / 256.0f;
					current->color.b = (64 + random() % 16) / 256.0f;
				} else {
					current->type = TYPE_AIR;
				}
			}
		}
	}

	// Calculate occlusion values
	fprintf(stderr, "Calculating occlusion\n");
	calculate_occlusion();

	// Create VBO
	fprintf(stderr, "Creating vertex buffer\n");
	glGenBuffers(1, &resources.vertex_buffer_handle);
	fill_vertex_buffer();
	fprintf(stderr, "Filled vertex buffer with %u vertices (%f MB)\n", vertex_amount, (sizeof(struct vertex) * vertex_amount) / (float)(1024 * 1024));

	// Create shaders
	resources.vertex_shader = make_shader(GL_VERTEX_SHADER, vertex_shader_file);
	if(!resources.vertex_shader) {
		exit(1);
	}
	resources.fragment_shader = make_shader(GL_FRAGMENT_SHADER, fragment_shader_file);
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
	resources.attributes.occlusion = glGetAttribLocation(resources.program, "occlusion");

	// Set camera position and target
	camera_position.x = WORLD_SIZE_X * 0.0f;
	camera_position.y = WORLD_SIZE_Y * 1.2f;
	camera_position.z = WORLD_SIZE_Z * 1.2f;
	camera_target.x = WORLD_SIZE_X * 0.5f;
	camera_target.y = WORLD_SIZE_Y * 0.5f;
	camera_target.z = WORLD_SIZE_Z * 0.5f;
}

void world_tick(int delta) {
	ticks += delta;

	// Move camera position
	camera_position.x = (GLfloat) sin(ticks / 1500.0f) * WORLD_SIZE_X * 1.2f + WORLD_SIZE_X * 0.5f;
	camera_position.y = (GLfloat) cos(ticks / 3500.0f) * WORLD_SIZE_Y * 1.2f + WORLD_SIZE_Y * 0.5f;
	camera_position.z = (GLfloat) cos(ticks / 1500.0f) * WORLD_SIZE_Z * 1.2f + WORLD_SIZE_Z * 0.5f;
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
	glVertexAttribPointer((GLuint) resources.attributes.occlusion, 1, GL_FLOAT, GL_FALSE, sizeof(struct vertex), BUFFER_OFFSET(sizeof(struct vec3) * 2 + sizeof(struct color)));
	glEnableVertexAttribArray((GLuint) resources.attributes.occlusion);

	glPushMatrix();

	// Position camera
	gluLookAt(camera_position.x, camera_position.y, camera_position.z, camera_target.x, camera_target.y, camera_target.z, 0.0f, 1.0f, 0.0f);

	// Draw quads, starting at offset 0, and specify the amount
	glDrawArrays(GL_QUADS, 0, (GLsizei) vertex_amount);

	glPopMatrix();

	// Clean up
	glDisableVertexAttribArray((GLuint) resources.attributes.position);
	glDisableVertexAttribArray((GLuint) resources.attributes.normal);
	glDisableVertexAttribArray((GLuint) resources.attributes.color);
	glDisableVertexAttribArray((GLuint) resources.attributes.occlusion);
}
