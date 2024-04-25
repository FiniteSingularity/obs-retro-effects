#include "posterize.h"
#include "../obs-utils.h"

void posterize_create(retro_effects_filter_data_t *filter)
{
	posterize_filter_data_t *data =
		bzalloc(sizeof(posterize_filter_data_t));
	filter->active_filter_data = data;
	posterize_set_functions(filter);
	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	posterize_filter_defaults(settings);
	obs_data_release(settings);
	posterize_load_effect(data);
}

void posterize_destroy(retro_effects_filter_data_t *filter)
{
	posterize_filter_data_t *data = filter->active_filter_data;
	obs_enter_graphics();
	if (data->effect_posterize) {
		gs_effect_destroy(data->effect_posterize);
	}

	if (data->color_source) {
		obs_weak_source_release(data->color_source);
	}

	obs_leave_graphics();

	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	obs_data_unset_user_value(settings, "posterize_levels");
	obs_data_unset_user_value(settings, "posterize_technique");
	obs_data_unset_user_value(settings, "posterize_map_source");
	obs_data_unset_user_value(settings, "posterize_color_1");
	obs_data_unset_user_value(settings, "posterize_color_2");
	obs_data_release(settings);

	bfree(filter->active_filter_data);
	filter->active_filter_data = NULL;
}

void posterize_filter_update(retro_effects_filter_data_t *data,
			     obs_data_t *settings)
{
	posterize_filter_data_t *filter = data->active_filter_data;
	filter->levels = (float)obs_data_get_int(settings, "posterize_levels") - 1.0f;

	vec4_from_rgba(&filter->color_1,
		       (uint32_t)obs_data_get_int(settings,
			"posterize_color_1"));
	vec4_from_rgba(&filter->color_2,
		       (uint32_t)obs_data_get_int(settings,
						  "posterize_color_2"));

	filter->technique =
		(uint32_t)obs_data_get_int(settings, "posterize_technique");

	const char *color_source_name =
		obs_data_get_string(settings, "posterize_map_source");
	obs_source_t *color_source =
		(color_source_name && strlen(color_source_name))
			? obs_get_source_by_name(color_source_name)
			: NULL;
	if (color_source) {
		obs_weak_source_release(filter->color_source);
		filter->color_source =
			obs_source_get_weak_source(color_source);
		obs_source_release(color_source);
	} else {
		if (filter->color_source) {
			obs_weak_source_release(filter->color_source);
		}
		filter->color_source = NULL;
	}
}

void posterize_filter_defaults(obs_data_t *settings) {
	obs_data_set_default_int(settings, "posterize_levels", 4);
	obs_data_set_default_int(settings, "posterize_color_1", 4278190080);
	obs_data_set_default_int(settings, "posterize_color_2", 4294967295);
}

void posterize_filter_properties(retro_effects_filter_data_t *data,
				 obs_properties_t *props)
{
	UNUSED_PARAMETER(data);
	obs_properties_add_int(props, "posterize_levels", "Levels", 2, 256,
			       1);

	obs_property_t *technique = obs_properties_add_list(
		props, "posterize_technique", obs_module_text("RetroEffects.Posterize.Technique"),
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

	obs_property_list_add_int(technique,
		obs_module_text(POSTERIZE_COLOR_PASSTHROUGH_LABEL),
		POSTERIZE_COLOR_PASSTHROUGH);

	obs_property_list_add_int(
		technique, obs_module_text(POSTERIZE_COLOR_MAP_LABEL),
		POSTERIZE_COLOR_MAP);

	obs_property_list_add_int(technique,
		obs_module_text(POSTERIZE_COLOR_SOURCE_MAP_LABEL),
		POSTERIZE_COLOR_SOURCE_MAP);

	obs_property_set_modified_callback(technique, posterize_technique_modified);

	obs_property_t *map_source = obs_properties_add_list(
		props, "posterize_map_source",
		obs_module_text("RetroEffects.Posterize.MapSource"),
		OBS_COMBO_TYPE_EDITABLE, OBS_COMBO_FORMAT_STRING);
	obs_enum_sources(add_source_to_list, map_source);
	obs_enum_scenes(add_source_to_list, map_source);
	obs_property_list_insert_string(
		map_source, 0,
		obs_module_text("RetroEffects.Posterize.MapSource.None"),
		"");

	obs_properties_add_color_alpha(
		props, "posterize_color_1",
		obs_module_text("RetroEffects.Posterize.Color1"));
	obs_properties_add_color_alpha(
		props, "posterize_color_2",
		obs_module_text("RetroEffects.Posterize.Color2"));
}

void posterize_filter_video_render(retro_effects_filter_data_t *data)
{
	base_filter_data_t *base = data->base;
	posterize_filter_data_t *filter = data->active_filter_data;

	get_input_source(base);
	if (!base->input_texture_generated) {
		base->rendering = false;
		obs_source_skip_video_filter(base->context);
		return;
	}

	gs_texture_t *image = gs_texrender_get_texture(base->input_texrender);
	gs_effect_t *effect = filter->effect_posterize;

	if (!effect || !image) {
		return;
	}

	base->output_texrender =
		create_or_reset_texrender(base->output_texrender);

	if (filter->param_image) {
		gs_effect_set_texture(filter->param_image, image);
	}

	if (filter->param_uv_size) {
		struct vec2 uv_size;
		uv_size.x = (float)base->width;
		uv_size.y = (float)base->height;
		gs_effect_set_vec2(filter->param_uv_size, &uv_size);
	}

	if (filter->param_levels) {
		gs_effect_set_float(filter->param_levels, filter->levels);
	}

	if (filter->param_color_1) {
		gs_effect_set_vec4(filter->param_color_1, &filter->color_1);
	}

	if (filter->param_color_2) {
		gs_effect_set_vec4(filter->param_color_2, &filter->color_2);
	}

	gs_texrender_t *color_source_render = NULL;
	gs_texture_t *source_texture = NULL;
	if (filter->technique == POSTERIZE_COLOR_SOURCE_MAP) {
		obs_source_t *source =
			filter->color_source
				? obs_weak_source_get_source(
					  filter->color_source)
				: NULL;
		if (!source) {
			gs_texrender_t *tmp = base->output_texrender;
			base->output_texrender = base->input_texrender;
			base->input_texrender = tmp;
			return;
		}

		const enum gs_color_space preferred_spaces[] = {
			GS_CS_SRGB,
			GS_CS_SRGB_16F,
			GS_CS_709_EXTENDED,
		};
		const enum gs_color_space space = obs_source_get_color_space(
			source, OBS_COUNTOF(preferred_spaces),
			preferred_spaces);
		const enum gs_color_format format =
			gs_get_format_from_space(space);

		// Set up a tex renderer for source
		color_source_render = gs_texrender_create(format, GS_ZS_NONE);
		uint32_t base_width = obs_source_get_width(source);
		uint32_t base_height = obs_source_get_height(source);

		gs_blend_state_push();
		gs_blend_function(GS_BLEND_ONE, GS_BLEND_ZERO);
		if (gs_texrender_begin_with_color_space(color_source_render,
							base_width, base_height,
							space)) {
			const float w = (float)base_width;
			const float h = (float)base_height;
			struct vec4 clear_color;

			vec4_zero(&clear_color);
			gs_clear(GS_CLEAR_COLOR, &clear_color, 0.0f, 0);
			gs_ortho(0.0f, w, 0.0f, h, -100.0f, 100.0f);
			obs_source_video_render(source);
			gs_texrender_end(color_source_render);
		}
		gs_blend_state_pop();
		obs_source_release(source);
		source_texture = gs_texrender_get_texture(color_source_render);

		if (filter->param_color_source && source_texture) {
			gs_effect_set_texture(filter->param_color_source,
						      source_texture);
		}
	}

	set_render_parameters();
	set_blending_parameters();

	const char *technique = filter->technique == POSTERIZE_COLOR_PASSTHROUGH
					? "Draw"
					: filter->technique == POSTERIZE_COLOR_MAP
					? "DrawColorMap" : "DrawSourceColorMap";

	if (gs_texrender_begin(base->output_texrender, base->width,
			       base->height)) {
		gs_ortho(0.0f, (float)base->width, 0.0f, (float)base->height,
			 -100.0f, 100.0f);
		while (gs_effect_loop(effect, technique))
			gs_draw_sprite(image, 0, base->width, base->height);
		gs_texrender_end(base->output_texrender);
	}

	gs_blend_state_pop();
	if (color_source_render) {
		gs_texrender_destroy(color_source_render);
	}
}

static void posterize_set_functions(retro_effects_filter_data_t *filter)
{
	filter->filter_properties = posterize_filter_properties;
	filter->filter_video_render = posterize_filter_video_render;
	filter->filter_destroy = posterize_destroy;
	filter->filter_defaults = posterize_filter_defaults;
	filter->filter_update = posterize_filter_update;
	filter->filter_video_tick = NULL;
}

static void posterize_load_effect(posterize_filter_data_t *filter)
{
	if (filter->effect_posterize != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect_posterize);
		filter->effect_posterize = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/posterize.effect");
	shader_text = load_shader_from_file(filename.array);
	char *errors = NULL;
	dstr_free(&filename);

	obs_enter_graphics();
	filter->effect_posterize =
		gs_effect_create(shader_text, NULL, &errors);
	obs_leave_graphics();

	bfree(shader_text);
	if (filter->effect_posterize == NULL) {
		blog(LOG_WARNING,
		     "[obs-retro-effects] Unable to load posterize.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	} else {
		size_t effect_count = gs_effect_get_num_params(
			filter->effect_posterize);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_posterize,
				effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_image = param;
			} else if (strcmp(info.name, "uv_size") == 0) {
				filter->param_uv_size = param;
			} else if (strcmp(info.name, "levels") == 0) {
				filter->param_levels = param;
			} else if (strcmp(info.name, "color_1") == 0) {
				filter->param_color_1 = param;
			} else if (strcmp(info.name, "color_2") == 0) {
				filter->param_color_2 = param;
			} else if (strcmp(info.name, "color_source") == 0) {
				filter->param_color_source = param;
			}
		}
	}
}

static bool posterize_technique_modified(obs_properties_t *props,
					 obs_property_t *p,
					 obs_data_t *settings)
{
	// retro_effects_filter_data_t *filter = data;
	UNUSED_PARAMETER(p);

	int technique = (int)obs_data_get_int(settings, "posterize_technique");
	switch (technique) {
	case POSTERIZE_COLOR_PASSTHROUGH:
		setting_visibility("posterize_map_source", false, props);
		setting_visibility("posterize_color_1", false, props);
		setting_visibility("posterize_color_2", false, props);
		break;
	case POSTERIZE_COLOR_MAP:
		setting_visibility("posterize_map_source", false, props);
		setting_visibility("posterize_color_1", true, props);
		setting_visibility("posterize_color_2", true, props);
		break;
	case POSTERIZE_COLOR_SOURCE_MAP:
		setting_visibility("posterize_map_source", true, props);
		setting_visibility("posterize_color_1", false, props);
		setting_visibility("posterize_color_2", false, props);
		break;
	}
	return true;
}
