

#include <gs/gs.h>


float glsl_mod(float a, float b) {
    return a - b * floor(a/b);
}

float fSphere(gs_vec3 p, float r) {
	return gs_vec3_len(p) - r;
}

// Plane with normal n (n is normalized) at some distance from the origin
float fPlane(gs_vec3 p, gs_vec3 n, float distanceFromOrigin) {
	return gs_vec3_dot(p, n) + distanceFromOrigin;
}

// Repeat only a few times: from indices <start> to <stop> (similar to above, but more flexible)
float pModInterval1(float p, float size, float start, float stop) {
	float halfsize = size*0.5;
	float c = floor((p + halfsize)/size);
	p = glsl_mod((p+halfsize), size) - halfsize;
	if (c > stop) { //yes, this might not be the best thing numerically.
		p += size*(c - stop);
		c = stop;
	}
	if (c <start) {
		p += size*(c - start);
		c = start;
	}
	return p;
}

float fOpUnion(float a, float b) {
    if (a < b) return a;
    return b;
}

// The "Round" variant uses a quarter-circle to join the two objects smoothly:
float fOpUnionRound(float a, float b, float r) {
    gs_vec2 u = gs_v2(r-a, r-b);
    if (u.x < 0) u.x = 0;
    if (u.y < 0) u.y = 0;
	return gs_max(r, gs_min(a, b)) - gs_vec2_len(u);
}