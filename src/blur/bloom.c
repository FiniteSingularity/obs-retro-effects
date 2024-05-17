#include "bloom.h"

void bloom_create(retro_effects_filter_data_t *filter)
{
	bloom_data_t *data = bzalloc(sizeof(blur_data_t));

	data->blur = filter->blur_data;
	data->gaussian_radius = 5.0f;
	data->levels.x = 0.299f;
	data->levels.y = 0.587f;
	data->levels.z = 0.114f;
	set_gaussian_radius(data->gaussian_radius, data->blur);
	// Load in effects
	load_brightness_threshold_effect(data);
	load_bloom_combine_effect(data);
	filter->bloom_data = data;
}

void bloom_destroy(retro_effects_filter_data_t *filter)
{

	obs_enter_graphics();
	if (filter->bloom_data->brightness_threshold_effect) {
		gs_effect_destroy(
			filter->bloom_data->brightness_threshold_effect);
	}

	if (filter->bloom_data->combine_effect) {
		gs_effect_destroy(filter->bloom_data->combine_effect);
	}

	if (filter->bloom_data->brightness_threshold_buffer) {
		gs_texrender_destroy(
			filter->bloom_data->brightness_threshold_buffer);
	}

	if (filter->bloom_data->output) {
		gs_texrender_destroy(filter->bloom_data->output);
	}

	obs_leave_graphics();

	bfree(filter->bloom_data);
	filter->bloom_data = NULL;
}

// 1. Threshold the brightness
// 2. blur the thresholded brightness
// 3. Combine blur with original --> output
void bloom_render(gs_texture_t *texture, bloom_data_t *bloom_data)
{
	gs_effect_t *effect_threshold = bloom_data->brightness_threshold_effect;

	if (!effect_threshold || !texture) {
		// set bloom output to original texture
		return;
	}

	if (bloom_data->gaussian_radius != bloom_data->bloom_size) {
		bloom_data->gaussian_radius = bloom_data->bloom_size;
		set_gaussian_radius(bloom_data->gaussian_radius, bloom_data->blur);
	}

	uint32_t width = gs_texture_get_width(texture);
	uint32_t height = gs_texture_get_height(texture);

	bloom_data->brightness_threshold_buffer = create_or_reset_texrender(bloom_data->brightness_threshold_buffer);

	// 1. First pass- apply 1D blur kernel to horizontal dir.
	if (bloom_data->param_bt_image) {
		gs_effect_set_texture(bloom_data->param_bt_image, texture);
	}
	if (bloom_data->param_bt_threshold) {
		gs_effect_set_float(bloom_data->param_bt_threshold, bloom_data->brightness_threshold);
	}
	if (bloom_data->param_bt_levels) {
		gs_effect_set_vec3(bloom_data->param_bt_levels,
				   &bloom_data->levels);
	}

	set_blending_parameters();

	if (gs_texrender_begin(bloom_data->brightness_threshold_buffer, width, height)) {
		gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f,
			 100.0f);
		while (gs_effect_loop(effect_threshold, "Draw"))
			gs_draw_sprite(texture, 0, width, height);
		gs_texrender_end(bloom_data->brightness_threshold_buffer);
	}
	gs_blend_state_pop();

	gs_texture_t *bt_texture =
		gs_texrender_get_texture(bloom_data->brightness_threshold_buffer);

	gaussian_area_blur(bt_texture, bloom_data->blur);

	// bloom_data->blur->output_texrender contains the bloom layer
	// Merge with original texture.
	gs_texture_t *bloom_texture =
		gs_texrender_get_texture(bloom_data->blur->blur_output);

	gs_effect_t *effect_combine = bloom_data->combine_effect;

	if (!effect_combine || !bloom_texture) {
		// set bloom output to original texture
		return;
	}

	if (bloom_data->param_combine_image) {
		gs_effect_set_texture(bloom_data->param_combine_image, texture);
	}
	if (bloom_data->param_combine_bloom_image) {
		gs_effect_set_texture(bloom_data->param_combine_bloom_image, bloom_texture);
	}
	if (bloom_data->param_combine_intensity) {
		gs_effect_set_float(bloom_data->param_combine_intensity, bloom_data->bloom_intensity);
	}
	bloom_data->output = create_or_reset_texrender(bloom_data->output);

	set_blending_parameters();

	if (gs_texrender_begin(bloom_data->output, width, height)) {
		gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f,
			 100.0f);
		while (gs_effect_loop(effect_combine, "Draw"))
			gs_draw_sprite(texture, 0, width, height);
		gs_texrender_end(bloom_data->output);
	}
	gs_blend_state_pop();
}

static void load_brightness_threshold_effect(bloom_data_t *filter)
{
	if (filter->brightness_threshold_effect != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->brightness_threshold_effect);
		filter->brightness_threshold_effect = NULL;
		obs_leave_graphics();
	}

	const char *effect_file_path = "/shaders/brightness-threshold.effect";

	filter->brightness_threshold_effect = load_shader_effect(
		filter->brightness_threshold_effect, effect_file_path);
	if (filter->brightness_threshold_effect) {
		size_t effect_count = gs_effect_get_num_params(
			filter->brightness_threshold_effect);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->brightness_threshold_effect, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_bt_image = param;
			} else if (strcmp(info.name, "threshold") == 0) {
				filter->param_bt_threshold = param;
			} else if (strcmp(info.name, "levels") == 0) {
				filter->param_bt_levels = param;
			}
		}
	}
}

static void load_bloom_combine_effect(bloom_data_t *filter)
{
	if (filter->combine_effect != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->combine_effect);
		filter->combine_effect = NULL;
		obs_leave_graphics();
	}

	const char *effect_file_path = "/shaders/bloom-combine.effect";

	filter->combine_effect =
		load_shader_effect(filter->combine_effect, effect_file_path);
	if (filter->combine_effect) {
		size_t effect_count =
			gs_effect_get_num_params(filter->combine_effect);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->combine_effect, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "image") == 0) {
				filter->param_combine_image = param;
			} else if (strcmp(info.name, "bloom_image") == 0) {
				filter->param_combine_bloom_image = param;
			} else if (strcmp(info.name, "intensity") == 0) {
				filter->param_combine_intensity = param;
			}
		}
	}
}
