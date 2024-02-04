#pragma once

#include <obs-module.h>
#include "base-filter.h"

#define PLUGIN_INFO                                                                                               \
	"<a href=\"https://github.com/finitesingularity/obs-retro-effects/\">Retro Effects</a> (" PROJECT_VERSION \
	") by <a href=\"https://twitch.tv/finitesingularity\">FiniteSingularity</a>"

#define RETRO_FILTER_FRAME_SKIP 1
#define RETRO_FILTER_FRAME_SKIP_LABEL "RetroEffects.FrameSkip"
#define RETRO_FILTER_INTERLACE 2
#define RETRO_FILTER_INTERLACE_LABEL "RetroEffects.Interlace"
#define RETRO_FILTER_CA 3
#define RETRO_FILTER_CA_LABEL "RetroEffects.ChromaticAberration"

struct retro_effects_filter_data;
typedef struct retro_effects_filter_data retro_effects_filter_data_t;

struct retro_effects_filter_data {
	base_filter_data_t *base;
	void *active_filter_data;
	int active_filter;
	bool initial_load;

	// Filter Function Pointers
	void (*filter_properties)(retro_effects_filter_data_t *,
				  obs_properties_t *); // Properties UI setup.
	void (*filter_defaults)(obs_data_t *);                           // defaults
	void (*filter_update)(retro_effects_filter_data_t *,
			      obs_data_t *);                     // update
	void (*filter_video_render)(retro_effects_filter_data_t *);      // render
	void (*filter_destroy)(retro_effects_filter_data_t *);           // destroy
	void (*filter_video_tick)(retro_effects_filter_data_t *, float); // Video Tick

	// Parameters go here
	int frames_to_skip;
	int frames_skipped;
};
