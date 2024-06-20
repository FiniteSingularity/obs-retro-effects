#include <obs-module.h>
#include "../obs-retro-effects.h"

struct analog_glitch_filter_data;
typedef struct analog_glitch_filter_data analog_glitch_filter_data_t;

struct analog_glitch_filter_data {
	gs_effect_t *effect_analog_glitch;

	gs_eparam_t *param_image;
	gs_eparam_t *param_uv_size;
	gs_eparam_t *param_time;
	gs_eparam_t *param_speed_primary;
	gs_eparam_t *param_speed_secondary;
	gs_eparam_t *param_speed_interference;
	gs_eparam_t *param_scale_primary;
	gs_eparam_t *param_scale_secondary;
	gs_eparam_t *param_scale_interference;
	gs_eparam_t *param_threshold_primary;
	gs_eparam_t *param_threshold_secondary;
	gs_eparam_t *param_secondary_influence;
	gs_eparam_t *param_max_disp;
	gs_eparam_t *param_interference_mag;
	gs_eparam_t *param_line_mag;
	gs_eparam_t *param_desaturation_amount;
	gs_eparam_t *param_color_drift;
	gs_eparam_t *param_interference_alpha;

	float time;
	float speed_primary;
	float speed_secondary;
	float speed_interference;
	float scale_primary;
	float scale_secondary;
	float scale_interference;

	float threshold_primary;
	float threshold_secondary;
	float secondary_influence;
	float max_disp;
	float interference_mag;
	float line_mag;
	bool interference_alpha;

	float desaturation_amount;
	float color_drift;

	bool loading_effect;
	bool reload_effect;
};

extern void analog_glitch_create(retro_effects_filter_data_t *filter);
extern void analog_glitch_destroy(retro_effects_filter_data_t *filter);
extern void analog_glitch_filter_video_render(retro_effects_filter_data_t *data);
extern void analog_glitch_filter_video_tick(retro_effects_filter_data_t *data,
				     float seconds);
extern void analog_glitch_filter_properties(retro_effects_filter_data_t *data,
					    obs_properties_t *props);
extern void analog_glitch_filter_defaults(obs_data_t *settings);
extern void analog_glitch_filter_update(retro_effects_filter_data_t *data,
					obs_data_t *settings);
static void analog_glitch_set_functions(retro_effects_filter_data_t *filter);
static void analog_glitch_load_effect(analog_glitch_filter_data_t *filter);
