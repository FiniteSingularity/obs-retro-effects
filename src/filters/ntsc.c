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
	obs_data_release(settings);

	bfree(filter->active_filter_data);
	filter->active_filter_data = NULL;
}

void ntsc_filter_update(retro_effects_filter_data_t *data, obs_data_t *settings)
{
	ntsc_filter_data_t *filter = data->active_filter_data;
}

void ntsc_filter_defaults(obs_data_t *settings) {}

void ntsc_filter_properties(retro_effects_filter_data_t *data,
			    obs_properties_t *props)
{
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

	set_render_parameters();
	set_blending_parameters();

	dstr_init_copy(&technique, "Draw");

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
	filter->filter_video_tick = NULL;
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
	filter->effect_ntsc_encode =
		gs_effect_create(shader_dstr.array, NULL, &errors);
	obs_leave_graphics();

	dstr_free(&shader_dstr);
	bfree(shader_text);

	if (filter->effect_ntsc_encode == NULL) {
		blog(LOG_WARNING,
		     "[obs-composite-blur] Unable to load ntsc-encode.effect file.  Errors:\n%s",
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
	filter->effect_ntsc_decode =
		gs_effect_create(shader_dstr.array, NULL, &errors);
	obs_leave_graphics();

	dstr_free(&shader_dstr);
	bfree(shader_text);

	if (filter->effect_ntsc_decode == NULL) {
		blog(LOG_WARNING,
		     "[obs-composite-blur] Unable to load ntsc-decode.effect file.  Errors:\n%s",
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
			}
		}
	}
}
