#include "crt.h"
#include "../obs-utils.h"
#include "../blur/blur.h"

void crt_create(retro_effects_filter_data_t *filter)
{
	crt_filter_data_t *data = bzalloc(sizeof(crt_filter_data_t));
	filter->active_filter_data = data;
	data->reload_effect = false;
	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	crt_filter_defaults(settings);
	crt_set_functions(filter);
	crt_load_effect(data);
	crt_composite_load_effect(data);
	obs_data_release(settings);
}

void crt_destroy(retro_effects_filter_data_t *filter)
{
	crt_filter_data_t *data = filter->active_filter_data;
	obs_enter_graphics();
	if (data->effect_crt) {
		gs_effect_destroy(data->effect_crt);
	}
	if (data->effect_crt_composite) {
		gs_effect_destroy(data->effect_crt_composite);
	}
	if (data->phospher_mask_texrender) {
		gs_texrender_destroy(data->phospher_mask_texrender);
	}
	obs_leave_graphics();

	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	obs_data_unset_user_value(settings, "crt_phosphor_type");
	obs_data_unset_user_value(settings, "crt_mask_intensity");
	obs_data_unset_user_value(settings, "crt_bloom");
	obs_data_unset_user_value(settings, "crt_bloom_threshold");
	obs_data_unset_user_value(settings, "crt_corner_radius");
	obs_data_unset_user_value(settings, "crt_barrel_distort");
	obs_data_unset_user_value(settings, "crt_vignette");
	obs_data_unset_user_value(settings, "crt_black_level");
	obs_data_unset_user_value(settings, "crt_white_level");
	obs_data_release(settings);

	bfree(filter->active_filter_data);
	filter->active_filter_data = NULL;
}

void crt_filter_update(retro_effects_filter_data_t *data, obs_data_t *settings)
{
	crt_filter_data_t *filter = data->active_filter_data;

	int phosphor_type = (int)obs_data_get_int(settings, "crt_phosphor_type");

	if (filter->phosphor_type != phosphor_type) {
		filter->phosphor_type = phosphor_type;
		filter->reload_effect = true;
	}

	if (filter->reload_effect) {
		filter->reload_effect = false;
		crt_load_effect(filter);
	}

	filter->phosphor_size.x =
		(float)obs_data_get_double(settings, "crt_phosphor_width")/9.0f;
	filter->phosphor_size.y =
		(float)obs_data_get_double(settings, "crt_phosphor_height")/9.0f;
	filter->brightness =
		(float)obs_data_get_double(settings, "crt_bloom_threshold") / 100.0f;

	float bloom = (float)obs_data_get_double(settings, "crt_bloom") *
		      30.0f / 100.0f;
	data->blur_data->radius = bloom;
	set_gaussian_radius(bloom, data->blur_data);

	filter->mask_intensity =
		(float)obs_data_get_double(settings, "crt_mask_intensity") /
		100.0f;	

	filter->black_level =
		(float)obs_data_get_double(settings, "crt_black_level") /
		255.0f;
	filter->white_level =
		(float)obs_data_get_double(settings, "crt_white_level") /
		255.0f;
	filter->barrel_distortion =
		(float)obs_data_get_double(settings, "crt_barrel_distort") *
		0.05f;
	filter->vignette_intensity =
		(float)obs_data_get_double(settings, "crt_vignette") * 1.0f /
		100.0f;
	filter->corner_radius =
		(float)obs_data_get_double(settings, "crt_corner_radius");
}

void crt_filter_defaults(obs_data_t *settings) {
	obs_data_set_default_int(settings, "crt_phosphor_type", 17);
	obs_data_set_default_double(settings, "crt_mask_intensity", 70.0);
	obs_data_set_default_double(settings, "crt_bloom", 5.0);
	obs_data_set_default_double(settings, "crt_bloom_threshold", 75.0);
	//obs_data_set_default_double(settings, "crt_bloom_intensity", 50.0);
	obs_data_set_default_double(settings, "crt_corner_radius", 25.0);
	obs_data_set_default_double(settings, "crt_barrel_distort", 1.7);
	obs_data_set_default_double(settings, "crt_vignette", 15.0);
	obs_data_set_default_double(settings, "crt_black_level", 20.0);
	obs_data_set_default_double(settings, "crt_white_level", 190.0);
}

void crt_filter_properties(retro_effects_filter_data_t *data,
			   obs_properties_t *props)
{
	obs_data_t *settings = obs_source_get_settings(data->base->context);
	obs_properties_t *phosphor_mask = obs_properties_create();
	obs_properties_add_int_slider(
		phosphor_mask, "crt_phosphor_type",
		obs_module_text("RetroEffects.CRT.PhosphorLayout"), 0, 24, 1);

	obs_property_t *p = obs_properties_add_float_slider(
		phosphor_mask, "crt_mask_intensity",
		obs_module_text("RetroEffects.CRT.PhoshporMaskIntensity"), 0.0, 100.0,
		0.1);
	obs_properties_add_group(
		props, "crt_phosphor_mask",
		obs_module_text("RetroEffects.CRT.PhosphorMask"),
		OBS_GROUP_NORMAL, phosphor_mask);

	obs_properties_t *phosphor_bloom = obs_properties_create();
	p = obs_properties_add_float_slider(
		phosphor_bloom, "crt_bloom",
		obs_module_text("RetroEffects.CRT.BloomSize"), 0.0, 32.0, 0.1);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		phosphor_bloom, "crt_bloom_threshold",
		obs_module_text("RetroEffects.CRT.BloomThreshold"), 0.0, 100.0,
		0.1);
	obs_property_float_set_suffix(p, "%");

	//p = obs_properties_add_float_slider(
	//	phosphor_bloom, "crt_bloom_intensity",
	//	obs_module_text("RetroEffects.CRT.BloomIntensity"), 0.0, 100.0,
	//	0.1);
	//obs_property_float_set_suffix(p, "%");

	obs_properties_add_group(
		props, "crt_phosphor_bloom",
		obs_module_text("RetroEffects.CRT.PhosphorBloom"),
		OBS_GROUP_NORMAL, phosphor_bloom);

	obs_properties_t *crt_geometry = obs_properties_create();
	p = obs_properties_add_float_slider(
		crt_geometry, "crt_corner_radius",
		obs_module_text("RetroEffects.CRT.CornerRadius"), 0.0, 500.0,
		1.0);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		crt_geometry, "crt_barrel_distort",
		obs_module_text("RetroEffects.CRT.BarrelDistortion"), 0.0, 100.0,
		0.1);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_float_slider(
		crt_geometry, "crt_vignette",
		obs_module_text("RetroEffects.CRT.Vignette"), 0.0,
					100.0, 0.1);
	obs_property_float_set_suffix(p, "%");
	obs_properties_add_group(
		props, "crt_geometry",
		obs_module_text("RetroEffects.CRT.Geometry"),
		OBS_GROUP_NORMAL, crt_geometry);

	obs_properties_t *color_correction = obs_properties_create();
	obs_properties_add_float_slider(
		color_correction, "crt_black_level",
		obs_module_text("RetroEffects.CRT.BlackLevel"),
					0.0, 255.0, 0.1);
	obs_properties_add_float_slider(
		color_correction, "crt_white_level",
		obs_module_text("RetroEffects.CRT.WhiteLevel"),
					0.0, 255.0, 0.1);
	obs_properties_add_group(props, "crt_color_correction",
				 obs_module_text("RetroEffects.CRT.ColorCorrection"),
				 OBS_GROUP_NORMAL, color_correction);
	obs_data_release(settings);
}

void crt_filter_video_render(retro_effects_filter_data_t *data)
{
	base_filter_data_t *base = data->base;
	crt_filter_data_t *filter = data->active_filter_data;
	if (filter->loading_effect) {
		base->rendering = false;
		obs_source_skip_video_filter(base->context);
		return;
	}
	crt_filter_render_crt_mask(data);
	crt_filter_render_blur(data);
	crt_filter_render_composite(data);
}

static void crt_filter_render_crt_mask(retro_effects_filter_data_t* data)
{
	base_filter_data_t *base = data->base;
	crt_filter_data_t *filter = data->active_filter_data;

	get_input_source(base);
	if (!base->input_texture_generated) {
		base->rendering = false;
		obs_source_skip_video_filter(base->context);
		return;
	}

	gs_texture_t *image = gs_texrender_get_texture(base->input_texrender);
	gs_effect_t *effect = filter->effect_crt;

	if (!effect || !image) {
		return;
	}

	filter->phospher_mask_texrender =
		create_or_reset_texrender(filter->phospher_mask_texrender);

	if (filter->param_uv_size) {
		struct vec2 uv_size;
		uv_size.x = (float)base->width;
		uv_size.y = (float)base->height;
		gs_effect_set_vec2(filter->param_uv_size, &uv_size);
	}
	if (filter->param_image) {
		gs_effect_set_texture_srgb(filter->param_image, image);
	}

	if (filter->param_mask_intensity) {
		gs_effect_set_float(filter->param_mask_intensity,
				    filter->mask_intensity);
	}

	if (filter->param_vignette_intensity) {
		gs_effect_set_float(filter->param_vignette_intensity,
				    filter->vignette_intensity);
	}

	if (filter->param_corner_radius) {
		gs_effect_set_float(filter->param_corner_radius,
				    filter->corner_radius);
	}

	const bool previous_framebuffer_srgb = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(true);

	set_render_parameters();
	set_blending_parameters();

	struct dstr technique;
	dstr_init_copy(&technique, "Draw");

	if (gs_texrender_begin(filter->phospher_mask_texrender, base->width,
			       base->height)) {
		gs_ortho(0.0f, (float)base->width, 0.0f, (float)base->height,
			 -100.0f, 100.0f);
		while (gs_effect_loop(effect, technique.array))
			gs_draw_sprite(image, 0, base->width, base->height);
		gs_texrender_end(filter->phospher_mask_texrender);
	}
	dstr_free(&technique);
	gs_blend_state_pop();

	gs_enable_framebuffer_srgb(previous_framebuffer_srgb);
}

static void crt_filter_render_blur(retro_effects_filter_data_t *data)
{
	crt_filter_data_t *filter = data->active_filter_data;
	gs_texture_t *image = gs_texrender_get_texture(filter->phospher_mask_texrender);

	// Creates a gaussian blurred verison of the phospher mask
	gaussian_area_blur(image, data->blur_data);
}

static void crt_filter_render_composite(retro_effects_filter_data_t* data)
{
	base_filter_data_t *base = data->base;
	crt_filter_data_t *filter = data->active_filter_data;

	gs_texture_t *image = gs_texrender_get_texture(filter->phospher_mask_texrender);
	gs_texture_t *blur_image = gs_texrender_get_texture(data->blur_data->blur_output);

	gs_effect_t *effect = filter->effect_crt_composite;

	if (!effect || !image || !blur_image) {
		return;
	}

	base->output_texrender =
		create_or_reset_texrender(base->output_texrender);

	if (filter->param_image_composite) {
		gs_effect_set_texture_srgb(filter->param_image_composite, image);
	}

	if (filter->param_blur_image_composite) {
		gs_effect_set_texture_srgb(filter->param_blur_image_composite, blur_image);
	}

	if (filter->param_brightness_composite) {
		gs_effect_set_float(filter->param_brightness_composite,
				    filter->brightness);
	}

	if (filter->param_black_level_composite) {
		gs_effect_set_float(filter->param_black_level_composite,
				    filter->black_level);
	}

	if (filter->param_white_level_composite) {
		gs_effect_set_float(filter->param_white_level_composite,
				    filter->white_level);
	}

	if (filter->param_distort_composite) {
		gs_effect_set_float(filter->param_distort_composite, filter->barrel_distortion);
	}

	const bool previous_framebuffer_srgb = gs_framebuffer_srgb_enabled();
	gs_enable_framebuffer_srgb(true);

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

	gs_enable_framebuffer_srgb(previous_framebuffer_srgb);
}

static void crt_set_functions(retro_effects_filter_data_t *filter)
{
	filter->filter_properties = crt_filter_properties;
	filter->filter_video_render = crt_filter_video_render;
	filter->filter_destroy = crt_destroy;
	filter->filter_defaults = crt_filter_defaults;
	filter->filter_update = crt_filter_update;
	filter->filter_video_tick = NULL;
}

static void crt_load_effect(crt_filter_data_t *filter)
{
	filter->loading_effect = true;
	if (filter->effect_crt != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect_crt);
		filter->effect_crt = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/crt.effect");
	shader_text = load_shader_from_file(filename.array);
	char *errors = NULL;
	dstr_free(&filename);

	struct dstr shader_dstr = {0};
	dstr_init(&shader_dstr);

	dstr_printf(&shader_dstr, "#define PHOSPHOR_LAYOUT_%i\n", filter->phosphor_type);
	dstr_cat(&shader_dstr, shader_text);

	obs_enter_graphics();
	int device_type = gs_get_device_type();
	if (device_type == GS_DEVICE_OPENGL) {
		dstr_insert(&shader_dstr, 0, "#define OPENGL 1\n");
	}
	filter->effect_crt = gs_effect_create(shader_dstr.array, NULL, &errors);
	obs_leave_graphics();

	dstr_free(&shader_dstr);
	bfree(shader_text);
	if (filter->effect_crt == NULL) {
		blog(LOG_WARNING,
		     "[obs-retro-effects] Unable to load crt.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	} else {
		size_t effect_count =
			gs_effect_get_num_params(filter->effect_crt);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_crt, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_image = param;
			} else if (strcmp(info.name, "uv_size") == 0) {
				filter->param_uv_size = param;
			} else if (strcmp(info.name, "mask_intensity") == 0) {
				filter->param_mask_intensity = param;
			} else if (strcmp(info.name, "vignette_intensity") == 0) {
				filter->param_vignette_intensity = param;
			} else if (strcmp(info.name, "corner_radius") == 0) {
				filter->param_corner_radius = param;
			}
		}
	}
	filter->loading_effect = false;
}

static void crt_composite_load_effect(crt_filter_data_t *filter)
{
	filter->loading_effect = true;
	if (filter->effect_crt_composite != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect_crt_composite);
		filter->effect_crt_composite = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/crt-composite.effect");
	shader_text = load_shader_from_file(filename.array);
	char *errors = NULL;
	dstr_free(&filename);

	struct dstr shader_dstr = {0};
	dstr_init_copy(&shader_dstr, shader_text);

	obs_enter_graphics();
	filter->effect_crt_composite = gs_effect_create(shader_dstr.array, NULL, &errors);
	obs_leave_graphics();

	dstr_free(&shader_dstr);
	bfree(shader_text);
	if (filter->effect_crt_composite == NULL) {
		blog(LOG_WARNING,
		     "[obs-retro-effects] Unable to load crt-composite.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	} else {
		size_t effect_count =
			gs_effect_get_num_params(filter->effect_crt_composite);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_crt_composite, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_image_composite = param;
			} else if (strcmp(info.name, "blur_image") == 0) {
				filter->param_blur_image_composite = param;
			} else if (strcmp(info.name, "brightness") == 0) {
				filter->param_brightness_composite = param;
			} else if (strcmp(info.name, "black_level") == 0) {
				filter->param_black_level_composite = param;
			} else if (strcmp(info.name, "white_level") == 0) {
				filter->param_white_level_composite = param;
			} else if (strcmp(info.name, "dist") == 0) {
				filter->param_distort_composite = param;
			}
		}
	}
	filter->loading_effect = false;
}
