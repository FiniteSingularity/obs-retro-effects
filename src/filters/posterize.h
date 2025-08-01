#include <obs-module.h>
#include <util/dstr.h>

#include "../obs-retro-effects.h"

#define POSTERIZE_COLOR_PASSTHROUGH 0
#define POSTERIZE_COLOR_PASSTHROUGH_LABEL "RetroEffects.Posterize.Passthrough"
#define POSTERIZE_COLOR_MAP 1
#define POSTERIZE_COLOR_MAP_LABEL "RetroEffects.Posterize.ColorMap"
#define POSTERIZE_COLOR_SOURCE_MAP 2
#define POSTERIZE_COLOR_SOURCE_MAP_LABEL "RetroEffects.Posterize.SourceColorMap"

struct posterize_filter_data;
typedef struct posterize_filter_data posterize_filter_data_t;

struct posterize_filter_data {
	gs_effect_t *effect_posterize;

	gs_eparam_t *param_image;
	gs_eparam_t *param_uv_size;
	gs_eparam_t *param_levels;
	gs_eparam_t *param_color_1;
	gs_eparam_t *param_color_2;
	gs_eparam_t *param_color_source;

	struct dstr color_source_name;
	bool has_color_source;
	obs_weak_source_t *color_source;
	float levels;
	uint32_t technique;
	struct vec4 color_1;
	struct vec4 color_2;

};

extern void posterize_create(retro_effects_filter_data_t *filter);
extern void posterize_destroy(retro_effects_filter_data_t *filter);
extern void posterize_unset_settings(retro_effects_filter_data_t* filter);

extern void posterize_filter_video_render(retro_effects_filter_data_t *data);
extern void posterize_filter_properties(retro_effects_filter_data_t *data,
					obs_properties_t *props);
extern void posterize_filter_defaults(obs_data_t *settings);
extern void posterize_filter_update(retro_effects_filter_data_t *data,
				    obs_data_t *settings);
static void posterize_set_functions(retro_effects_filter_data_t *filter);
static void posterize_load_effect(posterize_filter_data_t *filter);

static bool posterize_technique_modified(obs_properties_t *props,
					 obs_property_t *p,
					 obs_data_t *settings);
static void posterize_filter_video_tick(retro_effects_filter_data_t* data, float seconds);
void load_color_source(posterize_filter_data_t* filter);
