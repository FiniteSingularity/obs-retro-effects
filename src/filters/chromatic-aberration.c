#include "chromatic-aberration.h"
#include "../obs-utils.h"

void chromatic_aberration_create(retro_effects_filter_data_t *filter)
{
	chromatic_aberration_filter_data_t *data =
		bzalloc(sizeof(chromatic_aberration_filter_data_t));
	filter->active_filter_data = data;
	chromatic_aberration_set_functions(filter);
	chromatic_aberration_load_effect(data);
}

void chromatic_aberration_destroy(retro_effects_filter_data_t *filter)
{
	chromatic_aberration_filter_data_t *data = filter->active_filter_data;
	obs_enter_graphics();
	if (data->effect_chromatic_aberration) {
		gs_effect_destroy(data->effect_chromatic_aberration);
	}

	obs_leave_graphics();

	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	obs_data_unset_user_value(settings, "ca_type");
	obs_data_unset_user_value(settings, "ca_red_offset");
	obs_data_unset_user_value(settings, "ca_red_offset_angle");
	obs_data_unset_user_value(settings, "ca_green_offset");
	obs_data_unset_user_value(settings, "ca_green_offset_angle");
	obs_data_unset_user_value(settings, "ca_blue_offset");
	obs_data_unset_user_value(settings, "ca_blue_offset_angle");
	obs_data_release(settings);

	bfree(filter->active_filter_data);
	filter->active_filter_data = NULL;
}

void chromatic_aberration_filter_update(retro_effects_filter_data_t *data,
					obs_data_t *settings)
{
	chromatic_aberration_filter_data_t *filter = data->active_filter_data;
	filter->offsets.x =
		(float)obs_data_get_double(settings, "ca_red_offset");
	filter->offsets.y =
		(float)obs_data_get_double(settings, "ca_green_offset");
	filter->offsets.z =
		(float)obs_data_get_double(settings, "ca_blue_offset");

	double angle_r = obs_data_get_double(settings, "ca_red_offset_angle") *
			 M_PI / 180.0;
	double angle_g =
		obs_data_get_double(settings, "ca_green_offset_angle") * M_PI /
		180.0;
	double angle_b = obs_data_get_double(settings, "ca_blue_offset_angle") *
			 M_PI / 180.0;

	filter->offset_cos_angles.x = (float)cos(angle_r);
	filter->offset_cos_angles.y = (float)cos(angle_g);
	filter->offset_cos_angles.z = (float)cos(angle_b);
	filter->offset_sin_angles.x = (float)sin(angle_r);
	filter->offset_sin_angles.y = (float)sin(angle_g);
	filter->offset_sin_angles.z = (float)sin(angle_b);

	filter->ca_type = (uint32_t)obs_data_get_int(settings, "ca_type");
}

void chromatic_aberration_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "ca_type", 1);
	obs_data_set_default_double(settings, "ca_red_offset", 3.0);
	obs_data_set_default_double(settings, "ca_red_offset_angle", 0.0);
	obs_data_set_default_double(settings, "ca_green_offset", 0.0);
	obs_data_set_default_double(settings, "ca_green_offset_angle", 0.0);
	obs_data_set_default_double(settings, "ca_blue_offset", 3.0);
	obs_data_set_default_double(settings, "ca_blue_offset_angle", 180.0);
}

void chromatic_aberration_filter_properties(retro_effects_filter_data_t *data,
					    obs_properties_t *props)
{
	UNUSED_PARAMETER(data);

	obs_property_t *ca_type_list = obs_properties_add_list(
		props, "ca_type", obs_module_text("RetroEffects.CA.Type"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(ca_type_list,
				  obs_module_text(CA_TYPE_MANUAL_LABEL),
				  CA_TYPE_MANUAL);
	obs_property_list_add_int(ca_type_list,
				  obs_module_text(CA_TYPE_LENS_LABEL),
				  CA_TYPE_LENS);
	obs_property_set_modified_callback(ca_type_list, ca_type_modified);

	obs_property_t *p = obs_properties_add_float_slider(
		props, "ca_red_offset",
		obs_module_text("RetroEffects.CA.RedOffset"), -500.0, 500.0,
		0.1);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		props, "ca_red_offset_angle",
		obs_module_text("RetroEffects.CA.RedOffsetAngle"), -360.0,
		360.0, 0.1);
	obs_property_float_set_suffix(p, "deg");

	p = obs_properties_add_float_slider(
		props, "ca_green_offset",
		obs_module_text("RetroEffects.CA.GreenOffset"), -500.0, 500.0,
		0.1);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		props, "ca_green_offset_angle",
		obs_module_text("RetroEffects.CA.GreenOffsetAngle"), -360.0,
		360.0, 0.1);
	obs_property_float_set_suffix(p, "deg");

	p = obs_properties_add_float_slider(
		props, "ca_blue_offset",
		obs_module_text("RetroEffects.CA.BlueOffset"), -500.0, 500.0,
		0.1);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		props, "ca_blue_offset_angle",
		obs_module_text("RetroEffects.CA.BlueOffsetAngle"), -360.0,
		360.0, 0.1);
	obs_property_float_set_suffix(p, "deg");
}

static bool ca_type_modified(obs_properties_t *props, obs_property_t *p,
			     obs_data_t *settings)
{
	UNUSED_PARAMETER(p);

	int ca_type = (int)obs_data_get_int(settings, "ca_type");
	switch (ca_type) {
	case CA_TYPE_MANUAL:
		setting_visibility("ca_red_offset_angle", true, props);
		setting_visibility("ca_green_offset_angle", true, props);
		setting_visibility("ca_blue_offset_angle", true, props);
		break;
	case CA_TYPE_LENS:
		setting_visibility("ca_red_offset_angle", false, props);
		setting_visibility("ca_green_offset_angle", false, props);
		setting_visibility("ca_blue_offset_angle", false, props);
		break;
	}

	return true;
}

void chromatic_aberration_filter_video_render(retro_effects_filter_data_t *data)
{
	base_filter_data_t *base = data->base;
	chromatic_aberration_filter_data_t *filter = data->active_filter_data;

	get_input_source(base);
	if (!base->input_texture_generated) {
		base->rendering = false;
		obs_source_skip_video_filter(base->context);
		return;
	}

	gs_texture_t *image = gs_texrender_get_texture(base->input_texrender);
	gs_effect_t *effect = filter->effect_chromatic_aberration;

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
	if (filter->param_scale) {
		float w = (float)base->width;
		float h = (float)base->height;
		float scale = 1.0f / (sqrtf(w * w + h * h) / 2.0f);
		gs_effect_set_float(filter->param_scale, scale);
	}
	if (filter->param_image) {
		gs_effect_set_texture(filter->param_image, image);
	}
	if (filter->param_channel_offsets) {
		gs_effect_set_vec3(filter->param_channel_offsets,
				   &filter->offsets);
	}
	if (filter->param_channel_offset_cos_angles) {
		gs_effect_set_vec3(filter->param_channel_offset_cos_angles,
				   &filter->offset_cos_angles);
	}
	if (filter->param_channel_offset_sin_angles) {
		gs_effect_set_vec3(filter->param_channel_offset_sin_angles,
				   &filter->offset_sin_angles);
	}

	set_render_parameters();
	set_blending_parameters();

	const char *technique = filter->ca_type == CA_TYPE_MANUAL ? "Draw"
								  : "DrawLens";

	if (gs_texrender_begin(base->output_texrender, base->width,
			       base->height)) {
		gs_ortho(0.0f, (float)base->width, 0.0f, (float)base->height,
			 -100.0f, 100.0f);
		while (gs_effect_loop(effect, technique))
			gs_draw_sprite(image, 0, base->width, base->height);
		gs_texrender_end(base->output_texrender);
	}

	gs_blend_state_pop();
}

static void
chromatic_aberration_set_functions(retro_effects_filter_data_t *filter)
{
	filter->filter_properties = chromatic_aberration_filter_properties;
	filter->filter_video_render = chromatic_aberration_filter_video_render;
	filter->filter_destroy = chromatic_aberration_destroy;
	filter->filter_defaults = chromatic_aberration_filter_defaults;
	filter->filter_update = chromatic_aberration_filter_update;
	filter->filter_video_tick = NULL;
}

static void
chromatic_aberration_load_effect(chromatic_aberration_filter_data_t *filter)
{
	if (filter->effect_chromatic_aberration != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect_chromatic_aberration);
		filter->effect_chromatic_aberration = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/chromatic-aberration.effect");
	shader_text = load_shader_from_file(filename.array);
	char *errors = NULL;
	dstr_free(&filename);

	obs_enter_graphics();
	filter->effect_chromatic_aberration =
		gs_effect_create(shader_text, NULL, &errors);
	obs_leave_graphics();

	bfree(shader_text);
	if (filter->effect_chromatic_aberration == NULL) {
		blog(LOG_WARNING,
		     "[obs-retro-effects] Unable to load chromatic-aberration.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	} else {
		size_t effect_count = gs_effect_get_num_params(
			filter->effect_chromatic_aberration);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_chromatic_aberration,
				effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_image = param;
			} else if (strcmp(info.name, "uv_size") == 0) {
				filter->param_uv_size = param;
			} else if (strcmp(info.name, "channel_offsets") == 0) {
				filter->param_channel_offsets = param;
			} else if (strcmp(info.name,
					  "channel_offset_cos_angles") == 0) {
				filter->param_channel_offset_cos_angles = param;
			} else if (strcmp(info.name,
					  "channel_offset_sin_angles") == 0) {
				filter->param_channel_offset_sin_angles = param;
			} else if (strcmp(info.name, "scale") == 0) {
				filter->param_scale = param;
			}
		}
	}
}
