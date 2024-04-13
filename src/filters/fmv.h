#include <obs-module.h>
#include "../obs-retro-effects.h"

struct fmv_filter_data;
typedef struct fmv_filter_data fmv_filter_data_t;

struct fmv_filter_data {
	gs_effect_t *effect_fmv;

	gs_eparam_t *param_image;
	gs_eparam_t *param_uv_size;
	gs_eparam_t *param_prev_frame;
	gs_eparam_t *param_prev_frame_valid;

	gs_eparam_t *param_colors_per_channel;
	gs_eparam_t *param_threshold_prev_frame;
	gs_eparam_t *param_threshold_solid;
	gs_eparam_t *param_threshold_gradient;

	gs_texrender_t *texrender_downsampled_input;
	gs_texrender_t *texrender_previous_frame;
	gs_texrender_t *texrender_downsampled_output;

	float px_scale;
	int colors_per_channel;

	float quality;
	bool custom_thresholds;
	
	float threshold_prev_frame;
	float threshold_solid;
	float threshold_gradient;

	bool loading_effect;
	bool reload_effect;
};

extern void fmv_create(retro_effects_filter_data_t *filter);
extern void fmv_destroy(retro_effects_filter_data_t *filter);
extern void fmv_filter_video_render(retro_effects_filter_data_t *data);
extern void fmv_filter_properties(retro_effects_filter_data_t *data,
					obs_properties_t *props);
extern void fmv_filter_defaults(obs_data_t *settings);
extern void fmv_filter_update(retro_effects_filter_data_t *data,
				    obs_data_t *settings);
static void fmv_set_functions(retro_effects_filter_data_t *filter);
static void fmv_load_effect(fmv_filter_data_t *filter);
