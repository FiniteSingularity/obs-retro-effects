#include <obs-module.h>
#include "../obs-retro-effects.h"

struct frame_skip_filter_data;
typedef struct frame_skip_filter_data frame_skip_filter_data_t;

struct frame_skip_filter_data {
	int frames_to_skip;
	int frames_skipped;
};

extern void frame_skip_create(retro_effects_filter_data_t *filter);
extern void frame_skip_destroy(retro_effects_filter_data_t *filter);
extern void frame_skip_unset_settings(retro_effects_filter_data_t* filter);

extern void frame_skip_filter_video_render(retro_effects_filter_data_t *data);
extern void frame_skip_filter_properties(retro_effects_filter_data_t *data,
					 obs_properties_t *props);
extern void frame_skip_filter_defaults(obs_data_t *settings);
extern void frame_skip_filter_update(retro_effects_filter_data_t *data,
			      obs_data_t *settings);
static void frame_skip_set_functions(retro_effects_filter_data_t *filter);
