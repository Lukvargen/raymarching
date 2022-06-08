#ifndef GAME_DATA_H
#define GAME_DATA_H

#include <gs/gs.h>
#include <gs/util/gs_idraw.h>

#include "verlet_object.h"

#define SPHERES_COUNT 10

typedef struct camera_params_t
{
    gs_vec4 pos;
    gs_mat4 rot_matrix;
} camera_params_t;

typedef struct fps_camera_t {
    float pitch;
    gs_camera_t cam;
} fps_camera_t;

typedef struct game_data_t 
{
	gs_command_buffer_t gcb;
	gs_immediate_draw_t gsi;

	fps_camera_t fps_camera;

	gs_handle(gs_graphics_vertex_buffer_t) quad_vbo;
	gs_handle(gs_graphics_index_buffer_t) quad_ibo;
	gs_handle(gs_graphics_pipeline_t) raymarch_pip;
	gs_handle(gs_graphics_shader_t) raymarch_shader;
	gs_handle(gs_graphics_uniform_t)  raymarch_u_camera;
	gs_handle(gs_graphics_uniform_t) raymarch_u_viewport;
	gs_handle(gs_graphics_uniform_t) raymarch_u_texture1;
	gs_handle(gs_graphics_uniform_t) raymarch_u_texture2;
	gs_handle(gs_graphics_uniform_t) raymarch_u_texture3;
	gs_handle(gs_graphics_uniform_t) raymarch_u_bumpmapGS;
    gs_handle(gs_graphics_storage_buffer_t)  raymarch_u_spheres_buffer;

	gs_handle(gs_graphics_uniform_t) raymarch_u_res;
	gs_handle(gs_graphics_uniform_t) raymarch_u_mouse;
	gs_handle(gs_graphics_uniform_t) raymarch_u_time;

	gs_asset_texture_t	texture1;
	gs_asset_texture_t	texture2;
	gs_asset_texture_t	texture3;
	gs_asset_texture_t	bumpmapGS;


	
	// framebuffer
	gs_handle(gs_graphics_renderpass_t) rp;
	gs_handle(gs_graphics_framebuffer_t) fbo;
	gs_handle(gs_graphics_texture_t)	rt;


    gs_dyn_array(verlet_object_t) verlet_objects;
    gs_vec4 gpu_spheres[SPHERES_COUNT];

} game_data_t;

#endif