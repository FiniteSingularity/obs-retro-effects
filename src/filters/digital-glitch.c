#include "digital-glitch.h"
#include "../obs-utils.h"

void digital_glitch_create(retro_effects_filter_data_t *filter)
{
	digital_glitch_filter_data_t *data =
		bzalloc(sizeof(digital_glitch_filter_data_t));
	filter->active_filter_data = data;
	data->reload_effect = false;
	digital_glitch_set_functions(filter);
	obs_data_t *settings = obs_source_get_settings(filter->base->context);
	da_init(data->horiz_grid);
	da_init(data->vert_grid);
	data->grid_next_update = 0.0f;
	data->rgb_drift_next_update = 0.0f;
	digital_glitch_filter_defaults(settings);
	obs_data_release(settings);
	digital_glitch_load_effect(data);
}

void digital_glitch_destroy(retro_effects_filter_data_t *filter)
{
	digital_glitch_filter_data_t *data = filter->active_filter_data;
	obs_enter_graphics();
	if (data->effect_digital_glitch) {
		gs_effect_destroy(data->effect_digital_glitch);
	}
	if (data->horiz_grid_texture) {
		gs_texture_destroy(data->horiz_grid_texture);
	}
	if (data->vert_grid_texture) {
		gs_texture_destroy(data->vert_grid_texture);
	}
	if (data->rgb_drift_texture) {
		gs_texture_destroy(data->rgb_drift_texture);
	}

	obs_leave_graphics();

	da_free(data->horiz_grid);
	da_free(data->vert_grid);
	da_free(data->rgb_drift_grid);

	bfree(filter->active_filter_data);
	filter->active_filter_data = NULL;
}

void digital_glitch_unset_settings(retro_effects_filter_data_t* filter)
{
	obs_data_t* settings = obs_source_get_settings(filter->base->context);
	obs_data_unset_user_value(settings, "digital_glitch_amount");
	obs_data_unset_user_value(settings, "digital_glitch_max_disp");
	obs_data_unset_user_value(settings, "digital_glitch_min_block_width");
	obs_data_unset_user_value(settings, "digital_glitch_max_block_width");
	obs_data_unset_user_value(settings, "digital_glitch_min_block_height");
	obs_data_unset_user_value(settings, "digital_glitch_max_block_height");
	obs_data_unset_user_value(settings, "digital_glitch_min_rgb_height");
	obs_data_unset_user_value(settings, "digital_glitch_max_rgb_height");
	obs_data_unset_user_value(settings, "digital_glitch_min_rgb_interval");
	obs_data_unset_user_value(settings, "digital_glitch_max_rgb_interval");
	obs_data_release(settings);
}

void digital_glitch_filter_update(retro_effects_filter_data_t *data,
				  obs_data_t *settings)
{
	digital_glitch_filter_data_t *filter = data->active_filter_data;

	filter->amount = (float)obs_data_get_double(settings, "digital_glitch_amount") / 100.0f;
	filter->max_disp = (float)obs_data_get_double(settings, "digital_glitch_max_disp");
	filter->min_block_width = (uint32_t)obs_data_get_int(settings, "digital_glitch_min_block_width");
	filter->max_block_width = (uint32_t)obs_data_get_int(settings, "digital_glitch_max_block_width");
	filter->min_block_height = (uint32_t)obs_data_get_int(settings, "digital_glitch_min_block_height");
	filter->max_block_height = (uint32_t)obs_data_get_int(settings, "digital_glitch_max_block_height");
	filter->block_interval.x = (float)obs_data_get_double(settings, "digital_glitch_min_block_interval");
	filter->block_interval.y = (float)obs_data_get_double(settings, "digital_glitch_max_block_interval");

	filter->max_rgb_drift = (float)obs_data_get_double(settings, "digital_glitch_max_rgb_drift");
	filter->min_rgb_drift_height = (uint32_t)obs_data_get_int(settings, "digital_glitch_min_rgb_height");
	filter->max_rgb_drift_height = (uint32_t)obs_data_get_int(settings, "digital_glitch_max_rgb_height");
	filter->rgb_drift_interval.x = (float)obs_data_get_double(settings, "digital_glitch_min_rgb_interval");
	filter->rgb_drift_interval.y = (float)obs_data_get_double(settings, "digital_glitch_max_rgb_interval");

	if (filter->reload_effect) {
		filter->reload_effect = false;
		digital_glitch_load_effect(filter);
	}
}

void digital_glitch_filter_defaults(obs_data_t *settings)
{
	obs_data_set_default_double(settings, "digital_glitch_amount", 50.0);
	obs_data_set_default_double(settings, "digital_glitch_max_disp", 550.0);
	obs_data_set_default_int(settings, "digital_glitch_min_block_width", 60);
	obs_data_set_default_int(settings, "digital_glitch_max_block_width", 600);
	obs_data_set_default_int(settings, "digital_glitch_min_block_height", 30);
	obs_data_set_default_int(settings, "digital_glitch_max_block_height", 200);
	obs_data_set_default_double(settings, "digital_glitch_min_block_interval", 0.1);
	obs_data_set_default_double(settings, "digital_glitch_max_block_interval", 0.3);
	obs_data_set_default_double(settings, "digital_glitch_max_rgb_drift", 50.0);
	obs_data_set_default_double(settings, "digital_glitch_min_rgb_height", 30.0);
	obs_data_set_default_double(settings, "digital_glitch_max_rgb_height", 200.0);
	obs_data_set_default_double(settings, "digital_glitch_min_rgb_interval", 0.1);
	obs_data_set_default_double(settings, "digital_glitch_max_rgb_interval", 0.5);
}

void digital_glitch_filter_properties(retro_effects_filter_data_t *data,
				      obs_properties_t *props)
{
	UNUSED_PARAMETER(data);
	obs_property_t *p = obs_properties_add_float_slider(
		props, "digital_glitch_amount",
		obs_module_text("RetroEffects.DigitalGlitch.Amount"), 0.0, 100.0, 0.1);
	obs_property_float_set_suffix(p, "%");

	obs_properties_t *displacement_group = obs_properties_create();

	p = obs_properties_add_float_slider(
		displacement_group, "digital_glitch_max_disp",
		obs_module_text("RetroEffects.DigitalGlitch.MaxDisplacement"),
		0.0, 8000.0, 1.0);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_int_slider(
		displacement_group, "digital_glitch_min_block_width",
		obs_module_text("RetroEffects.DigitalGlitch.MinBlockWidth"), 0,
		4000, 1);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_int_slider(
		displacement_group, "digital_glitch_max_block_width",
		obs_module_text("RetroEffects.DigitalGlitch.MaxBlockWidth"), 0,
		4000, 1);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_int_slider(
		displacement_group, "digital_glitch_min_block_height",
		obs_module_text("RetroEffects.DigitalGlitch.MinBlockHeight"), 0,
		4000, 1);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_int_slider(
		displacement_group, "digital_glitch_max_block_height",
		obs_module_text("RetroEffects.DigitalGlitch.MaxBlockHeight"), 0,
		4000, 1);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		displacement_group, "digital_glitch_min_block_interval",
		obs_module_text("RetroEffects.DigitalGlitch.MinBlockInterval"),
		0.0, 4.0, 0.01);
	obs_property_float_set_suffix(p, " s");

	p = obs_properties_add_float_slider(
		displacement_group, "digital_glitch_max_block_interval",
		obs_module_text("RetroEffects.DigitalGlitch.MaxBlockInterval"), 0.0,
		4.0, 0.01);
	obs_property_float_set_suffix(p, " s");

	obs_properties_add_group(
		props, "digital_glitch_displacement",
		obs_module_text("RetroEffects.DigitalGlitch.Displacement"),
		OBS_GROUP_NORMAL, displacement_group);

	obs_properties_t *rgb_drift_group = obs_properties_create();

	p = obs_properties_add_float_slider(
		rgb_drift_group, "digital_glitch_max_rgb_drift",
		obs_module_text("RetroEffects.DigitalGlitch.MaxRGBDrift"), 0.0,
		8000.0, 1.0);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_int_slider(
		rgb_drift_group, "digital_glitch_min_rgb_height",
		obs_module_text("RetroEffects.DigitalGlitch.MinRGBHeight"), 0,
		4000, 1);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_int_slider(
		rgb_drift_group, "digital_glitch_max_rgb_height",
		obs_module_text("RetroEffects.DigitalGlitch.MaxRGBHeight"), 0,
		4000, 1);
	obs_property_float_set_suffix(p, "px");

	p = obs_properties_add_float_slider(
		rgb_drift_group, "digital_glitch_min_rgb_interval",
		obs_module_text("RetroEffects.DigitalGlitch.MinRGBInterval"),
		0.0, 4.0, 0.01);
	obs_property_float_set_suffix(p, " s");

	p = obs_properties_add_float_slider(
		rgb_drift_group, "digital_glitch_max_rgb_interval",
		obs_module_text("RetroEffects.DigitalGlitch.MaxRGBInterval"),
		0.0, 4.0, 0.01);
	obs_property_float_set_suffix(p, " s");

	obs_properties_add_group(
		props, "digital_glitch_rgb_drift",
		obs_module_text("RetroEffects.DigitalGlitch.RGBDrift"),
		OBS_GROUP_NORMAL, rgb_drift_group);

}

void digital_glitch_filter_video_render(retro_effects_filter_data_t *data)
{
	base_filter_data_t *base = data->base;
	digital_glitch_filter_data_t *filter = data->active_filter_data;

	get_input_source(base);
	if (!base->input_texture_generated || filter->loading_effect) {
		base->rendering = false;
		obs_source_skip_video_filter(base->context);
		return;
	}

	gs_texture_t *image = gs_texrender_get_texture(base->input_texrender);
	gs_effect_t *effect = filter->effect_digital_glitch;

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
		float time = (float)floor(filter->local_time + sin(11.0 * filter->local_time) + sin(filter->local_time)) * 0.77f;
		gs_effect_set_float(filter->param_time,
				    time);
	}
	if (filter->param_horiz_grid) {
		gs_effect_set_texture(filter->param_horiz_grid, filter->horiz_grid_texture);
	}
	if (filter->param_vert_grid) {
		gs_effect_set_texture(filter->param_vert_grid, filter->vert_grid_texture);
	}

	if (filter->param_max_disp) {
		gs_effect_set_float(filter->param_max_disp, filter->max_disp);
	}

	if (filter->param_max_rgb_drift) {
		gs_effect_set_float(filter->param_max_rgb_drift,
				    filter->max_rgb_drift);
	}

	if (filter->param_rgb_drift_grid) {
		gs_effect_set_texture(filter->param_rgb_drift_grid,
				      filter->rgb_drift_texture);
	}

	if (filter->param_amount) {
		gs_effect_set_float(filter->param_amount, filter->amount);
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

static void digital_glitch_set_functions(retro_effects_filter_data_t *filter)
{
	filter->filter_properties = digital_glitch_filter_properties;
	filter->filter_video_render = digital_glitch_filter_video_render;
	filter->filter_destroy = digital_glitch_destroy;
	filter->filter_defaults = digital_glitch_filter_defaults;
	filter->filter_update = digital_glitch_filter_update;
	filter->filter_video_tick = digital_glitch_filter_video_tick;
	filter->filter_unset_settings = digital_glitch_unset_settings;
}

void digital_glitch_filter_video_tick(retro_effects_filter_data_t *data,
				      float seconds)
{
	digital_glitch_filter_data_t *filter = data->active_filter_data;
	filter->local_time += seconds;
	if (data->base->width == 0u || data->base->height == 0u) {
		return;
	}
	uint32_t min_h_band_size = filter->min_block_width;
	uint32_t max_h_band_size = filter->max_block_width;
	uint32_t min_v_band_size = filter->min_block_height;
	uint32_t max_v_band_size = filter->max_block_height;

	if (filter->local_time > filter->grid_next_update) {
		if (filter->horiz_grid.capacity != data->base->width) {
			da_resize(filter->horiz_grid, data->base->width);
		}
		if (filter->vert_grid.capacity != data->base->height) {
			da_resize(filter->vert_grid, data->base->height);
		}
		// Code to fill in with random "Bands"
		// Horizontal
		size_t i = 0;
		while (i < filter->horiz_grid.capacity) {
			float intensity = (float)rand() / (float)RAND_MAX;
			float r = (float)rand() / (float)RAND_MAX;
			float range =
				(float)(max_h_band_size - min_h_band_size + 1);
			uint32_t h_size =
				(uint32_t)floor(r * range) + min_h_band_size;
			size_t end = i + h_size;
			for (; i < end; i++) {
				if (i == filter->horiz_grid.capacity) {
					break;
				}
				filter->horiz_grid.array[i] = intensity;
			}
		}
		i = 0;
		while (i < filter->vert_grid.capacity) {
			float intensity = (float)rand() / (float)RAND_MAX;
			float r = (float)rand() / (float)RAND_MAX;
			float range =
				(float)(max_v_band_size - min_v_band_size + 1);
			uint32_t v_size =
				(uint32_t)floor(r * range) + min_v_band_size;
			size_t end = i + v_size;
			for (; i < end; i++) {
				if (i == filter->vert_grid.capacity) {
					break;
				}
				filter->vert_grid.array[i] = intensity;
			}
		}
		obs_enter_graphics();
		if (filter->horiz_grid_texture) {
			gs_texture_destroy(filter->horiz_grid_texture);
		}
		filter->horiz_grid_texture = gs_texture_create(
			(uint32_t)filter->horiz_grid.num, 1u, GS_R32F, 1u,
			(const uint8_t **)&filter->horiz_grid.array, 0);
		if (!filter->horiz_grid_texture) {
			blog(LOG_WARNING,
			     "Horiz Grid Texture couldn't be created.");
		}
		if (filter->vert_grid_texture) {
			gs_texture_destroy(filter->vert_grid_texture);
		}
		filter->vert_grid_texture = gs_texture_create(
			1u, (uint32_t)filter->vert_grid.num, GS_R32F, 1u,
			(const uint8_t **)&filter->vert_grid.array, 0);
		if (!filter->vert_grid_texture) {
			blog(LOG_WARNING,
			     "Vert Grid Texture couldn't be created.");
		}
		obs_leave_graphics();
		float interval = (float)rand() / (float)RAND_MAX *
					 (filter->block_interval.y -
					  filter->block_interval.x) +
				 filter->block_interval.x;

		filter->grid_next_update = filter->local_time + interval;
	}
	if (filter->local_time > filter->rgb_drift_next_update) {
		if (filter->rgb_drift_grid.capacity != data->base->height) {
			da_resize(filter->rgb_drift_grid, data->base->height);
		}
		size_t i = 0;
		while (i < filter->rgb_drift_grid.capacity) {
			float intensity = (float)rand() / (float)RAND_MAX;
			float r = (float)rand() / (float)RAND_MAX;
			float range = (float)(filter->max_rgb_drift_height - filter->min_rgb_drift_height + 1);
			uint32_t v_size = (uint32_t)floor(r * range) + filter->min_rgb_drift_height;
			size_t end = i + v_size;
			for (; i < end; i++) {
				if (i == filter->rgb_drift_grid.capacity) {
					break;
				}
				filter->rgb_drift_grid.array[i] = intensity;
			}
		}
		obs_enter_graphics();
		if (filter->rgb_drift_texture) {
			gs_texture_destroy(filter->rgb_drift_texture);
		}
		filter->rgb_drift_texture = gs_texture_create(
			1u, (uint32_t)filter->rgb_drift_grid.num, GS_R32F, 1u,
			(const uint8_t **)&filter->rgb_drift_grid.array, 0);
		if (!filter->rgb_drift_texture) {
			blog(LOG_WARNING,
			     "Vert Grid Texture couldn't be created.");
		}
		obs_leave_graphics();
		float interval = (float)rand() / (float)RAND_MAX *
					 (filter->rgb_drift_interval.y -
					  filter->rgb_drift_interval.x) +
				 filter->rgb_drift_interval.x;

		filter->rgb_drift_next_update = filter->local_time + interval;
	}
}

static void digital_glitch_load_effect(digital_glitch_filter_data_t *filter)
{
	filter->loading_effect = true;
	if (filter->effect_digital_glitch != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->effect_digital_glitch);
		filter->effect_digital_glitch = NULL;
		obs_leave_graphics();
	}

	char *shader_text = NULL;
	struct dstr filename = {0};
	dstr_cat(&filename, obs_get_module_data_path(obs_current_module()));
	dstr_cat(&filename, "/shaders/digital-glitch.effect");
	shader_text = load_shader_from_file(filename.array);
	char *errors = NULL;
	dstr_free(&filename);

	struct dstr shader_dstr = {0};
	dstr_init_copy(&shader_dstr, shader_text);

	obs_enter_graphics();
	filter->effect_digital_glitch =
		gs_effect_create(shader_dstr.array, NULL, &errors);
	obs_leave_graphics();

	dstr_free(&shader_dstr);
	bfree(shader_text);
	if (filter->effect_digital_glitch == NULL) {
		blog(LOG_WARNING,
		     "[obs-retro-effects] Unable to load scan-lines.effect file.  Errors:\n%s",
		     (errors == NULL || strlen(errors) == 0 ? "(None)"
							    : errors));
		bfree(errors);
	} else {
		size_t effect_count =
			gs_effect_get_num_params(filter->effect_digital_glitch);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->effect_digital_glitch, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_image = param;
			} else if (strcmp(info.name, "uv_size") == 0) {
				filter->param_uv_size = param;
			} else if (strcmp(info.name, "time") == 0) {
				filter->param_time = param;
			} else if (strcmp(info.name, "vert_grid") == 0) {
				filter->param_vert_grid = param;
			} else if (strcmp(info.name, "horiz_grid") == 0) {
				filter->param_horiz_grid = param;
			} else if (strcmp(info.name, "rgb_drift_grid") == 0) {
				filter->param_rgb_drift_grid = param;
			} else if (strcmp(info.name, "max_disp") == 0) {
				filter->param_max_disp = param;
			} else if (strcmp(info.name, "amount") == 0) {
				filter->param_amount = param;
			} else if (strcmp(info.name, "max_rgb_drift") == 0) {
				filter->param_max_rgb_drift = param;
			}
		}
	}
	filter->loading_effect = false;
}

static void generate_banded_texture() {

}
