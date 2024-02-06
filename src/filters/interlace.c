#include "interlace.h"
#include "../obs-utils.h"

void interlace_create(retro_effects_filter_data_t *filter)
{
	interlace_filter_data_t *data =
		bzalloc(sizeof(interlace_filter_data_t));
	filter->active_filter_data = data;
	interlace_set_functions(filter);
	load_interlace_effect(data);
}

void interlace_destroy(retro_effects_filter_data_t *filter)
{
	interlace_filter_data_t *data = filter->active_filter_data;
	obs_enter_graphics();
	if (data->effect_interlace) {
		gs_effect_destroy(data->effect_interlace);
	}
	if (data->buffer_texrender) {
		gs_texrender_destroy(data->buffer_texrender);
	}
	obs_leave_graphics();

	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	obs_data_unset_user_value(settings, "interlace_thickness");
	obs_data_unset_user_value(settings, "interlace_brightness_reduction");
	obs_data_unset_user_value(settings, "interlace_reduce_alpha");
	obs_data_release(settings);

	bfree(filter->active_filter_data);
	filter->active_filter_data = NULL;
}

void interlace_filter_update(retro_effects_filter_data_t *data,
			     obs_data_t *settings)
{
	interlace_filter_data_t *filter = data->active_filter_data;

	filter->thickness =
		(int)obs_data_get_int(settings, "interlace_thickness");
	float br = 1.0f - (float)obs_data_get_double(
				  settings, "interlace_brightness_reduction") /
				  100.0f;
	float br_alpha = obs_data_get_bool(settings, "interlace_reduce_alpha")
				 ? br
				 : 1.0f;
	filter->brightness_reduction.x = br;
	filter->brightness_reduction.y = br;
	filter->brightness_reduction.z = br;
	filter->brightness_reduction.w = br_alpha;
}

void interlace_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_int(settings, "interlace_thickness", 1);
	obs_data_set_default_double(settings, "interlace_brightness_reduction",
				    0.0);
	obs_data_set_default_bool(settings, "interlace_reduce_alpha", false);
}

void interlace_filter_properties(retro_effects_filter_data_t *data,
				 obs_properties_t *props)
{
	UNUSED_PARAMETER(data);
	obs_properties_add_int_slider(props, "interlace_thickness",
				      "Interlace Thickness", 1, 50, 1);
	obs_properties_add_float_slider(props, "interlace_brightness_reduction",
					"Brightness Reduction", 0.0, 100.0,
					0.1);
	obs_properties_add_bool(props, "interlace_reduce_alpha",
				"Reduce Alpha");
}

void interlace_filter_video_render(retro_effects_filter_data_t *data)
{
	base_filter_data_t *base = data->base;
	interlace_filter_data_t *filter = data->active_filter_data;

	get_input_source(base);
	if (!base->input_texture_generated) {
		base->rendering = false;
		obs_source_skip_video_filter(base->context);
		return;
	}

	gs_texture_t *image = gs_texrender_get_texture(base->input_texrender);
	gs_effect_t *effect = filter->effect_interlace;

	if (!effect || !image) {
		return;
	}

	gs_texrender_t *tmp = base->output_texrender;
	base->output_texrender = filter->buffer_texrender;
	filter->buffer_texrender = tmp;
	gs_texture_t *prior_frame =
		gs_texrender_get_texture(filter->buffer_texrender);

	base->output_texrender =
		create_or_reset_texrender(base->output_texrender);

	if (filter->param_uv_size) {
		struct vec2 uv_size;
		uv_size.x = (float)base->width;
		uv_size.y = (float)base->height;
		gs_effect_set_vec2(filter->param_uv_size, &uv_size);
	}
	if (filter->param_frame) {
		filter->frame = (filter->frame + 1) % 2;
		gs_effect_set_float(filter->param_frame, (float)filter->frame);
	}
	if (filter->param_image) {
		gs_effect_set_texture(filter->param_image, image);
	}
	if (filter->param_prior_frame) {
		gs_effect_set_texture(filter->param_prior_frame, prior_frame);
	}
	if (filter->param_thickness) {
		gs_effect_set_float(filter->param_thickness,
				    (float)filter->thickness);
	}
	if (filter->param_brightness_reduction) {
		gs_effect_set_vec4(filter->param_brightness_reduction,
				   &filter->brightness_reduction);
	}

	set_render_parameters();
	set_blending_parameters();

	if (gs_texrender_begin(base->output_texrender, base->width,
			       base->height)) {
		gs_ortho(0.0f, (float)base->width, 0.0f, (float)base->height,
			 -100.0f, 100.0f);
		while (gs_effect_loop(effect, "Draw"))
			gs_draw_sprite(image, 0, base->width, base->height);
		gs_texrender_end(base->output_texrender);
	}

	gs_blend_state_pop();
}

static void interlace_set_functions(retro_effects_filter_data_t *filter)
{
	filter->filter_properties = interlace_filter_properties;
	filter->filter_video_render = interlace_filter_video_render;
	filter->filter_destroy = interlace_destroy;
	filter->filter_defaults = interlace_filter_defaults;
	filter->filter_update = interlace_filter_update;
	filter->filter_video_tick = NULL;
}

static void load_interlace_effect(interlace_filter_data_t *filter)
{
	if (filter->effect_interlace != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect_interlace);
		filter->effect_interlace = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/interlace.effect");
	shader_text = load_shader_from_file(filename.array);
	char *errors = NULL;
	dstr_free(&filename);

	obs_enter_graphics();
	filter->effect_interlace = gs_effect_create(shader_text, NULL, &errors);
	obs_leave_graphics();

	bfree(shader_text);
	if (filter->effect_interlace == NULL) {
		blog(LOG_WARNING,
		     "[obs-composite-blur] Unable to load interlace.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	} else {
		size_t effect_count =
			gs_effect_get_num_params(filter->effect_interlace);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_interlace, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_image = param;
			} else if (strcmp(info.name, "prior_frame") == 0) {
				filter->param_prior_frame = param;
			} else if (strcmp(info.name, "uv_size") == 0) {
				filter->param_uv_size = param;
			} else if (strcmp(info.name, "frame") == 0) {
				filter->param_frame = param;
			} else if (strcmp(info.name, "thickness") == 0) {
				filter->param_thickness = param;
			} else if (strcmp(info.name, "brightness_reduction") ==
				   0) {
				filter->param_brightness_reduction = param;
			}
		}
	}
}
