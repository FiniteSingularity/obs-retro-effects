#pragma once
#include <obs-module.h>
#include <util/darray.h>
#include "../obs-retro-effects.h"
#include "../obs-utils.h"

#include "blur.h"

struct bloom_data;
typedef struct bloom_data bloom_data_t;

// 1. Threshold the brightness
// 2. bloom the thresholded brightness
// 3. Combine blur with original --> output

struct bloom_data {
	blur_data_t *blur;

	gs_effect_t *brightness_threshold_effect;
	gs_effect_t *combine_effect;

	gs_texrender_t *brightness_threshold_buffer;
	gs_texrender_t *output;

	gs_eparam_t *param_bt_image;
	gs_eparam_t *param_bt_threshold;

	gs_eparam_t *param_combine_image;
	gs_eparam_t *param_combine_bloom_image;
	gs_eparam_t *param_combine_intensity;

	float bloom_size;
	float bloom_intensity;
	float brightness_threshold;
};

extern void bloom_create(retro_effects_filter_data_t *filter);
extern void bloom_destroy(retro_effects_filter_data_t *filter);
extern void bloom_render(gs_texture_t *texture, bloom_data_t *bloom_data);
static void load_brightness_threshold_effect(bloom_data_t *filter);
static void load_bloom_combine_effect(bloom_data_t *filter);
