#include "ntsc.h"
#include "../obs-utils.h"

void ntsc_create(retro_effects_filter_data_t *filter)
{
	ntsc_filter_data_t *data = bzalloc(sizeof(ntsc_filter_data_t));
	filter->active_filter_data = data;
	ntsc_set_functions(filter);
	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	ntsc_filter_defaults(settings);
	obs_data_release(settings);
	ntsc_load_effects(data);
}

void ntsc_destroy(retro_effects_filter_data_t *filter)
{
	ntsc_filter_data_t *data = filter->active_filter_data;
	obs_enter_graphics();
	if (data->effect_ntsc_encode) {
		gs_effect_destroy(data->effect_ntsc_encode);
	}
	if (data->effect_ntsc_decode) {
		gs_effect_destroy(data->effect_ntsc_decode);
	}

	if (data->encode_texrender) {
		gs_texrender_destroy(data->encode_texrender);
	}

	obs_leave_graphics();

	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	obs_data_unset_user_value(settings, "ntsc_tuning_offset");
	obs_data_unset_user_value(settings, "ntsc_luma_noise");
	obs_data_unset_user_value(settings, "ntsc_luma_band_size");
	obs_data_unset_user_value(settings, "ntsc_luma_band_strength");
	obs_data_unset_user_value(settings, "ntsc_luma_band_count");
	obs_data_unset_user_value(settings, "ntsc_chroma_bleed_size");
	obs_data_unset_user_value(settings, "ntsc_chroma_bleed_strength");
	obs_data_unset_user_value(settings, "ntsc_chroma_bleed_steps");
	obs_data_unset_user_value(settings, "ntsc_brightness");
	obs_data_unset_user_value(settings, "ntsc_saturation");

	obs_data_release(settings);

	bfree(filter->active_filter_data);
	filter->active_filter_data = NULL;
}

void ntsc_filter_update(retro_effects_filter_data_t *data, obs_data_t *settings)
{
	ntsc_filter_data_t *filter = data->active_filter_data;
	filter->tuning_offset = (float)obs_data_get_double(settings, "ntsc_tuning_offset");
	filter->luma_noise =
		(float)obs_data_get_double(settings, "ntsc_luma_noise")/100.0f;
	filter->luma_band_size =
		(float)obs_data_get_double(settings, "ntsc_luma_band_size");
	filter->luma_band_strength =
		(float)obs_data_get_double(settings, "ntsc_luma_band_strength")/400.0f;
	filter->luma_band_count =
		(int)obs_data_get_int(settings, "ntsc_luma_band_count") * 2 + 1;

	filter->chroma_bleed_size =
		(float)obs_data_get_double(settings, "ntsc_chroma_bleed_size");
	filter->chroma_bleed_strength =
		(float)obs_data_get_double(settings, "ntsc_chroma_bleed_strength") / 100.0f;
	filter->chroma_bleed_steps =
		(int)obs_data_get_int(settings, "ntsc_chroma_bleed_steps");

	filter->brightness = (float)obs_data_get_double(settings, "ntsc_brightness") / 100.0f;
	filter->saturation = (float)obs_data_get_double(settings, "ntsc_saturation") / 100.0f;
}

void ntsc_filter_defaults(obs_data_t *settings) {
	obs_data_set_default_double(settings, "ntsc_tuning_offset", 1.0);
	obs_data_set_default_double(settings, "ntsc_luma_noise", 0.0);
	obs_data_set_default_double(settings, "ntsc_luma_band_size", 10.0);
	obs_data_set_default_double(settings, "ntsc_luma_band_strength", 70.0);
	obs_data_set_default_int(settings, "ntsc_luma_band_count", 1);

	obs_data_set_default_double(settings, "ntsc_chroma_bleed_size", 50.0);
	obs_data_set_default_double(settings, "ntsc_chroma_bleed_strength", 70.0);
	obs_data_set_default_int(settings, "ntsc_chroma_bleed_steps", 15);

	obs_data_set_default_double(settings, "ntsc_brightness", 100.0);
	obs_data_set_default_double(settings, "ntsc_saturation", 100.0);
}

void ntsc_filter_properties(retro_effects_filter_data_t *data,
			    obs_properties_t *props)
{
	UNUSED_PARAMETER(data);

	obs_properties_add_float_slider(
		props, "ntsc_tuning_offset",
		obs_module_text("RetroEffects.NTSC.TuningOffset"), -1000.0, 1000.0, 0.1);
	obs_property_t *p = NULL;

	obs_properties_t *luma_settings = obs_properties_create();

	p = obs_properties_add_float_slider(
		luma_settings, "ntsc_luma_noise",
		obs_module_text("RetroEffects.NTSC.LumaNoise"), 0.0, 100.0,
		0.1);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_float_slider(
		luma_settings, "ntsc_luma_band_size",
		obs_module_text("RetroEffects.NTSC.LumaBandSize"), 0.0, 100.0,
		0.1);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		luma_settings, "ntsc_luma_band_strength",
		obs_module_text("RetroEffects.NTSC.LumaBandStrength"), 0.0, 400.0,
		0.1);
	obs_property_float_set_suffix(p, "%");

	obs_properties_add_int_slider(
		luma_settings, "ntsc_luma_band_count",
		obs_module_text("RetroEffects.NTSC.LumaBandCount"), 1,
		8, 1);

	obs_properties_add_group(
		props, "ntsc_luma_settings",
		obs_module_text("RetroEffects.NTSC.Luma"),
		OBS_GROUP_NORMAL, luma_settings);

	obs_properties_t *chroma_settings = obs_properties_create();

	p = obs_properties_add_float_slider(
		chroma_settings, "ntsc_chroma_bleed_size",
		obs_module_text("RetroEffects.NTSC.ChromaBleedSize"), 0.0, 100.0,
		0.1);
	obs_property_float_set_suffix(p, "px");

	obs_properties_add_int_slider(
		chroma_settings, "ntsc_chroma_bleed_steps",
		obs_module_text("RetroEffects.NTSC.ChromaBleedSteps"), 1, 30, 1);

	p = obs_properties_add_float_slider(
		chroma_settings, "ntsc_chroma_bleed_strength",
		obs_module_text("RetroEffects.NTSC.ChromaBleedStrength"), 0.0,
		100.0, 0.1);
	obs_property_float_set_suffix(p, "%");

	obs_properties_add_group(props, "ntsc_chroma_settings",
		obs_module_text("RetroEffects.NTSC.Chroma"),
		OBS_GROUP_NORMAL, chroma_settings);

	obs_properties_t *picture_adjustment = obs_properties_create();

	p = obs_properties_add_float_slider(
		picture_adjustment, "ntsc_brightness",
		obs_module_text("RetroEffects.NTSC.Brightness"),
		0.0, 200.0, 0.1);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_float_slider(
		picture_adjustment, "ntsc_saturation",
		obs_module_text("RetroEffects.NTSC.Saturation"), 0.0, 200.0,
		0.1);
	obs_property_float_set_suffix(p, "%");

	obs_properties_add_group(props, "ntsc_picture",
		obs_module_text("RetroEffects.NTSC.Picture"),
		OBS_GROUP_NORMAL, picture_adjustment);
}

void ntsc_filter_video_tick(retro_effects_filter_data_t *data, float seconds)
{
	UNUSED_PARAMETER(seconds);
	ntsc_filter_data_t *filter = data->active_filter_data;

	float height = (float)data->base->height * 1.10f;
	if (filter->tuning_offset <= 20.0f) {
		filter->y_offset = floorf(filter->y_offset / 1.8f);
		return;
	}

	float y_increment =
		(filter->tuning_offset-20.0f) * ((height) / 400.0f);
	filter->y_offset += y_increment;
	filter->y_offset = fmodf(filter->y_offset, height);
}

void ntsc_filter_video_render(retro_effects_filter_data_t *data)
{
	base_filter_data_t *base = data->base;
	ntsc_filter_data_t *filter = data->active_filter_data;
	get_input_source(base);
	if (!base->input_texture_generated || filter->loading_effect) {
		base->rendering = false;
		obs_source_skip_video_filter(base->context);
		return;
	}

	gs_texture_t *image = gs_texrender_get_texture(base->input_texrender);
	gs_effect_t *effect_encode = filter->effect_ntsc_encode;
	gs_effect_t *effect_decode = filter->effect_ntsc_decode;

	if (!effect_decode || !effect_encode || !image) {
		return;
	}

	// Pass 1- Encode NTSC Signal from RGB
	filter->encode_texrender =
		create_or_reset_texrender(filter->encode_texrender);

	if (filter->param_encode_uv_size) {
		struct vec2 uv_size;
		uv_size.x = (float)base->width;
		uv_size.y = (float)base->height;
		gs_effect_set_vec2(filter->param_encode_uv_size, &uv_size);
	}
	if (filter->param_encode_image) {
		gs_effect_set_texture(filter->param_encode_image, image);
	}
	if (filter->param_encode_tuning_offset) {
		gs_effect_set_float(filter->param_encode_tuning_offset,
				    filter->tuning_offset);
	}
	if (filter->param_encode_frame) {
		gs_effect_set_float(filter->param_encode_frame,
				    (float)base->frame);
	}

	if (filter->param_encode_y_offset) {
		gs_effect_set_float(filter->param_encode_y_offset,
				    fmaxf(0.0f, filter->y_offset - 20.0f));
	}

	if (filter->param_encode_luma_noise) {
		gs_effect_set_float(filter->param_encode_luma_noise,
				    filter->luma_noise);
	}

	set_render_parameters();
	set_blending_parameters();

	struct dstr technique;
	dstr_init_copy(&technique, "Draw");


	if (gs_texrender_begin(filter->encode_texrender, base->width,
			       base->height)) {
		gs_ortho(0.0f, (float)base->width, 0.0f, (float)base->height,
			 -100.0f, 100.0f);
		while (gs_effect_loop(effect_encode, technique.array))
			gs_draw_sprite(image, 0, base->width, base->height);
		gs_texrender_end(filter->encode_texrender);
	}
	gs_blend_state_pop();

	// Pass 2- Decode NTSC Signal to RGB
	base->output_texrender =
		create_or_reset_texrender(base->output_texrender);

	gs_texture_t *image_encoded =
		gs_texrender_get_texture(filter->encode_texrender);

	if (filter->param_decode_uv_size) {
		struct vec2 uv_size;
		uv_size.x = (float)base->width;
		uv_size.y = (float)base->height;
		gs_effect_set_vec2(filter->param_decode_uv_size, &uv_size);
	}
	if (filter->param_decode_image) {
		gs_effect_set_texture(filter->param_decode_image,
				      image_encoded);
	}

	if (filter->param_decode_luma_band_size) {
		gs_effect_set_float(filter->param_decode_luma_band_size,
				    filter->luma_band_size);
	}
	if (filter->param_decode_luma_band_strength) {
		gs_effect_set_float(filter->param_decode_luma_band_strength,
				    filter->luma_band_strength);
	}
	if (filter->param_decode_luma_band_count) {
		gs_effect_set_int(filter->param_decode_luma_band_count,
				    filter->luma_band_count);
	}

	if (filter->param_decode_chroma_bleed_size) {
		gs_effect_set_float(filter->param_decode_chroma_bleed_size,
				    filter->chroma_bleed_size);
	}
	if (filter->param_decode_chroma_bleed_strength) {
		gs_effect_set_float(filter->param_decode_chroma_bleed_strength,
				    filter->chroma_bleed_strength);
	}
	if (filter->param_decode_chroma_bleed_steps) {
		gs_effect_set_int(filter->param_decode_chroma_bleed_steps,
				  filter->chroma_bleed_steps);
	}
	if (filter->param_decode_brightness) {
		gs_effect_set_float(filter->param_decode_brightness,
				    filter->brightness);
	}
	if (filter->param_decode_chroma_bleed_steps) {
		gs_effect_set_float(filter->param_decode_saturation,
				  filter->saturation);
	}

	set_render_parameters();
	set_blending_parameters();

	dstr_copy(&technique, "Draw");

	if (gs_texrender_begin(base->output_texrender, base->width,
			       base->height)) {
		gs_ortho(0.0f, (float)base->width, 0.0f, (float)base->height,
			 -100.0f, 100.0f);
		while (gs_effect_loop(effect_decode, technique.array))
			gs_draw_sprite(image, 0, base->width, base->height);
		gs_texrender_end(base->output_texrender);
	}
	dstr_free(&technique);
	gs_blend_state_pop();
}

static void ntsc_set_functions(retro_effects_filter_data_t *filter)
{
	filter->filter_properties = ntsc_filter_properties;
	filter->filter_video_render = ntsc_filter_video_render;
	filter->filter_destroy = ntsc_destroy;
	filter->filter_defaults = ntsc_filter_defaults;
	filter->filter_update = ntsc_filter_update;
	filter->filter_video_tick = ntsc_filter_video_tick;
}

static void ntsc_load_effects(ntsc_filter_data_t *filter)
{
	filter->loading_effect = true;
	ntsc_load_effect_encode(filter);
	ntsc_load_effect_decode(filter);
	filter->loading_effect = false;
}

static void ntsc_load_effect_encode(ntsc_filter_data_t *filter)
{
	if (filter->effect_ntsc_encode != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect_ntsc_encode);
		filter->effect_ntsc_encode = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/ntsc-encode.effect");
	shader_text = load_shader_from_file(filename.array);
	char *errors = NULL;
	dstr_free(&filename);

	struct dstr shader_dstr = {0};
	dstr_init_copy(&shader_dstr, shader_text);

	obs_enter_graphics();
	int device_type = gs_get_device_type();
	if (device_type == GS_DEVICE_OPENGL) {
		dstr_insert(&shader_dstr, 0, "#define OPENGL 1\n");
	}
	filter->effect_ntsc_encode =
		gs_effect_create(shader_dstr.array, NULL, &errors);
	obs_leave_graphics();

	dstr_free(&shader_dstr);
	bfree(shader_text);

	if (filter->effect_ntsc_encode == NULL) {
		blog(LOG_WARNING,
		     "[obs-retro-effects] Unable to load ntsc-encode.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	} else {
		size_t effect_count =
			gs_effect_get_num_params(filter->effect_ntsc_encode);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_ntsc_encode, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_encode_image = param;
			} else if (strcmp(info.name, "uv_size") == 0) {
				filter->param_encode_uv_size = param;
			} else if (strcmp(info.name, "tuning_offset") == 0) {
				filter->param_encode_tuning_offset = param;
			} else if (strcmp(info.name, "frame") == 0) {
				filter->param_encode_frame = param;
			} else if (strcmp(info.name, "y_offset") == 0) {
				filter->param_encode_y_offset = param;
			} else if (strcmp(info.name, "luma_noise") == 0) {
				filter->param_encode_luma_noise = param;
			}
		}
	}
}

static void ntsc_load_effect_decode(ntsc_filter_data_t *filter)
{
	if (filter->effect_ntsc_decode != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect_ntsc_decode);
		filter->effect_ntsc_decode = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/ntsc-decode.effect");
	shader_text = load_shader_from_file(filename.array);
	char *errors = NULL;
	dstr_free(&filename);

	struct dstr shader_dstr = {0};
	dstr_init_copy(&shader_dstr, shader_text);

	obs_enter_graphics();
	int device_type = gs_get_device_type();
	if (device_type == GS_DEVICE_OPENGL) {
		dstr_insert(&shader_dstr, 0, "#define OPENGL 1\n");
	}
	filter->effect_ntsc_decode =
		gs_effect_create(shader_dstr.array, NULL, &errors);
	obs_leave_graphics();

	dstr_free(&shader_dstr);
	bfree(shader_text);

	if (filter->effect_ntsc_decode == NULL) {
		blog(LOG_WARNING,
		     "[obs-retro-effects] Unable to load ntsc-decode.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	} else {
		size_t effect_count =
			gs_effect_get_num_params(filter->effect_ntsc_decode);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_ntsc_decode, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_decode_image = param;
			} else if (strcmp(info.name, "uv_size") == 0) {
				filter->param_decode_uv_size = param;
			} else if (strcmp(info.name, "luma_band_size") == 0) {
				filter->param_decode_luma_band_size = param;
			} else if (strcmp(info.name, "luma_band_strength") == 0) {
				filter->param_decode_luma_band_strength = param;
			} else if (strcmp(info.name, "luma_band_count") == 0) {
				filter->param_decode_luma_band_count = param;
			} else if (strcmp(info.name, "chroma_bleed_size") == 0) {
				filter->param_decode_chroma_bleed_size = param;
			} else if (strcmp(info.name, "chroma_bleed_strength") == 0) {
				filter->param_decode_chroma_bleed_strength = param;
			} else if (strcmp(info.name, "chroma_bleed_steps") == 0) {
				filter->param_decode_chroma_bleed_steps = param;
			} else if (strcmp(info.name, "brightness") == 0) {
				filter->param_decode_brightness = param;
			} else if (strcmp(info.name, "saturation") == 0) {
				filter->param_decode_saturation = param;
			}
		}
	}
}
