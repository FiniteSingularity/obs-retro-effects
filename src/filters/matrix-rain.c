#include "matrix-rain.h"
#include "../obs-utils.h"

void matrix_rain_create(retro_effects_filter_data_t *filter)
{
	matrix_rain_filter_data_t *data =
		bzalloc(sizeof(matrix_rain_filter_data_t));
	filter->active_filter_data = data;
	data->reload_effect = false;
	matrix_rain_set_functions(filter);
	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	matrix_rain_filter_defaults(settings);
	obs_data_release(settings);
	matrix_rain_load_effect(data);
}

void matrix_rain_destroy(retro_effects_filter_data_t *filter)
{
	matrix_rain_filter_data_t *data = filter->active_filter_data;
	obs_enter_graphics();
	if (data->effect_matrix_rain) {
		gs_effect_destroy(data->effect_matrix_rain);
	}

	if (data->font_image) {
		gs_image_file_free(data->font_image);
		bfree(data->font_image);
	}

	obs_leave_graphics();

	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	obs_data_unset_user_value(settings, "matrix_rain_scale");
	obs_data_unset_user_value(settings, "matrix_rain_noise_shift");
	obs_data_unset_user_value(settings, "matrix_rain_colorize");
	obs_data_unset_user_value(settings, "matrix_rain_text_color");
	obs_data_unset_user_value(settings, "matrix_rain_background_color");
	obs_data_release(settings);

	bfree(filter->active_filter_data);
	filter->active_filter_data = NULL;
}

void matrix_rain_filter_update(retro_effects_filter_data_t *data,
			       obs_data_t *settings)
{
	matrix_rain_filter_data_t *filter = data->active_filter_data;

	if (filter->reload_effect) {
		filter->reload_effect = false;
		matrix_rain_load_effect(filter);
	}

	filter->scale =
		(float)obs_data_get_double(settings, "matrix_rain_scale");
	filter->noise_shift =
		(float)obs_data_get_double(settings, "matrix_rain_noise_shift");
	filter->colorize = obs_data_get_bool(settings, "matrix_rain_colorize");
	vec4_from_rgba(&filter->text_color,
		       (uint32_t)obs_data_get_int(settings,
						  "matrix_rain_text_color"));
	vec4_from_rgba(&filter->background_color,
		       (uint32_t)obs_data_get_int(
			       settings, "matrix_rain_background_color"));

	struct dstr font_image_path = {0};
	dstr_cat(&font_image_path,
		 obs_get_module_data_path(obs_current_module()));
	dstr_cat(&font_image_path, "/images/matrix-font-1-14-bloom.png");

	// Todo- compare if there was a change in mask_image_path
	if (1 != 0) {
		//strcpy(data->mask_image_path, mask_image_file);
		if (filter->font_image == NULL) {
			filter->font_image = bzalloc(sizeof(gs_image_file_t));
		} else {
			obs_enter_graphics();
			gs_image_file_free(filter->font_image);
			obs_leave_graphics();
		}
		if (font_image_path.len > 0 && font_image_path.array) {
			gs_image_file_init(filter->font_image,
					   font_image_path.array);
			obs_enter_graphics();
			gs_image_file_init_texture(filter->font_image);
			filter->font_texture_size.x = (float)gs_texture_get_width(filter->font_image->texture);
			filter->font_texture_size.y = (float)gs_texture_get_height(filter->font_image->texture);
			obs_leave_graphics();

		}
	}
	filter->font_num_chars = 14.0f;
	dstr_free(&font_image_path);
}

void matrix_rain_filter_defaults(obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);
}

void matrix_rain_filter_properties(retro_effects_filter_data_t *data,
				   obs_properties_t *props)
{
	UNUSED_PARAMETER(data);
	obs_properties_add_float_slider(
		props, "matrix_rain_scale",
		obs_module_text("RetroEffects.MatrixRain.Scale"), 0.1, 20.0,
		0.1);

	obs_properties_add_float_slider(
		props, "matrix_rain_noise_shift",
		obs_module_text("RetroEffects.MatrixRain.NoiseShift"), -4000.0,
		4000.0, 0.1);

	obs_properties_add_bool(
		props, "matrix_rain_colorize",
		obs_module_text("RetroEffects.MatrixRain.Colorize"));

	obs_properties_add_color_alpha(
		props, "matrix_rain_text_color",
		obs_module_text("RetroEffects.MatrixRain.TextColor"));

	obs_properties_add_color_alpha(
		props, "matrix_rain_background_color",
		obs_module_text("RetroEffects.MatrixRain.BackgroundColor"));
}

void matrix_rain_filter_video_tick(retro_effects_filter_data_t *data,
				   float seconds)
{
	matrix_rain_filter_data_t *filter = data->active_filter_data;
	filter->local_time += seconds;
}

void matrix_rain_filter_video_render(retro_effects_filter_data_t *data)
{
	base_filter_data_t *base = data->base;
	matrix_rain_filter_data_t *filter = data->active_filter_data;

	get_input_source(base);
	if (!base->input_texture_generated || filter->loading_effect) {
		base->rendering = false;
		obs_source_skip_video_filter(base->context);
		return;
	}

	gs_texture_t *image = gs_texrender_get_texture(base->input_texrender);
	gs_effect_t *effect = filter->effect_matrix_rain;

	gs_texture_t *font_texture = NULL;
	if (filter->font_image) {
		font_texture = filter->font_image->texture;
	}

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

	if (filter->param_font_image) {
		gs_effect_set_texture(filter->param_font_image, font_texture);
	}

	if (filter->param_font_texture_size) {
		gs_effect_set_vec2(filter->param_font_texture_size,
				   &filter->font_texture_size);
	}
	if (filter->param_font_texture_num_chars) {
		gs_effect_set_float(filter->param_font_texture_num_chars,
				    filter->font_num_chars);
	}
	if (filter->param_scale) {
		gs_effect_set_float(filter->param_scale, filter->scale);
	}
	if (filter->param_noise_shift) {
		gs_effect_set_float(filter->param_noise_shift,
				    filter->noise_shift);
	}
	if (filter->param_local_time) {
		gs_effect_set_float(filter->param_local_time,
				    filter->local_time);
	}
	if (filter->param_colorize) {
		gs_effect_set_bool(filter->param_colorize, filter->colorize);
	}
	if (filter->param_text_color) {
		gs_effect_set_vec4(filter->param_text_color,
				   &filter->text_color);
	}
	if (filter->param_background_color) {
		gs_effect_set_vec4(filter->param_background_color,
				   &filter->background_color);
	}

	set_render_parameters();
	set_blending_parameters();

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

static void matrix_rain_set_functions(retro_effects_filter_data_t *filter)
{
	filter->filter_properties = matrix_rain_filter_properties;
	filter->filter_video_render = matrix_rain_filter_video_render;
	filter->filter_destroy = matrix_rain_destroy;
	filter->filter_defaults = matrix_rain_filter_defaults;
	filter->filter_update = matrix_rain_filter_update;
	filter->filter_video_tick = matrix_rain_filter_video_tick;
}

static void matrix_rain_load_effect(matrix_rain_filter_data_t *filter)
{
	filter->loading_effect = true;
	if (filter->effect_matrix_rain != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect_matrix_rain);
		filter->effect_matrix_rain = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/matrix-rain.effect");
	shader_text = load_shader_from_file(filename.array);
	char *errors = NULL;
	dstr_free(&filename);

	struct dstr shader_dstr = {0};
	dstr_init_copy(&shader_dstr, shader_text);

	obs_enter_graphics();
	filter->effect_matrix_rain =
		gs_effect_create(shader_dstr.array, NULL, &errors);
	obs_leave_graphics();

	dstr_free(&shader_dstr);
	bfree(shader_text);
	if (filter->effect_matrix_rain == NULL) {
		blog(LOG_WARNING,
		     "[obs-retro-effects] Unable to load matrix-rain.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	} else {
		size_t effect_count =
			gs_effect_get_num_params(filter->effect_matrix_rain);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_matrix_rain, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_image = param;
			} else if (strcmp(info.name, "uv_size") == 0) {
				filter->param_uv_size = param;
			} else if (strcmp(info.name, "font_image") == 0) {
				filter->param_font_image = param;
			} else if (strcmp(info.name, "font_texture_size") ==
				   0) {
				filter->param_font_texture_size = param;
			} else if (strcmp(info.name,
					  "font_texture_num_chars") == 0) {
				filter->param_font_texture_num_chars = param;
			} else if (strcmp(info.name, "scale") == 0) {
				filter->param_scale = param;
			} else if (strcmp(info.name, "noise_shift") == 0) {
				filter->param_noise_shift = param;
			} else if (strcmp(info.name, "local_time") == 0) {
				filter->param_local_time = param;
			} else if (strcmp(info.name, "colorize") == 0) {
				filter->param_colorize = param;
			} else if (strcmp(info.name, "text_color") == 0) {
				filter->param_text_color = param;
			} else if (strcmp(info.name, "background_color") == 0) {
				filter->param_background_color = param;
			}
		}
	}
	filter->loading_effect = false;
}
