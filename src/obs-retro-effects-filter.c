#include "obs-retro-effects-filter.h"
#include "obs-retro-effects.h"

struct obs_source_info obs_retro_effects_filter = {
	.id = "obs_retro_effects_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_VIDEO | OBS_SOURCE_SRGB,
	.get_name = retro_effects_filter_name,
	.create = retro_effects_filter_create,
	.destroy = retro_effects_filter_destroy,
	.update = retro_effects_filter_update,
	.video_render = retro_effects_filter_video_render,
	.video_tick = retro_effects_filter_video_tick,
	.get_width = retro_effects_filter_width,
	.get_height = retro_effects_filter_height,
	.get_properties = retro_effects_filter_properties,
	.get_defaults = retro_effects_filter_defaults};

static const char *retro_effects_filter_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("Retro Effects");
}

static void *retro_effects_filter_create(obs_data_t *settings,
					 obs_source_t *source)
{
	// This function should initialize all pointers in the data
	// structure.
	retro_effects_filter_data_t *filter =
		bzalloc(sizeof(retro_effects_filter_data_t));

	filter->context = source;

	filter->base = bzalloc(sizeof(base_filter_data_t));
	filter->base->input_texrender =
		create_or_reset_texrender(filter->base->input_texrender);
	filter->base->output_texrender =
		create_or_reset_texrender(filter->base->output_texrender);
	filter->base->param_output_image = NULL;
	filter->base->rendered = false;
	filter->base->rendering = false;
	filter->frames_skipped = 0;

	load_output_effect(filter);
	obs_source_update(source, settings);

	return filter;
}

static void retro_effects_filter_destroy(void *data)
{
	// This function should clear up all memory the plugin uses.
	retro_effects_filter_data_t *filter = data;


	obs_enter_graphics();
	if (filter->base->input_texrender) {
		gs_texrender_destroy(filter->base->input_texrender);
	}
	if (filter->base->output_texrender) {
		gs_texrender_destroy(filter->base->output_texrender);
	}
	if (filter->base->output_effect) {
		gs_effect_destroy(filter->base->output_effect);
	}
	obs_leave_graphics();

	bfree(filter->base);
	bfree(filter);
}

static uint32_t retro_effects_filter_width(void *data)
{
	retro_effects_filter_data_t *filter = data;
	return filter->base->width;
}

static uint32_t retro_effects_filter_height(void *data)
{
	retro_effects_filter_data_t *filter = data;
	return filter->base->height;
}

static void retro_effects_filter_update(void *data, obs_data_t *settings)
{
	// Called after UI is updated, should assign new UI values to
	// data structure pointers/values/etc..
	retro_effects_filter_data_t *filter = data;
	filter->frames_to_skip = (int)obs_data_get_int(settings, "skip_frames");
}

static void retro_effects_filter_video_render(void *data, gs_effect_t *effect)
{
	UNUSED_PARAMETER(effect);
	retro_effects_filter_data_t *filter = data;

	if (filter->base->rendered) {
		draw_output(filter);
		return;
	}

	filter->base->rendering = true;

	if (filter->frames_skipped < filter->frames_to_skip) {
		filter->frames_skipped += 1;
	} else {
		get_input_source(filter);
		if (!filter->base->input_texture_generated) {
			filter->base->rendering = false;
			obs_source_skip_video_filter(filter->context);
			return;
		}
		// 2. Render the filter
		// Call a rendering function, e.g.:
		retro_effects_render_filter(filter);
		filter->frames_skipped = 0;
	}


	// 3. Draw result (filter->output_texrender) to source
	draw_output(filter);
	filter->base->rendered = true;
	filter->base->rendering = false;
}

static obs_properties_t *retro_effects_filter_properties(void *data)
{
	retro_effects_filter_data_t *filter = data;

	obs_properties_t *props = obs_properties_create();
	obs_properties_set_param(props, filter, NULL);

	obs_properties_add_int_slider(
		props, "skip_frames",
		obs_module_text("RetroEffects.FrameSkip.NumFrames"), 0, 600, 1);

	return props;
}

static void retro_effects_filter_video_tick(void *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	retro_effects_filter_data_t *filter = data;

	obs_source_t *target = obs_filter_get_target(filter->context);
	if (!target) {
		return;
	}
	filter->base->width = (uint32_t)obs_source_get_base_width(target);
	filter->base->height = (uint32_t)obs_source_get_base_height(target);

	filter->base->rendered = false;
}

static void retro_effects_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "example_int", 10);
}

static void get_input_source(retro_effects_filter_data_t *filter)
{
	// Use the OBS default effect file as our effect.
	gs_effect_t *pass_through = obs_get_base_effect(OBS_EFFECT_DEFAULT);

	// Set up our color space info.
	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	const enum gs_color_space source_space = obs_source_get_color_space(
		obs_filter_get_target(filter->context),
		OBS_COUNTOF(preferred_spaces), preferred_spaces);

	const enum gs_color_format format =
		gs_get_format_from_space(source_space);

	// Set up our input_texrender to catch the output texture.
	filter->base->input_texrender =
		create_or_reset_texrender(filter->base->input_texrender);

	// Start the rendering process with our correct color space params,
	// And set up your texrender to recieve the created texture.
	if (obs_source_process_filter_begin_with_color_space(
		    filter->context, format, source_space,
		    OBS_NO_DIRECT_RENDERING) &&
	    gs_texrender_begin(filter->base->input_texrender,
			       filter->base->width, filter->base->height)) {

		set_blending_parameters();
		gs_ortho(0.0f, (float)filter->base->width, 0.0f,
			 (float)filter->base->height, -100.0f, 100.0f);
		// The incoming source is pre-multiplied alpha, so use the
		// OBS default effect "DrawAlphaDivide" technique to convert
		// the colors back into non-pre-multiplied space.
		obs_source_process_filter_tech_end(
			filter->context, pass_through, filter->base->width,
			filter->base->height, "DrawAlphaDivide");
		gs_texrender_end(filter->base->input_texrender);
		gs_blend_state_pop();
		filter->base->input_texture_generated = true;
	}
}

static void draw_output(retro_effects_filter_data_t *filter)
{
	const enum gs_color_space preferred_spaces[] = {
		GS_CS_SRGB,
		GS_CS_SRGB_16F,
		GS_CS_709_EXTENDED,
	};

	const enum gs_color_space source_space = obs_source_get_color_space(
		obs_filter_get_target(filter->context),
		OBS_COUNTOF(preferred_spaces), preferred_spaces);

	const enum gs_color_format format =
		gs_get_format_from_space(source_space);

	if (!obs_source_process_filter_begin_with_color_space(
		    filter->context, format, source_space,
		    OBS_NO_DIRECT_RENDERING)) {
		return;
	}

	gs_texture_t *texture =
		gs_texrender_get_texture(filter->base->output_texrender);
	gs_effect_t *pass_through = filter->base->output_effect;

	if (filter->base->param_output_image) {
		gs_effect_set_texture(filter->base->param_output_image,
				      texture);
	}

	obs_source_process_filter_end(filter->context, pass_through,
				      filter->base->width,
				      filter->base->height);
}

static void retro_effects_render_filter(retro_effects_filter_data_t *filter)
{
	gs_texrender_t *tmp = filter->base->output_texrender;
	filter->base->output_texrender = filter->base->input_texrender;
	filter->base->input_texrender = tmp;
}

static void load_output_effect(retro_effects_filter_data_t *filter)
{
	if (filter->base->output_effect != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->base->output_effect);
		filter->base->output_effect = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/render_output.effect");
	shader_text = load_shader_from_file(filename.array);
	char *errors = NULL;
	dstr_free(&filename);

	obs_enter_graphics();
	filter->base->output_effect =
		gs_effect_create(shader_text, NULL, &errors);
	obs_leave_graphics();

	bfree(shader_text);
	if (filter->base->output_effect == NULL) {
		blog(LOG_WARNING,
		     "[obs-composite-blur] Unable to load output.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	} else {
		size_t effect_count =
			gs_effect_get_num_params(filter->base->output_effect);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->base->output_effect, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "output_image") == 0) {
				filter->base->param_output_image = param;
			}
		}
	}
}
