#include "analog-glitch.h"
#include "../obs-utils.h"

void analog_glitch_create(retro_effects_filter_data_t *filter)
{
	analog_glitch_filter_data_t *data =
		bzalloc(sizeof(analog_glitch_filter_data_t));
	filter->active_filter_data = data;
	data->reload_effect = false;
	analog_glitch_set_functions(filter);
	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	analog_glitch_filter_defaults(settings);
	obs_data_release(settings);
	analog_glitch_load_effect(data);
}

void analog_glitch_destroy(retro_effects_filter_data_t *filter)
{
	analog_glitch_filter_data_t *data = filter->active_filter_data;
	obs_enter_graphics();
	if (data->effect_analog_glitch) {
		gs_effect_destroy(data->effect_analog_glitch);
	}

	obs_leave_graphics();

	bfree(filter->active_filter_data);
	filter->active_filter_data = NULL;
}

void analog_glitch_unset_settings(retro_effects_filter_data_t* filter)
{
	obs_data_t* settings = obs_source_get_settings(filter->base->context);
	obs_data_unset_user_value(settings, "analog_glitch_primary_speed");
	obs_data_unset_user_value(settings, "analog_glitch_primary_scale");
	obs_data_unset_user_value(settings, "analog_glitch_secondary_speed");
	obs_data_unset_user_value(settings, "analog_glitch_secondary_scale");
	obs_data_unset_user_value(settings, "analog_glitch_interference_speed");
	obs_data_unset_user_value(settings, "analog_glitch_interference_scale");
	obs_data_unset_user_value(settings, "analog_glitch_primary_threshold");
	obs_data_unset_user_value(settings, "analog_glitch_secondary_threshold");
	obs_data_unset_user_value(settings, "analog_glitch_secondary_influence");
	obs_data_unset_user_value(settings, "analog_glitch_max_disp");
	obs_data_unset_user_value(settings, "analog_glitch_interference_magnitude");
	obs_data_unset_user_value(settings, "analog_glitch_line_magnitude");
	obs_data_unset_user_value(settings, "analog_glitch_interfence_alpha");
	obs_data_release(settings);
}

void analog_glitch_filter_update(retro_effects_filter_data_t *data,
				 obs_data_t *settings)
{
	analog_glitch_filter_data_t *filter = data->active_filter_data;
	filter->speed_primary = (float)obs_data_get_double(settings, "analog_glitch_primary_speed");
	filter->scale_primary = (float)obs_data_get_double(settings, "analog_glitch_primary_scale");
	filter->speed_secondary = (float)obs_data_get_double(settings, "analog_glitch_secondary_speed");
	filter->scale_secondary = (float)obs_data_get_double(settings, "analog_glitch_secondary_scale");

	filter->threshold_primary = (float)obs_data_get_double(settings, "analog_glitch_primary_threshold");
	filter->threshold_secondary = (float)obs_data_get_double(settings, "analog_glitch_secondary_threshold");
	filter->secondary_influence = (float)obs_data_get_double(settings, "analog_glitch_secondary_influence");
	filter->max_disp = (float)obs_data_get_double(settings, "analog_glitch_max_disp");
	filter->interference_mag = (float)obs_data_get_double(settings, "analog_glitch_interference_magnitude");
	filter->line_mag = (float)obs_data_get_double(settings, "analog_glitch_line_magnitude");
	filter->interference_alpha = obs_data_get_bool(settings, "analog_glitch_interfence_alpha");
	filter->desaturation_amount = (float)obs_data_get_double(settings, "analog_glitch_desat_amount")/100.0f;
	filter->color_drift = (float)obs_data_get_double(settings, "analog_glitch_ca_max_disp");

	if (filter->reload_effect) {
		filter->reload_effect = false;
		analog_glitch_load_effect(filter);
	}
}

void analog_glitch_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "analog_glitch_primary_speed", 2.0);
	obs_data_set_default_double(settings, "analog_glitch_primary_scale", 800.0);
	obs_data_set_default_double(settings, "analog_glitch_secondary_speed", 5.0);
	obs_data_set_default_double(settings, "analog_glitch_secondary_scale", 128.0);
	obs_data_set_default_double(settings, "analog_glitch_interference_speed", 5.0);
	obs_data_set_default_double(settings, "analog_glitch_interference_scale", 64.0);
	obs_data_set_default_double(settings, "analog_glitch_primary_threshold", 0.3);
	obs_data_set_default_double(settings, "analog_glitch_secondary_threshold", 0.7);
	obs_data_set_default_double(settings, "analog_glitch_secondary_influence", 0.15);
	obs_data_set_default_double(settings, "analog_glitch_max_disp", 250.0);
	obs_data_set_default_double(settings, "analog_glitch_interference_magnitude", 0.3);
	obs_data_set_default_double(settings, "analog_glitch_line_magnitude", 0.15);
	obs_data_set_default_bool(settings, "analog_glitch_interfence_alpha", false);
}

void analog_glitch_filter_properties(retro_effects_filter_data_t *data,
				     obs_properties_t *props)
{
	UNUSED_PARAMETER(data);
	obs_property_t *p = NULL;

	p = obs_properties_add_float_slider(
		props, "analog_glitch_max_disp",
		obs_module_text(
			"RetroEffects.AnalogGlitch.MaximumDisplacement"),
		0.0, 6000.0, 1.0);
	obs_property_float_set_suffix(p, "px");

	obs_properties_t *primary_wave_group = obs_properties_create();

	p = obs_properties_add_float_slider(
		primary_wave_group, "analog_glitch_primary_speed",
		obs_module_text("RetroEffects.AnalogGlitch.Primary.Speed"), 0.0, 100.0,
		0.01);
	obs_property_float_set_suffix(p, " s");
	p = obs_properties_add_float_slider(
		primary_wave_group, "analog_glitch_primary_scale",
		obs_module_text("RetroEffects.AnalogGlitch.Primary.Scale"), 0.0, 1000.0,
		0.1);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		primary_wave_group, "analog_glitch_primary_threshold",
		obs_module_text("RetroEffects.AnalogGlitch.Primary.Threshold"), 0.0, 1.0,
		0.01);

	obs_properties_add_group(
		props, "analog_glitch_primary",
		obs_module_text("RetroEffects.AnalogGlitch.Primary"),
		OBS_GROUP_NORMAL, primary_wave_group);

	obs_properties_t *secondary_wave_group = obs_properties_create();

	p = obs_properties_add_float_slider(
		secondary_wave_group, "analog_glitch_secondary_speed",
		obs_module_text("RetroEffects.AnalogGlitch.Secondary.Speed"),
		0.0, 100.0, 0.01);

	p = obs_properties_add_float_slider(
		secondary_wave_group, "analog_glitch_secondary_scale",
		obs_module_text("RetroEffects.AnalogGlitch.Secondary.Scale"),
		0.0, 1000.0, 0.1);

	p = obs_properties_add_float_slider(
		secondary_wave_group, "analog_glitch_secondary_threshold",
		obs_module_text(
			"RetroEffects.AnalogGlitch.Secondary.Threshold"),
		0.0, 1.0, 0.01);

	p = obs_properties_add_float_slider(
		secondary_wave_group, "analog_glitch_secondary_influence",
		obs_module_text("RetroEffects.AnalogGlitch.Secondary.Influence"), 0.0,
		1.0, 0.01);

	obs_properties_add_group(
		props, "analog_glitch_secondary",
		obs_module_text("RetroEffects.AnalogGlitch.Secondary"),
		OBS_GROUP_NORMAL, secondary_wave_group);

	obs_properties_t *interference_group = obs_properties_create();

	p = obs_properties_add_float_slider(
		interference_group, "analog_glitch_interference_magnitude",
		obs_module_text("RetroEffects.AnalogGlitch.Interference.Magnitude"), 0.0,
		1.0, 0.01);

	p = obs_properties_add_float_slider(
		interference_group, "analog_glitch_line_magnitude",
		obs_module_text("RetroEffects.AnalogGlitch.Line.Magnitude"), 0.0,
		1.0, 0.01);

	p = obs_properties_add_bool(interference_group, "analog_glitch_interfence_alpha",
		obs_module_text(
			"RetroEffects.AnalogGlitch.Interference.AffectsAlpha"));

	obs_properties_add_group(
		props, "analog_glitch_interference",
		obs_module_text("RetroEffects.AnalogGlitch.Interference"),
		OBS_GROUP_NORMAL, interference_group);

	obs_properties_t *ca_group = obs_properties_create();

	p = obs_properties_add_float_slider(
		ca_group, "analog_glitch_ca_max_disp",
		obs_module_text(
			"RetroEffects.AnalogGlitch.CA.MaxDisp"),
		0.0, 250.0, 1.0);

	obs_properties_add_group(
		props, "analog_glitch_ca",
		obs_module_text("RetroEffects.AnalogGlitch.CA"),
		OBS_GROUP_NORMAL, ca_group);

	obs_properties_t *desat_group = obs_properties_create();

	p = obs_properties_add_float_slider(
		desat_group, "analog_glitch_desat_amount",
		obs_module_text("RetroEffects.AnalogGlitch.DeSat.Amount"), 0.0,
		100.0, 0.1);

	obs_property_float_set_suffix(p, "%");

	obs_properties_add_group(
		props, "analog_glitch_desat",
		obs_module_text("RetroEffects.AnalogGlitch.DeSat"),
		OBS_GROUP_NORMAL, desat_group);
}


void analog_glitch_filter_video_render(retro_effects_filter_data_t *data)
{
	base_filter_data_t *base = data->base;
	analog_glitch_filter_data_t *filter = data->active_filter_data;

	get_input_source(base);
	if (!base->input_texture_generated || filter->loading_effect) {
		base->rendering = false;
		obs_source_skip_video_filter(base->context);
		return;
	}

	gs_texture_t *image = gs_texrender_get_texture(base->input_texrender);
	gs_effect_t *effect = filter->effect_analog_glitch;

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

	if (filter->param_time) {
		gs_effect_set_float(filter->param_time, filter->time);
	}

	if (filter->param_scale_primary) {
		gs_effect_set_float(filter->param_scale_primary,
				    filter->scale_primary);
	}
	if (filter->param_speed_primary) {
		gs_effect_set_float(filter->param_speed_primary,
				    filter->speed_primary);
	}

	if (filter->param_scale_secondary) {
		gs_effect_set_float(filter->param_scale_secondary,
				    filter->scale_secondary);
	}
	if (filter->param_speed_secondary) {
		gs_effect_set_float(filter->param_speed_secondary,
				    filter->speed_secondary);
	}

	if (filter->param_threshold_primary) {
		gs_effect_set_float(filter->param_threshold_primary,
				    filter->threshold_primary);
	}
	if (filter->param_threshold_secondary) {
		gs_effect_set_float(filter->param_threshold_secondary,
				    filter->threshold_secondary);
	}

	if (filter->param_secondary_influence) {
		gs_effect_set_float(filter->param_secondary_influence,
				    filter->secondary_influence);
	}

	if (filter->param_max_disp) {
		gs_effect_set_float(filter->param_max_disp,
				    filter->max_disp);
	}
	if (filter->param_interference_mag) {
		gs_effect_set_float(filter->param_interference_mag,
				    filter->interference_mag);
	}
	if (filter->param_line_mag) {
		gs_effect_set_float(filter->param_line_mag,
			            filter->line_mag);
	}

	if (filter->param_desaturation_amount) {
		gs_effect_set_float(filter->param_desaturation_amount,
				    filter->desaturation_amount);
	}

	if (filter->param_color_drift) {
		gs_effect_set_float(filter->param_color_drift,
				    filter->color_drift);
	}

	if (filter->param_interference_alpha) {
		float int_alpha = filter->interference_alpha ? 1.0f : 0.0f;
		gs_effect_set_float(filter->param_interference_alpha,
				    int_alpha);
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

static void analog_glitch_set_functions(retro_effects_filter_data_t *filter)
{
	filter->filter_properties = analog_glitch_filter_properties;
	filter->filter_video_render = analog_glitch_filter_video_render;
	filter->filter_destroy = analog_glitch_destroy;
	filter->filter_defaults = analog_glitch_filter_defaults;
	filter->filter_update = analog_glitch_filter_update;
	filter->filter_video_tick = analog_glitch_filter_video_tick;
	filter->filter_unset_settings = analog_glitch_unset_settings;
}

void analog_glitch_filter_video_tick(retro_effects_filter_data_t *data,
				  float seconds)
{
	analog_glitch_filter_data_t *filter = data->active_filter_data;
	filter->time += seconds;
}

static void analog_glitch_load_effect(analog_glitch_filter_data_t *filter)
{
	filter->loading_effect = true;
	if (filter->effect_analog_glitch != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect_analog_glitch);
		filter->effect_analog_glitch = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/analog-glitch.effect");
	shader_text = load_shader_from_file(filename.array);
	char *errors = NULL;
	dstr_free(&filename);

	struct dstr shader_dstr = {0};
	dstr_init_copy(&shader_dstr, shader_text);

	//dstr_cat(&shader_dstr, shader_text);

	obs_enter_graphics();
	int device_type = gs_get_device_type();
	if (device_type == GS_DEVICE_OPENGL) {
		dstr_insert(&shader_dstr, 0, "#define OPENGL 1\n");
	}
	filter->effect_analog_glitch =
		gs_effect_create(shader_dstr.array, NULL, &errors);
	obs_leave_graphics();

	dstr_free(&shader_dstr);
	bfree(shader_text);
	if (filter->effect_analog_glitch == NULL) {
		blog(LOG_WARNING,
		     "[obs-retro-effects] Unable to load analog-gitch.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	} else {
		size_t effect_count =
			gs_effect_get_num_params(filter->effect_analog_glitch);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_analog_glitch, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_image = param;
			} else if (strcmp(info.name, "uv_size") == 0) {
				filter->param_uv_size = param;
			} else if (strcmp(info.name, "time") == 0) {
				filter->param_time = param;
			} else if (strcmp(info.name, "speed_primary") == 0) {
				filter->param_speed_primary = param;
			} else if (strcmp(info.name, "speed_secondary") == 0) {
				filter->param_speed_secondary = param;
			} else if (strcmp(info.name, "scale_primary") == 0) {
				filter->param_scale_primary = param;
			} else if (strcmp(info.name, "scale_secondary") == 0) {
				filter->param_scale_secondary = param;
			} else if (strcmp(info.name, "threshold_primary") == 0) {
				filter->param_threshold_primary = param;
			} else if (strcmp(info.name, "threshold_secondary") == 0) {
				filter->param_threshold_secondary = param;
			} else if (strcmp(info.name, "secondary_influence") == 0) {
				filter->param_secondary_influence = param;
			} else if (strcmp(info.name, "max_disp") == 0) {
				filter->param_max_disp = param;
			} else if (strcmp(info.name, "interference_mag") == 0) {
				filter->param_interference_mag = param;
			} else if (strcmp(info.name, "line_mag") == 0) {
				filter->param_line_mag = param;
			} else if (strcmp(info.name, "desaturation_amount") == 0) {
				filter->param_desaturation_amount = param;
			} else if (strcmp(info.name, "color_drift") == 0) {
				filter->param_color_drift = param;
			} else if (strcmp(info.name, "interference_alpha") == 0) {
				filter->param_interference_alpha = param;
			}
		}
	}
	filter->loading_effect = false;
}
