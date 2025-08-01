#include <obs-module.h>
#include "../obs-retro-effects.h"

struct cathode_boot_filter_data;
typedef struct cathode_boot_filter_data cathode_boot_filter_data_t;

struct cathode_boot_filter_data {
	gs_effect_t *effect_cathode_boot;

	gs_eparam_t *param_image;
	gs_eparam_t *param_uv_size;
	gs_eparam_t *param_progress;
	gs_eparam_t *param_vert_range;
	gs_eparam_t *param_horiz_range;
	gs_eparam_t *param_fade_range;
	gs_eparam_t *param_thickness;
	gs_eparam_t *param_glow_size;
	gs_eparam_t *param_glow_color;
	gs_eparam_t* param_bg_color;

	float progress;

	bool loading_effect;
	bool reload_effect;
	float glow_size;
	struct vec2 vert_range;
	struct vec2 horiz_range;
	struct vec2 fade_range;
	struct vec2 thickness;
	struct vec4 glow_color;
	struct vec4 bg_color;
};

extern void cathode_boot_create(retro_effects_filter_data_t *filter);
extern void cathode_boot_destroy(retro_effects_filter_data_t *filter);
extern void cathode_boot_unset_settings(retro_effects_filter_data_t* filter);

extern void cathode_boot_filter_video_render(retro_effects_filter_data_t *data);
extern void cathode_boot_filter_properties(retro_effects_filter_data_t *data,
					   obs_properties_t *props);
extern void cathode_boot_filter_defaults(obs_data_t *settings);
extern void cathode_boot_filter_update(retro_effects_filter_data_t *data,
				       obs_data_t *settings);
static void cathode_boot_set_functions(retro_effects_filter_data_t *filter);
static void cathode_boot_load_effect(cathode_boot_filter_data_t *filter);
