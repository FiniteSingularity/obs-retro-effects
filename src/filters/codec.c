#include "codec.h"
#include "../obs-utils.h"

void codec_create(retro_effects_filter_data_t *filter)
{
	codec_filter_data_t *data = bzalloc(sizeof(codec_filter_data_t));
	filter->active_filter_data = data;
	data->reload_effect = false;
	codec_set_functions(filter);
	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	codec_filter_defaults(settings);
	obs_data_release(settings);
	codec_load_effect(data);
}

void codec_destroy(retro_effects_filter_data_t *filter)
{
	codec_filter_data_t *data = filter->active_filter_data;
	obs_enter_graphics();
	if (data->effect_codec) {
		gs_effect_destroy(data->effect_codec);
	}
	if (data->texrender_downsampled_input) {
		gs_texrender_destroy(data->texrender_downsampled_input);
	}
	if (data->texrender_previous_frame) {
		gs_texrender_destroy(data->texrender_previous_frame);
	}
	if (data->texrender_downsampled_output) {
		gs_texrender_destroy(data->texrender_downsampled_output);
	}
	obs_leave_graphics();

	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	obs_data_unset_user_value(settings, "codec_px_scale");
	obs_data_unset_user_value(settings, "codec_colors_per_channel");
	obs_data_unset_user_value(settings, "codec_quality");
	obs_data_unset_user_value(settings, "codec_custom_thresholds");
	obs_data_unset_user_value(settings, "codec_rpza_threshold_prev_frame");
	obs_data_unset_user_value(settings, "codec_rpza_threshold_solid");
	obs_data_unset_user_value(settings, "codec_rpza_threshold_gradient");
	obs_data_release(settings);

	bfree(filter->active_filter_data);
	filter->active_filter_data = NULL;
}

float lerp(float x, float y, float a)
{
	return (1 - a) * x + a * y;
}

void codec_filter_update(retro_effects_filter_data_t *data,
					obs_data_t *settings)
{
	codec_filter_data_t *filter = data->active_filter_data;
	filter->codec_type = (uint32_t)obs_data_get_int(settings, "codec_type");
	filter->px_scale = (float)obs_data_get_double(settings, "codec_px_scale");
	filter->colors_per_channel = (int)obs_data_get_int(settings, "codec_colors_per_channel");
	filter->quality = (float)obs_data_get_double(settings, "codec_quality");
	filter->custom_thresholds = obs_data_get_bool(settings, "codec_custom_thresholds");
	filter->rpza_threshold_prev_frame = (float)obs_data_get_double(settings, "codec_rpza_threshold_prev_frame");
	filter->rpza_threshold_solid = (float)obs_data_get_double(settings, "codec_rpza_threshold_solid");
	filter->rpza_threshold_gradient = (float)obs_data_get_double(settings, "codec_rpza_threshold_gradient");

	if (!filter->custom_thresholds) {
		filter->rpza_threshold_prev_frame = lerp(0.5f, 0.0f, filter->quality); // TODO fine-tune
		filter->rpza_threshold_solid = lerp(0.2f, 0.0f, filter->quality);
		filter->rpza_threshold_gradient = lerp(1.0f, 0.0f, filter->quality);
	}

	if (filter->reload_effect) {
		filter->reload_effect = false;
		codec_load_effect(filter);
	}
}

void codec_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "codec_type", CODEC_TYPE_RPZA);
	obs_data_set_default_double(settings, "codec_px_scale", 5.0);
	obs_data_set_default_int(settings, "codec_colors_per_channel", 32);
	obs_data_set_default_double(settings, "codec_quality", 0.85);
	obs_data_set_default_bool(settings, "codec_custom_thresholds", false);
	obs_data_set_default_double(settings, "codec_rpza_threshold_prev_frame", 0.14);
	obs_data_set_default_double(settings, "codec_rpza_threshold_solid", 0.1);
	obs_data_set_default_double(settings, "codec_rpza_threshold_gradient", 0.18);
}

void codec_filter_properties(retro_effects_filter_data_t *data,
					    obs_properties_t *props)
{
	UNUSED_PARAMETER(data);

	// Codec Type
	obs_property_t *codec_type = obs_properties_add_list(
		props, "codec_type", obs_module_text("RetroEffects.Dither.Type"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(codec_type,
				  obs_module_text(CODEC_TYPE_RPZA_LABEL),
				  CODEC_TYPE_RPZA);
	obs_property_set_modified_callback(codec_type,
					   codec_type_modified);

	obs_properties_add_float_slider(props, "codec_px_scale", obs_module_text("RetroEffects.Codec.PxScale"), 1.0f, 16.0f, 0.01f );
	obs_properties_add_int_slider(props, "codec_colors_per_channel", obs_module_text("RetroEffects.Codec.ColorsPerChannel"), 1, 256, 1 );
	obs_properties_add_float_slider(props, "codec_quality", obs_module_text("RetroEffects.Codec.Quality"), 0.0f, 1.0f, 0.01f );

	obs_properties_t *thresholds_group = obs_properties_create();

	obs_properties_add_float_slider(thresholds_group, "codec_rpza_threshold_prev_frame", obs_module_text("RetroEffects.Codec.ThresholdPrevFrame"), 0.0f, 1.0f, 0.01f );
	obs_properties_add_float_slider(thresholds_group, "codec_rpza_threshold_solid", obs_module_text("RetroEffects.Codec.ThresholdSolid"), 0.0f, 1.0f, 0.01f );
	obs_properties_add_float_slider(thresholds_group, "codec_rpza_threshold_gradient", obs_module_text("RetroEffects.Codec.ThresholdGradient"), 0.0f, 1.0f, 0.01f );

	obs_properties_add_group(props, "codec_custom_thresholds", obs_module_text("RetroEffects.Codec.CustomThresholds"), OBS_GROUP_CHECKABLE, thresholds_group);
}


void codec_bilinear_downscale(gs_texrender_t* src, gs_texrender_t* dest, int srcWidth, int srcHeight, int destWidth, int destHeight)
{
	UNUSED_PARAMETER(srcWidth);
	UNUSED_PARAMETER(srcHeight);

	set_render_parameters();
	set_blending_parameters();

	gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_DEFAULT);

	gs_texture_t *src_tex = gs_texrender_get_texture(src);
	if (!src_tex) { return; }

	// Set upscale params
	{
		gs_eparam_t *image_param = gs_effect_get_param_by_name(effect, "image");
		if (image_param) {
			// TODO should this be gs_effect_set_texture_srgb?
			gs_effect_set_texture(image_param, src_tex);
		}
		gs_eparam_t *multiplier_param = gs_effect_get_param_by_name(effect, "multiplier");
		if (multiplier_param) {
			gs_effect_set_float(multiplier_param, 1.0f);
		}
	}

	if (gs_texrender_begin(dest, destWidth, destHeight)) {
		gs_ortho(0.0f, (float)destWidth, 0.0f, (float)destHeight,
			 -100.0f, 100.0f);
		while (gs_effect_loop(effect, "Draw"))
			gs_draw_sprite(src_tex, 0, destWidth, destHeight);
		gs_texrender_end(dest);
	}

	gs_blend_state_pop(); // matches gs_blend_state_push in set_blending_parameters()
}

void codec_area_upscale(gs_texrender_t* src, gs_texrender_t* dest, int srcWidth, int srcHeight, int destWidth, int destHeight)
{
	set_render_parameters();
	set_blending_parameters();

	gs_effect_t *effect = obs_get_base_effect(OBS_EFFECT_AREA);

	gs_texture_t *src_tex = gs_texrender_get_texture(src);
	if (!src_tex) { return; }

	// Set upscale params
	{
		gs_eparam_t *image_param = gs_effect_get_param_by_name(effect, "image");
		if (image_param) {
			gs_effect_set_texture(image_param, src_tex);
		}
		gs_eparam_t *bdim_param = gs_effect_get_param_by_name(effect, "base_dimension");
		if (bdim_param) {
			struct vec2 bdim;
			vec2_set(&bdim, (float)srcWidth, (float)srcHeight);
			gs_effect_set_vec2(bdim_param, &bdim);
		}
		gs_eparam_t *bdim_i_param = gs_effect_get_param_by_name(effect, "base_dimension_i");
		if (bdim_i_param) {
			struct vec2 bdim_i;
			vec2_set(&bdim_i, 1.0f / (float)srcWidth, 1.0f / (float)srcHeight);
			gs_effect_set_vec2(bdim_i_param, &bdim_i);
		}
		gs_eparam_t *multiplier_param = gs_effect_get_param_by_name(effect, "multiplier");
		if (multiplier_param) {
			gs_effect_set_float(multiplier_param, 1.0f);
		}
	}

	if (gs_texrender_begin(dest, destWidth, destHeight)) {
		gs_ortho(0.0f, (float)destWidth, 0.0f, (float)destHeight,
			 -100.0f, 100.0f);
		while (gs_effect_loop(effect, "DrawUpscale"))
			gs_draw_sprite(src_tex, 0, destWidth, destHeight);
		gs_texrender_end(dest);
	}

	gs_blend_state_pop(); // matches gs_blend_state_push in set_blending_parameters()
}


void codec_filter_video_render(retro_effects_filter_data_t *data)
{
	base_filter_data_t *base = data->base;
	codec_filter_data_t *filter = data->active_filter_data;

	get_input_source(base);
	if (!base->input_texture_generated || filter->loading_effect) {
		base->rendering = false;
		obs_source_skip_video_filter(base->context);
		return;
	}

	gs_texture_t *input_tex = gs_texrender_get_texture(base->input_texrender);
	if (!input_tex) { return; }

	gs_effect_t *effect = filter->effect_codec;
	if (!effect) { return; }

	base->output_texrender =
		create_or_reset_texrender(base->output_texrender);

	filter->texrender_downsampled_input =
		create_or_reset_texrender(filter->texrender_downsampled_input);
	filter->texrender_previous_frame =
		create_or_reset_texrender(filter->texrender_previous_frame);
	filter->texrender_downsampled_output =
		create_or_reset_texrender(filter->texrender_downsampled_output);

	const int pxWidth = (int)(base->width / filter->px_scale);
	const int pxHeight = (int)(base->height / filter->px_scale);

	// Downscale current input frame (bilinear downscale)
	// IN: base->input_texrender
	// OUT: filter->texrender_downsampled_input
	codec_bilinear_downscale(base->input_texrender, filter->texrender_downsampled_input, base->width, base->height, pxWidth, pxHeight);

	// Check for previous frame.
	// If none available (due to first frame or resize etc), copy in downscaled frame just so we have something the right size
	// OUT: filter->texrender_previous_frame

	bool prev_frame_valid = true;
	gs_texture_t *prev_frame_tex = gs_texrender_get_texture(filter->texrender_previous_frame);
	if (!prev_frame_tex) {
		prev_frame_valid = false;
		codec_bilinear_downscale(filter->texrender_downsampled_input, filter->texrender_previous_frame, pxWidth, pxHeight, pxWidth, pxHeight);
	}

	// Render FMV effect
	// IN: filter->texrender_downsampled_input
	// IN: filter->texrender_previous_frame
	// OUT: filter->texrender_downsampled_output
	// Set FMV effect params

	if (filter->param_uv_size) {
		struct vec2 uv_size;
		uv_size.x = (float)pxWidth;
		uv_size.y = (float)pxHeight;
		gs_effect_set_vec2(filter->param_uv_size, &uv_size);
	}
	if (filter->param_image) {
		gs_texture_t *downsampled_input_tex = gs_texrender_get_texture(filter->texrender_downsampled_input);
		gs_effect_set_texture(filter->param_image, downsampled_input_tex);
	}
	if (filter->param_prev_frame) {
		gs_texture_t *prev_frame_tex = gs_texrender_get_texture(filter->texrender_previous_frame);
		gs_effect_set_texture(filter->param_prev_frame, prev_frame_tex);
	}
	if (filter->param_prev_frame_valid) {
		gs_effect_set_float(filter->param_prev_frame_valid, prev_frame_valid ? 1.0f : 0.0f);
	}
	if (filter->param_colors_per_channel) {
		gs_effect_set_float(filter->param_colors_per_channel, (float)filter->colors_per_channel);
	}
	if (filter->param_rpza_threshold_prev_frame) {
		gs_effect_set_float(filter->param_rpza_threshold_prev_frame, filter->rpza_threshold_prev_frame);
	}
	if (filter->param_rpza_threshold_solid) {
		gs_effect_set_float(filter->param_rpza_threshold_solid, filter->rpza_threshold_solid);
	}
	if (filter->param_rpza_threshold_gradient) {
		gs_effect_set_float(filter->param_rpza_threshold_gradient, filter->rpza_threshold_gradient);
	}

	set_render_parameters();
	set_blending_parameters();

	gs_texture_t *downsampled_input_tex = gs_texrender_get_texture(filter->texrender_downsampled_input);

	// Once we have multiple codec types, select between different techniques instead of just "Draw"

	if (gs_texrender_begin(filter->texrender_downsampled_output, pxWidth, pxHeight)) {
		gs_ortho(0.0f, (float)pxWidth, 0.0f, (float)pxHeight,
			 -100.0f, 100.0f);
		while (gs_effect_loop(effect, "DrawRPZA"))
			gs_draw_sprite(downsampled_input_tex, 0, pxWidth, pxHeight);
		gs_texrender_end(filter->texrender_downsampled_output);
	}

	gs_blend_state_pop(); // matches gs_blend_state_push in set_blending_parameters()

	// Copy this frame to prev frame
	// IN: filter->texrender_downsampled_output
	// OUT: filter->texrender_previous_frame
	codec_bilinear_downscale(filter->texrender_downsampled_output, filter->texrender_previous_frame, pxWidth, pxHeight, pxWidth, pxHeight);

	// Upscale to output tex (area upscale)
	// IN: filter->texrender_downsampled_output
	// OUT: base->output_texrender
	codec_area_upscale(filter->texrender_downsampled_output, base->output_texrender, pxWidth, pxHeight, base->width, base->height);
}

static void
codec_set_functions(retro_effects_filter_data_t *filter)
{
	filter->filter_properties = codec_filter_properties;
	filter->filter_video_render = codec_filter_video_render;
	filter->filter_destroy = codec_destroy;
	filter->filter_defaults = codec_filter_defaults;
	filter->filter_update = codec_filter_update;
	filter->filter_video_tick = NULL;
}

static void codec_load_effect(codec_filter_data_t *filter)
{
	filter->loading_effect = true;
	if (filter->effect_codec != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect_codec);
		filter->effect_codec = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/codec.effect");
	shader_text = load_shader_from_file(filename.array);
	char *errors = NULL;
	dstr_free(&filename);

	obs_enter_graphics();
	filter->effect_codec =
		gs_effect_create(shader_text, NULL, &errors);
	obs_leave_graphics();

	bfree(shader_text);
	if (filter->effect_codec == NULL) {
		blog(LOG_WARNING,
		     "[obs-composite-blur] Unable to load codec.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	} else {
		size_t effect_count = gs_effect_get_num_params(
			filter->effect_codec);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_codec,
				effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_image = param;
			} else if (strcmp(info.name, "uv_size") == 0) {
				filter->param_uv_size = param;
			} else if (strcmp(info.name, "prev_frame") == 0) {
				filter->param_prev_frame = param;
			} else if (strcmp(info.name, "prev_frame_valid") == 0) {
				filter->param_prev_frame_valid = param;
			} else if (strcmp(info.name, "colors_per_channel") == 0) {
				filter->param_colors_per_channel = param;
			} else if (strcmp(info.name, "rpza_threshold_prev_frame") == 0) {
				filter->param_rpza_threshold_prev_frame = param;
			} else if (strcmp(info.name, "rpza_threshold_solid") == 0) {
				filter->param_rpza_threshold_solid = param;
			} else if (strcmp(info.name, "rpza_threshold_gradient") == 0) {
				filter->param_rpza_threshold_gradient = param;
			}
		}
	}
	filter->loading_effect = false;
}


static bool codec_type_modified(obs_properties_t *props, obs_property_t *p,
				 obs_data_t *settings)
{
	// Once we add more codec types, control parameter visibility here
	UNUSED_PARAMETER(props);
	UNUSED_PARAMETER(p);
	UNUSED_PARAMETER(settings);
	return true;
}
