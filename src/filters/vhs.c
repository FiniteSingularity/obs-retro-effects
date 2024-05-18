#include "vhs.h"
#include "../obs-utils.h"

void vhs_create(retro_effects_filter_data_t *filter)
{
	vhs_filter_data_t *data = bzalloc(sizeof(vhs_filter_data_t));
	filter->active_filter_data = data;
	data->reload_effect = false;
	vhs_set_functions(filter);
	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	vhs_filter_defaults(settings);
	obs_data_release(settings);
	vhs_load_effect(data);
}

void vhs_destroy(retro_effects_filter_data_t *filter)
{
	vhs_filter_data_t *data = filter->active_filter_data;
	obs_enter_graphics();
	if (data->effect_vhs) {
		gs_effect_destroy(data->effect_vhs);
	}

	obs_leave_graphics();

	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	//obs_data_unset_user_value(settings, "vhs_size");
	obs_data_unset_user_value(settings, "vhs_wrinkle_occurrence_prob");
	obs_data_unset_user_value(settings, "vhs_wrinkle_size");
	obs_data_unset_user_value(settings, "vhs_wrinkle_duration");
	obs_data_unset_user_value(settings, "vhs_jitter_min_size");
	obs_data_unset_user_value(settings, "vhs_jitter_max_size");
	obs_data_unset_user_value(settings, "vhs_jitter_min_period");
	obs_data_unset_user_value(settings, "vhs_jitter_max_period");
	obs_data_unset_user_value(settings, "vhs_jitter_min_interval");
	obs_data_unset_user_value(settings, "vhs_jitter_max_interval");
	obs_data_unset_user_value(settings, "vhs_pop_lines_amount");
	obs_data_unset_user_value(settings, "vhs_head_switch_primary_thickness");
	obs_data_unset_user_value(settings, "vhs_head_switch_primary_offset");
	obs_data_unset_user_value(settings, "vhs_head_switch_secondary_thickness");
	obs_data_unset_user_value(settings, "vhs_head_switch_secondary_horiz_offset");
	obs_data_unset_user_value(settings, "vhs_head_switch_secondary_vert_amount");
	obs_data_release(settings);

	bfree(filter->active_filter_data);
	filter->active_filter_data = NULL;
}

void vhs_filter_update(retro_effects_filter_data_t *data, obs_data_t *settings)
{
	vhs_filter_data_t *filter = data->active_filter_data;

	filter->tape_wrinkle_duration = (float)obs_data_get_double(settings, "vhs_wrinkle_duration");
	filter->tape_wrinkle_size = (float)obs_data_get_double(settings, "vhs_wrinkle_size");
	filter->tape_wrinkle_occurrence = (float)obs_data_get_double(settings, "vhs_wrinkle_occurrence");
	filter->pop_line_prob =
		(float)(obs_data_get_double(settings, "vhs_pop_lines_amount") /
		(100.0 * 1000.0));
	if (filter->reload_effect) {
		filter->reload_effect = false;
		vhs_load_effect(filter);
	}

	filter->hs_primary_offset = (float)obs_data_get_double(
		settings, "vhs_head_switch_primary_offset");
	filter->hs_primary_thickness = (float)obs_data_get_double(
		settings, "vhs_head_switch_primary_thickness");
	filter->hs_secondary_horiz_offset = (float)obs_data_get_double(
		settings, "vhs_head_switch_secondary_horiz_offset");
	filter->hs_secondary_vert_offset = (float)obs_data_get_double(
		settings, "vhs_head_switch_secondary_vert_amount");
	filter->hs_secondary_thickness = (float)obs_data_get_double(
		settings, "vhs_head_switch_secondary_thickness");
	filter->frame_jitter_min_size = (float)obs_data_get_double(
		settings, "vhs_jitter_min_size");
	filter->frame_jitter_max_size =
		(float)obs_data_get_double(settings, "vhs_jitter_max_size");
	filter->frame_jitter_min_period = (float)obs_data_get_double(
		settings, "vhs_jitter_min_period");
	filter->frame_jitter_max_period = (float)obs_data_get_double(
		settings, "vhs_jitter_max_period");
	filter->frame_jitter_min_interval = (float)obs_data_get_double(
		settings, "vhs_jitter_min_interval");
	filter->frame_jitter_max_interval = (float)obs_data_get_double(
		settings, "vhs_jitter_max_interval");
}

void vhs_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "vhs_wrinkle_occurrence_prob", 10.0);
	obs_data_set_default_double(settings, "vhs_wrinkle_size", 5.0);
	obs_data_set_default_double(settings, "vhs_wrinkle_duration", 3.0);
	obs_data_set_default_double(settings, "vhs_jitter_min_size", 1.0);
	obs_data_set_default_double(settings, "vhs_jitter_max_size", 10.0);
	obs_data_set_default_double(settings, "vhs_jitter_min_period", 0.1);
	obs_data_set_default_double(settings, "vhs_jitter_max_period", 1.0);
	obs_data_set_default_double(settings, "vhs_jitter_min_interval", 0.0);
	obs_data_set_default_double(settings, "vhs_jitter_max_interval", 10.0);
	obs_data_set_default_double(settings, "vhs_pop_lines_amount", 10.0);
	obs_data_set_default_double(settings, "vhs_head_switch_primary_thickness", 15.0);
	obs_data_set_default_double(settings, "vhs_head_switch_primary_offset", 30.0);
	obs_data_set_default_double(settings, "vhs_head_switch_secondary_thickness", 15.0);
	obs_data_set_default_double(settings, "vhs_head_switch_secondary_horiz_offset", 30.0);
	obs_data_set_default_double(settings, "vhs_head_switch_secondary_vert_amount", 5.0);
}

void vhs_filter_properties(retro_effects_filter_data_t *data,
			   obs_properties_t *props)
{
	UNUSED_PARAMETER(data);
	// vhs Type
	//obs_data_t *settings = obs_source_get_settings(data->base->context);
	obs_property_t *p;

	obs_properties_t *jitter_group = obs_properties_create();
	p = obs_properties_add_float_slider(
		jitter_group, "vhs_jitter_min_size",
		obs_module_text("RetroEffects.VHS.Jitter.MinSize"), 0.0, 20.0,
		1.0);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		jitter_group, "vhs_jitter_max_size",
		obs_module_text("RetroEffects.VHS.Jitter.MaxSize"), 0.0, 20.0,
		1.0);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		jitter_group, "vhs_jitter_min_period",
		obs_module_text("RetroEffects.VHS.Jitter.MinPeriod"), 0.01, 1.0,
		0.01);
	obs_property_float_set_suffix(p, "s");

	p = obs_properties_add_float_slider(
		jitter_group, "vhs_jitter_max_period",
		obs_module_text("RetroEffects.VHS.Jitter.MaxPeriod"), 0.01, 1.0,
		0.01);
	obs_property_float_set_suffix(p, "s");

	p = obs_properties_add_float_slider(
		jitter_group, "vhs_jitter_min_interval",
		obs_module_text("RetroEffects.VHS.Jitter.MinInterval"), 0.0,
		100.0, 0.1
	);
	obs_property_float_set_suffix(p, "s");

	p = obs_properties_add_float_slider(
		jitter_group, "vhs_jitter_max_interval",
		obs_module_text("RetroEffects.VHS.Jitter.MaxInterval"), 0.0,
		100.0, 0.1);
	obs_property_float_set_suffix(p, "s");

	obs_properties_add_group(
		props, "vhs_jitter_group",
		obs_module_text("RetroEffects.VHS.Jitter"),
		OBS_GROUP_NORMAL, jitter_group);

	obs_properties_t *tape_wrinkle_group = obs_properties_create();
	p = obs_properties_add_float_slider(
		tape_wrinkle_group, "vhs_wrinkle_occurrence",
		obs_module_text("RetroEffects.VHS.Wrinkle.Occurrence"), -20.0, 120.0, 0.1);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_float_slider(
		tape_wrinkle_group, "vhs_wrinkle_size",
		obs_module_text("RetroEffects.VHS.Wrinkle.Size"), 0.0,
		20.0, 0.1);
	obs_property_float_set_suffix(p, "%");

	p = obs_properties_add_float_slider(
		tape_wrinkle_group, "vhs_wrinkle_duration",
		obs_module_text("RetroEffects.VHS.Wrinkle.Duration"), 0.1, 20.0,
		0.1);
	obs_property_float_set_suffix(p, "s");

	obs_properties_add_group(
		props, "vhs_wrinkle_group",
		obs_module_text("RetroEffects.VHS.Wrinkle"),
		OBS_GROUP_NORMAL, tape_wrinkle_group);

	obs_properties_t *pop_lines_group = obs_properties_create();
	p = obs_properties_add_float_slider(
		pop_lines_group, "vhs_pop_lines_amount",
		obs_module_text("RetroEffects.VHS.PopLines.Amount"), 0.0, 100.0,
		0.1);
	obs_property_float_set_suffix(p, "%");

	obs_properties_add_group(
		props, "vhs_pop_lines_group",
		obs_module_text("RetroEffects.VHS.PopLines"),
		OBS_GROUP_NORMAL, pop_lines_group);

	obs_properties_t *head_switching = obs_properties_create();

	p = obs_properties_add_float_slider(
		head_switching, "vhs_head_switch_primary_thickness",
		obs_module_text("RetroEffects.VHS.HeadSwitch.PrimaryThickness"), 0.0, 100.0,
		1.0);
	obs_property_float_set_suffix(p, "px");
	p = obs_properties_add_float_slider(
		head_switching, "vhs_head_switch_primary_offset",
		obs_module_text("RetroEffects.VHS.HeadSwitch.PrimaryAmount"),
		-100.0, 100.0, 1.0);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		head_switching, "vhs_head_switch_secondary_thickness",
		obs_module_text("RetroEffects.VHS.HeadSwitch.SecondaryThickness"),
		0.0, 100.0, 1.0);
	obs_property_float_set_suffix(p, "px");
	p = obs_properties_add_float_slider(
		head_switching, "vhs_head_switch_secondary_horiz_offset",
		obs_module_text("RetroEffects.VHS.HeadSwitch.SecondaryHorizontalOffset"),
		-100.0, 100.0, 1.0);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		head_switching, "vhs_head_switch_secondary_vert_amount",
		obs_module_text(
			"RetroEffects.VHS.HeadSwitch.SecondaryAmount"),
		0, 10.0, 0.1);
	obs_property_float_set_suffix(p, "px");

	obs_properties_add_group(props, "vhs_head_switching_group",
				 obs_module_text("RetroEffects.VHS.HeadSwitch"),
				 OBS_GROUP_NORMAL, head_switching);
}

void vhs_filter_video_render(retro_effects_filter_data_t *data)
{
	base_filter_data_t *base = data->base;
	vhs_filter_data_t *filter = data->active_filter_data;

	get_input_source(base);
	if (!base->input_texture_generated || filter->loading_effect) {
		base->rendering = false;
		obs_source_skip_video_filter(base->context);
		return;
	}

	gs_texture_t *image = gs_texrender_get_texture(base->input_texrender);
	gs_effect_t *effect = filter->effect_vhs;

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
	if (filter->param_wrinkle_position) {
		gs_effect_set_float(filter->param_wrinkle_position,
				    filter->wrinkle_position);
	}
	if (filter->param_wrinkle_size) {
		gs_effect_set_float(filter->param_wrinkle_size,
				    filter->tape_wrinkle_size / 100.0f);
	}
	if (filter->param_time) {
		gs_effect_set_float(filter->param_time, filter->time);
	}
	if (filter->param_pop_line_prob) {
		gs_effect_set_float(filter->param_pop_line_prob,
				    filter->pop_line_prob);
	}
	if (filter->param_hs_primary_offset) {
		gs_effect_set_float(filter->param_hs_primary_offset,
				    filter->hs_primary_offset);
	}
	if (filter->param_hs_primary_thickness) {
		gs_effect_set_float(filter->param_hs_primary_thickness,
				    filter->hs_primary_thickness);
	}
	if (filter->param_hs_secondary_vert_offset) {
		// At Zero -secondary_thickness
		// At 1.0 -secondary_thickness + (mult * amount)/
		float offset = -filter->hs_secondary_thickness;
		offset += (filter->hs_secondary_vert_offset * filter->hs_primary_offset);
		gs_effect_set_float(filter->param_hs_secondary_vert_offset,
				    offset);
	}
	if (filter->param_hs_secondary_horiz_offset) {
		gs_effect_set_float(filter->param_hs_secondary_horiz_offset,
				    filter->hs_secondary_horiz_offset);
	}
	if (filter->param_hs_secondary_thickness) {
		gs_effect_set_float(filter->param_hs_secondary_thickness,
				    filter->hs_secondary_thickness);
	}
	if (filter->param_horizontal_offset) {
		gs_effect_set_float(filter->param_horizontal_offset,
				    filter->current_jitter);
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

static void vhs_set_functions(retro_effects_filter_data_t *filter)
{
	filter->filter_properties = vhs_filter_properties;
	filter->filter_video_render = vhs_filter_video_render;
	filter->filter_destroy = vhs_destroy;
	filter->filter_defaults = vhs_filter_defaults;
	filter->filter_update = vhs_filter_update;
	filter->filter_video_tick = vhs_filter_video_tick;
}

void vhs_filter_video_tick(retro_effects_filter_data_t *data,
				   float seconds)
{
	
	vhs_filter_data_t *filter = data->active_filter_data;
	filter->time += seconds * 100.0f;
	filter->local_time += seconds;

	//filter->local_time += filter->speed_factor * seconds;
	if (filter->jitter) {
		const float step = filter->jitter_step * seconds *
			filter->jitter_size / filter->jitter_period;
		const float inc = filter->current_jitter + step;
		filter->current_jitter = inc < filter->jitter_size ? inc : filter->jitter_size;
		//filter->current_jitter = min(filter->current_jitter + step, filter->jitter_size);
		if (filter->current_jitter >= filter->jitter_size && filter->current_jitter > 0.001f) {
			filter->jitter_step = -1.0;
		} else if (filter->current_jitter < 0.001f) {
			filter->jitter = false;
			filter->current_jitter = 0.0f;
			// Pick a new interval/start time
			const float interval =
				(float)((double)rand() / (double)RAND_MAX) *
				(filter->frame_jitter_max_interval -
				 filter->frame_jitter_min_interval) +
				filter->frame_jitter_min_interval;
			filter->jitter_start_time =
				filter->local_time + interval;
		}
	} else if(filter->local_time > filter->jitter_start_time) {
		filter->jitter_size =
			(float)((double)rand() / (double)RAND_MAX) *
				(filter->frame_jitter_max_size -
				 filter->frame_jitter_min_size) +
			filter->frame_jitter_min_size;
		filter->jitter_period =
			(float)((double)rand() / (double)RAND_MAX) *
				(filter->frame_jitter_max_period -
				 filter->frame_jitter_min_period) +
			filter->frame_jitter_min_period;
		filter->current_jitter = 0.0f;
		filter->jitter_step = 1.0;
		filter->jitter = filter->jitter_size > 0.001f;
	}

	if (filter->active_wrinkle) {
		float speed = 1.0f / filter->tape_wrinkle_duration;
		float increment = speed * seconds;
		filter->wrinkle_position -= increment;
		if (filter->wrinkle_position < -0.2f) {
			filter->active_wrinkle = false;
		}
	} else {
		double r = (double)rand() / (double)RAND_MAX;
		float occurrence = filter->tape_wrinkle_occurrence / 100.0f;
		float p = occurrence * seconds;
		if ((float)r < p) {
			filter->wrinkle_position = 1.2f;
			filter->active_wrinkle = true;
		}
	}
}

static void vhs_load_effect(vhs_filter_data_t *filter)
{
	filter->loading_effect = true;
	if (filter->effect_vhs != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect_vhs);
		filter->effect_vhs = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/vhs.effect");
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
	filter->effect_vhs = gs_effect_create(shader_dstr.array, NULL, &errors);
	obs_leave_graphics();

	dstr_free(&shader_dstr);
	bfree(shader_text);
	if (filter->effect_vhs == NULL) {
		blog(LOG_WARNING,
		     "[obs-retro-effects] Unable to load vhs.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	} else {
		size_t effect_count =
			gs_effect_get_num_params(filter->effect_vhs);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_vhs, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_image = param;
			} else if (strcmp(info.name, "uv_size") == 0) {
				filter->param_uv_size = param;
			} else if (strcmp(info.name, "wrinkle_size") == 0) {
				filter->param_wrinkle_size = param;
			} else if (strcmp(info.name, "wrinkle_position") == 0) {
				filter->param_wrinkle_position = param;
			} else if (strcmp(info.name, "pop_line_prob") == 0) {
				filter->param_pop_line_prob = param;
			} else if (strcmp(info.name, "time") == 0) {
				filter->param_time = param;
			} else if (strcmp(info.name, "hs_primary_offset") == 0) {
				filter->param_hs_primary_offset = param;
			} else if (strcmp(info.name, "hs_primary_thickness") == 0) {
				filter->param_hs_primary_thickness = param;
			} else if (strcmp(info.name, "hs_secondary_vert_offset") == 0) {
				filter->param_hs_secondary_vert_offset = param;
			}  else if (strcmp(info.name, "hs_secondary_horiz_offset") == 0) {
				filter->param_hs_secondary_horiz_offset = param;
			} else if (strcmp(info.name, "hs_secondary_thickness") == 0) {
				filter->param_hs_secondary_thickness = param;
			} else if (strcmp(info.name, "horizontal_offset") == 0) {
				filter->param_horizontal_offset = param;
			}
		}
	}
	filter->loading_effect = false;
}
