#pragma once

#include <obs-module.h>

#define PLUGIN_INFO                                                                                               \
	"<a href=\"https://github.com/finitesingularity/obs-retro-effects/\">Retro Effects</a> (" PROJECT_VERSION \
	") by <a href=\"https://twitch.tv/finitesingularity\">FiniteSingularity</a>"

struct base_filter_data;
typedef struct base_filter_data base_filter_data_t;

struct base_filter_data {
	obs_source_t *context;

	bool input_texture_generated;
	gs_texrender_t *input_texrender;
	bool output_rendered;
	gs_texrender_t *output_texrender;

	gs_effect_t *output_effect;
	gs_eparam_t *param_output_image;

	bool rendered;
	bool rendering;

	uint32_t active_filter;

	uint32_t width;
	uint32_t height;
	uint32_t frame;
	float time;
};
