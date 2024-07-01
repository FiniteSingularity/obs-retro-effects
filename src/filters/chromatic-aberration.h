#include <obs-module.h>
#include "../obs-retro-effects.h"

#define CA_TYPE_MANUAL 1
#define CA_TYPE_MANUAL_LABEL "RetroEffects.CA.Manual"
#define CA_TYPE_LENS 2
#define CA_TYPE_LENS_LABEL "RetroEffects.CA.Lens"


struct chromatic_aberration_filter_data;
typedef struct chromatic_aberration_filter_data
	chromatic_aberration_filter_data_t;

struct chromatic_aberration_filter_data {
	gs_effect_t *effect_chromatic_aberration;

	gs_eparam_t *param_image;
	gs_eparam_t *param_uv_size;
	gs_eparam_t *param_channel_offsets;
	gs_eparam_t *param_channel_offset_cos_angles;
	gs_eparam_t *param_channel_offset_sin_angles;
	gs_eparam_t *param_scale;

	uint32_t ca_type;

	struct vec3 offsets;
	struct vec3 offset_cos_angles;
	struct vec3 offset_sin_angles;
};

extern void chromatic_aberration_create(retro_effects_filter_data_t *filter);
extern void chromatic_aberration_destroy(retro_effects_filter_data_t *filter);
extern void chromatic_aberration_unset_settings(retro_effects_filter_data_t* filter);

extern void chromatic_aberration_filter_video_render(retro_effects_filter_data_t *data);
extern void chromatic_aberration_filter_properties(retro_effects_filter_data_t *data,
					obs_properties_t *props);
extern void chromatic_aberration_filter_defaults(obs_data_t *settings);
extern void chromatic_aberration_filter_update(retro_effects_filter_data_t *data,
				    obs_data_t *settings);
static void chromatic_aberration_set_functions(retro_effects_filter_data_t *filter);
static void chromatic_aberration_load_effect(chromatic_aberration_filter_data_t *filter);
static bool ca_type_modified(obs_properties_t *props, obs_property_t *p,
			     obs_data_t *settings);
