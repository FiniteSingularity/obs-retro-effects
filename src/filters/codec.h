#include <obs-module.h>
#include "../obs-retro-effects.h"

#define CODEC_TYPE_RPZA 1
#define CODEC_TYPE_RPZA_LABEL "RetroEffects.Codec.RPZA"

struct codec_filter_data;
typedef struct codec_filter_data codec_filter_data_t;

struct codec_filter_data {
	gs_effect_t *effect_codec;

	gs_eparam_t *param_image;
	gs_eparam_t *param_uv_size;
	gs_eparam_t *param_prev_frame;
	gs_eparam_t *param_prev_frame_valid;

	gs_eparam_t *param_colors_per_channel;

	gs_eparam_t *param_rpza_threshold_prev_frame;
	gs_eparam_t *param_rpza_threshold_solid;
	gs_eparam_t *param_rpza_threshold_gradient;

	gs_texrender_t *texrender_downsampled_input;
	gs_texrender_t *texrender_previous_frame;
	gs_texrender_t *texrender_downsampled_output;

	uint32_t codec_type;

	float px_scale;
	int colors_per_channel;

	float quality;
	bool custom_thresholds;
	
	float rpza_threshold_prev_frame;
	float rpza_threshold_solid;
	float rpza_threshold_gradient;

	bool loading_effect;
	bool reload_effect;
};

extern void codec_create(retro_effects_filter_data_t *filter);
extern void codec_destroy(retro_effects_filter_data_t *filter);
extern void codec_filter_video_render(retro_effects_filter_data_t *data);
extern void codec_filter_properties(retro_effects_filter_data_t *data,
					obs_properties_t *props);
extern void codec_filter_defaults(obs_data_t *settings);
extern void codec_filter_update(retro_effects_filter_data_t *data,
				    obs_data_t *settings);
static void codec_set_functions(retro_effects_filter_data_t *filter);
static void codec_load_effect(codec_filter_data_t *filter);

static bool codec_type_modified(obs_properties_t *props, obs_property_t *p,
				 obs_data_t *settings);
