#include <stdlib.h>
#include <GL/glew.h>
#include <math.h>
#include <stdbool.h>
#include <stdio.h>

#include "occlusion.h"

static struct ray *generate_rays(int amount) {
	struct ray *rays = (struct ray *) malloc(sizeof(struct ray) * (unsigned int) amount);

	double inc = M_PI * (3 - sqrt(5));
	double off = 2 / (double) amount;

	for(int i = 0; i < amount; i++) {
		double y = i * off - 1 + (off / 2);
		double r = sqrt(1 - y * y);
		double phi = i * inc;
		rays[i].x = (float) (cos(phi) * r);
		rays[i].y = (float) y;
		rays[i].z = (float) (sin(phi) * r);
	}

	return rays;
}

static struct offset *generate_intersecting_offsets(struct ray *ray, int amount) {
	struct offset *offsets = (struct offset *) malloc(sizeof(struct offset) * (unsigned int) amount);

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
		}
		x += sx;
		y += sy;
		z += sz;
	}

	return offsets;
}

static struct directions normalize_rays(struct ray *rays) {
	struct directions normal = {0, 0, 0, 0, 0, 0};
	for(int i = 0; i < RAY_AMOUNT; i++) {
		struct ray *ray = rays + i;
		normal.left += ray->colliding.left;
		normal.right += ray->colliding.right;
		normal.up += ray->colliding.up;
		normal.down += ray->colliding.down;
		normal.front += ray->colliding.front;
		normal.back += ray->colliding.back;
	}
	return normal;
}

void calculate_occlusion(struct block *world) {
	printf("Generating rays\n");
	struct ray *rays = generate_rays(RAY_AMOUNT);

	printf("Normalizing rays\n");
	struct directions normal = normalize_rays(rays);

	printf("Generating ray offsets\n");
	struct offset *ray_offsets[RAY_AMOUNT];
	for(int i = 0; i < RAY_AMOUNT; i++) {
		struct ray *ray = rays + i;
		ray_offsets[i] = generate_intersecting_offsets(ray, OFFSET_AMOUNT);
	}

	// Calculate occlusion per face
	printf("Calculating face occlusion\n");
	for(int x = 0; x < WORLD_SIZE_X; x++) {
		for(int y = 0; y < WORLD_SIZE_Y; y++) {
			for(int z = 0; z < WORLD_SIZE_Z; z++) {
				struct block *current = SELECT_BLOCK_P(x, y, z);
				current->occlusion.left = 0;
				current->occlusion.right = 0;
				current->occlusion.up = 0;
				current->occlusion.down = 0;
				current->occlusion.front = 0;
				current->occlusion.back = 0;
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
						if(SELECT_BLOCK(rx, ry, rz).type != TYPE_AIR) {
							collided = true;
							break;
						}
					}
					if(!collided) {
						// Add light from escaped ray to current block
						current->occlusion.left += ray->colliding.left;
						current->occlusion.right += ray->colliding.right;
						current->occlusion.up += ray->colliding.up;
						current->occlusion.down += ray->colliding.down;
						current->occlusion.front += ray->colliding.front;
						current->occlusion.back += ray->colliding.back;
					}
				}
				// Normalize current block
				current->occlusion.left = 1 - (current->occlusion.left / normal.left);
				current->occlusion.right = 1 - (current->occlusion.right / normal.right);
				current->occlusion.up = 1 - (current->occlusion.up / normal.up);
				current->occlusion.down = 1 - (current->occlusion.down / normal.down);
				current->occlusion.front = 1 - (current->occlusion.front / normal.front);
				current->occlusion.back = 1 - (current->occlusion.back / normal.back);
				//printf("normalized = (%f, %f, %f, %f, %f, %f)\n", current->occlusion.left, current->occlusion.right, current->occlusion.up, current->occlusion.down, current->occlusion.front, current->occlusion.back);
			}
		}
	}

	for(int i = 0; i < RAY_AMOUNT; i++) {
		free(ray_offsets[i]);
	}
	free(rays);
}
