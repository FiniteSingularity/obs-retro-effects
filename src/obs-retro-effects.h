#pragma once

#include <obs-module.h>
#include "base-filter.h"

#define PLUGIN_INFO                                                                                               \
	"<a href=\"https://github.com/finitesingularity/obs-retro-effects/\">Retro Effects</a> (" PROJECT_VERSION \
	") by <a href=\"https://twitch.tv/finitesingularity\">FiniteSingularity</a>"

struct retro_effects_filter_data;
typedef struct retro_effects_filter_data retro_effects_filter_data_t;

struct retro_effects_filter_data {
	obs_source_t *context;

	base_filter_data_t *base;

	// Parameters go here
	int frames_to_skip;
	int frames_skipped;
};
