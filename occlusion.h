#ifndef _OCCLUSION_H
#define _OCCLUSION_H

#include "world.h"

#define RAY_AMOUNT 128
#define OFFSET_AMOUNT 1024

struct ray {
	float x;
	float y;
	float z;
	struct directions colliding;
};

struct offset {
	int x;
	int y;
	int z;
	//float depth;
};

void calculate_occlusion(void);

#endif /* !defined _OCCLUSION_H */
