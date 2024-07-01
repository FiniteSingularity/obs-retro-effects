#include <obs-module.h>
#include "../obs-retro-effects.h"
#include "../blur/bloom.h"

#define BLOOM_THRESHOLD_TYPE_LUMINANCE 0
#define BLOOM_THRESHOLD_TYPE_LUMINANCE_LABEL "RetroEffects.Bloom.ThresholdType.Luminance"
#define BLOOM_THRESHOLD_TYPE_RED 1
#define BLOOM_THRESHOLD_TYPE_RED_LABEL "RetroEffects.Bloom.ThresholdType.Red"
#define BLOOM_THRESHOLD_TYPE_GREEN 2
#define BLOOM_THRESHOLD_TYPE_GREEN_LABEL "RetroEffects.Bloom.ThresholdType.Green"
#define BLOOM_THRESHOLD_TYPE_BLUE 3
#define BLOOM_THRESHOLD_TYPE_BLUE_LABEL "RetroEffects.Bloom.ThresholdType.Blue"
#define BLOOM_THRESHOLD_TYPE_CUSTOM 4
#define BLOOM_THRESHOLD_TYPE_CUSTOM_LABEL "RetroEffects.Bloom.ThresholdType.Custom"

struct bloom_f_filter_data;
typedef struct bloom_f_filter_data bloom_f_filter_data_t;

struct bloom_f_filter_data {
	gs_eparam_t *param_image;
	gs_eparam_t *param_uv_size;

	float intensity;
	float threshold;
	float size;

	float level_red;
	float level_green;
	float level_blue;
	uint8_t threshold_type;

	bool loading_effect;
	bool reload_effect;
};

extern void bloom_f_create(retro_effects_filter_data_t *filter);
extern void bloom_f_destroy(retro_effects_filter_data_t *filter);
extern void bloom_f_unset_settings(retro_effects_filter_data_t* filter);

extern void bloom_f_filter_video_render(retro_effects_filter_data_t *data);
extern void bloom_f_filter_properties(retro_effects_filter_data_t *data,
				      obs_properties_t *props);
extern void bloom_f_filter_defaults(obs_data_t *settings);
extern void bloom_f_filter_update(retro_effects_filter_data_t *data,
				  obs_data_t *settings);
static void bloom_f_set_functions(retro_effects_filter_data_t *filter);
static bool threshold_type_modified(void *data, obs_properties_t *props,
				    obs_property_t *p, obs_data_t *settings);
