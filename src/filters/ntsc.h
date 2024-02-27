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
	gs_eparam_t *param_encode_tuning_offset;
	gs_eparam_t *param_encode_frame;
	gs_eparam_t *param_encode_y_offset;
	gs_eparam_t *param_encode_luma_noise;

	gs_eparam_t *param_decode_image;
	gs_eparam_t *param_decode_uv_size;
	gs_eparam_t *param_decode_luma_band_size;
	gs_eparam_t *param_decode_luma_band_strength;
	gs_eparam_t *param_decode_luma_band_count;
	gs_eparam_t *param_decode_chroma_bleed_size;
	gs_eparam_t *param_decode_chroma_bleed_strength;
	gs_eparam_t *param_decode_chroma_bleed_steps;
	gs_eparam_t *param_decode_brightness;
	gs_eparam_t *param_decode_saturation;

	bool loading_effect;

	uint32_t frame;
	float tuning_offset;
	float y_offset;
	float luma_noise;
	float luma_band_size;
	float luma_band_strength;
	int luma_band_count;
	float chroma_bleed_size;
	float chroma_bleed_strength;
	int chroma_bleed_steps;
	float brightness;
	float saturation;
};

extern void ntsc_create(retro_effects_filter_data_t *filter);
extern void ntsc_destroy(retro_effects_filter_data_t *filter);
extern void ntsc_filter_video_tick(retro_effects_filter_data_t *data,
				   float seconds);
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
