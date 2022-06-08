

#include "physics.h"

#include "game_data.h"
#include "verlet_object.h"

#include "sdf.h"




float map(gs_vec3 p)
{
    float time = (float)(gs_platform_elapsed_time() / 1000.0); 
    //time = 0;
	// plane
	float plane_dist = fPlane(p, gs_v3(0, 1, 0), 14.0);
	float planeID = 7.0;

    gs_vec3 sp = p;
	sp.x = pModInterval1(sp.x, 45.0, 1, 30);
	sp.z = pModInterval1(sp.z, 45.0, -30, 30);

	gs_vec3 sp1 = sp;
	sp1.y += sin(time*0.5) * 20;
	sp1.x += cos(time) * 4;
	sp1.z += sin(time) * 4;
	float sphere_dist = fSphere(sp1, 9.0);
	float sphereID = 7.0;

	gs_vec3 sp2 = sp;
	sp2.y += sin(time) * 30;
	float sphere_dist2 = fSphere(sp2, 9.0);
	float sphereID2 = 7.0;


    p.x = pModInterval1(p.x, 45.0, 1, 30);
    float test_sphere_dist = fSphere(p, 100.0);

	float res;
    res = fOpUnionRound(sphere_dist, sphere_dist2, 5.0);
    res = fOpUnionRound(res, plane_dist, 5.0);
    res = fOpUnionRound(res, test_sphere_dist, 1.0);
    //res = fOpUnion(res, plane_dist);

	return res;
	

}


void verlet_update_position(verlet_object_t* verlet_object, float dt)
{
	gs_vec3 velocity = gs_vec3_sub(verlet_object->position_current, verlet_object->position_old);
	verlet_object->position_old = verlet_object->position_current;

	for (int i = 0; i < 3; i++) {
		verlet_object->position_current.xyz[i] = verlet_object->position_current.xyz[i] + velocity.xyz[i] + verlet_object->acceleration.xyz[i] * dt * dt;
	}
	verlet_object->acceleration = (gs_vec3){0};

}

void verlet_accelerate(verlet_object_t* verlet_object, gs_vec3 acc)
{
	verlet_object->acceleration = gs_vec3_add(verlet_object->acceleration, acc);
}


void verlet_solve_collisions(game_data_t* gd)
{
	
	int objects_size = gs_dyn_array_size(gd->verlet_objects);
	for (int i = 0; i < objects_size; i++) {
		verlet_object_t* o1 = &gd->verlet_objects[i];

		for (int j = i+1; j < objects_size; j++) {
			verlet_object_t* o2 = &gd->verlet_objects[j];
			gs_vec3 collision_axis = gs_vec3_sub(o1->position_current, o2->position_current);
			float dist = gs_vec3_len(collision_axis);
			float min_distance = o1->radius+o2->radius;
			if (dist < min_distance) {
				gs_vec3 n;
				n.x = collision_axis.x / dist;
				n.y = collision_axis.y / dist;
				float delta = min_distance - dist;
				o1->position_current = gs_vec3_add(o1->position_current, gs_vec3_scale(n, 0.5 * delta));
				o2->position_current = gs_vec3_sub(o2->position_current, gs_vec3_scale(n, 0.5 * delta));
			}
		}
	}
}


gs_vec3 get_normal(gs_vec3 p)
{
   float EPSILON = 0.001;
    // vec3(map(p).x) - vec3(map(p - e.xyy).x, map(p - e.yxy).x, map(p - e.yyx).x);
    //gs_vec3 n;
    //n.x = map(p) - 
    float map_p = map(p);
    gs_vec3 n = gs_vec3_sub(gs_v3(map_p,map_p,map_p), gs_v3(map(gs_vec3_sub(p, gs_v3(EPSILON, 0, 0))), map(gs_vec3_sub(p, gs_v3(0, EPSILON, 0))), map(gs_vec3_sub(p, gs_v3(0, 0, EPSILON)))));

    //vec3 n = vec3(map(p).x) - vec3(map(p - e.xyy).x, map(p - e.yxy).x, map(p - e.yyx).x);
   return gs_vec3_norm(n);
}

void sim_update(game_data_t* gd, float dt)
{
	gs_vec3 gravity = {0.f, -250.f, 0.f};
	for (int i = 0; i < gs_dyn_array_size(gd->verlet_objects); i++) {
		verlet_accelerate(&gd->verlet_objects[i], gravity);
	}

	// apply constraints
    /*
	gs_vec2 circle_pos = gs_v2(320, 320);
	float circle_radius = 200;
	for (int i = 0; i < gs_dyn_array_size(gd->verlet_objects); i++) {
		verlet_object_t* v_o = &gd->verlet_objects[i];
		gs_vec3 to_circle = gs_vec3_sub(v_o->position_current, circle_pos);
		float dist = gs_vec3_len(to_circle);
		if (dist > circle_radius - v_o->radius) {
			gs_vec2 n = to_circle;
			n.x /= dist;
			n.y /= dist;
			v_o->position_current = gs_vec2_add(circle_pos, gs_vec2_scale(n, circle_radius - v_o->radius));
		}
	}
    */

    for (int i = 0; i < gs_dyn_array_size(gd->verlet_objects); i++) {
        
        verlet_object_t* v_o = &gd->verlet_objects[i];
        float dist = map(v_o->position_current);

        float diff = dist- v_o->radius;

        if (diff < 0) {
            gs_vec3 n = get_normal(v_o->position_current); //gs_v3(0, 1, 0);
            v_o->position_current = gs_vec3_sub(v_o->position_current, gs_vec3_scale(n, diff));
        }
    }




	//CLOCK_START(SOLVE_COLLISIONS);
	verlet_solve_collisions(gd);
	//CLOCK_END(SOLVE_COLLISIONS);

	for (int i = 0; i < gs_dyn_array_size(gd->verlet_objects); i++) {
		verlet_update_position(&gd->verlet_objects[i], dt);
	}


}


