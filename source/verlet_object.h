#ifndef VERLET_OBJECT
#define VERLET_OBJECT

#include <gs/gs.h>

typedef struct verlet_object_t
{
	gs_vec3 position_current;
	gs_vec3 position_old;
	gs_vec3 acceleration;
	gs_color_t color;
	float radius;

} verlet_object_t;


#endif