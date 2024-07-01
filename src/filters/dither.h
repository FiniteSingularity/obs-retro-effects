#include <obs-module.h>
#include "../obs-retro-effects.h"

#define DITHER_TYPE_ORDERED 1
#define DITHER_TYPE_ORDERED_LABEL "RetroEffects.Dither.Ordered"
#define DITHER_TYPE_UNORDERED 2
#define DITHER_TYPE_UNORDERED_LABEL "RetroEffects.Dither.Unordered"

struct dither_filter_data;
typedef struct dither_filter_data dither_filter_data_t;

struct dither_filter_data {
	gs_effect_t *effect_dither;

	gs_eparam_t *param_image;
	gs_eparam_t *param_uv_size;
	gs_eparam_t *param_dither_size;
	gs_eparam_t *param_contrast;
	gs_eparam_t *param_gamma;
	gs_eparam_t *param_offset;
	gs_eparam_t *param_color_steps;

	uint32_t dither_type;
	uint32_t bayer_size;
	float dither_size;
	float contrast;
	float gamma;
	struct vec2 offset;
	int color_steps;
	bool monochromatic;
	bool round_to_pixel;
	bool loading_effect;
	bool reload_effect;
};

extern void dither_create(retro_effects_filter_data_t *filter);
extern void dither_destroy(retro_effects_filter_data_t *filter);
extern void dither_unset_settings(retro_effects_filter_data_t* filter);

extern void dither_filter_video_render(retro_effects_filter_data_t *data);
extern void dither_filter_properties(retro_effects_filter_data_t *data,
					obs_properties_t *props);
extern void dither_filter_defaults(obs_data_t *settings);
extern void dither_filter_update(retro_effects_filter_data_t *data,
				    obs_data_t *settings);
static void dither_set_functions(retro_effects_filter_data_t *filter);
static void dither_load_effect(dither_filter_data_t *filter);
static bool dither_type_modified(obs_properties_t *props, obs_property_t *p,
				 obs_data_t *settings);
static bool dither_bayer_size_modified(void *data, obs_properties_t *props,
				       obs_property_t *p, obs_data_t *settings);
static bool dither_round_to_pixel_modified(void *data, obs_properties_t *props,
				       obs_property_t *p, obs_data_t *settings);
