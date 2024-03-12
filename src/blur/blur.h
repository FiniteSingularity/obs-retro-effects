#pragma once
#include <obs-module.h>
#include <util/darray.h>
#include "../obs-retro-effects.h"
#include "../obs-utils.h"
#include "gaussian-kernel.h"

#define MIN_GAUSSIAN_BLUR_RADIUS 0.01f

typedef DARRAY(float) fDarray;

struct blur_data;
typedef struct blur_data blur_data_t;

struct blur_data {
	gs_effect_t *gaussian_effect;
	gs_effect_t *dual_kawase_up_effect;
	gs_effect_t *dual_kawase_down_effect;

	gs_texrender_t *blur_buffer_1;
	gs_texrender_t *blur_buffer_2;
	gs_texrender_t *blur_output;

	uint32_t device_type;

	// Gaussuan Blur
	float radius;
	gs_eparam_t *param_kernel_size;
	size_t kernel_size;
	gs_eparam_t *param_offset;
	fDarray offset;
	gs_eparam_t *param_weight;
	fDarray kernel;
	gs_eparam_t *param_kernel_texture;
	gs_texture_t *kernel_texture;
	gs_eparam_t *param_texel_step;
	gs_eparam_t *param_uv_size;
	struct vec2 texel_step;
	float angle;

	// Kawase Blur
	int kawase_passes;
};

extern void blur_create(retro_effects_filter_data_t *filter);
extern void blur_destroy(retro_effects_filter_data_t *filter);
extern void gaussian_area_blur(gs_texture_t *texture, blur_data_t *data);
extern void gaussian_directional_blur(gs_texture_t *texture, blur_data_t *data);
extern void set_gaussian_radius(float radius, blur_data_t *filter);
static void load_1d_gaussian_effect(blur_data_t *filter);
static void sample_kernel(float radius, blur_data_t *filter);
