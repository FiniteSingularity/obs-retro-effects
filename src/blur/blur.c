#include "blur.h"

void blur_create(retro_effects_filter_data_t *filter)
{
	blur_data_t *data = bzalloc(sizeof(blur_data_t));
	
	da_init(data->kernel);
	da_init(data->offset);
	// Set device_type
	obs_enter_graphics();
	data->device_type = gs_get_device_type();
	obs_leave_graphics();
	// Load in effects
	load_1d_gaussian_effect(data);
	filter->blur_data = data;
}

void blur_destroy(retro_effects_filter_data_t *filter)
{
	
	obs_enter_graphics();
	if (filter->blur_data->gaussian_effect) {
		gs_effect_destroy(filter->blur_data->gaussian_effect);
	}
	if (filter->blur_data->dual_kawase_up_effect) {
		gs_effect_destroy(filter->blur_data->dual_kawase_up_effect);
	}
	if (filter->blur_data->dual_kawase_down_effect) {
		gs_effect_destroy(filter->blur_data->dual_kawase_down_effect);
	}

	if (filter->blur_data->blur_buffer_1) {
		gs_texrender_destroy(filter->blur_data->blur_buffer_1);
	}
	if (filter->blur_data->blur_buffer_2) {
		gs_texrender_destroy(filter->blur_data->blur_buffer_2);
	}
	if (filter->blur_data->blur_output) {
		gs_texrender_destroy(filter->blur_data->blur_output);
	}
	if (filter->blur_data->kernel_texture) {
		gs_texture_destroy(filter->blur_data->kernel_texture);
	}
	obs_leave_graphics();

	da_free(filter->blur_data->offset);
	da_free(filter->blur_data->kernel);

	bfree(filter->blur_data);
	filter->blur_data = NULL;
}

/*
 *  Performs an area blur using the gaussian kernel. Blur is
 *  equal in both x and y directions.
 */
void gaussian_area_blur(gs_texture_t *texture, blur_data_t *data)
{
	gs_effect_t *effect = data->gaussian_effect;

	if (!effect || !texture) {
		return;
	}

	uint32_t width = gs_texture_get_width(texture);
	uint32_t height = gs_texture_get_height(texture);

	if (data->radius < MIN_GAUSSIAN_BLUR_RADIUS) {
		data->blur_output =
			create_or_reset_texrender(data->blur_output);
		texrender_set_texture(texture, data->blur_output);
		return;
	}

	data->blur_buffer_1 = create_or_reset_texrender(data->blur_buffer_1);

	// 1. First pass- apply 1D blur kernel to horizontal dir.
	gs_eparam_t *image = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture(image, texture);

	switch (data->device_type) {
	case GS_DEVICE_DIRECT3D_11:
		if (data->param_weight) {
			gs_effect_set_val(data->param_weight,
					  data->kernel.array,
					  data->kernel.num * sizeof(float));
		}
		if (data->param_offset) {
			gs_effect_set_val(data->param_offset,
					  data->offset.array,
					  data->offset.num * sizeof(float));
		}
		break;
	case GS_DEVICE_OPENGL:
		if (data->param_kernel_texture) {
			gs_effect_set_texture(data->param_kernel_texture,
					      data->kernel_texture);
		}
	}

	const int k_size = (int)data->kernel_size;
	if (data->param_kernel_size) {
		gs_effect_set_int(data->param_kernel_size, k_size);
	}

	struct vec2 texel_step;
	texel_step.x = 1.0f / (float)width;
	texel_step.y = 0.0f;
	if (data->param_texel_step) {
		gs_effect_set_vec2(data->param_texel_step, &texel_step);
	}

	set_blending_parameters();

	if (gs_texrender_begin(data->blur_buffer_1, width, height)) {
		gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f,
			 100.0f);
		while (gs_effect_loop(effect, "Draw"))
			gs_draw_sprite(texture, 0, width, height);
		gs_texrender_end(data->blur_buffer_1);
	}

	// 2. Save texture from first pass in variable "texture"
	gs_texture_t *texture2 = gs_texrender_get_texture(data->blur_buffer_1);

	// 3. Second Pass- Apply 1D blur kernel vertically.
	image = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture(image, texture2);

	if (data->device_type == GS_DEVICE_OPENGL &&
	    data->param_kernel_texture) {
		gs_effect_set_texture(data->param_kernel_texture,
				      data->kernel_texture);
	}

	texel_step.x = 0.0f;
	texel_step.y = 1.0f / (float)height;
	if (data->param_texel_step) {
		gs_effect_set_vec2(data->param_texel_step, &texel_step);
	}

	data->blur_output = create_or_reset_texrender(data->blur_output);

	if (gs_texrender_begin(data->blur_output, width, height)) {
		gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f,
			 100.0f);
		while (gs_effect_loop(effect, "Draw"))
			gs_draw_sprite(texture, 0, width, height);
		gs_texrender_end(data->blur_output);
	}

	gs_blend_state_pop();
}

/*
 *  Performs a directional blur using the gaussian kernel.
 */
void gaussian_directional_blur(gs_texture_t *texture, blur_data_t *data)
{
	gs_effect_t *effect = data->gaussian_effect;

	if (!effect || !texture) {
		return;
	}

	uint32_t width = gs_texture_get_width(texture);
	uint32_t height = gs_texture_get_height(texture);

	if (data->radius < MIN_GAUSSIAN_BLUR_RADIUS) {
		data->blur_output =
			create_or_reset_texrender(data->blur_output);
		texrender_set_texture(texture, data->blur_output);
		return;
	}

	// 1. Single pass- blur only in one direction
	gs_eparam_t *image = gs_effect_get_param_by_name(effect, "image");
	gs_effect_set_texture(image, texture);

	switch (data->device_type) {
	case GS_DEVICE_DIRECT3D_11:
		if (data->param_weight) {
			gs_effect_set_val(data->param_weight,
					  data->kernel.array,
					  data->kernel.num * sizeof(float));
		}
		if (data->param_offset) {
			gs_effect_set_val(data->param_offset,
					  data->offset.array,
					  data->offset.num * sizeof(float));
		}
		break;
	case GS_DEVICE_OPENGL:
		if (data->param_kernel_texture) {
			gs_effect_set_texture(data->param_kernel_texture,
					      data->kernel_texture);
		}
		break;
	}

	const int k_size = (int)data->kernel_size;
	if (data->param_kernel_size) {
		gs_effect_set_int(data->param_kernel_size, k_size);
	}

	struct vec2 texel_step;
	float rads = -data->angle * (M_PI / 180.0f);
	texel_step.x = (float)cos(rads) / width;
	texel_step.y = (float)sin(rads) / height;
	if (data->param_texel_step) {
		gs_effect_set_vec2(data->param_texel_step, &texel_step);
	}

	set_blending_parameters();

	data->blur_output = create_or_reset_texrender(data->blur_output);

	const char *technique = "Draw";

	if (gs_texrender_begin(data->blur_output, width, height)) {
		gs_ortho(0.0f, (float)width, 0.0f, (float)height, -100.0f,
			 100.0f);
		while (gs_effect_loop(effect, technique))
			gs_draw_sprite(texture, 0, width, height);
		gs_texrender_end(data->blur_output);
	}

	gs_blend_state_pop();
}


static void load_1d_gaussian_effect(blur_data_t *filter)
{
	if (filter->gaussian_effect != NULL) {
		obs_enter_graphics();
		gs_effect_destroy(filter->gaussian_effect);
		filter->gaussian_effect = NULL;
		obs_leave_graphics();
	}

	const char *effect_file_path =
		filter->device_type == GS_DEVICE_DIRECT3D_11
			? "/shaders/gaussian_1d.effect"
			: "/shaders/gaussian_1d_texture.effect";

	filter->gaussian_effect =
		load_shader_effect(filter->gaussian_effect, effect_file_path);
	if (filter->gaussian_effect) {
		size_t effect_count = gs_effect_get_num_params(filter->gaussian_effect);
		for (size_t effect_index = 0; effect_index < effect_count;
		     effect_index++) {
			gs_eparam_t *param = gs_effect_get_param_by_idx(
				filter->gaussian_effect, effect_index);
			struct gs_effect_param_info info;
			gs_effect_get_param_info(param, &info);
			if (strcmp(info.name, "uv_size") == 0) {
				filter->param_uv_size = param;
			} else if (strcmp(info.name, "texel_step") == 0) {
				filter->param_texel_step = param;
			} else if (strcmp(info.name, "offset") == 0) {
				filter->param_offset = param;
			} else if (strcmp(info.name, "weight") == 0) {
				filter->param_weight = param;
			} else if (strcmp(info.name, "kernel_size") == 0) {
				filter->param_kernel_size = param;
			} else if (strcmp(info.name, "kernel_texture") == 0) {
				filter->param_kernel_texture = param;
			}
		}
	}
}

void set_gaussian_radius(float radius, blur_data_t* filter) {
	filter->radius = radius;
	if (radius > MIN_GAUSSIAN_BLUR_RADIUS) {
		sample_kernel(radius, filter);
	}
}

static void sample_kernel(float radius, blur_data_t *filter)
{
	const size_t max_size = 128;
	const float max_radius = 250.0;
	const float min_radius = 0.0;

	fDarray d_weights;
	da_init(d_weights);

	radius *= 3.0f;
	radius = (float)fmax(fmin(radius, max_radius), min_radius);

	// 1. Calculate discrete weights
	const float bins_per_pixel =
		((2.f * (float)gaussian_kernel_size - 1.f)) /
		(1.f + 2.f * radius);
	size_t current_bin = 0;
	float fractional_bin = 0.5f;
	float ceil_radius = (radius - (float)floor(radius)) < 0.001f
				    ? radius
				    : (float)ceil(radius);
	float fractional_extra = 1.0f - (ceil_radius - radius);

	for (int i = 0; i <= (int)ceil_radius; i++) {
		float fractional_pixel = i < (int)ceil_radius ? 1.0f
					 : fractional_extra < 0.002f
						 ? 1.0f
						 : fractional_extra;
		float bpp_mult = i == 0 ? 0.5f : 1.0f;
		float weight = 1.0f / bpp_mult * fractional_bin *
			       gaussian_kernel[current_bin];
		float remaining_bins =
			bpp_mult * fractional_pixel * bins_per_pixel -
			fractional_bin;
		while ((int)floor(remaining_bins) > 0) {
			current_bin++;
			weight +=
				1.0f / bpp_mult * gaussian_kernel[current_bin];
			remaining_bins -= 1.f;
		}
		current_bin++;
		if (remaining_bins > 1.e-6f) {
			weight += 1.0f / bpp_mult *
				  gaussian_kernel[current_bin] * remaining_bins;
			fractional_bin = 1.0f - remaining_bins;
		} else {
			fractional_bin = 1.0f;
		}
		if (weight > 1.0001f || weight < 0.0f) {
			blog(LOG_WARNING,
			     "   === BAD WEIGHT VALUE FOR GAUSSIAN === [%d] %f",
			     (int)(d_weights.num + 1), weight);
			weight = 0.0;
		}
		da_push_back(d_weights, &weight);
	}

	fDarray d_offsets;
	da_init(d_offsets);

	// 2. Calculate discrete offsets
	for (int i = 0; i <= (int)ceil_radius; i++) {
		float val = (float)i;
		da_push_back(d_offsets, &val);
	}

	fDarray weights;
	da_init(weights);

	fDarray offsets;
	da_init(offsets);

	DARRAY(float) weight_offset_texture;
	da_init(weight_offset_texture);

	// 3. Calculate linear sampled weights and offsets
	da_push_back(weights, &d_weights.array[0]);
	da_push_back(offsets, &d_offsets.array[0]);

	da_push_back(weight_offset_texture, &d_weights.array[0]);
	da_push_back(weight_offset_texture, &d_offsets.array[0]);

	for (size_t i = 1; i < d_weights.num - 1; i += 2) {
		const float weight =
			d_weights.array[i] + d_weights.array[i + 1];
		const float offset =
			(d_offsets.array[i] * d_weights.array[i] +
			 d_offsets.array[i + 1] * d_weights.array[i + 1]) /
			weight;

		da_push_back(weights, &weight);
		da_push_back(offsets, &offset);

		da_push_back(weight_offset_texture, &weight);
		da_push_back(weight_offset_texture, &offset);
	}
	if (d_weights.num % 2 == 0) {
		const float weight = d_weights.array[d_weights.num - 1];
		const float offset = d_offsets.array[d_offsets.num - 1];
		da_push_back(weights, &weight);
		da_push_back(offsets, &offset);

		da_push_back(weight_offset_texture, &weight);
		da_push_back(weight_offset_texture, &offset);
	}

	// 4. Pad out kernel arrays to length of max_size
	const size_t padding = max_size - weights.num;
	filter->kernel_size = weights.num;

	for (size_t i = 0; i < padding; i++) {
		float pad = 0.0f;
		da_push_back(weights, &pad);
	}
	da_free(filter->kernel);
	filter->kernel = weights;

	for (size_t i = 0; i < padding; i++) {
		float pad = 0.0f;
		da_push_back(offsets, &pad);
	}

	da_free(filter->offset);
	filter->offset = offsets;

	// Generate the kernel and offsets as a texture for OpenGL systems
	// where the red value is the kernel weight and the green value
	// is the offset value. This is only used for OpenGL systems, and
	// should probably be macro'd out on windows builds, but generating
	// the texture is quite cheap.
	obs_enter_graphics();
	if (filter->kernel_texture) {
		gs_texture_destroy(filter->kernel_texture);
	}

	filter->kernel_texture = gs_texture_create(
		(uint32_t)weight_offset_texture.num / 2u, 1u, GS_RG32F, 1u,
		(const uint8_t **)&weight_offset_texture.array, 0);

	if (!filter->kernel_texture) {
		blog(LOG_WARNING, "Gaussian Texture couldn't be created.");
	}
	obs_leave_graphics();
	da_free(d_weights);
	da_free(d_offsets);
	da_free(weight_offset_texture);
}
