#include "cathode-boot.h"
#include "../obs-utils.h"

void cathode_boot_create(retro_effects_filter_data_t *filter)
{
	cathode_boot_filter_data_t *data =
		bzalloc(sizeof(cathode_boot_filter_data_t));
	filter->active_filter_data = data;
	data->reload_effect = false;
	cathode_boot_set_functions(filter);
	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	cathode_boot_filter_defaults(settings);
	obs_data_release(settings);
	cathode_boot_load_effect(data);
}

void cathode_boot_destroy(retro_effects_filter_data_t *filter)
{
	cathode_boot_filter_data_t *data = filter->active_filter_data;
	obs_enter_graphics();
	if (data->effect_cathode_boot) {
		gs_effect_destroy(data->effect_cathode_boot);
	}

	obs_leave_graphics();

	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	obs_data_unset_user_value(settings, "cathode_boot_progress");
	obs_data_unset_user_value(settings, "cathode_boot_vert_start");
	obs_data_unset_user_value(settings, "cathode_boot_vert_end");
	obs_data_unset_user_value(settings, "cathode_boot_horiz_start");
	obs_data_unset_user_value(settings, "cathode_boot_horiz_end");
	obs_data_unset_user_value(settings, "cathode_boot_fade_start");
	obs_data_unset_user_value(settings, "cathode_boot_fade_end");
	obs_data_release(settings);

	bfree(filter->active_filter_data);
	filter->active_filter_data = NULL;
}

void cathode_boot_filter_update(retro_effects_filter_data_t *data,
				obs_data_t *settings)
{
	cathode_boot_filter_data_t *filter = data->active_filter_data;
	if (filter->reload_effect) {
		filter->reload_effect = false;
		cathode_boot_load_effect(filter);
	}
	filter->progress = (float)obs_data_get_double(settings, "cathode_boot_progress") / 100.0f;
	filter->vert_range.x = (float)obs_data_get_double(settings, "cathode_boot_vert_start") / 100.0f;
	filter->vert_range.y = (float)obs_data_get_double(settings, "cathode_boot_vert_end") / 100.0f;
	filter->horiz_range.x = (float)obs_data_get_double(settings, "cathode_boot_horiz_start") / 100.0f;
	filter->horiz_range.y = (float)obs_data_get_double(settings, "cathode_boot_horiz_end") / 100.0f;
	filter->fade_range.x = (float)obs_data_get_double(settings, "cathode_boot_fade_start") / 100.0f;
	filter->fade_range.y = (float)obs_data_get_double(settings, "cathode_boot_fade_end") / 100.0f;
}

void cathode_boot_filter_defaults(obs_data_t *settings) {
	obs_data_set_default_double(settings, "cathode_boot_progress", 0.0);
	obs_data_set_default_double(settings, "cathode_boot_vert_start", 0.0);
	obs_data_set_default_double(settings, "cathode_boot_vert_end", 70.0);
	obs_data_set_default_double(settings, "cathode_boot_horiz_start", 50.0);
	obs_data_set_default_double(settings, "cathode_boot_horiz_end", 90.0);
	obs_data_set_default_double(settings, "cathode_boot_fade_start", 90.0);
	obs_data_set_default_double(settings, "cathode_boot_fade_end", 100.0);
}

void cathode_boot_filter_properties(retro_effects_filter_data_t *data,
				    obs_properties_t *props)
{
	obs_property_t *p = obs_properties_add_float_slider(
		props, "cathode_boot_progress",
		obs_module_text("RetroEffects.CathodeBoot.Progress"), 0.0,
		100.0, 0.1);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_float_slider(
		props, "cathode_boot_vert_start",
		obs_module_text("RetroEffects.CathodeBoot.VertStart"), 0.0,
		100.0, 0.1);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_float_slider(
		props, "cathode_boot_vert_end",
		obs_module_text("RetroEffects.CathodeBoot.VertEnd"), 0.0,
		100.0, 0.1);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_float_slider(
		props, "cathode_boot_horiz_start",
		obs_module_text("RetroEffects.CathodeBoot.HorizStart"), 0.0,
		100.0, 0.1);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_float_slider(
		props, "cathode_boot_horiz_end",
		obs_module_text("RetroEffects.CathodeBoot.HorizEnd"), 0.0, 100.0,
		0.1);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_float_slider(
		props, "cathode_boot_fade_start",
		obs_module_text("RetroEffects.CathodeBoot.FadeStart"), 0.0,
		100.0, 0.1);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_float_slider(
		props, "cathode_boot_fade_end",
		obs_module_text("RetroEffects.CathodeBoot.FadeEnd"), 0.0,
		100.0, 0.1);
	obs_property_float_set_suffix(p, "%");

	// cathode_boot Type
}

void cathode_boot_filter_video_render(retro_effects_filter_data_t *data)
{
	base_filter_data_t *base = data->base;
	cathode_boot_filter_data_t *filter = data->active_filter_data;

	get_input_source(base);
	if (!base->input_texture_generated || filter->loading_effect) {
		base->rendering = false;
		obs_source_skip_video_filter(base->context);
		return;
	}

	gs_texture_t *image = gs_texrender_get_texture(base->input_texrender);
	gs_effect_t *effect = filter->effect_cathode_boot;

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
	if (filter->param_progress) {
		gs_effect_set_float(filter->param_progress, filter->progress);
	}

	if (filter->param_vert_range) {
		gs_effect_set_vec2(filter->param_vert_range, &filter->vert_range);
	}

	if (filter->param_horiz_range) {
		gs_effect_set_vec2(filter->param_horiz_range, &filter->horiz_range);
	}

	if (filter->param_fade_range) {
		gs_effect_set_vec2(filter->param_fade_range, &filter->fade_range);
	}

	set_render_parameters();
	set_blending_parameters();

	//const char *technique = filter->monochromatic ? "DrawMonoOrdered" : "DrawColorOrdered";
	struct dstr technique;
	dstr_init_copy(&technique, "Draw");

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

static void cathode_boot_set_functions(retro_effects_filter_data_t *filter)
{
	filter->filter_properties = cathode_boot_filter_properties;
	filter->filter_video_render = cathode_boot_filter_video_render;
	filter->filter_destroy = cathode_boot_destroy;
	filter->filter_defaults = cathode_boot_filter_defaults;
	filter->filter_update = cathode_boot_filter_update;
	filter->filter_video_tick = NULL;
}

static void cathode_boot_load_effect(cathode_boot_filter_data_t *filter)
{
	filter->loading_effect = true;
	if (filter->effect_cathode_boot != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect_cathode_boot);
		filter->effect_cathode_boot = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/cathode-boot.effect");
	shader_text = load_shader_from_file(filename.array);
	char *errors = NULL;
	dstr_free(&filename);

	struct dstr shader_dstr = {0};
	dstr_init_copy(&shader_dstr, shader_text);

	obs_enter_graphics();
	filter->effect_cathode_boot =
		gs_effect_create(shader_dstr.array, NULL, &errors);
	obs_leave_graphics();

	dstr_free(&shader_dstr);
	bfree(shader_text);
	if (filter->effect_cathode_boot == NULL) {
		blog(LOG_WARNING,
		     "[obs-retro-effects] Unable to load cathode_boot-blue-noise.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	} else {
		size_t effect_count =
			gs_effect_get_num_params(filter->effect_cathode_boot);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_cathode_boot, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_image = param;
			} else if (strcmp(info.name, "uv_size") == 0) {
				filter->param_uv_size = param;
			} else if (strcmp(info.name, "progress") == 0) {
				filter->param_progress = param;
			} else if (strcmp(info.name, "vert_range") == 0) {
				filter->param_vert_range = param;
			} else if (strcmp(info.name, "horiz_range") == 0) {
				filter->param_horiz_range = param;
			} else if (strcmp(info.name, "fade_range") == 0) {
				filter->param_fade_range = param;
			}
		}
	}
	filter->loading_effect = false;
}
