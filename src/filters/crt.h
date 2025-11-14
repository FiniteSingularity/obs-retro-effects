#include <obs-module.h>
#include <graphics/image-file.h>

#include "../obs-retro-effects.h"

struct crt_filter_data;
typedef struct crt_filter_data crt_filter_data_t;

struct crt_filter_data {
	gs_effect_t *effect_crt;
	gs_effect_t *effect_crt_composite;

	gs_texrender_t *phospher_mask_texrender;

	gs_eparam_t *param_image;
	gs_eparam_t *param_uv_size;
	gs_eparam_t *param_mask_intensity;
	gs_eparam_t *param_phosphor_type;
	gs_eparam_t *param_vignette_intensity;
	gs_eparam_t *param_corner_radius;

	gs_eparam_t *param_image_composite;
	gs_eparam_t *param_blur_image_composite;
	gs_eparam_t *param_brightness_composite;
	gs_eparam_t *param_distort_composite;
	gs_eparam_t *param_black_level_composite;
	gs_eparam_t *param_white_level_composite;


	bool loading_effect;
	bool reload_effect;

	float bloom_threshold;
	float bloom_size;
	float bloom_intensity;
	float mask_intensity;
	int phosphor_type;

	float barrel_distortion;
	float black_level;
	float white_level;
	float vignette_intensity;
	float corner_radius;

	struct vec2 phosphor_size;
};

extern void crt_create(retro_effects_filter_data_t *filter);
extern void crt_destroy(retro_effects_filter_data_t *filter);
extern void crt_unset_settings(retro_effects_filter_data_t* filter);

extern void crt_filter_video_render(retro_effects_filter_data_t *data);
extern void crt_filter_properties(retro_effects_filter_data_t *data,
				  obs_properties_t *props);
extern void crt_filter_defaults(obs_data_t *settings);
extern void crt_filter_update(retro_effects_filter_data_t *data,
			      obs_data_t *settings);
static void crt_set_functions(retro_effects_filter_data_t *filter);

static void crt_load_effect(crt_filter_data_t *filter);
static void crt_composite_load_effect(crt_filter_data_t *filter);

static void crt_filter_render_crt_mask(retro_effects_filter_data_t *data);
static void crt_filter_render_blur(retro_effects_filter_data_t *data);
static void crt_filter_render_bloom(retro_effects_filter_data_t *data);
static void crt_filter_render_composite(retro_effects_filter_data_t *data);
