#include <stdlib.h>
#include <GL/glew.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <assert.h>

#include "occlusion.h"

static void dump_ray(struct ray *ray) {
	fprintf(stderr, "Dumping ray:\n");
	fprintf(stderr, "\tx = %f\n", ray->x);
	fprintf(stderr, "\ty = %f\n", ray->y);
	fprintf(stderr, "\tz = %f\n", ray->z);
}

static void dump_directions(struct directions *directions) {
	fprintf(stderr, "Dumping directions:\n");
	fprintf(stderr, "\tright = %f\n", directions->right);
	fprintf(stderr, "\tleft = %f\n", directions->left);
	fprintf(stderr, "\tup = %f\n", directions->up);
	fprintf(stderr, "\tdown = %f\n", directions->down);
	fprintf(stderr, "\tfront = %f\n", directions->front);
	fprintf(stderr, "\tback = %f\n", directions->back);
}

static struct ray *generate_rays(int amount) {
	struct ray *rays = (struct ray *) malloc(sizeof(struct ray) * (unsigned int) amount);
	assert(rays != NULL);

	double inc = M_PI * (3 - sqrt(5));
	double off = 2 / (double) amount;

	for(int i = 0; i < amount; i++) {
		double y = i * off - 1 + (off / 2);
		double r = sqrt(1 - y * y);
		double phi = i * inc;
		rays[i].x = (float) (cos(phi) * r);
		rays[i].y = (float) y;
		rays[i].z = (float) (sin(phi) * r);
		// Determine from which faces the ray 'escapes'
		rays[i].colliding.right = (rays[i].x < 0) ? -rays[i].x : 0.0f;
		rays[i].colliding.left = (rays[i].x > 0) ? rays[i].x : 0.0f;
		rays[i].colliding.up = (rays[i].y < 0) ? -rays[i].y : 0.0f;
		rays[i].colliding.down = (rays[i].y > 0) ? rays[i].y : 0.0f;
		rays[i].colliding.front = (rays[i].z < 0) ? -rays[i].z : 0.0f;
		rays[i].colliding.back = (rays[i].z > 0) ? rays[i].z : 0.0f;
	}

	return rays;
}

static struct offset *generate_intersecting_offsets(struct ray *ray, int amount) {
	struct offset *offsets = (struct offset *) malloc(sizeof(struct offset) * (unsigned int) amount);
	assert(offsets != NULL);

	// Ray has unit length; scale to make steps of 0.2
	float sx = ray->x * 0.2f;
	float sy = ray->y * 0.2f;
	float sz = ray->z * 0.2f;

	// The current position
	float x = 0;
	float y = 0;
	float z = 0;

	// The cell a point was last added for
	int last_x = 0;
	int last_y = 0;
	int last_z = 0;

	int i = 0;
	while(i < amount) {
		// Round towards zero to get the block coordinate
		int new_x = (int) x;
		int new_y = (int) y;
		int new_z = (int) z;
		if(new_x != last_x || new_y != last_y || new_z != last_z) {
			// Arrived in a new block; add to offsets
			offsets[i].x = new_x;
			offsets[i].y = new_y;
			offsets[i].z = new_z;
			last_x = new_x;
			last_y = new_y;
			last_z = new_z;
			i++;
#if 0
			// Render the offsets (starting at the center of the map) as red blocks
			struct block *tmp = get_block(new_x + WORLD_SIZE_X / 2, new_y + WORLD_SIZE_Y / 2, new_z + WORLD_SIZE_Z / 2);
			if(tmp != NULL) {
				tmp->type = TYPE_STONE;
				tmp->color.r = 1.0f;
				tmp->color.g = 0.0f;
				tmp->color.b = 0.0f;
			}
#endif
		}
		x += sx;
		y += sy;
		z += sz;
	}

	return offsets;
}

static struct directions calculate_face_totals(struct ray *rays) {
	struct directions totals = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	for(int i = 0; i < RAY_AMOUNT; i++) {
		totals.right += rays[i].colliding.right;
		totals.left += rays[i].colliding.left;
		totals.up += rays[i].colliding.up;
		totals.down += rays[i].colliding.down;
		totals.front += rays[i].colliding.front;
		totals.back += rays[i].colliding.back;
	}

	float epsilon = 0.000001f;
	if(
		(totals.right > -epsilon && totals.right < epsilon) ||
		(totals.left > -epsilon && totals.left < epsilon) ||
		(totals.up > -epsilon && totals.up < epsilon) ||
		(totals.down > -epsilon && totals.down < epsilon) ||
		(totals.front > -epsilon && totals.front < epsilon) ||
		(totals.back > -epsilon && totals.back < epsilon)
	) {
		fprintf(stderr, "Error: not enough light colliding with one of the faces\n");
		dump_directions(&totals);
		for(int i = 0; i < RAY_AMOUNT; i++) {
			dump_ray(&rays[i]);
			dump_directions(&(rays[i].colliding));
		}
		exit(1);
	}

	return totals;
}

void calculate_occlusion() {
	fprintf(stderr, "Generating %d rays\n", RAY_AMOUNT);
	struct ray *rays = generate_rays(RAY_AMOUNT);
	struct directions face_totals = calculate_face_totals(rays);

	fprintf(stderr, "Generating %d ray offsets per ray (%d total)\n", OFFSET_AMOUNT, RAY_AMOUNT * OFFSET_AMOUNT);
	struct offset *ray_offsets[RAY_AMOUNT];
	for(int i = 0; i < RAY_AMOUNT; i++) {
		ray_offsets[i] = generate_intersecting_offsets(rays + i, OFFSET_AMOUNT);
	}

	// Calculate occlusion per face
	fprintf(stderr, "Calculating face occlusion\n");
	for(int x = 0; x < WORLD_SIZE_X; x++) {
		for(int y = 0; y < WORLD_SIZE_Y; y++) {
			for(int z = 0; z < WORLD_SIZE_Z; z++) {
				// Only calculate occlusion for AIR blocks with neighbors
				struct block *block = get_block(x, y, z);
				if(block->type != TYPE_AIR || !has_neighbors(x, y, z)) {
					continue;
				}

				block->occlusion.right = 0;
				block->occlusion.left = 0;
				block->occlusion.up = 0;
				block->occlusion.down = 0;
				block->occlusion.front = 0;
				block->occlusion.back = 0;

				int escaped = 0;
				for(int i = 0; i < RAY_AMOUNT; i++) {
					struct ray *ray = rays + i;

					bool collided = false;
					for(int j = 0; j < OFFSET_AMOUNT; j++) {
						int rx = x + ray_offsets[i][j].x;
						int ry = y + ray_offsets[i][j].y;
						int rz = z + ray_offsets[i][j].z;

						if(rx < 0 || rx >= WORLD_SIZE_X || ry < 0 || ry >= WORLD_SIZE_Y || rz < 0 || rz >= WORLD_SIZE_Z) {
							// Ray has escaped the world
							break;
						}
						if(get_block(rx, ry, rz)->type != TYPE_AIR) {
							collided = true;
							break;
						}
					}
					if(!collided) {
						// Add light from escaped ray to the block face it would collide with
						block->occlusion.right += ray->colliding.right;
						block->occlusion.left += ray->colliding.left;
						block->occlusion.up += ray->colliding.up;
						block->occlusion.down += ray->colliding.down;
						block->occlusion.front += ray->colliding.front;
						block->occlusion.back += ray->colliding.back;

						escaped++;
					}
				}
				//fprintf(stderr, "%4d/%d rays escaped from (%d,%d,%d) = %d%%\n", escaped, RAY_AMOUNT, x, y, z, (int) (escaped * 100 / RAY_AMOUNT));

				// Normalize occlusion values
				float epsilon = 0.0000001f;
				block->occlusion.right	= 1 - (block->occlusion.right / face_totals.right);
				block->occlusion.left	= 1 - (block->occlusion.left / face_totals.left);
				block->occlusion.up		= 1 - (block->occlusion.up / face_totals.up);
				block->occlusion.down	= 1 - (block->occlusion.down / face_totals.down);
				block->occlusion.front	= 1 - (block->occlusion.front / face_totals.front);
				block->occlusion.back	= 1 - (block->occlusion.back / face_totals.back);
			}
		}
	}

	for(int i = 0; i < RAY_AMOUNT; i++) {
		free(ray_offsets[i]);
	}
	free(rays);
}
