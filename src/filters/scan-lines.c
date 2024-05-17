#include "scan-lines.h"
#include "../obs-utils.h"

void scan_lines_create(retro_effects_filter_data_t *filter)
{
	scan_lines_filter_data_t *data =
		bzalloc(sizeof(scan_lines_filter_data_t));
	filter->active_filter_data = data;
	data->reload_effect = false;
	scan_lines_set_functions(filter);
	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	scan_lines_filter_defaults(settings);
	obs_data_release(settings);
	scan_lines_load_effect(data);
}

void scan_lines_destroy(retro_effects_filter_data_t *filter)
{
	scan_lines_filter_data_t *data = filter->active_filter_data;
	obs_enter_graphics();
	if (data->effect_scan_lines) {
		gs_effect_destroy(data->effect_scan_lines);
	}

	obs_leave_graphics();

	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	obs_data_unset_user_value(settings, "scanlines_period");
	obs_data_unset_user_value(settings, "scanlines_offset");
	obs_data_unset_user_value(settings, "scanlines_speed");
	obs_data_unset_user_value(settings, "scanlines_profile");
	obs_data_release(settings);

	bfree(filter->active_filter_data);
	filter->active_filter_data = NULL;
}

void scan_lines_filter_update(retro_effects_filter_data_t *data,
			      obs_data_t *settings)
{
	scan_lines_filter_data_t *filter = data->active_filter_data;

	filter->period = (float)obs_data_get_double(settings, "scanlines_period");
	//filter->offset = (float)obs_data_get_double(settings, "scanlines_offset");
	filter->intensity =
		(float)obs_data_get_double(settings, "scanlines_intensity") /
		100.0f;
	filter->speed = (float)obs_data_get_double(settings, "scanlines_speed");
	filter->profile = (uint8_t)obs_data_get_int(settings, "scanlines_profile");
	if (filter->reload_effect) {
		filter->reload_effect = false;
		scan_lines_load_effect(filter);
	}
}

void scan_lines_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "scanlines_period", 30.0);
	obs_data_set_default_double(settings, "scanlines_offset", 0.0);
	obs_data_set_default_double(settings, "scanlines_speed", 50.0);
	obs_data_set_default_int(settings, "scanlines_profile", SCANLINES_PROFILE_SIN);
}

void scan_lines_filter_properties(retro_effects_filter_data_t *data,
				  obs_properties_t *props)
{
	obs_property_t *scanline_profile_list = obs_properties_add_list(
		props, "scanlines_profile",
		obs_module_text("RetroEffects.Scanlines.Profile"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(
		scanline_profile_list,
		obs_module_text(SCANLINES_PROFILE_SIN_LABEL),
		SCANLINES_PROFILE_SIN);

	obs_property_list_add_int(
		scanline_profile_list,
		obs_module_text(SCANLINES_PROFILE_SQUARE_LABEL),
		SCANLINES_PROFILE_SQUARE);

	obs_property_list_add_int(
		scanline_profile_list,
		obs_module_text(SCANLINES_PROFILE_SAWTOOTH_LABEL),
		SCANLINES_PROFILE_SAWTOOTH);

	obs_property_list_add_int(
		scanline_profile_list,
		obs_module_text(SCANLINES_PROFILE_SMOOTHSTEP_LABEL),
		SCANLINES_PROFILE_SMOOTHSTEP);

	obs_property_list_add_int(
		scanline_profile_list,
		obs_module_text(SCANLINES_PROFILE_TRIANGULAR_LABEL),
		SCANLINES_PROFILE_TRIANGULAR);

	obs_property_t *p = obs_properties_add_float_slider(
		props, "scanlines_period",
		obs_module_text("RetroEffects.Scanlines.Period"), 2.0, 2000.0,
		1.0);
	obs_property_float_set_suffix(p, " px");

	//p = obs_properties_add_float_slider(
	//	props, "scanlines_offset",
	//	obs_module_text("RetroEffects.Scanlines.Offset"), -2000.0, 2000.0,
	//	1.0);

	p = obs_properties_add_float_slider(
		props, "scanlines_speed",
		obs_module_text("RetroEffects.Scanlines.Speed"), -2000.0,
		2000.0, 1.0);
	obs_property_float_set_suffix(p, " px/s");
	
	p = obs_properties_add_float_slider(
		props, "scanlines_intensity",
		obs_module_text("RetroEffects.Scanlines.Intensity"), 0.0, 200.0, 0.1);
	obs_property_float_set_suffix(p, "%");



}

void scan_lines_filter_video_render(retro_effects_filter_data_t *data)
{
	base_filter_data_t *base = data->base;
	scan_lines_filter_data_t *filter = data->active_filter_data;

	get_input_source(base);
	if (!base->input_texture_generated || filter->loading_effect) {
		base->rendering = false;
		obs_source_skip_video_filter(base->context);
		return;
	}

	gs_texture_t *image = gs_texrender_get_texture(base->input_texrender);
	gs_effect_t *effect = filter->effect_scan_lines;

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
	if (filter->param_period) {
		gs_effect_set_float(filter->param_period, filter->period);
	}
	if (filter->param_offset) {
		gs_effect_set_float(filter->param_offset, filter->offset);
	}
	if (filter->param_intensity) {
		gs_effect_set_float(filter->param_intensity, filter->intensity);
	}

	set_render_parameters();
	set_blending_parameters();

	//const char *technique = filter->monochromatic ? "DrawMonoOrdered" : "DrawColorOrdered";
	struct dstr technique;
	switch (filter->profile) {
	case SCANLINES_PROFILE_SIN:
		dstr_init_copy(&technique, "DrawSin");
		break;
	case SCANLINES_PROFILE_SQUARE:
		dstr_init_copy(&technique, "DrawSquare");
		break;
	case SCANLINES_PROFILE_SMOOTHSTEP:
		dstr_init_copy(&technique, "DrawSmoothstep");
		break;
	case SCANLINES_PROFILE_TRIANGULAR:
		dstr_init_copy(&technique, "DrawTriangular");
		break;
	default:
		dstr_init_copy(&technique, "DrawSin");
	}

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

static void scan_lines_set_functions(retro_effects_filter_data_t *filter)
{
	filter->filter_properties = scan_lines_filter_properties;
	filter->filter_video_render = scan_lines_filter_video_render;
	filter->filter_destroy = scan_lines_destroy;
	filter->filter_defaults = scan_lines_filter_defaults;
	filter->filter_update = scan_lines_filter_update;
	filter->filter_video_tick = scan_lines_filter_video_tick;
}

void scan_lines_filter_video_tick(retro_effects_filter_data_t *data,
				   float seconds)
{
	scan_lines_filter_data_t *filter = data->active_filter_data;
	filter->offset = (float)fmod(filter->offset + seconds * filter->speed, filter->period);
}

static void scan_lines_load_effect(scan_lines_filter_data_t *filter)
{
	filter->loading_effect = true;
	if (filter->effect_scan_lines != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect_scan_lines);
		filter->effect_scan_lines = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/scan-lines.effect");
	shader_text = load_shader_from_file(filename.array);
	char *errors = NULL;
	dstr_free(&filename);

	struct dstr shader_dstr = {0};
	dstr_init_copy(&shader_dstr, shader_text);

	obs_enter_graphics();
	filter->effect_scan_lines =
		gs_effect_create(shader_dstr.array, NULL, &errors);
	obs_leave_graphics();

	dstr_free(&shader_dstr);
	bfree(shader_text);
	if (filter->effect_scan_lines == NULL) {
		blog(LOG_WARNING,
		     "[obs-retro-effects] Unable to load scan-lines.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	} else {
		size_t effect_count =
			gs_effect_get_num_params(filter->effect_scan_lines);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_scan_lines, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_image = param;
			} else if (strcmp(info.name, "uv_size") == 0) {
				filter->param_uv_size = param;
			} else if (strcmp(info.name, "period") == 0) {
				filter->param_period = param;
			} else if (strcmp(info.name, "offset") == 0) {
				filter->param_offset = param;
			} else if (strcmp(info.name, "intensity") == 0) {
				filter->param_intensity = param;
			}
		}
	}
	filter->loading_effect = false;
}
