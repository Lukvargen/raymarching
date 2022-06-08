

#include <gs/gs.h>

// Plane with normal n (n is normalized) at some distance from the origin
float fPlane(gs_vec3 p, gs_vec3 n, float distanceFromOrigin) {
	return gs_vec3_dot(p, n) + distanceFromOrigin;
}