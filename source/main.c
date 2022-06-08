#define GS_IMPL
#include <gs/gs.h>

#define GS_IMMEDIATE_DRAW_IMPL
#include <gs/util/gs_idraw.h>


#define GS_GUI_IMPL
#include <gs/util/gs_gui.h>

#include "game_data.h"

#include "sdf.h"

#include "physics.h"


#define CLOCK_START(X) clock_t X = clock()
#define CLOCK_END(X) gs_println(#X" %f", (double)(clock()-X))

#define CLOCK_FUNC(func) do { \
	clock_t _start = clock();\
	func;\
	gs_println(#func" %f", (double)(clock()-_start));\
} while(0)


#define RES_WIDTH 1920 / 1
#define RES_HEIGHT 1080 / 1




void init_frame_buffer(game_data_t* gd);
void init_raymarch(game_data_t* gd);
void draw_raymarch(game_data_t* gd, gs_command_buffer_t* gcb, gs_vec4 viewport);
void camera_update(game_data_t* gd, float delta);

void draw_game(game_data_t* gd)
{
	gs_command_buffer_t* gcb = &gd->gcb;

	gs_vec2 ws = gs_platform_framebuffer_sizev(gs_platform_main_window());

    gsi_camera2D(&gd->gsi, (uint32_t)ws.x, (uint32_t)ws.y);

	gs_vec2 circle_pos = gs_v2(320, 320);
	float circle_radius = 200;
	//gsi_circle(&gd->gsi, circle_pos.x, circle_pos.y, circle_radius, 64, 10, 10, 10, 255, GS_GRAPHICS_PRIMITIVE_TRIANGLES);


	gs_graphics_renderpass_begin(gcb, gd->rp);
		
		gs_graphics_clear_desc_t clear = (gs_graphics_clear_desc_t) {
		.actions = &(gs_graphics_clear_action_t){.color = {0.05f, 0.05f, 0.05f, 1.f}}
		};
		gs_graphics_clear(gcb, &clear);

		//gs_graphics_set_viewport(gcb, 0, 0, ws.x, ws.y); // where to draw
		gs_vec2 center = gs_vec2_scale(ws, 0.5);
		gs_vec4 viewport = {0, 0, RES_WIDTH, RES_HEIGHT};
		gs_graphics_set_viewport(gcb, 0, 0, RES_WIDTH, RES_HEIGHT);

		draw_raymarch(gd, gcb, viewport);

	gs_graphics_renderpass_end(gcb);

	// render to backbuffer
	gs_graphics_renderpass_begin(gcb, GS_GRAPHICS_RENDER_PASS_DEFAULT);
		gs_graphics_set_viewport(gcb, 0, 0, ws.x, ws.y); // where to draw
		gs_graphics_clear(gcb, &clear);


		gsi_texture(&gd->gsi, gd->rt);
		gsi_ortho(&gd->gsi, 0, RES_WIDTH, RES_HEIGHT, 0, 0, 10);
		gsi_rectvd(&gd->gsi, gs_v2(0.f, 0.f), gs_v2((float)RES_WIDTH, (float)RES_HEIGHT), gs_v2(0.f, 1.f), gs_v2(1.f, 0.f), GS_COLOR_WHITE, GS_GRAPHICS_PRIMITIVE_TRIANGLES);
		
		gsi_draw(&gd->gsi, &gd->gcb);


	gs_graphics_renderpass_end(gcb);
	/*
	gs_graphics_renderpass_begin(gcb, GS_GRAPHICS_RENDER_PASS_DEFAULT);
		
		gs_graphics_clear_desc_t clear = (gs_graphics_clear_desc_t) {
		.actions = &(gs_graphics_clear_action_t){.color = {0.05f, 0.05f, 0.05f, 1.f}}
		};
		gs_graphics_clear(gcb, &clear);

		//gs_graphics_set_viewport(gcb, 0, 0, ws.x, ws.y); // where to draw
		gs_vec2 center = gs_vec2_scale(ws, 0.5);
		gs_vec4 viewport = {center.x- RES_WIDTH/2, center.y-RES_HEIGHT/2, RES_WIDTH, RES_HEIGHT};
		gs_graphics_set_viewport(gcb, viewport.x, viewport.y, viewport.z, viewport.w);
		draw_raymarch(gd, gcb, viewport);

	gs_graphics_renderpass_end(gcb);*/



	//gsi_renderpass_submit(&gd->gsi, &gd->gcb, ws.x, ws.y, gs_color(100, 100, 100, 255));
	gs_graphics_command_buffer_submit(&gd->gcb);
}


void init() 
{
	game_data_t* gd = gs_user_data(game_data_t);

	gd->gcb = gs_command_buffer_new();
	gd->gsi = gs_immediate_draw_new();





	gd->fps_camera.cam = gs_camera_perspective();
    gd->fps_camera.cam.transform.position = gs_v3(0.0, 8.0, 0.0);

	init_frame_buffer(gd);
	init_raymarch(gd);

}


void update()
{
	gs_vec2 ws = gs_platform_window_sizev(gs_platform_main_window());
	game_data_t* gd = gs_user_data(game_data_t);
	float delta = gs_platform_delta_time();

	if (gs_platform_key_pressed(GS_KEYCODE_ESC)) {
		gs_quit();
	}

	if (gs_platform_key_pressed(GS_KEYCODE_F)) {
		bool32_t locked = gs_platform_mouse_locked();
		gs_platform_lock_mouse(gs_platform_main_window(), !locked);
	}

	camera_update(gd, delta);

	sim_update(gd, delta);
	gs_graphics_storage_buffer_update(gd->raymarch_u_spheres_buffer, &(gs_graphics_storage_buffer_desc_t) {
        .data = gd->gpu_spheres,
        .size = sizeof(gd->gpu_spheres),
        .usage = GS_GRAPHICS_BUFFER_USAGE_DYNAMIC,
        
        .update = {
            .type = GS_GRAPHICS_BUFFER_UPDATE_SUBDATA,
            .offset = 0
        },
    });

	draw_game(gd);
}

// Globals
game_data_t gdata = {0};

gs_app_desc_t gs_main(int32_t argc, char** argv)
{
	double width = RES_WIDTH;
	double height = RES_HEIGHT;
#ifdef GS_PLATFORM_WEB
	emscripten_get_element_css_size("#canvas", &width, &height);
#endif
	return (gs_app_desc_t) {
		.window_width = width,
		.window_height = height,
		.init = init,
		.update = update,
		.window_title = "RayMarching",
		.frame_rate = 120.f,
		.user_data = &gdata,
		.enable_vsync = true
	};
}



void init_frame_buffer(game_data_t* gd)
{
	gd->fbo = gs_graphics_framebuffer_create(NULL);
	
	// construct color render target
	gd->rt = gs_graphics_texture_create(
		&(gs_graphics_texture_desc_t) {
			.width = RES_WIDTH,
			.height = RES_HEIGHT,
			.format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8,
			.wrap_s = GS_GRAPHICS_TEXTURE_WRAP_CLAMP_TO_EDGE,
			.wrap_t = GS_GRAPHICS_TEXTURE_WRAP_CLAMP_TO_EDGE,
			.min_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
			.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
			//.render_target = true
		}
	);

	// construct render pass
	gd->rp = gs_graphics_renderpass_create(
		&(gs_graphics_renderpass_desc_t) {
			.fbo = gd->fbo,
			.color = &gd->rt, // color buffer array to bind to frame buffer
			.color_size = sizeof(gd->rt)
		}
	);
}

// screen
void init_raymarch(game_data_t* gd)
{
	float screen_v_data[] = {
    -1.0f, -1.0f,  0.0f, 0.0f,  // Top Left
     1.0f, -1.0f,  1.0f, 0.0f,  // Top Right 
    -1.0f,  1.0f,  0.0f, 1.0f,  // Bottom Left
     1.0f,  1.0f,  1.0f, 1.0f   // Bottom Right
	};

	int screen_i_data[] = {
		0, 3, 2,
		0, 1, 3
	};

	gd->quad_vbo = gs_graphics_vertex_buffer_create(
		&(gs_graphics_vertex_buffer_desc_t) {
			.data = screen_v_data,
			.size = sizeof(screen_v_data)
		}
	);
	gd->quad_ibo = gs_graphics_index_buffer_create(
		&(gs_graphics_index_buffer_desc_t) {
			.data = screen_i_data,
			.size = sizeof(screen_i_data)
		}
	);
	
	char* vertex_shader = gs_read_file_contents_into_string_null_term("source/raymarchShaderVertex.glsl", "rb", NULL);
	char* hg_sdf_shader = gs_read_file_contents_into_string_null_term("source/hg_sdf.glsl", "rb", NULL);
	char* fragment_shader = gs_read_file_contents_into_string_null_term("source/raymarchShaderFragment.glsl", "rb", NULL);
	char* replace_pos = strstr(fragment_shader, "HG_SDF");

	gd->raymarch_shader = gs_graphics_shader_create(
		&(gs_graphics_shader_desc_t) {
			.sources = (gs_graphics_shader_source_desc_t[]) {
				{.type = GS_GRAPHICS_SHADER_STAGE_VERTEX, .source = vertex_shader},
				{.type = GS_GRAPHICS_SHADER_STAGE_FRAGMENT, .source = fragment_shader},
			},
			.size = 2 * sizeof(gs_graphics_shader_source_desc_t),
			.name = "raymarching_shader"
		}
	);

	gd->raymarch_u_camera = gs_graphics_uniform_create (
		&(gs_graphics_uniform_desc_t) {
			.name = "u_camera",                                                         
			.layout = (gs_graphics_uniform_layout_desc_t[]){
				{.type = GS_GRAPHICS_UNIFORM_VEC4, .fname = ".pos"},
				{.type = GS_GRAPHICS_UNIFORM_MAT4, .fname = ".rot"},
			},
			.layout_size = 2 * sizeof(gs_graphics_uniform_layout_desc_t)
		}
	);

	gd->raymarch_u_viewport = gs_graphics_uniform_create(&(gs_graphics_uniform_desc_t){
		.name = "u_viewport",
		.layout = &(gs_graphics_uniform_layout_desc_t){.type = GS_GRAPHICS_UNIFORM_VEC4}
	});
	gd->raymarch_u_texture1 = gs_graphics_uniform_create(&(gs_graphics_uniform_desc_t){
		.name = "u_texture1",
		.layout = &(gs_graphics_uniform_layout_desc_t){.type = GS_GRAPHICS_UNIFORM_SAMPLER2D}
	});
	gd->raymarch_u_texture2 = gs_graphics_uniform_create(&(gs_graphics_uniform_desc_t){
		.name = "u_texture2",
		.layout = &(gs_graphics_uniform_layout_desc_t){.type = GS_GRAPHICS_UNIFORM_SAMPLER2D}
	});
	gd->raymarch_u_texture3 = gs_graphics_uniform_create(&(gs_graphics_uniform_desc_t){
		.name = "u_texture3",
		.layout = &(gs_graphics_uniform_layout_desc_t){.type = GS_GRAPHICS_UNIFORM_SAMPLER2D}
	});
	gd->raymarch_u_bumpmapGS = gs_graphics_uniform_create(&(gs_graphics_uniform_desc_t){
		.name = "u_bumpmapGS",
		.layout = &(gs_graphics_uniform_layout_desc_t){.type = GS_GRAPHICS_UNIFORM_SAMPLER2D}
	});

	gd->raymarch_u_spheres_buffer = gs_graphics_storage_buffer_create (&(gs_graphics_storage_buffer_desc_t){
		.name = "u_spheres_buffer",
		.data = NULL,
		.size = sizeof(gd->gpu_spheres),
		.usage = GS_GRAPHICS_BUFFER_USAGE_DYNAMIC,
		.access = GS_GRAPHICS_ACCESS_READ_ONLY
	});

	for (int i=0; i < SPHERES_COUNT; i++) {
		gd->gpu_spheres[i] = gs_v4(i*2, i*2, i*2, 4.0);
	}
	gs_graphics_storage_buffer_update(gd->raymarch_u_spheres_buffer, &(gs_graphics_storage_buffer_desc_t) {
        .data = gd->gpu_spheres,
        .size = sizeof(gd->gpu_spheres),
        .usage = GS_GRAPHICS_BUFFER_USAGE_DYNAMIC,
        
        .update = {
            .type = GS_GRAPHICS_BUFFER_UPDATE_SUBDATA,
            .offset = 0
        },
    });


	gd->raymarch_u_res = gs_graphics_uniform_create(&(gs_graphics_uniform_desc_t){
		.name = "u_res",
		.layout = &(gs_graphics_uniform_layout_desc_t){.type = GS_GRAPHICS_UNIFORM_VEC2}
	});
	gd->raymarch_u_mouse = gs_graphics_uniform_create(&(gs_graphics_uniform_desc_t){
		.name = "u_mouse",
		.layout = &(gs_graphics_uniform_layout_desc_t){.type = GS_GRAPHICS_UNIFORM_VEC2}
	});
	gd->raymarch_u_time = gs_graphics_uniform_create(&(gs_graphics_uniform_desc_t){
		.name = "u_time",
		.layout = &(gs_graphics_uniform_layout_desc_t){.type = GS_GRAPHICS_UNIFORM_FLOAT}
	});

	gd->raymarch_pip = gs_graphics_pipeline_create (
        &(gs_graphics_pipeline_desc_t) {
            .raster = {
                .shader = gd->raymarch_shader,
                .index_buffer_element_size = sizeof(uint32_t) 
            },
            .layout = {
                .attrs = (gs_graphics_vertex_attribute_desc_t[]){
                    {.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_pos"},
					{.format = GS_GRAPHICS_VERTEX_ATTRIBUTE_FLOAT2, .name = "a_uv"}
                },
                .size = 2 * sizeof(gs_graphics_vertex_attribute_desc_t)
            }
        }
    );



	// load texture
	gs_graphics_texture_desc_t desc = (gs_graphics_texture_desc_t){
		.format = GS_GRAPHICS_TEXTURE_FORMAT_RGBA8,
		.min_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
		.mag_filter = GS_GRAPHICS_TEXTURE_FILTER_NEAREST,
		.wrap_s = GS_GRAPHICS_TEXTURE_WRAP_REPEAT,
		.wrap_t = GS_GRAPHICS_TEXTURE_WRAP_REPEAT
	};
	bool success = gs_asset_texture_load_from_file("./assets/pattern2.png", &gd->texture1, &desc, true, false);
	if (!success) {
		gs_println("failed to load texture pattern");
	}
	success = gs_asset_texture_load_from_file("./assets/repeating.png", &gd->texture2, &desc, true, false);
	if (!success) {
		gs_println("failed to load texture repeating");
	}
	success = gs_asset_texture_load_from_file("./assets/pattern1.png", &gd->texture3, &desc, true, false);
	if (!success) {
		gs_println("failed to load texture patterh1");
	}

	success = gs_asset_texture_load_from_file("./assets/Brick.jpg", &gd->bumpmapGS, &desc, true, false);
	if (!success) {
		gs_println("failed to load texture bumpmap");
	}


}

void draw_raymarch(game_data_t* gd, gs_command_buffer_t* gcb, gs_vec4 viewport)
{
	gs_vec2 ws = gs_v2(RES_WIDTH, RES_HEIGHT);//gs_platform_window_sizev(gs_platform_main_window());
	
	gs_vec2 m_pos = gs_platform_mouse_positionv();
	//gs_println("mpos %f %f", m_pos.x, m_pos.y);
	m_pos.x -= viewport.x;
	m_pos.y -= viewport.y;
	float time = (float)(gs_platform_elapsed_time() / 1000.0);

	static camera_params_t camera = {0};
	camera.pos.x = gd->fps_camera.cam.transform.position.x;
    camera.pos.y = gd->fps_camera.cam.transform.position.y;
    camera.pos.z = gd->fps_camera.cam.transform.position.z;
	camera.rot_matrix = gs_quat_to_mat4(gd->fps_camera.cam.transform.rotation);



	gs_graphics_bind_uniform_desc_t uniforms[] = {
		(gs_graphics_bind_uniform_desc_t){.uniform = gd->raymarch_u_texture1, .data = &gd->texture1, .binding = 0},
		(gs_graphics_bind_uniform_desc_t){.uniform = gd->raymarch_u_texture2, .data = &gd->texture2, .binding = 1},
		(gs_graphics_bind_uniform_desc_t){.uniform = gd->raymarch_u_texture3, .data = &gd->texture3, .binding = 2},
		(gs_graphics_bind_uniform_desc_t){.uniform = gd->raymarch_u_bumpmapGS, .data = &gd->bumpmapGS, .binding = 3},

		(gs_graphics_bind_uniform_desc_t){.uniform = gd->raymarch_u_camera, .data = &camera},
		(gs_graphics_bind_uniform_desc_t){.uniform = gd->raymarch_u_viewport, .data = &viewport},
		(gs_graphics_bind_uniform_desc_t){.uniform = gd->raymarch_u_res, .data = &ws},
		(gs_graphics_bind_uniform_desc_t){.uniform = gd->raymarch_u_mouse, .data = &m_pos},
		(gs_graphics_bind_uniform_desc_t){.uniform = gd->raymarch_u_time, .data = &time},
	};

	gs_graphics_bind_storage_buffer_desc_t storage_buffers[] = {
		(gs_graphics_bind_storage_buffer_desc_t){.buffer = gd->raymarch_u_spheres_buffer, .binding=4}
	};

	gs_graphics_bind_desc_t binds = {
		.vertex_buffers = {&(gs_graphics_bind_vertex_buffer_desc_t){.buffer = gd->quad_vbo}},
		.index_buffers = {.desc = &(gs_graphics_bind_index_buffer_desc_t){.buffer = gd->quad_ibo}},
		.uniforms = {.desc = uniforms, .size = sizeof(uniforms)},
		.storage_buffers = {.desc=storage_buffers, .size = sizeof(storage_buffers)}
	};
	
	gs_graphics_pipeline_bind(gcb, gd->raymarch_pip);
	gs_graphics_apply_bindings(gcb, &binds);
	gs_graphics_draw(gcb, &(gs_graphics_draw_desc_t){.start = 0, .count = 6});

}



void camera_update(game_data_t* gd, float delta)
{

    if (gs_platform_mouse_locked()) {

		static gs_vec3 velocity;

        gs_vec2 mouse_delta = gs_platform_mouse_deltav();
        float old_pitch = gd->fps_camera.pitch;
        gd->fps_camera.pitch = gs_clamp(old_pitch + -mouse_delta.y * 0.1, -89.f, 89.f);

        gs_camera_offset_orientation(&gd->fps_camera.cam, mouse_delta.x * 0.1, old_pitch - gd->fps_camera.pitch);

        float speed = 4;
        float run = 1.0;
        gs_vec3 dir = {0};
        if (gs_platform_key_down(GS_KEYCODE_A))
            dir = gs_vec3_add(dir, gs_camera_left(&gd->fps_camera.cam));
        if (gs_platform_key_down(GS_KEYCODE_D))
            dir = gs_vec3_add(dir, gs_camera_right(&gd->fps_camera.cam));
        if (gs_platform_key_down(GS_KEYCODE_W))
            dir = gs_vec3_add(dir, gs_camera_backward(&gd->fps_camera.cam));
        if (gs_platform_key_down(GS_KEYCODE_S))
            dir = gs_vec3_add(dir, gs_camera_forward(&gd->fps_camera.cam));
        
        if (gs_platform_key_down(GS_KEYCODE_SPACE))
            dir = gs_vec3_add(dir, gs_camera_up(&gd->fps_camera.cam));
        if (gs_platform_key_down(GS_KEYCODE_LEFT_SHIFT))
            run = 10.0;
        if (gs_platform_key_down(GS_KEYCODE_LEFT_ALT))
            run *= 100.0;
            //dir = gs_vec3_add(dir, gs_camera_down(&fps_camera.cam));
		
		dir = gs_vec3_norm(dir);
		gs_vec3 target_vel = gs_vec3_scale(dir, run * speed);

        velocity.x = gs_interp_linear(velocity.x, target_vel.x, 0.8*delta);
		velocity.y = gs_interp_linear(velocity.y, target_vel.y, 0.8*delta);
		velocity.z = gs_interp_linear(velocity.z, target_vel.z, 0.8*delta);
		gd->fps_camera.cam.transform.position = gs_vec3_add(gd->fps_camera.cam.transform.position, gs_vec3_scale(target_vel, delta));

        //gd->fps_camera.cam.transform.position = gs_vec3_add(gd->fps_camera.cam.transform.position, gs_vec3_scale(dir, run * speed * delta));
 

        
    }
}





float map(gs_vec3 p)
{
	// plane
	float plane_dist = fPlane(p, gs_v3(0, 1, 0), 14.0);
	float planeID = 7.0;

	float res = plane_dist;

	return res;
	

}