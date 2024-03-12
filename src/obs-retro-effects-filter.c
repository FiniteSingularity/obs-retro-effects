#include "obs-retro-effects-filter.h"
#include "obs-retro-effects.h"
#include "filters/frame-skip.h"
#include "filters/interlace.h"
#include "filters/chromatic-aberration.h"
#include "filters/posterize.h"
#include "filters/dither.h"
#include "filters/crt.h"
#include "filters/ntsc.h"
#include "filters/cathode-boot.h"
#include "filters/matrix-rain.h"
#include "blur/blur.h"
#include "blur/bloom.h"

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

	filter->base = bzalloc(sizeof(base_filter_data_t));
	filter->base->context = source;
	filter->base->input_texrender =
		create_or_reset_texrender(filter->base->input_texrender);
	filter->base->output_texrender =
		create_or_reset_texrender(filter->base->output_texrender);
	filter->base->param_output_image = NULL;
	filter->base->rendered = false;
	filter->base->rendering = false;
	filter->base->frame = 0;
	filter->frames_skipped = 0;
	filter->initial_load = true;

	blur_create(filter);
	bloom_create(filter);

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

	filter->filter_destroy(filter);
	blur_destroy(filter);

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
	retro_effects_filter_data_t *filter = data;

	int new_active_filter = (int)obs_data_get_int(settings, "filter_type");
	int old_active_filter = filter->active_filter;
	filter->active_filter = new_active_filter;
	if (filter->initial_load) {
		load_filter(filter, 0);
	} else if (new_active_filter != old_active_filter) {
		load_filter(filter, old_active_filter);
		obs_source_update_properties(filter->base->context);
	}

	if (filter->filter_update) {
		filter->filter_update(filter, settings);
	}
	filter->initial_load = false;
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

	if (filter->filter_video_render) {
		filter->filter_video_render(filter);
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

	obs_property_t *filter_list = obs_properties_add_list(
		props, "filter_type", obs_module_text("RetroEffects.Filter"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(filter_list,
				  obs_module_text(RETRO_FILTER_CA_LABEL),
				  RETRO_FILTER_CA);
	obs_property_list_add_int(
		filter_list, obs_module_text(RETRO_FILTER_FRAME_SKIP_LABEL),
		RETRO_FILTER_FRAME_SKIP);
	obs_property_list_add_int(filter_list,
				  obs_module_text(RETRO_FILTER_INTERLACE_LABEL),
				  RETRO_FILTER_INTERLACE);
	obs_property_list_add_int(filter_list,
				  obs_module_text(RETRO_FILTER_POSTERIZE_LABEL),
				  RETRO_FILTER_POSTERIZE);
	obs_property_list_add_int(filter_list,
				  obs_module_text(RETRO_FILTER_DITHER_LABEL),
				  RETRO_FILTER_DITHER);
	obs_property_list_add_int(filter_list,
				  obs_module_text(RETRO_FILTER_CRT_LABEL),
				  RETRO_FILTER_CRT);
	obs_property_list_add_int(filter_list,
				  obs_module_text(RETRO_FILTER_NTSC_LABEL),
				  RETRO_FILTER_NTSC);
	obs_property_list_add_int(filter_list,
				  obs_module_text(RETRO_FILTER_CATHODE_BOOT_LABEL),
		                  RETRO_FILTER_CATHODE_BOOT);
	obs_property_list_add_int(filter_list,
				  obs_module_text(RETRO_FILTER_MATRIX_RAIN_LABEL),
				  RETRO_FILTER_MATRIX_RAIN);

	obs_property_set_modified_callback2(filter_list, filter_type_modified,
					    data);

	if (filter->filter_properties) {
		filter->filter_properties(filter, props);
	}

	return props;
}

static bool filter_type_modified(void *data, obs_properties_t *props,
				 obs_property_t *p, obs_data_t *settings)
{
	// retro_effects_filter_data_t *filter = data;
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	UNUSED_PARAMETER(settings);

	return false;
}

static void load_filter(retro_effects_filter_data_t *filter, int old_type)
{
	if (old_type != 0) {
		// Clear out old settings.
		// Destroy old type data object.
		if (filter->filter_destroy) {
			filter->filter_destroy(filter);
		}
	}
	// load in new data object.
	switch (filter->active_filter) {
	case RETRO_FILTER_CA:
		chromatic_aberration_create(filter);
		break;
	case RETRO_FILTER_FRAME_SKIP:
		frame_skip_create(filter);
		break;
	case RETRO_FILTER_INTERLACE:
		interlace_create(filter);
		break;
	case RETRO_FILTER_POSTERIZE:
		posterize_create(filter);
		break;
	case RETRO_FILTER_DITHER:
		dither_create(filter);
		break;
	case RETRO_FILTER_CRT:
		crt_create(filter);
		break;
	case RETRO_FILTER_NTSC:
		ntsc_create(filter);
		break;
	case RETRO_FILTER_CATHODE_BOOT:
		cathode_boot_create(filter);
		break;
	case RETRO_FILTER_MATRIX_RAIN:
		matrix_rain_create(filter);
		break;
	}
}

static void retro_effects_filter_video_tick(void *data, float seconds)
{
	retro_effects_filter_data_t *filter = data;

	obs_source_t *target = obs_filter_get_target(filter->base->context);
	if (!target) {
		return;
	}
	filter->base->width = (uint32_t)obs_source_get_base_width(target);
	filter->base->height = (uint32_t)obs_source_get_base_height(target);
	filter->base->frame += 1u;
	if (filter->filter_video_tick) {
		filter->filter_video_tick(filter, seconds);
	}

	filter->base->rendered = false;
}

static void retro_effects_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "filter_type",
				 RETRO_FILTER_FRAME_SKIP);
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
