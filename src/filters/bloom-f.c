#include "bloom-f.h"
#include "../obs-utils.h"

void bloom_f_create(retro_effects_filter_data_t *filter)
{
	bloom_f_filter_data_t *data = bzalloc(sizeof(bloom_f_filter_data_t));
	filter->active_filter_data = data;
	data->reload_effect = false;
	bloom_f_set_functions(filter);
	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	bloom_f_filter_defaults(settings);
	obs_data_release(settings);
	//bloom_f_load_effect(data);
}

void bloom_f_destroy(retro_effects_filter_data_t *filter)
{
	//bloom_f_filter_data_t *data = filter->active_filter_data;
	obs_enter_graphics();

	obs_leave_graphics();

	obs_data_t *settings = obs_source_get_settings(filter->base->context);

	obs_data_unset_user_value(settings, "bloom_intensity");
	obs_data_unset_user_value(settings, "bloom_threshold");
	obs_data_unset_user_value(settings, "bloom_size");
	obs_data_unset_user_value(settings, "bloom_threshold_type");
	obs_data_unset_user_value(settings, "bloom_red_level");
	obs_data_unset_user_value(settings, "bloom_green_level");
	obs_data_unset_user_value(settings, "bloom_blue_level");

	obs_data_release(settings);

	bfree(filter->active_filter_data);
	filter->active_filter_data = NULL;
}

void bloom_f_filter_update(retro_effects_filter_data_t *data,
			   obs_data_t *settings)
{
	bloom_f_filter_data_t *filter = data->active_filter_data;

	filter->threshold =
		(float)obs_data_get_double(settings, "bloom_threshold") /
		100.0f;

	filter->size = (float)obs_data_get_double(settings, "bloom_size");

	filter->intensity =
		(float)obs_data_get_double(settings, "bloom_intensity") /
		100.0f;

	if (filter->threshold_type == BLOOM_THRESHOLD_TYPE_CUSTOM) {
		filter->level_red = (float)obs_data_get_double(
			settings, "bloom_red_level");
		filter->level_green = (float)obs_data_get_double(
			settings, "bloom_green_level");
		filter->level_blue = (float)obs_data_get_double(
			settings, "bloom_blue_level");
	}

	if (filter->reload_effect) {
		filter->reload_effect = false;
	}
}

void bloom_f_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "bloom_intensity", 50.0);
	obs_data_set_default_double(settings, "bloom_threshold", 70.0);
	obs_data_set_default_double(settings, "bloom_size", 6.0);
	obs_data_set_default_int(settings, "bloom_threshold_type",
				 BLOOM_THRESHOLD_TYPE_LUMINANCE);
	obs_data_set_default_double(settings, "bloom_red_level", 0.299);
	obs_data_set_default_double(settings, "bloom_green_level", 0.587);
	obs_data_set_default_double(settings, "bloom_blue_level", 0.114);
}

void bloom_f_filter_properties(retro_effects_filter_data_t *data,
			       obs_properties_t *props)
{
	UNUSED_PARAMETER(data);
	obs_property_t *p = obs_properties_add_float_slider(
		props, "bloom_intensity",
		obs_module_text("RetroEffects.Bloom.Intensity"), 0.0,
		100.0, 0.01);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_float_slider(
		props, "bloom_size",
		obs_module_text("RetroEffects.Bloom.Size"), 0.0, 100.0,
		0.01);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		props, "bloom_threshold",
		obs_module_text("RetroEffects.Bloom.Threshold"), 0.0, 100.0,
		0.01);
	obs_property_float_set_suffix(p, "%");

	obs_property_t *bloom_threshold_type_list = obs_properties_add_list(
		props, "bloom_threshold_type",
		obs_module_text("RetroEffects.Bloom.ThresholdType"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(
		bloom_threshold_type_list,
		obs_module_text(BLOOM_THRESHOLD_TYPE_LUMINANCE_LABEL),
		BLOOM_THRESHOLD_TYPE_LUMINANCE);
	obs_property_list_add_int(
		bloom_threshold_type_list,
		obs_module_text(BLOOM_THRESHOLD_TYPE_RED_LABEL),
		BLOOM_THRESHOLD_TYPE_RED);
	obs_property_list_add_int(
		bloom_threshold_type_list,
		obs_module_text(BLOOM_THRESHOLD_TYPE_GREEN_LABEL),
		BLOOM_THRESHOLD_TYPE_GREEN);
	obs_property_list_add_int(
		bloom_threshold_type_list,
		obs_module_text(BLOOM_THRESHOLD_TYPE_BLUE_LABEL),
		BLOOM_THRESHOLD_TYPE_BLUE);
	obs_property_list_add_int(
		bloom_threshold_type_list,
		obs_module_text(BLOOM_THRESHOLD_TYPE_CUSTOM_LABEL),
		BLOOM_THRESHOLD_TYPE_CUSTOM);

	obs_property_set_modified_callback2(bloom_threshold_type_list,
				threshold_type_modified,
				data->active_filter_data);

	obs_properties_t *levels_group = obs_properties_create();

	obs_properties_add_float_slider(
		levels_group, "bloom_red_level",
		obs_module_text("RetroEffects.Bloom.Levels.Red"), -1.0, 1.0,
		0.01);
	obs_properties_add_float_slider(
		levels_group, "bloom_green_level",
		obs_module_text("RetroEffects.Bloom.Levels.Green"), -1.0, 1.0, 0.01);
	obs_properties_add_float_slider(
		levels_group, "bloom_blue_level",
		obs_module_text("RetroEffects.Bloom.Levels.Blue"), -1.0, 1.0, 0.01);
	obs_properties_add_group(
		props, "bloom_levels_group",
		obs_module_text("RetroEffects.Bloom.Levels"),
		OBS_GROUP_NORMAL, levels_group);
}

void bloom_f_filter_video_render(retro_effects_filter_data_t *data)
{
	base_filter_data_t *base = data->base;
	bloom_f_filter_data_t *filter = data->active_filter_data;

	get_input_source(base);
	if (!base->input_texture_generated || filter->loading_effect) {
		base->rendering = false;
		obs_source_skip_video_filter(base->context);
		return;
	}

	gs_texture_t *image = gs_texrender_get_texture(base->input_texrender);

	data->bloom_data->brightness_threshold = filter->threshold;
	data->bloom_data->bloom_intensity = filter->intensity;
	data->bloom_data->bloom_size = filter->size;
	data->bloom_data->levels.x = filter->level_red;
	data->bloom_data->levels.y = filter->level_green;
	data->bloom_data->levels.z = filter->level_blue;

	bloom_render(image, data->bloom_data);

	gs_texrender_t *tmp = base->output_texrender;
	base->output_texrender = data->bloom_data->output;
	data->bloom_data->output = tmp;
}

static void bloom_f_set_functions(retro_effects_filter_data_t *filter)
{
	filter->filter_properties = bloom_f_filter_properties;
	filter->filter_video_render = bloom_f_filter_video_render;
	filter->filter_destroy = bloom_f_destroy;
	filter->filter_defaults = bloom_f_filter_defaults;
	filter->filter_update = bloom_f_filter_update;
	filter->filter_video_tick = NULL;
}

static bool threshold_type_modified(void *data, obs_properties_t *props,
					   obs_property_t *p,
					   obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(settings);
	setting_visibility("bloom_levels_group", false, props);
	bloom_f_filter_data_t *filter = data;
	filter->threshold_type = (uint8_t)obs_data_get_int(settings, "bloom_threshold_type");
	switch (filter->threshold_type) {
	case BLOOM_THRESHOLD_TYPE_LUMINANCE:
		filter->level_red = 0.299f;
		filter->level_green = 0.587f;
		filter->level_blue = 0.114f;
		break;
	case BLOOM_THRESHOLD_TYPE_RED:
		filter->level_red = 1.0f;
		filter->level_green = -0.5f;
		filter->level_blue = -0.5f;
		break;
	case BLOOM_THRESHOLD_TYPE_GREEN:
		filter->level_red = -0.5f;
		filter->level_green = 1.0f;
		filter->level_blue = -0.5f;
		break;
	case BLOOM_THRESHOLD_TYPE_BLUE:
		filter->level_red = -0.5f;
		filter->level_green = -0.5f;
		filter->level_blue = 1.0f;
		break;
	case BLOOM_THRESHOLD_TYPE_CUSTOM:
		setting_visibility("bloom_levels_group",true, props);
		break;
	}
	return true;
}
