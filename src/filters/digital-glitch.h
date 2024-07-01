#include <obs-module.h>
#include <util/darray.h>
#include "../obs-retro-effects.h"

typedef DARRAY(float) fdDarray;

struct digital_glitch_filter_data;
typedef struct digital_glitch_filter_data digital_glitch_filter_data_t;

struct digital_glitch_filter_data {
	gs_effect_t *effect_digital_glitch;

	gs_eparam_t *param_image;
	gs_eparam_t *param_uv_size;
	gs_eparam_t *param_time;
	gs_eparam_t *param_vert_grid;
	gs_eparam_t *param_horiz_grid;
	gs_eparam_t *param_rgb_drift_grid;
	gs_eparam_t *param_max_disp;
	gs_eparam_t *param_max_rgb_drift;
	gs_eparam_t *param_amount;

	gs_texture_t *vert_grid_texture;
	gs_texture_t *horiz_grid_texture;
	gs_texture_t *rgb_drift_texture;

	fdDarray vert_grid;
	fdDarray horiz_grid;
	fdDarray rgb_drift_grid;

	float max_disp;
	float amount;
	float local_time;
	float max_rgb_drift;
	float grid_next_update;
	float rgb_drift_next_update;
	uint32_t min_block_width;
	uint32_t max_block_width;
	uint32_t min_block_height;
	uint32_t max_block_height;
	uint32_t min_rgb_drift_height;
	uint32_t max_rgb_drift_height;

	struct vec2 block_interval;
	struct vec2 rgb_drift_interval;


	bool loading_effect;
	bool reload_effect;
};

extern void digital_glitch_create(retro_effects_filter_data_t *filter);
extern void digital_glitch_destroy(retro_effects_filter_data_t *filter);
extern void digital_glitch_unset_settings(retro_effects_filter_data_t* filter);

extern void digital_glitch_filter_video_render(retro_effects_filter_data_t *data);
extern void digital_glitch_filter_properties(retro_effects_filter_data_t *data,
					     obs_properties_t *props);
extern void digital_glitch_filter_defaults(obs_data_t *settings);
extern void digital_glitch_filter_update(retro_effects_filter_data_t *data,
					 obs_data_t *settings);
extern void digital_glitch_filter_video_tick(retro_effects_filter_data_t *data,
					     float seconds);
static void digital_glitch_set_functions(retro_effects_filter_data_t *filter);
static void digital_glitch_load_effect(digital_glitch_filter_data_t *filter);
