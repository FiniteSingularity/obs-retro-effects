#include "dither.h"
#include "../obs-utils.h"

void dither_create(retro_effects_filter_data_t *filter)
{
	dither_filter_data_t *data = bzalloc(sizeof(dither_filter_data_t));
	filter->active_filter_data = data;
	data->reload_effect = false;
	dither_set_functions(filter);
	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	dither_filter_defaults(settings);
	obs_data_release(settings);
	dither_load_effect(data);
}

void dither_destroy(retro_effects_filter_data_t *filter)
{
	dither_filter_data_t *data = filter->active_filter_data;
	obs_enter_graphics();
	if (data->effect_dither) {
		gs_effect_destroy(data->effect_dither);
	}

	obs_leave_graphics();

	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	obs_data_unset_user_value(settings, "dither_size");
	obs_data_unset_user_value(settings, "dither_type");
	obs_data_unset_user_value(settings, "dither_bayer_size");
	obs_data_unset_user_value(settings, "dither_color_steps");
	obs_data_unset_user_value(settings, "dither_mono");
	obs_data_unset_user_value(settings, "dither_round");
	obs_data_unset_user_value(settings, "dither_contrast");
	obs_data_unset_user_value(settings, "dither_gamma");
	obs_data_unset_user_value(settings, "dither_offset_x");
	obs_data_unset_user_value(settings, "dither_offset_y");
	obs_data_release(settings);

	bfree(filter->active_filter_data);
	filter->active_filter_data = NULL;
}

void dither_filter_update(retro_effects_filter_data_t *data,
					obs_data_t *settings)
{
	dither_filter_data_t *filter = data->active_filter_data;
	filter->dither_size = (float)obs_data_get_double(settings, "dither_size");
	filter->dither_type = (uint32_t)obs_data_get_int(settings, "dither_type");
	filter->bayer_size = (uint32_t)obs_data_get_int(settings, "dither_bayer_size");
	filter->color_steps = (uint32_t)obs_data_get_int(settings, "dither_color_steps");
	filter->monochromatic = obs_data_get_bool(settings, "dither_mono");
	filter->round_to_pixel = obs_data_get_bool(settings, "dither_round");
	filter->contrast = (float)obs_data_get_double(settings, "dither_contrast") * 255.0f;
	filter->gamma = (float)obs_data_get_double(settings, "dither_gamma");
	filter->offset.x = (float)obs_data_get_double(settings, "dither_offset_x");
	filter->offset.y = (float)obs_data_get_double(settings, "dither_offset_y");
	if (filter->reload_effect) {
		filter->reload_effect = false;
		dither_load_effect(filter);
	}
}

void dither_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "dither_color_steps", 1);
	obs_data_set_default_int(settings, "dither_type", DITHER_TYPE_ORDERED);
	obs_data_set_default_int(settings, "dither_bayer_size", 4);
	obs_data_set_default_bool(settings, "dither_mono", false);
	obs_data_set_default_bool(settings, "dither_round", true);
	obs_data_set_default_double(settings, "dither_size", 1.0);
	obs_data_set_default_double(settings, "dither_contrast", 0.0);
	obs_data_set_default_double(settings, "dither_gamma", 1.0);
	obs_data_set_default_double(settings, "dither_offset_x", 0.0);
	obs_data_set_default_double(settings, "dither_offset_y", 1.0);
}

void dither_filter_properties(retro_effects_filter_data_t *data,
					    obs_properties_t *props)
{

	// Dither Type
	obs_property_t *dither_type = obs_properties_add_list(
		props, "dither_type", obs_module_text("RetroEffects.Dither.Type"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(dither_type,
				  obs_module_text(DITHER_TYPE_ORDERED_LABEL),
				  DITHER_TYPE_ORDERED);
	obs_property_list_add_int(dither_type,
				  obs_module_text(DITHER_TYPE_UNORDERED_LABEL),
				  DITHER_TYPE_UNORDERED);
	obs_property_set_modified_callback(dither_type,
					   dither_type_modified);

	obs_property_t *bayer_size = obs_properties_add_list(
		props, "dither_bayer_size",
		obs_module_text("RetroEffects.Dither.BayerSize"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(bayer_size, "2x2", 2);
	obs_property_list_add_int(bayer_size, "4x4", 4);
	obs_property_list_add_int(bayer_size, "8x8", 8);

	obs_property_set_modified_callback2(bayer_size,
					   dither_bayer_size_modified, data->active_filter_data);

	// Dither Size
	obs_property_t *p = obs_properties_add_float_slider(
		props, "dither_size",
		obs_module_text("RetroEffects.Dither.Size"), 0.4, 10.0,
		0.1);
	obs_property_float_set_suffix(p, "px");



	obs_properties_add_int_slider(
		props, "dither_color_steps",
		obs_module_text("RetroEffects.Dither.ColorSteps"), 1, 32, 1);

	obs_properties_add_bool(props, "dither_mono", obs_module_text("RetroEffects.Dither.Monochromatic"));

	obs_property_t * dither_round = obs_properties_add_bool(
		props, "dither_round",
		obs_module_text("RetroEffects.Dither.RoundToPixel"));
	obs_property_set_modified_callback2(dither_round,
					   dither_round_to_pixel_modified, data->active_filter_data);

	// Contrast
	obs_properties_add_float_slider(
		props, "dither_contrast",
		obs_module_text("RetroEffects.Dither.Contrast"), -1.0, 1.0, 0.1);
	// Gamma
	obs_properties_add_float_slider(
		props, "dither_gamma",
		obs_module_text("RetroEffects.Dither.Gamma"), 0.25, 2.0, 0.025);

	p = obs_properties_add_float_slider(
		props,
		"dither_offset_x", obs_module_text("RetroEffects.Dither.OffsetX"),
		-4000.0, 4000.0, 1.0);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		props, "dither_offset_y",
		obs_module_text("RetroEffects.Dither.OffsetY"), -4000.0, 4000.0,
		1.0);
	obs_property_float_set_suffix(p, "px");
}

void dither_filter_video_render(retro_effects_filter_data_t *data)
{
	base_filter_data_t *base = data->base;
	dither_filter_data_t *filter = data->active_filter_data;

	get_input_source(base);
	if (!base->input_texture_generated || filter->loading_effect) {
		base->rendering = false;
		obs_source_skip_video_filter(base->context);
		return;
	}

	gs_texture_t *image = gs_texrender_get_texture(base->input_texrender);
	gs_effect_t *effect = filter->effect_dither;

	if (!effect || !image) {
		return;
	}

	base->output_texrender =
		create_or_reset_texrender(base->output_texrender);

	if (filter->param_uv_size) {
		struct vec2 uv_size;
		uv_size.x = (float)base->width;
		uv_size.y = (float)base->height;
		gs_effect_set_vec2(filter->param_uv_size, &uv_size);
	}
	if (filter->param_image) {
		gs_effect_set_texture(filter->param_image, image);
	}
	if (filter->param_offset) {
		gs_effect_set_vec2(filter->param_offset, &filter->offset);
	}
	if (filter->param_dither_size) {
		gs_effect_set_float(filter->param_dither_size, filter->dither_size);
	}
	if (filter->param_contrast) {
		gs_effect_set_float(filter->param_contrast,
				    filter->contrast);
	}
	if (filter->param_gamma) {
		gs_effect_set_float(filter->param_gamma,
				    filter->gamma);
	}
	if (filter->param_color_steps) {
		gs_effect_set_float(filter->param_color_steps,
				    (float)filter->color_steps);
	}

	set_render_parameters();
	set_blending_parameters();

	//const char *technique = filter->monochromatic ? "DrawMonoOrdered" : "DrawColorOrdered";
	struct dstr technique;
	dstr_init_copy(&technique, "Draw");
	dstr_cat(&technique, filter->monochromatic ? "Mono" : "Color");
	dstr_cat(&technique,
		 filter->dither_type == DITHER_TYPE_ORDERED ? "Ordered" : "BN");

	if (gs_texrender_begin(base->output_texrender, base->width,
			       base->height)) {
		gs_ortho(0.0f, (float)base->width, 0.0f, (float)base->height,
			 -100.0f, 100.0f);
		while (gs_effect_loop(effect, technique.array))
			gs_draw_sprite(image, 0, base->width, base->height);
		gs_texrender_end(base->output_texrender);
	}
	dstr_free(&technique);
	gs_blend_state_pop();
}

static void
dither_set_functions(retro_effects_filter_data_t *filter)
{
	filter->filter_properties = dither_filter_properties;
	filter->filter_video_render = dither_filter_video_render;
	filter->filter_destroy = dither_destroy;
	filter->filter_defaults = dither_filter_defaults;
	filter->filter_update = dither_filter_update;
	filter->filter_video_tick = NULL;
}

static void dither_load_effect(dither_filter_data_t *filter)
{
	filter->loading_effect = true;
	if (filter->effect_dither != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect_dither);
		filter->effect_dither = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/dither-blue-noise.effect");
	shader_text = load_shader_from_file(filename.array);
	char *errors = NULL;
	dstr_free(&filename);

	struct dstr shader_dstr = {0};
	dstr_init_copy(&shader_dstr, "#define USE_BAYER");
	int bayer_size = filter->bayer_size > 0 ? filter->bayer_size : 4;
	dstr_catf(&shader_dstr, "%i\n", bayer_size);

	if (filter->round_to_pixel) {
		dstr_cat(&shader_dstr, "#define ROUND_UV_TO_PIXEL\n");
	}

	dstr_cat(&shader_dstr, shader_text);

	obs_enter_graphics();
	filter->effect_dither =
		gs_effect_create(shader_dstr.array, NULL, &errors);
	obs_leave_graphics();

	dstr_free(&shader_dstr);
	bfree(shader_text);
	if (filter->effect_dither == NULL) {
		blog(LOG_WARNING,
		     "[obs-retro-effects] Unable to load dither-blue-noise.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	} else {
		size_t effect_count = gs_effect_get_num_params(
			filter->effect_dither);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_dither,
				effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_image = param;
			} else if (strcmp(info.name, "uv_size") == 0) {
				filter->param_uv_size = param;
			} else if (strcmp(info.name, "dither_size") == 0) {
				filter->param_dither_size = param;
			} else if (strcmp(info.name, "contrast") == 0) {
				filter->param_contrast = param;
			} else if (strcmp(info.name, "gamma") == 0) {
				filter->param_gamma = param;
			} else if (strcmp(info.name, "offset") == 0) {
				filter->param_offset = param;
			} else if (strcmp(info.name, "color_steps") == 0) {
				filter->param_color_steps = param;
			}
		}
	}
	filter->loading_effect = false;
}

static bool dither_bayer_size_modified(void* data, obs_properties_t *props,
				       obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(settings);
	dither_filter_data_t *filter = data;
	filter->reload_effect = true;
	return false;
}

static bool dither_round_to_pixel_modified(void* data, obs_properties_t *props,
				       obs_property_t *p, obs_data_t *settings)
{
	UNUSED_PARAMETER(p);
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(settings);
	dither_filter_data_t *filter = data;
	filter->reload_effect = true;
	return false;
}

static bool dither_type_modified(obs_properties_t *props,
					 obs_property_t *p,
					 obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	int dither_type = (int)obs_data_get_int(settings, "dither_type");
	switch (dither_type) {
	case DITHER_TYPE_ORDERED:
		setting_visibility("dither_offset_x", false, props);
		setting_visibility("dither_offset_y", false, props);
		setting_visibility("dither_bayer_size", true, props);
		break;
	case DITHER_TYPE_UNORDERED:
		setting_visibility("dither_offset_x", true, props);
		setting_visibility("dither_offset_y", true, props);
		setting_visibility("dither_bayer_size", false, props);
		break;
	}
	return true;
}
