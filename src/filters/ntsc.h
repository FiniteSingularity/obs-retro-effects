#include <obs-module.h>
#include "../obs-retro-effects.h"

struct ntsc_filter_data;
typedef struct ntsc_filter_data ntsc_filter_data_t;

struct ntsc_filter_data {
	gs_effect_t *effect_ntsc_encode;
	gs_effect_t *effect_ntsc_decode;

	gs_texrender_t *encode_texrender;

	gs_eparam_t *param_encode_image;
	gs_eparam_t *param_encode_uv_size;

	gs_eparam_t *param_decode_image;
	gs_eparam_t *param_decode_uv_size;

	bool loading_effect;
};

extern void ntsc_create(retro_effects_filter_data_t *filter);
extern void ntsc_destroy(retro_effects_filter_data_t *filter);
extern void ntsc_filter_video_render(retro_effects_filter_data_t *data);
extern void ntsc_filter_properties(retro_effects_filter_data_t *data,
				   obs_properties_t *props);
extern void ntsc_filter_defaults(obs_data_t *settings);
extern void ntsc_filter_update(retro_effects_filter_data_t *data,
			       obs_data_t *settings);
static void ntsc_set_functions(retro_effects_filter_data_t *filter);
static void ntsc_load_effects(ntsc_filter_data_t *filter);
static void ntsc_load_effect_encode(ntsc_filter_data_t *filter);
static void ntsc_load_effect_decode(ntsc_filter_data_t *filter);
