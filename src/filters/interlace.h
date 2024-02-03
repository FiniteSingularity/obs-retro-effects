#include <obs-module.h>
#include "../obs-retro-effects.h"

struct interlace_filter_data;
typedef struct interlace_filter_data interlace_filter_data_t;

struct interlace_filter_data {
	gs_effect_t *effect_interlace;
	gs_texrender_t *buffer_texrender;

	gs_eparam_t *param_image;
	gs_eparam_t *param_prior_frame;
	gs_eparam_t *param_frame;
	gs_eparam_t *param_uv_size;
	gs_eparam_t *param_thickness;
	gs_eparam_t *param_brightness_reduction;

	int frame; //0 if top, 1 if bottom
	int thickness;
	struct vec4 brightness_reduction;
};

extern void interlace_create(retro_effects_filter_data_t *filter);
extern void interlace_destroy(retro_effects_filter_data_t *filter);
extern void interlace_filter_video_render(retro_effects_filter_data_t *data);
extern void interlace_filter_properties(retro_effects_filter_data_t *data,
					 obs_properties_t *props);
extern void interlace_filter_defaults(obs_data_t *settings);
extern void interlace_filter_update(retro_effects_filter_data_t *data,
				     obs_data_t *settings);
static void interlace_set_functions(retro_effects_filter_data_t *filter);
static void load_interlace_effect(interlace_filter_data_t *filter);
