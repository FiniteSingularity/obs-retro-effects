#include "matrix-rain.h"
#include "../obs-utils.h"
#include "../blur/blur.h"

void matrix_rain_create(retro_effects_filter_data_t *filter)
{
	matrix_rain_filter_data_t *data =
		bzalloc(sizeof(matrix_rain_filter_data_t));
	filter->active_filter_data = data;
	data->reload_effect = false;
	matrix_rain_set_functions(filter);
	struct dstr filepath = {0};
	dstr_cat(&filepath, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filepath, "/presets/matrix-rain.json");
	data->textures_data = obs_data_create_from_json_file(filepath.array);
	dstr_free(&filepath);

	dstr_init_copy(&data->custom_texture_file, "a");

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

	if (data->textures_data) {
		obs_data_release(data->textures_data);
	}

	obs_leave_graphics();

	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	obs_data_unset_user_value(settings, "matrix_rain_scale");
	obs_data_unset_user_value(settings, "matrix_rain_noise_shift");
	obs_data_unset_user_value(settings, "matrix_rain_colorize");
	obs_data_unset_user_value(settings, "matrix_rain_text_color");
	obs_data_unset_user_value(settings, "matrix_rain_background_color");
	obs_data_release(settings);

	dstr_free(&data->custom_texture_file);

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

	filter->min_brightness =
		(float)obs_data_get_double(settings, "matrix_min_brightness");

	filter->max_brightness =
		(float)obs_data_get_double(settings, "matrix_max_brightness");

	filter->min_fade_value =
		(float)obs_data_get_double(settings, "matrix_min_fade_value");

	filter->active_rain_brightness = (float)obs_data_get_double(
		settings, "matrix_active_rain_brightness");

	filter->fade_distance = fmaxf(
		(float)obs_data_get_double(settings, "matrix_fade_distance"),
		0.001f);

	filter->speed_factor =
		(float)obs_data_get_double(settings, "matrix_speed_factor");

	data->bloom_data->bloom_intensity =
		(float)obs_data_get_double(settings, "matrix_bloom_intensity");

	const float bloom_radius =
		(float)obs_data_get_double(settings, "matrix_bloom_radius");
	set_gaussian_radius(bloom_radius, data->blur_data);

	data->bloom_data->brightness_threshold =
		(float)obs_data_get_double(settings, "matrix_bloom_threshold");

	if (filter->char_set !=
	    (uint32_t)obs_data_get_int(settings, "matrix_char_set")) {
		filter->char_set =
			(uint32_t)obs_data_get_int(settings, "matrix_char_set");
		dstr_copy(&filter->custom_texture_file, "a");
		if (filter->char_set != 0) {

			obs_data_array_t *data_array = obs_data_get_array(
				filter->textures_data, "textures");

			obs_data_t *texture_dat = obs_data_array_item(
				data_array, filter->char_set - 1);

			const char *filename =
				obs_data_get_string(texture_dat, "file");
			float num_chars =
				(float)obs_data_get_int(texture_dat, "chars");

			struct dstr font_image_path = {0};
			dstr_cat(
				&font_image_path,
				obs_get_module_data_path(obs_current_module()));
			dstr_cat(&font_image_path, filename);

			set_character_texture(filter, font_image_path.array,
					      num_chars);
			dstr_free(&font_image_path);
		}
	}

	if (filter->char_set == 0) {
		filter->font_num_chars = (float)obs_data_get_int(
			settings, "matrix_rain_texture_chars");
	}

	const char *custom_texture_file =
		obs_data_get_string(settings, "matrix_rain_texture");
	if (filter->char_set == 0 && strlen(custom_texture_file) > 0 &&
	    strcmp(custom_texture_file, filter->custom_texture_file.array) !=
		    0) {
		dstr_copy(&filter->custom_texture_file, custom_texture_file);
		set_character_texture(filter, filter->custom_texture_file.array,
				      filter->font_num_chars);
	}
}

void set_character_texture(matrix_rain_filter_data_t *filter,
			   const char *filename, float num_chars)
{
	if (filter->font_image == NULL) {
		filter->font_image = bzalloc(sizeof(gs_image_file_t));
	} else {
		obs_enter_graphics();
		gs_image_file_free(filter->font_image);
		obs_leave_graphics();
	}
	if (filename) {
		gs_image_file_init(filter->font_image, filename);
		obs_enter_graphics();
		gs_image_file_init_texture(filter->font_image);
		filter->font_texture_size.x = (float)gs_texture_get_width(
			filter->font_image->texture);
		filter->font_texture_size.y = (float)gs_texture_get_height(
			filter->font_image->texture);
		obs_leave_graphics();
	}
	filter->font_num_chars = num_chars;
}

void matrix_rain_filter_defaults(obs_data_t *settings)
{
	UNUSED_PARAMETER(settings);
	obs_data_set_default_int(settings, "matrix_char_set", 1);
	obs_data_set_default_double(settings, "matrix_rain_scale", 0.40);
	obs_data_set_default_double(settings, "matrix_rain_noise_shift", 0.0);
	obs_data_set_default_bool(settings, "matrix_rain_colorize", false);
	obs_data_set_default_int(settings, "matrix_rain_text_color",
				 0xFF89f76e);
	obs_data_set_default_int(settings, "matrix_rain_background_color",
				 0xFF000000);
	obs_data_set_default_double(settings, "matrix_min_brightness", 0.0);
	obs_data_set_default_double(settings, "matrix_max_brightness", 1.0);
	obs_data_set_default_double(settings, "matrix_min_fade_value", 0.0);
	obs_data_set_default_double(settings, "matrix_active_rain_brightness",
				    0.3);
	obs_data_set_default_double(settings, "matrix_fade_distance", 0.8);
	obs_data_set_default_double(settings, "matrix_speed_factor", 1.0);
	obs_data_set_default_double(settings, "matrix_bloom_radius", 6.0);
	obs_data_set_default_double(settings, "matrix_bloom_threshold", 0.3);
	obs_data_set_default_double(settings, "matrix_bloom_intensity", 2.25);
}

void matrix_rain_filter_properties(retro_effects_filter_data_t *data,
				   obs_properties_t *props)
{
	matrix_rain_filter_data_t *filter = data->active_filter_data;

	obs_property_t *char_set_list = obs_properties_add_list(
		props, "matrix_char_set",
		obs_module_text("RetroEffects.MatrixRain.CharSet"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_data_array_t *data_array = NULL;

	data_array = obs_data_get_array(filter->textures_data, "textures");

	obs_property_list_add_int(
		char_set_list,
		obs_module_text("RetroEffects.MatrixRain.CustomCharSet"), 0);

	for (size_t i = 0; i < obs_data_array_count(data_array); i++) {
		obs_data_t *preset = obs_data_array_item(data_array, i);
		const char *name = obs_data_get_string(preset, "name");
		obs_property_list_add_int(char_set_list, name, i + 1);
		obs_data_release(preset);
	}

	obs_property_set_modified_callback(char_set_list,
					   setting_char_set_modified);

	obs_properties_t *custom_texture_group = obs_properties_create();

	obs_properties_add_path(
		custom_texture_group, "matrix_rain_texture",
		obs_module_text("RetroEffects.MatrixRain.CharacterTexture"),
		OBS_PATH_FILE,
		"Textures (*.bmp *.tga *.png *.jpeg *.jpg *.gif);;", NULL);

	obs_properties_add_int(
		custom_texture_group, "matrix_rain_texture_chars",
		obs_module_text(
			"RetroEffects.MatrixRain.CharacterTextureCount"),
		1, 255, 1);

	obs_properties_add_group(
		props, "matrix_rain_custom_texture_group",
		obs_module_text("RetroEffects.MatrixRain.CustomCharacters"),
		OBS_GROUP_NORMAL, custom_texture_group);

	obs_properties_add_float_slider(
		props, "matrix_rain_scale",
		obs_module_text("RetroEffects.MatrixRain.Scale"), 0.01, 20.0,
		0.01);

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

	obs_properties_add_float_slider(
		props, "matrix_min_brightness",
		obs_module_text("RetroEffects.MatrixRain.BlackLevel"), 0.0, 1.0,
		0.01);

	obs_properties_add_float_slider(
		props, "matrix_max_brightness",
		obs_module_text("RetroEffects.MatrixRain.WhiteLevel"), 0.0, 1.0,
		0.01);

	obs_properties_add_float_slider(
		props, "matrix_min_fade_value",
		obs_module_text("RetroEffects.MatrixRain.MinFadeValue"), 0.0,
		1.0, 0.01);

	obs_properties_add_float_slider(
		props, "matrix_active_rain_brightness",
		obs_module_text("RetroEffects.MatrixRain.ActiveRainBrightness"),
		0.0, 1.0, 0.01);

	obs_properties_add_float_slider(
		props, "matrix_fade_distance",
		obs_module_text("RetroEffects.MatrixRain.FadeDistance"), 0.0,
		1.0, 0.01);

	obs_properties_add_float_slider(
		props, "matrix_speed_factor",
		obs_module_text("RetroEffects.MatrixRain.RainSpeed"), 0.0, 10.0,
		0.01);

	obs_properties_add_float_slider(
		props, "matrix_bloom_radius",
		obs_module_text("RetroEffects.MatrixRain.BloomRadius"), 0.0,
		20.0, 0.01);

	obs_properties_add_float_slider(
		props, "matrix_bloom_threshold",
		obs_module_text("RetroEffects.MatrixRain.BloomThreshold"), 0.0,
		1.0, 0.01);

	obs_properties_add_float_slider(
		props, "matrix_bloom_intensity",
		obs_module_text("RetroEffects.MatrixRain.BloomIntensity"), 0.0,
		3.0, 0.01);
}

bool setting_char_set_modified(obs_properties_t *props,
			       obs_property_t *property, obs_data_t *settings)
{
	UNUSED_PARAMETER(property);
	uint32_t texture =
		(uint32_t)obs_data_get_int(settings, "matrix_char_set");
	setting_visibility("matrix_rain_custom_texture_group", texture == 0,
			   props);
	return true;
}

void matrix_rain_filter_video_tick(retro_effects_filter_data_t *data,
				   float seconds)
{
	matrix_rain_filter_data_t *filter = data->active_filter_data;
	filter->local_time += filter->speed_factor * seconds;
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

	//base->output_texrender =
	//	create_or_reset_texrender(base->output_texrender);
	filter->matrix_rain_texrender =
		create_or_reset_texrender(filter->matrix_rain_texrender);

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
	if (filter->param_min_brightness) {
		gs_effect_set_float(filter->param_min_brightness,
				    filter->min_brightness);
	}

	if (filter->param_max_brightness) {
		gs_effect_set_float(filter->param_max_brightness,
				    filter->max_brightness);
	}

	if (filter->param_max_brightness) {
		gs_effect_set_float(filter->param_min_fade_value,
				    filter->min_fade_value);
	}

	if (filter->param_active_rain_brightness) {
		gs_effect_set_float(filter->param_active_rain_brightness,
				    filter->active_rain_brightness);
	}

	if (filter->param_fade_distance) {
		gs_effect_set_float(filter->param_fade_distance,
				    filter->fade_distance);
	}

	set_render_parameters();
	set_blending_parameters();

	struct dstr technique;
	dstr_init_copy(&technique, "Draw");

	if (gs_texrender_begin(filter->matrix_rain_texrender, base->width,
			       base->height)) {
		gs_ortho(0.0f, (float)base->width, 0.0f, (float)base->height,
			 -100.0f, 100.0f);
		while (gs_effect_loop(effect, technique.array))
			gs_draw_sprite(image, 0, base->width, base->height);
		gs_texrender_end(filter->matrix_rain_texrender);
	}
	dstr_free(&technique);
	gs_blend_state_pop();

	gs_texture_t *matrix_rain_texture =
		gs_texrender_get_texture(filter->matrix_rain_texrender);

	data->bloom_data->brightness_threshold = 0.4f;

	bloom_render(matrix_rain_texture, data->bloom_data);

	gs_texrender_t *tmp = base->output_texrender;
	base->output_texrender = data->bloom_data->output;
	data->bloom_data->output = tmp;
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
			} else if (strcmp(info.name, "min_brightness") == 0) {
				filter->param_min_brightness = param;
			} else if (strcmp(info.name, "max_brightness") == 0) {
				filter->param_max_brightness = param;
			} else if (strcmp(info.name, "min_fade_value") == 0) {
				filter->param_min_fade_value = param;
			} else if (strcmp(info.name,
					  "active_rain_brightness") == 0) {
				filter->param_active_rain_brightness = param;
			} else if (strcmp(info.name, "fade_distance") == 0) {
				filter->param_fade_distance = param;
			}
		}
	}
	filter->loading_effect = false;
}
