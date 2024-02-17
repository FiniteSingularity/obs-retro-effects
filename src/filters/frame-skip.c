#include "frame-skip.h"
#include "../obs-utils.h"

void frame_skip_create(retro_effects_filter_data_t *filter)
{
	frame_skip_filter_data_t *data =
		bzalloc(sizeof(frame_skip_filter_data_t));
	data->frames_skipped = 0;
	filter->active_filter_data = data;
	frame_skip_set_functions(filter);
}

void frame_skip_destroy(retro_effects_filter_data_t *filter)
{
	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	obs_data_unset_user_value(settings, "skip_frames");
	obs_data_release(settings);
	bfree(filter->active_filter_data);
	filter->active_filter_data = NULL;
}

void frame_skip_filter_update(retro_effects_filter_data_t *data,
			      obs_data_t *settings)
{
	frame_skip_filter_data_t *filter = data->active_filter_data;

	filter->frames_to_skip = (int)obs_data_get_int(settings, "skip_frames");
}

void frame_skip_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "skip_frames", 0);
}

void frame_skip_filter_properties(retro_effects_filter_data_t *data,
				  obs_properties_t *props)
{
	UNUSED_PARAMETER(data);
	obs_properties_add_int_slider(
		props, "skip_frames",
		obs_module_text("RetroEffects.FrameSkip.NumFrames"), 0, 600, 1);
}

void frame_skip_filter_video_render(retro_effects_filter_data_t *data)
{

	base_filter_data_t *base = data->base;

	frame_skip_filter_data_t *filter = data->active_filter_data;

	if (filter->frames_skipped < filter->frames_to_skip) {
		filter->frames_skipped += 1;
	} else {
		get_input_source(base);
		if (!base->input_texture_generated) {
			base->rendering = false;
			obs_source_skip_video_filter(base->context);
			return;
		}
		// swap output and input to update the drawn frame
		gs_texrender_t *tmp = base->output_texrender;
		base->output_texrender = base->input_texrender;
		base->input_texrender = tmp;

		filter->frames_skipped = 0;
	}
}

static void frame_skip_set_functions(retro_effects_filter_data_t *filter)
{
	filter->filter_properties = frame_skip_filter_properties;
	filter->filter_video_render = frame_skip_filter_video_render;
	filter->filter_destroy = frame_skip_destroy;
	filter->filter_defaults = frame_skip_filter_defaults;
	filter->filter_update = frame_skip_filter_update;
	filter->filter_video_tick = NULL;
}
